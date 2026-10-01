#include "rr_render.h"
#include "game_audio.h"
#include "rr_controls.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "bsp_audio.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define STRIP_ROWS 40
/* Swap mirror axes if the physical board is held the opposite way up.
 * Landscape direction and UP/DOWN placement require actual device acceptance. */
#define LANDSCAPE_MIRROR_X true
static const char *TAG = "dusk_riders";
enum { ACTION_DOWN=5, ACTION_UP=6 };
typedef struct { int input; uint32_t ms; } key_event_t;
static QueueHandle_t inputs, sounds;
static rr_action_key_t action_key;
static SemaphoreHandle_t transfer_done;
static rr_game_t game;
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
    int input = -1;
    if (event == BSP_BTN_PRESS) input = key == BSP_BTN_DOWN ? ACTION_DOWN : key == BSP_BTN_UP ? RR_LEFT : RR_RIGHT;
    else if (event == BSP_BTN_RELEASE && key == BSP_BTN_DOWN) input = ACTION_UP;
    else if (event == BSP_BTN_LONG && key == BSP_BTN_UP) input = RR_HOME;
    if (input >= 0) {
        key_event_t message={input,(uint32_t)(esp_timer_get_time()/1000)};
        xQueueSend(inputs, &message, 0);
    }
}

/* Optional audio worker owns all codec writes. Failure degrades to silent play. */
static void sound_worker(void *arg)
{
    (void)arg;
    bool ready = bsp_audio_init() == ESP_OK && bsp_audio_set_format(GAME_AUDIO_HZ,16,1) == ESP_OK;
    if (ready) bsp_audio_set_volume(25);
    else ESP_LOGW(TAG,"Audio unavailable; continuing silently");
    int16_t pcm[160];
    for (;;) {
        int effect;
        if (xQueueReceive(sounds,&effect,portMAX_DELAY) != pdTRUE || !ready) continue;
        for (unsigned start = 0; start < GAME_AUDIO_SAMPLES; start += 160) {
            for (unsigned i = 0; i < 160; ++i) pcm[i] = game_audio_sample(effect,start+i);
            if (bsp_audio_write(pcm,sizeof(pcm)) != ESP_OK) {
                ready = false; ESP_LOGW(TAG,"Audio write failed; continuing silently"); break;
            }
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
    for (int y = 0, index=0; y < RR_HEIGHT; y += STRIP_ROWS,index^=1) {
        uint16_t *pixels=strip[index];
        int64_t start=esp_timer_get_time();
        rr_render_strip(&game,pixels,y,STRIP_ROWS);
        for (int i = 0; i < RR_WIDTH*STRIP_ROWS; ++i)
            pixels[i] = (uint16_t)((pixels[i] << 8) | (pixels[i] >> 8));
        render_us+=esp_timer_get_time()-start;
        start=esp_timer_get_time();
        if(pending && xSemaphoreTake(transfer_done,pdMS_TO_TICKS(1000))!=pdTRUE)goto fail;
        transfer_wait_us+=esp_timer_get_time()-start;
        if(esp_lcd_panel_draw_bitmap(bsp_display_panel(),0,y,RR_WIDTH,y+STRIP_ROWS,pixels)!=ESP_OK)goto fail;
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
void rr_device_run(void)
{
    inputs = xQueueCreate(12,sizeof(key_event_t));
    sounds = xQueueCreate(4,sizeof(int));
    transfer_done = xSemaphoreCreateBinary();
    for(int i=0;i<2;++i)strip[i] = heap_caps_malloc(RR_WIDTH*STRIP_ROWS*sizeof(uint16_t),MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL);
    if (!inputs || !sounds || !transfer_done || !strip[0] || !strip[1]) {
        ESP_LOGE(TAG,"Not enough memory for game resources");
        goto cleanup;
    }
    if (bsp_display_init() != ESP_OK) goto cleanup;
    if (esp_lcd_panel_swap_xy(bsp_display_panel(),true) != ESP_OK ||
        esp_lcd_panel_mirror(bsp_display_panel(),LANDSCAPE_MIRROR_X,!LANDSCAPE_MIRROR_X) != ESP_OK) goto cleanup;
    esp_lcd_panel_io_callbacks_t callbacks = { .on_color_trans_done = color_done };
    if (esp_lcd_panel_io_register_event_callbacks(bsp_display_io(),&callbacks,NULL) != ESP_OK) goto cleanup;
    rr_init(&game,0xD057u);
    bool battery_ready = bsp_battery_init() == ESP_OK;
    if (battery_ready) game.battery = bsp_battery_soc();
    if (bsp_button_init(button_event,NULL) != ESP_OK) {
        ESP_LOGE(TAG,"Button initialization failed; stopping before play");
        goto cleanup;
    }
    if (xTaskCreate(sound_worker,"ride_sound",4096,NULL,2,NULL) != pdPASS)
        ESP_LOGW(TAG,"No audio worker; continuing silently");
    bsp_display_backlight(85);
    int64_t previous = esp_timer_get_time(), next_battery = previous + 5000000;
    int64_t next_probe = previous + 5000000;
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        int health = game.health, attack = game.attack_ms;
        rr_phase_t phase = game.phase;
        key_event_t message;
        while (xQueueReceive(inputs,&message,0) == pdTRUE) {
            int input=message.input;
            if (input==ACTION_DOWN || input==ACTION_UP)
                input=rr_action_edge(&action_key,input==ACTION_DOWN,message.ms);
            if (input<0) continue;
            if (input == RR_HOME && game.phase == RR_RACING) continue;
            rr_input(&game,(rr_input_t)input);
        }
        int64_t now = esp_timer_get_time();
        int held_input=rr_action_poll(&action_key,(uint32_t)(now/1000));
        if (held_input>=0) rr_input(&game,(rr_input_t)held_input);
        rr_tick(&game,(uint32_t)((now-previous)/1000)); previous = now;
        if (battery_ready && now >= next_battery) {
            game.battery = bsp_battery_soc(); next_battery = now+5000000;
        }
        int effect = game.health < health ? 2 : game.attack_ms > attack ? 1 :
                     (phase == RR_TITLE && game.phase == RR_RACING) ||
                     (phase == RR_RACING && game.phase == RR_FINISHED) ? 3 : 0;
        if (effect) xQueueSend(sounds,&effect,0);
        if (!present()) {
            /* Retain in-flight memory/callback forever after timeout: no unsafe cleanup. */
            bsp_display_backlight(0);
            for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
        }
        if (now >= next_probe) {
            ESP_LOGI(TAG,"frame_ms=%lld render_ms=%lld wait_ms=%lld free=%u min=%u largest=%u phase=%d stage=%d",
                (long long)(esp_timer_get_time()-now)/1000,
                (long long)render_us/1000,(long long)transfer_wait_us/1000,
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),game.phase,game.stage);
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
    for(int i=0;i<2;++i)if(strip[i])heap_caps_free(strip[i]);
    if (inputs) vQueueDelete(inputs);
    if (sounds) vQueueDelete(sounds);
    if (transfer_done) vSemaphoreDelete(transfer_done);
}
