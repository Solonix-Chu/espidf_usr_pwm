/**
 * @file usr_pwm_example.c
 * @brief usr_pwm模块使用示例
 */

#include "usr_pwm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "USR_PWM_EXAMPLE";

void app_main_pwm_test(void)
{
    // 定义PWM通道配置
    usr_pwm_channel_config_t pwm_configs[] = {
        {
            .gpio_num = 18,                // GPIO18
            .channel = LEDC_CHANNEL_0,     // 通道0
            .timer = LEDC_TIMER_0,         // 定时器0
            .freq_hz = 5000,               // 5kHz
            .duty_resolution = LEDC_TIMER_13_BIT, // 13位分辨率
            .speed_mode = LEDC_LOW_SPEED_MODE,    // 低速模式
        },
        {
            .gpio_num = 19,                // GPIO19
            .channel = LEDC_CHANNEL_1,     // 通道1
            .timer = LEDC_TIMER_0,         // 定时器0 (共享定时器)
            .freq_hz = 5000,               // 5kHz
            .duty_resolution = LEDC_TIMER_13_BIT, // 13位分辨率
            .speed_mode = LEDC_LOW_SPEED_MODE,    // 低速模式
        },
    };
    
    // 初始化PWM模块
    usr_pwm_handle_t pwm_handle = NULL;
    esp_err_t ret = usr_pwm_init(pwm_configs, sizeof(pwm_configs) / sizeof(pwm_configs[0]), &pwm_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PWM初始化失败: %s", esp_err_to_name(ret));
        return;
    }
    
    // 启动PWM通道
    ret = usr_pwm_start(pwm_handle, LEDC_CHANNEL_0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启动通道0失败: %s", esp_err_to_name(ret));
    }
    
    ret = usr_pwm_start(pwm_handle, LEDC_CHANNEL_1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启动通道1失败: %s", esp_err_to_name(ret));
    }
    
    // 呼吸灯效果
    ESP_LOGI(TAG, "开始呼吸灯效果演示...");
    float duty = 0.0;
    bool increasing = true;
    
    for (int i = 0; i < 100; i++) {
        // 更新占空比
        if (increasing) {
            duty += 1.0;
            if (duty >= 100.0) {
                duty = 100.0;
                increasing = false;
            }
        } else {
            duty -= 1.0;
            if (duty <= 0.0) {
                duty = 0.0;
                increasing = true;
            }
        }
        
        // 设置通道0占空比
        ret = usr_pwm_set_duty_percent(pwm_handle, LEDC_CHANNEL_0, duty);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "设置通道0占空比失败: %s", esp_err_to_name(ret));
        }
        
        // 设置通道1占空比 (反向)
        ret = usr_pwm_set_duty_percent(pwm_handle, LEDC_CHANNEL_1, 100.0 - duty);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "设置通道1占空比失败: %s", esp_err_to_name(ret));
        }
        
        // 延时
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    // 改变频率
    ESP_LOGI(TAG, "改变PWM频率...");
    ret = usr_pwm_set_freq(pwm_handle, LEDC_TIMER_0, 1000); // 改为1kHz
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置频率失败: %s", esp_err_to_name(ret));
    }
    
    // 再次演示
    for (int i = 0; i < 50; i++) {
        // 更新占空比
        if (increasing) {
            duty += 2.0;
            if (duty >= 100.0) {
                duty = 100.0;
                increasing = false;
            }
        } else {
            duty -= 2.0;
            if (duty <= 0.0) {
                duty = 0.0;
                increasing = true;
            }
        }
        
        // 设置通道0占空比
        usr_pwm_set_duty_percent(pwm_handle, LEDC_CHANNEL_0, duty);
        
        // 设置通道1占空比 (反向)
        usr_pwm_set_duty_percent(pwm_handle, LEDC_CHANNEL_1, 100.0 - duty);
        
        // 延时
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // 停止PWM输出
    ESP_LOGI(TAG, "停止PWM输出...");
    ret = usr_pwm_stop(pwm_handle, LEDC_CHANNEL_0, 0); // 停止时输出低电平
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "停止通道0失败: %s", esp_err_to_name(ret));
    }
    
    ret = usr_pwm_stop(pwm_handle, LEDC_CHANNEL_1, 1); // 停止时输出高电平
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "停止通道1失败: %s", esp_err_to_name(ret));
    }
    
    // 释放PWM资源
    ESP_LOGI(TAG, "释放PWM资源...");
    ret = usr_pwm_deinit(pwm_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "释放PWM资源失败: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "PWM示例完成");
} 