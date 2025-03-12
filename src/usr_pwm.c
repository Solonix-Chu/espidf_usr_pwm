/**
 * @file usr_pwm.c
 * @brief 用户PWM模块实现
 */

#include "usr_pwm.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "USR_PWM";

// 全局变量，用于跟踪LEDC定时器的初始化状态
static bool g_timer_initialized[LEDC_TIMER_MAX] = {false};
static ledc_timer_config_t g_timer_configs[LEDC_TIMER_MAX];
static int g_pwm_ref_count = 0;  // 引用计数，用于跟踪使用PWM的模块数量

/**
 * @brief PWM模块句柄结构体
 */
struct usr_pwm_handle_s {
    usr_pwm_channel_config_t *channel_configs; /*!< 通道配置数组 */
    uint8_t channel_num;                      /*!< 通道数量 */
    bool *channel_started;                    /*!< 通道启动状态数组 */
};

esp_err_t usr_pwm_init(usr_pwm_channel_config_t *channel_configs, uint8_t channel_num, usr_pwm_handle_t *handle)
{
    if (channel_configs == NULL || channel_num == 0 || handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 分配句柄内存
    struct usr_pwm_handle_s *pwm_handle = calloc(1, sizeof(struct usr_pwm_handle_s));
    if (pwm_handle == NULL) {
        ESP_LOGE(TAG, "分配内存失败");
        return ESP_ERR_NO_MEM;
    }

    // 保存通道配置
    pwm_handle->channel_configs = malloc(channel_num * sizeof(usr_pwm_channel_config_t));
    if (pwm_handle->channel_configs == NULL) {
        ESP_LOGE(TAG, "分配通道配置内存失败");
        free(pwm_handle);
        return ESP_ERR_NO_MEM;
    }
    memcpy(pwm_handle->channel_configs, channel_configs, channel_num * sizeof(usr_pwm_channel_config_t));
    pwm_handle->channel_num = channel_num;

    // 分配通道启动状态数组
    pwm_handle->channel_started = calloc(channel_num, sizeof(bool));
    if (pwm_handle->channel_started == NULL) {
        ESP_LOGE(TAG, "分配通道状态内存失败");
        free(pwm_handle->channel_configs);
        free(pwm_handle);
        return ESP_ERR_NO_MEM;
    }

    // 初始化LEDC定时器和通道
    for (int i = 0; i < channel_num; i++) {
        ledc_timer_t timer = channel_configs[i].timer;
        
        // 检查定时器是否已初始化
        if (!g_timer_initialized[timer]) {
            // 配置LEDC定时器
            ledc_timer_config_t timer_config = {
                .speed_mode = channel_configs[i].speed_mode,
                .timer_num = timer,
                .duty_resolution = channel_configs[i].duty_resolution,
                .freq_hz = channel_configs[i].freq_hz,
                .clk_cfg = LEDC_AUTO_CLK
            };
            
            esp_err_t ret = ledc_timer_config(&timer_config);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "配置LEDC定时器失败: %s", esp_err_to_name(ret));
                free(pwm_handle->channel_started);
                free(pwm_handle->channel_configs);
                free(pwm_handle);
                return ret;
            }
            
            // 保存定时器配置
            memcpy(&g_timer_configs[timer], &timer_config, sizeof(ledc_timer_config_t));
            g_timer_initialized[timer] = true;
            ESP_LOGI(TAG, "LEDC定时器%d初始化成功，频率: %lu Hz", timer, channel_configs[i].freq_hz);
        } else {
            // 检查频率是否需要更新
            if (g_timer_configs[timer].freq_hz != channel_configs[i].freq_hz) {
                ESP_LOGW(TAG, "定时器%d已被初始化为%lu Hz，但当前请求%lu Hz", 
                         timer, g_timer_configs[timer].freq_hz, channel_configs[i].freq_hz);
                
                // 更新频率
                esp_err_t ret = ledc_set_freq(channel_configs[i].speed_mode, timer, channel_configs[i].freq_hz);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "更新LEDC频率失败: %s", esp_err_to_name(ret));
                    // 继续初始化，不返回错误
                } else {
                    g_timer_configs[timer].freq_hz = channel_configs[i].freq_hz;
                    ESP_LOGI(TAG, "LEDC定时器%d频率更新为: %lu Hz", timer, channel_configs[i].freq_hz);
                }
            }
        }
        
        // 配置LEDC通道
        ledc_channel_config_t channel_config = {
            .speed_mode = channel_configs[i].speed_mode,
            .channel = channel_configs[i].channel,
            .timer_sel = channel_configs[i].timer,
            .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = channel_configs[i].gpio_num,
            .duty = 0, // 初始占空比为0
            .hpoint = 0
        };
        
        esp_err_t ret = ledc_channel_config(&channel_config);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "配置LEDC通道失败: %s", esp_err_to_name(ret));
            free(pwm_handle->channel_started);
            free(pwm_handle->channel_configs);
            free(pwm_handle);
            return ret;
        }
        
        ESP_LOGI(TAG, "LEDC通道%d初始化成功，GPIO: %d", channel_configs[i].channel, channel_configs[i].gpio_num);
    }

    // 增加引用计数
    g_pwm_ref_count++;
    
    *handle = pwm_handle;
    ESP_LOGI(TAG, "PWM模块初始化成功，当前引用计数: %d", g_pwm_ref_count);
    return ESP_OK;
}

esp_err_t usr_pwm_set_duty_percent(usr_pwm_handle_t handle, ledc_channel_t channel, float duty_percent)
{
    if (handle == NULL || duty_percent < 0.0 || duty_percent > 100.0) {
        return ESP_ERR_INVALID_ARG;
    }

    // 查找通道配置
    ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer_t timer = LEDC_TIMER_0;
    ledc_timer_bit_t duty_resolution = LEDC_TIMER_13_BIT;
    bool found = false;
    
    for (int i = 0; i < handle->channel_num; i++) {
        if (handle->channel_configs[i].channel == channel) {
            speed_mode = handle->channel_configs[i].speed_mode;
            timer = handle->channel_configs[i].timer;
            duty_resolution = handle->channel_configs[i].duty_resolution;
            found = true;
            break;
        }
    }
    
    if (!found) {
        ESP_LOGE(TAG, "未找到通道%d的配置", channel);
        return ESP_ERR_NOT_FOUND;
    }

    // 计算占空比值
    uint32_t max_duty = (1 << duty_resolution) - 1;
    uint32_t duty = (uint32_t)((duty_percent / 100.0) * max_duty);
    
    // 设置占空比
    esp_err_t ret = ledc_set_duty(speed_mode, channel, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置占空比失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 更新占空比
    ret = ledc_update_duty(speed_mode, channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "更新占空比失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "通道%d占空比设置为%.2f%%", channel, duty_percent);
    return ESP_OK;
}

esp_err_t usr_pwm_set_freq(usr_pwm_handle_t handle, ledc_timer_t timer, uint32_t freq_hz)
{
    if (handle == NULL || freq_hz == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // 查找定时器配置
    ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;
    bool found = false;
    
    for (int i = 0; i < handle->channel_num; i++) {
        if (handle->channel_configs[i].timer == timer) {
            speed_mode = handle->channel_configs[i].speed_mode;
            found = true;
            break;
        }
    }
    
    if (!found) {
        ESP_LOGE(TAG, "未找到定时器配置");
        return ESP_ERR_NOT_FOUND;
    }

    // 设置频率
    uint32_t real_freq = ledc_set_freq(speed_mode, timer, freq_hz);
    if (real_freq == 0) {
        ESP_LOGE(TAG, "设置频率失败");
        return ESP_FAIL;
    }
    
    // 更新全局定时器配置
    if (g_timer_initialized[timer]) {
        g_timer_configs[timer].freq_hz = real_freq;
    }
    
    ESP_LOGI(TAG, "定时器%d频率设置为%lu画 Hz (实际: %lu Hz)", timer, freq_hz, real_freq);
    return ESP_OK;
}

esp_err_t usr_pwm_start(usr_pwm_handle_t handle, ledc_channel_t channel)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 查找通道配置
    int channel_index = -1;
    for (int i = 0; i < handle->channel_num; i++) {
        if (handle->channel_configs[i].channel == channel) {
            channel_index = i;
            break;
        }
    }
    
    if (channel_index == -1) {
        ESP_LOGE(TAG, "未找到通道%d的配置", channel);
        return ESP_ERR_NOT_FOUND;
    }

    // 标记通道为已启动
    handle->channel_started[channel_index] = true;
    
    ESP_LOGI(TAG, "通道%d已启动", channel);
    return ESP_OK;
}

esp_err_t usr_pwm_stop(usr_pwm_handle_t handle, ledc_channel_t channel, uint32_t idle_level)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 查找通道配置
    ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;
    int channel_index = -1;
    
    for (int i = 0; i < handle->channel_num; i++) {
        if (handle->channel_configs[i].channel == channel) {
            speed_mode = handle->channel_configs[i].speed_mode;
            channel_index = i;
            break;
        }
    }
    
    if (channel_index == -1) {
        ESP_LOGE(TAG, "未找到通道%d的配置", channel);
        return ESP_ERR_NOT_FOUND;
    }

    // 停止PWM输出
    esp_err_t ret = ledc_stop(speed_mode, channel, idle_level);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "停止PWM输出失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 标记通道为已停止
    handle->channel_started[channel_index] = false;
    
    ESP_LOGI(TAG, "通道%d已停止，空闲电平: %lu", channel, idle_level);
    return ESP_OK;
}

esp_err_t usr_pwm_deinit(usr_pwm_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 停止所有通道
    for (int i = 0; i < handle->channel_num; i++) {
        if (handle->channel_started[i]) {
            ledc_mode_t speed_mode = handle->channel_configs[i].speed_mode;
            ledc_channel_t channel = handle->channel_configs[i].channel;
            
            esp_err_t ret = ledc_stop(speed_mode, channel, 0);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "停止通道%d失败: %s", channel, esp_err_to_name(ret));
                // 继续释放其他资源，不返回错误
            }
        }
    }

    // 减少引用计数
    g_pwm_ref_count--;
    ESP_LOGI(TAG, "PWM模块释放，当前引用计数: %d", g_pwm_ref_count);

    // 只有当引用计数为0时才释放定时器资源
    if (g_pwm_ref_count == 0) {
        // 重置定时器初始化状态
        for (int i = 0; i < LEDC_TIMER_MAX; i++) {
            g_timer_initialized[i] = false;
        }
        ESP_LOGI(TAG, "所有LEDC定时器资源已释放");
    }

    // 释放内存
    free(handle->channel_started);
    free(handle->channel_configs);
    free(handle);

    ESP_LOGI(TAG, "PWM模块句柄已释放");
    return ESP_OK;
} 