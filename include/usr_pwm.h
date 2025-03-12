/**
 * @file usr_pwm.h
 * @brief 用户PWM模块头文件
 */

#ifndef USR_PWM_H
#define USR_PWM_H

#include "driver/ledc.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PWM通道配置结构体
 */
typedef struct {
    uint8_t gpio_num;          /*!< GPIO编号 */
    ledc_channel_t channel;    /*!< LEDC通道 */
    ledc_timer_t timer;        /*!< LEDC定时器 */
    uint32_t freq_hz;          /*!< PWM频率(Hz) */
    ledc_timer_bit_t duty_resolution; /*!< 占空比分辨率 */
    ledc_mode_t speed_mode;    /*!< 速度模式 */
} usr_pwm_channel_config_t;

/**
 * @brief PWM模块句柄
 */
typedef struct usr_pwm_handle_s* usr_pwm_handle_t;

/**
 * @brief 初始化PWM模块
 * 
 * @param[in] channel_configs PWM通道配置数组
 * @param[in] channel_num 通道数量
 * @param[out] handle 返回的PWM句柄
 * @return esp_err_t 
 */
esp_err_t usr_pwm_init(usr_pwm_channel_config_t *channel_configs, uint8_t channel_num, usr_pwm_handle_t *handle);

/**
 * @brief 设置PWM占空比
 * 
 * @param[in] handle PWM句柄
 * @param[in] channel LEDC通道
 * @param[in] duty_percent 占空比百分比(0.0-100.0)
 * @return esp_err_t 
 */
esp_err_t usr_pwm_set_duty_percent(usr_pwm_handle_t handle, ledc_channel_t channel, float duty_percent);

/**
 * @brief 设置PWM频率
 * 
 * @param[in] handle PWM句柄
 * @param[in] timer LEDC定时器
 * @param[in] freq_hz 频率(Hz)
 * @return esp_err_t 
 */
esp_err_t usr_pwm_set_freq(usr_pwm_handle_t handle, ledc_timer_t timer, uint32_t freq_hz);

/**
 * @brief 启动PWM输出
 * 
 * @param[in] handle PWM句柄
 * @param[in] channel LEDC通道
 * @return esp_err_t 
 */
esp_err_t usr_pwm_start(usr_pwm_handle_t handle, ledc_channel_t channel);

/**
 * @brief 停止PWM输出
 * 
 * @param[in] handle PWM句柄
 * @param[in] channel LEDC通道
 * @param[in] idle_level 停止时的电平状态
 * @return esp_err_t 
 */
esp_err_t usr_pwm_stop(usr_pwm_handle_t handle, ledc_channel_t channel, uint32_t idle_level);

/**
 * @brief 反初始化PWM模块
 * 
 * @param[in] handle PWM句柄
 * @return esp_err_t 
 */
esp_err_t usr_pwm_deinit(usr_pwm_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* USR_PWM_H */ 