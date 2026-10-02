#include "game_runtime.h"
#include "game_audio.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "bsp_audio.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdatomic.h>
#include <string.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define STRIP_ROWS 40
/* Swap mirror axes if the physical board is held the opposite way up.
 * Landscape direction and UP/DOWN placement require actual device acceptance. */
#define LANDSCAPE_MIRROR_X true
static const char *TAG = "game_runtime";
static const game_app_t *active_app;
static atomic_bool input_lost;
typedef struct { int key,down; uint32_t ms; } key_event_t;
static QueueHandle_t inputs, sounds;
static SemaphoreHandle_t transfer_done;
static uint16_t *strip[2];
static int64_t render_us, transfer_wait_us;

/* Runs in panel ISR. Release the DMA buffer only after transfer completion. */
static bool color_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *event, void *user)
{
    (void)io; (void)event; (void)user;
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(transfer_done, &wake);
    return wake == pdTRUE;
}

/* Shared esp_timer callback: bounded, nonblocking input queue only. */
static void button_event(bsp_btn_t key, bsp_btn_ev_t event, void *user)
{
    (void)user;
    if (event != BSP_BTN_PRESS && event != BSP_BTN_RELEASE) return;
    key_event_t message={(int)key,event==BSP_BTN_PRESS,(uint32_t)(esp_timer_get_time()/1000)};
    if (xQueueSend(inputs,&message,0)!=pdTRUE) atomic_store(&input_lost,true);
}

static atomic_int music_theme;

/* Optional audio worker owns all codec writes. Failure degrades to silent play. */
static void sound_worker(void *arg)
{
    (void)arg;
    bool ready = bsp_audio_init() == ESP_OK && bsp_audio_set_format(GAME_AUDIO_HZ,16,1) == ESP_OK;
    if (ready) bsp_audio_set_volume(25);
    else ESP_LOGW(TAG,"Audio unavailable; continuing silently");
    int16_t pcm[160];unsigned cursor=GAME_AUDIO_SAMPLES;int active=0;
    uint32_t music_cursor=0;
    for (;;) {
        int theme=atomic_load(&music_theme),effect=0;
        TickType_t timeout=(theme||cursor<GAME_AUDIO_SAMPLES)?0:pdMS_TO_TICKS(50);
        if(xQueueReceive(sounds,&effect,timeout)==pdTRUE){active=effect;cursor=0;}
        if(!ready){vTaskDelay(pdMS_TO_TICKS(50));continue;}
        if(!theme&&cursor>=GAME_AUDIO_SAMPLES)continue;
        for(unsigned i=0;i<160;i++){
            int sample=game_music_sample(theme,music_cursor++);
            if(cursor<GAME_AUDIO_SAMPLES)sample+=game_audio_sample(active,cursor++);
            pcm[i]=(int16_t)sample;
        }
        if(bsp_audio_write(pcm,sizeof(pcm))!=ESP_OK){
            ready=false;ESP_LOGW(TAG,"Audio write failed; continuing silently");
            cursor=GAME_AUDIO_SAMPLES;atomic_store(&music_theme,0);
        }
    }
}

/* Two 25 KiB strips overlap CPU rendering with one outstanding SPI transfer.
 * Completion is consumed before submitting another transfer; buffers alternate
 * so the CPU never writes the DMA-owned strip. The last transfer is joined. */
static bool present(void)
{
    bool pending=false;
    render_us=0;transfer_wait_us=0;
    for (int y = 0, index=0; y < 240; y += STRIP_ROWS,index^=1) {
        uint16_t *pixels=strip[index];
        int64_t start=esp_timer_get_time();
        active_app->render(pixels,y,STRIP_ROWS);
        for (int i = 0; i < 320*STRIP_ROWS; ++i)
            pixels[i] = (uint16_t)((pixels[i] << 8) | (pixels[i] >> 8));
        render_us+=esp_timer_get_time()-start;
        start=esp_timer_get_time();
        if(pending && xSemaphoreTake(transfer_done,pdMS_TO_TICKS(1000))!=pdTRUE)goto fail;
        transfer_wait_us+=esp_timer_get_time()-start;
        if(esp_lcd_panel_draw_bitmap(bsp_display_panel(),0,y,320,y+STRIP_ROWS,pixels)!=ESP_OK)goto fail;
        pending=true;
    }
    int64_t start=esp_timer_get_time();
    if(xSemaphoreTake(transfer_done,pdMS_TO_TICKS(1000))!=pdTRUE)goto fail;
    transfer_wait_us+=esp_timer_get_time()-start;
    return true;
fail:
    ESP_LOGE(TAG,"Display transfer failed; stopping before DMA buffer reuse");
    return false;
}

/* Single game owner: input, simulation, drawing and battery polling are serialized.
 * No LVGL task is initialized. Audio runs in its own bounded worker. */
void game_runtime_run(const game_app_t *app)
{
    nvs_handle_t storage=0;
    active_app=app;
    inputs = xQueueCreate(24,sizeof(key_event_t));
    sounds = xQueueCreate(4,sizeof(int));
    transfer_done = xSemaphoreCreateBinary();
    for(int i=0;i<2;++i)strip[i] = heap_caps_malloc(320*STRIP_ROWS*sizeof(uint16_t),MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL);
    if (!inputs || !sounds || !transfer_done || !strip[0] || !strip[1]) {
        ESP_LOGE(TAG,"Not enough memory for game resources");
        goto cleanup;
    }
    if (bsp_display_init() != ESP_OK) goto cleanup;
    if (esp_lcd_panel_swap_xy(bsp_display_panel(),true) != ESP_OK ||
        esp_lcd_panel_mirror(bsp_display_panel(),LANDSCAPE_MIRROR_X,!LANDSCAPE_MIRROR_X) != ESP_OK) goto cleanup;
    esp_lcd_panel_io_callbacks_t callbacks = { .on_color_trans_done = color_done };
    if (esp_lcd_panel_io_register_event_callbacks(bsp_display_io(),&callbacks,NULL) != ESP_OK) goto cleanup;
    app->init((uint32_t)(esp_timer_get_time()/1000));
    bool storage_ready=false;
    if(nvs_flash_init()==ESP_OK && nvs_open(app->name,NVS_READWRITE,&storage)==ESP_OK)storage_ready=true;
    else ESP_LOGW(TAG,"Progress storage unavailable; using session progress");
    game_progress_t saved={0};size_t saved_size=sizeof(saved);
    if(storage_ready && nvs_get_blob(storage,"progress",&saved,&saved_size)==ESP_OK && (saved_size==12||saved_size==sizeof(saved))){
        uint32_t medals=0;if(nvs_get_u32(storage,"medals",&medals)==ESP_OK)saved.medals=medals&1023u;
        app->restore(&saved);
    }
    app->progress(&saved);
    bool battery_ready = bsp_battery_init() == ESP_OK;
    if (battery_ready) app->battery(bsp_battery_soc());
    if (bsp_button_init(button_event,NULL) != ESP_OK) {
        ESP_LOGE(TAG,"Button initialization failed; stopping before play");
        goto cleanup;
    }
    if (xTaskCreate(sound_worker,"game_sound",4096,NULL,2,NULL) != pdPASS)
        ESP_LOGW(TAG,"No audio worker; continuing silently");
    bsp_display_backlight(85);
    int64_t previous = esp_timer_get_time(), next_battery = previous + 5000000;
    int64_t next_probe = previous + 5000000;
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        int64_t now = esp_timer_get_time();
        key_event_t message;
        if(atomic_exchange(&input_lost,false)) {
            xQueueReset(inputs);app->cancel((uint32_t)(now/1000));
            ESP_LOGW(TAG,"Input overflow; paused to prevent stuck holds");
        }
        while (xQueueReceive(inputs,&message,0)==pdTRUE)app->edge(message.key,message.down,message.ms);
        now=esp_timer_get_time();app->update((uint32_t)(now/1000));
        if (battery_ready && now >= next_battery) {
            app->battery(bsp_battery_soc());next_battery=now+5000000;
        }
        game_progress_t current;app->progress(&current);
        if(memcmp(&saved,&current,sizeof(current))!=0){
            if(storage_ready && (nvs_set_blob(storage,"progress",&current,12)!=ESP_OK || nvs_set_u32(storage,"medals",current.medals)!=ESP_OK || nvs_commit(storage)!=ESP_OK)) {
                ESP_LOGW(TAG,"Progress write failed; using session progress");storage_ready=false;
            }
            if(current.muted)xQueueReset(sounds);
            saved=current;
        }
        atomic_store(&music_theme,app->music?app->music():0);
        int effect=app->effect();
        if(effect && !current.muted)xQueueSend(sounds,&effect,0);
        if (!present()) {
            /* Retain in-flight memory/callback forever after timeout: no unsafe cleanup. */
            bsp_display_backlight(0);
            for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
        }
        if (now >= next_probe) {
            ESP_LOGI(TAG,"frame_ms=%lld render_ms=%lld wait_ms=%lld free=%u min=%u largest=%u game=%s",
                (long long)(esp_timer_get_time()-now)/1000,
                (long long)render_us/1000,(long long)transfer_wait_us/1000,
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),app->name);
            next_probe = now+5000000;
        }
        /* A slow frame must still let the idle task run; do not accumulate overdue
         * periodic deadlines and spin indefinitely under a heavy render load. */
        if (xTaskDelayUntil(&wake,pdMS_TO_TICKS(33)) == pdFALSE) {
            vTaskDelay(pdMS_TO_TICKS(1));
            wake = xTaskGetTickCount();
        }
    }
cleanup:
    if(storage)nvs_close(storage);
    for(int i=0;i<2;++i)if(strip[i])heap_caps_free(strip[i]);
    if (inputs) vQueueDelete(inputs);
    if (sounds) vQueueDelete(sounds);
    if (transfer_done) vSemaphoreDelete(transfer_done);
}
