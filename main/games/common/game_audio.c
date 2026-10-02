#include "game_audio.h"
/* Deterministic phase-integrated oscillators, attack/release envelopes and
 * short melodic cues. No allocations, file decoding or floating-point DSP. */
static int tone(unsigned sample,unsigned frequency)
{
    unsigned p=(sample*frequency*256u/GAME_AUDIO_HZ)&255u;
    return p<128?(int)p*2-127:383-(int)p*2;
}
int16_t game_audio_sample(int effect,unsigned sample)
{
    if(sample>=GAME_AUDIO_SAMPLES||effect<1||effect>7)return 0;
    unsigned local=sample,frequency=440,length=GAME_AUDIO_SAMPLES;
    if(effect==3||effect==5||effect==7){
        static const unsigned notes[3][4]={{523,659,784,1047},{784,988,1175,1568},{392,523,659,784}};
        length=800;local=sample%length;
        frequency=notes[effect==3?0:effect==5?1:2][sample/length];
    }else if(effect==4){frequency=1047;length=1600;local=sample%length;}
    else if(effect==6)frequency=330;
    else if(effect==1)frequency=660;
    else frequency=110;
    int envelope=(int)(local<80?local*4096/80:(length-1-local)*4096/(length-80));
    int wave=tone(sample,frequency);
    if(effect==2){uint32_t n=sample*747796405u+2891336453u;n^=n>>13;wave=(wave*2+(int)(n&255)-128)/3;}
    int amplitude=effect==2?4200:3000;
    return (int16_t)(wave*envelope/4096*amplitude/128);
}

int16_t game_music_sample(int theme,uint32_t sample)
{
    if(theme<1||theme>5)return 0;
    static const uint16_t melody[5][16]={
        {330,330,392,440,330,330,494,440,330,392,440,523,494,440,392,294},
        {523,659,784,659,587,698,880,698,659,784,988,784,587,698,784,0},
        {294,349,392,0,440,392,349,294,262,294,349,0,392,349,294,0},
        {523,0,659,784,523,0,784,988,587,0,698,880,659,784,988,0},
        {392,494,587,784,659,587,494,392,440,523,659,880,784,659,523,440}
    };
    static const unsigned beat[5]={2400,4800,4000,3200,2800};
    unsigned duration=beat[theme-1],local=sample%duration,index=sample/duration%16;
    unsigned frequency=melody[theme-1][index];
    int envelope=(int)(local<160?local*1024/160:(duration-1-local)*1024/(duration-160));
    int lead=frequency?tone(local,frequency)*envelope/1024*650/128:0;
    int bass=tone(local,index<8?131:147)*envelope/1024*220/128;
    return (int16_t)(lead+bass);
}
