#include "arithmetic.h"
#include <math.h>

#define MATH_FLAG 0

#if MATH_FLAG
// 滑动平均滤波
volatile float avg_data[16][9] = {{0}};                                 // 存储每个通道的数据
volatile int avg_index[16] = {0};                                       // 存储每个通道的当前索引
volatile float avg_sum[16] = {0};                                       // 存储每个通道的当前总和
#endif

#if 0
// 滑动中值滤波
volatile float mid_data[4][9] = {{0}};                                  // 存储每个通道的数据
volatile int mid_index[4] = {0};                                        // 存储每个通道的当前索引
volatile float mid_sum[4] = {0};                                        // 存储每个通道的当前总和

// 一阶低通滤波
volatile float pre_low_value[4] = {0};

// 一阶高通滤波
volatile float pre_high_sig[4]   = {0};
volatile float pre_high_value[4] = {0};

// 限值滤波
#endif

// 在文件开头添加新的常量定义
#define MAX_CHANGE_RATE 5.0f // 最大变化率

#if MATH_FLAG
/*
算法一：滑动平均滤波
方法：连续取N个采样值进行算术平均运算：（ N值的选取：一般流量，N=12；压力：N=4。）
    N值较大时：信号平滑度较高，但灵敏度较低；
    N值较小时：信号平滑度较低，但灵敏度较高；     
优点：适用于对一般具有随机干扰的信号进行滤波；这种信号的特点是有一个平均值，信号在某一数值范围附近上下波动
缺点：对于测量速度较慢或要求数据计算速度较快的实时控制不适用，比较浪费RAM。
*/
float averageFilter(float in_data, int channel) 
{
    // 获取上一次的值（如果是第一个数据，就使用输入值）
    float prev_value = (avg_index[channel] > 0) ? 
                        avg_data[channel][(avg_index[channel] - 1 + 9) % 9] : 
                        in_data;
    
    // 限幅滤波
    float filtered_data;
    float delta = fabs(in_data - prev_value);
    if (delta > MAX_CHANGE_RATE) {
        // 如果变化太大，使用限幅后的值
        filtered_data = (in_data > prev_value) ? 
                        prev_value + MAX_CHANGE_RATE : 
                        prev_value - MAX_CHANGE_RATE;
    } else {
        filtered_data = in_data;
    }
    
    // 滑动平均滤波
    avg_sum[channel] -= avg_data[channel][avg_index[channel]];         // 从总和中减去被替换的数据
    avg_data[channel][avg_index[channel]] = filtered_data;             // 添加限幅后的新数据
    avg_sum[channel] += avg_data[channel][avg_index[channel]];         // 将新数据加到总和中
    avg_index[channel] = (avg_index[channel] + 1) % 9;                 // 更新索引，循环使用数组
    
    return avg_sum[channel] / 9;                                       // 计算并返回平均值
}
#endif

#if 0
// 加权平均
float weightedAverageFilter(float in_data, int channel)
{
    static const float weights[9] = {0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0}; // 权重数组, 可根据需求调整
    float weighted_sum = 0.0;
    float total_weight = 0.0;
    
    // 从总和中减去被替换的数据
    avg_sum[channel] -= avg_data[channel][avg_index[channel]];              
    
    // 将新数据存入avg_data数组
    avg_data[channel][avg_index[channel]] = in_data;                        

    // 计算加权和
    for (int i = 0; i < 9; i++) 
    {
        int index = (avg_index[channel] + i) % 9;  // 循环使用数组
        weighted_sum += avg_data[channel][index] * weights[i];  // 数据乘以权重
        total_weight += weights[i];  // 计算权重和
    }
    
    // 更新索引
    avg_index[channel] = (avg_index[channel] + 1) % 9;                     
    
    return weighted_sum / total_weight;  // 返回加权平均值
}

/*
算法二：滑动中值滤波
方法： 连续采样N次（N取奇数），把N次采样值按大小排列，取中间值为本次有效值。
优点：克服偶然因素（对温度、液位的变化缓慢的被测参数有良好的滤波效果）
缺点：对流量、速度等快速变化的参数不宜
*/
float middleFilter(float in_data, int channel) 
{
    mid_sum[channel] -= mid_data[channel][mid_index[channel]];              // 从总和中减去被替换的数据
    mid_data[channel][mid_index[channel]] = in_data;                        // 添加新数据
    mid_sum[channel] += mid_data[channel][mid_index[channel]];              // 将新数据加到总和中
    
    mid_index[channel] = (mid_index[channel] + 1) % 9;                      // 更新索引，循环使用数组
    
    // 复制数据到临时数组用于排序
    float temp[9];
    for (int i = 0; i < 9; i++) 
    {
        temp[i] = mid_data[channel][i];
    }
    
    // 冒泡排序
    for (int i = 0; i < 9 - 1; i++) 
    {
        for (int j = 0; j < 9 - 1 - i; j++) 
        {
            if (temp[j] > temp[j + 1]) 
            {
                float tmp = temp[j];
                temp[j] = temp[j + 1];
                temp[j + 1] = tmp;
            }
        }
    }
    
    if(mid_index[channel] < 8)
    {
        return in_data;   // 返回原数据
    } else {
        return temp[5];   // 计算并返回中值
    }
}


/*
算法三：一阶低通滤波
* x：当前输入信号
* prev_y：上一次的输出（滤波后的值）
* alpha：滤波系数
*/
float low_pass_filter(float x, float prev_y, float alpha) 
{
    float y = alpha * x + (1.0f - alpha) * prev_y;
    return y;
}


/*
算法四：一阶高通滤波
* x：当前信号
* prev_x：前一次的输入信号
* prev_y：上一次的输出（滤波后的值）
* alpha ：滤波系数
*/
float high_pass_filter(float x, float prev_x, float prev_y, float alpha) 
{
    float y = alpha * (prev_y + x - prev_x);
    return y;
}


/*
算法五：限幅滤波法
方法：根据经验判断，确定两次采样允许的最大偏差值（设为A），每次检测到新值时判断：
      如果本次值与上次值之差<=A，则本次值有效，
      如果本次值与上次值之差>A，则本次值无效，放弃本次值，用上次值代替本次值。
优点：能克服偶然因素引起的脉冲干扰
缺点：无法抑制周期性的干扰，平滑度差
*/
float filter1(float in_data, int channel) 
{
    float max_gap = 20.110101;
    
    mid_sum[channel] -= mid_data[channel][mid_index[channel]];              // 从总和中减去被替换的数据
    mid_data[channel][mid_index[channel]] = in_data;                        // 添加新数据
    mid_sum[channel] += mid_data[channel][mid_index[channel]];              // 将新数据加到总和中
    
    float now_value = mid_data[channel][mid_index[channel]];                // 保存当前值
    float prev_value = mid_data[channel][(mid_index[channel] + 1) % 2];     // 保存前一个值
    
    mid_index[channel] = (mid_index[channel] + 1) % 2;                      // 更新索引，循环使用数组
    
    // 判断是否超出允许的最大差值
    if(fabs(now_value - prev_value) > max_gap)
    {
        return prev_value;
    } 
    else 
    {
        return in_data;
    }
}


/*
算法六：卡尔曼滤波
方法：
优点：
缺点：
参数：
float LastP;    // 上次估算协方差 初始化值为0.02
float Now_P;    // 当前估算协方差 初始化值为0
float out;      // 卡尔曼滤波器输出 初始化值为0
float Kg;       // 卡尔曼增益 初始化值为0
float Q;        // 过程噪声协方差 初始化值为0.001
float R;        // 观测噪声协方差 初始化值为0.543
*/
KFP KFP_value = {0.02,0,0,0,0.001,0.543};

float kalmanFilter(KFP *kfp, float input)
{
    // 预测协方差方程：k时刻系统估算协方差 = k-1时刻的系统协方差 + 过程噪声协方差
    kfp->Now_P = kfp->LastP + kfp->Q;
    
    // 卡尔曼增益方程：卡尔曼增益 = k时刻系统估算协方差 / （k时刻系统估算协方差 + 观测噪声协方差）
    kfp->Kg = kfp->Now_P / (kfp->Now_P + kfp->R);
    
    // 更新最优值方程：k时刻状态变量的最优值 = 状态变量的预测值 + 卡尔曼增益 * （测量值 - 状态变量的预测值）
    kfp->out = kfp->out + kfp->Kg * (input - kfp->out);  // 计算滤波后的值
    
    // 更新协方差方程：本次的系统协方差更新为下一次运算准备。
    kfp->LastP = (1 - kfp->Kg) * kfp->Now_P;
    
    return kfp->out;  // 返回滤波后的值
}

#endif


















