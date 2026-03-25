#include "user_define.h"
#include "tim.h"
static uint32_t last_play_ts = 0;

void play_one_pulse(void)
{
    // 拉高voice引脚
    HAL_GPIO_WritePin(VOICE_PORT, VOICE_PIN, GPIO_PIN_SET);
    // 延时10ms
    delay_100us();
    // 拉低voice引脚
    HAL_GPIO_WritePin(VOICE_PORT, VOICE_PIN, GPIO_PIN_RESET);
    delay_100us();
}

// voice_idx从1开始
void play_voice(uint8_t voice_index)
{

    // 先发一个脉冲复位
    play_one_pulse();

    uint8_t i;
    for (i = 0; i < voice_index; i++)
    {
        play_one_pulse();
    }
}

// voice_idx从1开始
void play_voice_no_repeat(uint8_t voice_index)
{
    static uint8_t last_play = 0;

    if (last_play == voice_index)
    {
        voice_index = (voice_index % 3) + 1; // 避免连续播放同一条
    }
    last_play = voice_index;

    // 先发一个脉冲复位
    play_one_pulse();

    uint8_t i;
    for (i = 0; i < voice_index; i++)
    {
        play_one_pulse();
    }

    // reset_data();
    last_play_ts = HAL_GetTick();
    // HAL_Delay(2000); // 延时10ms，确保语音模块有足够时间处理
}

// 1 别紧张拉（轻捏）
// 2 哇好犀利（轻捏）
// 3 不错哦（轻捏）
// 4 顶住我撑你（中捏）
// 5 快点啦（中捏）
// 6 咩问题（中捏）
// 7 冲鸭（重捏）
// 8 爆发吧小宇宙（重捏）
// 9 爽啊今晚加鸡腿（重捏）
void play_level1(void)
{
    // 生成1-3之间的随机数
    play_voice_no_repeat(rand() % 3 + 1);
}

void play_level2(void)
{
    // 生成4-6之间的随机数
    play_voice_no_repeat(rand() % 3 + 4);
}

void play_level3(void)
{
    // 生成7-9之间的随机数
    play_voice_no_repeat(rand() % 3 + 7);
}

void play_voice_base_result_data(float result)
{

    static uint32_t last_run_ts = 0;
    if (HAL_GetTick() - last_run_ts < 200)
        return; // 200ms内不重复检查

    if (HAL_GetTick() - last_play_ts < 2000)
        return; // 2s内不重复播放

    if (result > LEVEL_1 && result < LEVEL_2)
    {
        uint8_t voice_index = rand() % 3 + 1; // 生成1-3之间的随机数
        play_voice_no_repeat(voice_index);
    }
    else if (result >= LEVEL_2 && result < LEVEL_3)
    {
        uint8_t voice_index = rand() % 3 + 4; // 生成4-6之间的随机数
        play_voice_no_repeat(voice_index);
    }
    else if (result >= LEVEL_3)
    {
        uint8_t voice_index = rand() % 3 + 7; // 生成7-9之间的随机数
        play_voice_no_repeat(voice_index);
    }
}

void play_voice_base_g(float combin_acc)
{
    static uint32_t last_play_ts = 0;

    if (HAL_GetTick() - last_play_ts < 2000)
        return; // 2s内不重复播放

    if (combin_acc > G_THRESHOLD)
    {
        play_voice(8);
        last_play_ts = HAL_GetTick();
    }
}

void play_voice_base_wave_detector(wave_detector_t wd)
{

    static uint32_t last_run_ts = 0;

    if (wd.event_flag)
    {

        if (wd.event_peak <  LEVEL_2)
        {
            play_level1();
        }
        else if (wd.event_peak < LEVEL_3)
        {
            play_level2();
        }
        else
        {
            play_level3();
        }

        last_run_ts = HAL_GetTick();
    }
}
