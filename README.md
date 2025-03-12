 # usr_pwm 模块使用说明

## 简介

`usr_pwm` 是一个基于ESP-IDF的PWM（脉冲宽度调制）模块封装，提供简单易用的PWM控制接口。该模块基于ESP-IDF的LEDC驱动，支持ESP32系列芯片的PWM功能，可以同时控制多个PWM通道，并支持多个模块共享PWM资源。

## 功能特点

- 支持ESP32的多个PWM通道
- 支持设置PWM频率和占空比
- 支持启动和停止PWM输出
- 提供简单的初始化和反初始化接口
- 支持多模块共享PWM资源，避免重复初始化
- 支持设置停止时的空闲电平

## 文件结构

- `include/usr_pwm.h`: 模块头文件，定义接口和数据结构
- `src/usr_pwm.c`: 模块实现文件
- `src/usr_pwm_example.c`: 使用示例

## 安装方法

1. 将`usr_pwm`目录复制到您的ESP-IDF项目的`components`目录下
2. 在项目的`CMakeLists.txt`中添加对`usr_pwm`组件的依赖：
   ```cmake
   REQUIRES usr_pwm
   ```

## API说明

### 数据结构

#### usr_pwm_channel_config_t

PWM通道配置结构体，用于配置PWM通道的参数。

```c
typedef struct {
    uint8_t gpio_num;          // GPIO编号
    ledc_channel_t channel;    // LEDC通道
    ledc_timer_t timer;        // LEDC定时器
    uint32_t freq_hz;          // PWM频率(Hz)
    ledc_timer_bit_t duty_resolution; // 占空比分辨率
    ledc_mode_t speed_mode;    // 速度模式
} usr_pwm_channel_config_t;
```

#### usr_pwm_handle_t

PWM模块句柄，用于操作PWM。

```c
typedef struct usr_pwm_handle_s* usr_pwm_handle_t;
```

### 函数接口

#### usr_pwm_init

初始化PWM模块。

```c
esp_err_t usr_pwm_init(usr_pwm_channel_config_t *channel_configs, uint8_t channel_num, usr_pwm_handle_t *handle);
```

参数：
- `channel_configs`: PWM通道配置数组
- `channel_num`: 通道数量
- `handle`: 返回的PWM句柄

返回值：
- `ESP_OK`: 初始化成功
- 其他ESP错误码: 初始化失败

#### usr_pwm_set_duty_percent

设置PWM占空比。

```c
esp_err_t usr_pwm_set_duty_percent(usr_pwm_handle_t handle, ledc_channel_t channel, float duty_percent);
```

参数：
- `handle`: PWM句柄
- `channel`: LEDC通道
- `duty_percent`: 占空比百分比(0.0-100.0)

返回值：
- `ESP_OK`: 设置成功
- 其他ESP错误码: 设置失败

#### usr_pwm_set_freq

设置PWM频率。

```c
esp_err_t usr_pwm_set_freq(usr_pwm_handle_t handle, ledc_timer_t timer, uint32_t freq_hz);
```

参数：
- `handle`: PWM句柄
- `timer`: LEDC定时器
- `freq_hz`: 频率(Hz)

返回值：
- `ESP_OK`: 设置成功
- 其他ESP错误码: 设置失败

#### usr_pwm_start

启动PWM输出。

```c
esp_err_t usr_pwm_start(usr_pwm_handle_t handle, ledc_channel_t channel);
```

参数：
- `handle`: PWM句柄
- `channel`: LEDC通道

返回值：
- `ESP_OK`: 启动成功
- 其他ESP错误码: 启动失败

#### usr_pwm_stop

停止PWM输出。

```c
esp_err_t usr_pwm_stop(usr_pwm_handle_t handle, ledc_channel_t channel, uint32_t idle_level);
```

参数：
- `handle`: PWM句柄
- `channel`: LEDC通道
- `idle_level`: 停止时的电平状态(0或1)

返回值：
- `ESP_OK`: 停止成功
- 其他ESP错误码: 停止失败

#### usr_pwm_deinit

反初始化PWM模块，释放资源。

```c
esp_err_t usr_pwm_deinit(usr_pwm_handle_t handle);
```

参数：
- `handle`: PWM句柄

返回值：
- `ESP_OK`: 释放成功
- 其他ESP错误码: 释放失败

## 使用示例

### 基本使用流程

1. 定义PWM通道配置
2. 初始化PWM模块
3. 启动PWM输出
4. 设置PWM占空比和频率
5. 停止PWM输出
6. 释放PWM资源

```c
#include "usr_pwm.h"
#include "esp_log.h"

static const char *TAG = "PWM_EXAMPLE";

void pwm_example(void)
{
    // 1. 定义PWM通道配置
    usr_pwm_channel_config_t pwm_configs[] = {
        {
            .gpio_num = 18,                // GPIO18
            .channel = LEDC_CHANNEL_0,     // 通道0
            .timer = LEDC_TIMER_0,         // 定时器0
            .freq_hz = 5000,               // 5kHz
            .duty_resolution = LEDC_TIMER_13_BIT, // 13位分辨率
            .speed_mode = LEDC_LOW_SPEED_MODE,    // 低速模式
        },
    };
    
    // 2. 初始化PWM模块
    usr_pwm_handle_t pwm_handle = NULL;
    esp_err_t ret = usr_pwm_init(pwm_configs, sizeof(pwm_configs) / sizeof(pwm_configs[0]), &pwm_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PWM初始化失败: %s", esp_err_to_name(ret));
        return;
    }
    
    // 3. 启动PWM输出
    ret = usr_pwm_start(pwm_handle, LEDC_CHANNEL_0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启动PWM失败: %s", esp_err_to_name(ret));
    }
    
    // 4. 设置PWM占空比
    ret = usr_pwm_set_duty_percent(pwm_handle, LEDC_CHANNEL_0, 50.0); // 50%占空比
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置占空比失败: %s", esp_err_to_name(ret));
    }
    
    // 延时一段时间
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 改变频率
    ret = usr_pwm_set_freq(pwm_handle, LEDC_TIMER_0, 1000); // 改为1kHz
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "设置频率失败: %s", esp_err_to_name(ret));
    }
    
    // 延时一段时间
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 5. 停止PWM输出
    ret = usr_pwm_stop(pwm_handle, LEDC_CHANNEL_0, 0); // 停止时输出低电平
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "停止PWM失败: %s", esp_err_to_name(ret));
    }
    
    // 6. 释放PWM资源
    ret = usr_pwm_deinit(pwm_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "释放PWM资源失败: %s", esp_err_to_name(ret));
    }
}
```

### 多模块共享PWM资源

`usr_pwm`模块支持多个模块共享PWM资源，每个模块可以独立调用`usr_pwm_init`和`usr_pwm_deinit`，而不会导致资源冲突。

```c
// 模块A
void module_a_init(void)
{
    usr_pwm_channel_config_t pwm_configs[] = {
        {
            .gpio_num = 18,
            .channel = LEDC_CHANNEL_0,
            .timer = LEDC_TIMER_0,
            .freq_hz = 5000,
            .duty_resolution = LEDC_TIMER_13_BIT,
            .speed_mode = LEDC_LOW_SPEED_MODE,
        },
    };
    
    usr_pwm_handle_t pwm_handle_a = NULL;
    usr_pwm_init(pwm_configs, 1, &pwm_handle_a);
    
    // 使用PWM...
    
    // 模块卸载时
    // usr_pwm_deinit(pwm_handle_a);
}

// 模块B
void module_b_init(void)
{
    usr_pwm_channel_config_t pwm_configs[] = {
        {
            .gpio_num = 19,
            .channel = LEDC_CHANNEL_1,
            .timer = LEDC_TIMER_0, // 共享定时器
            .freq_hz = 5000,
            .duty_resolution = LEDC_TIMER_13_BIT,
            .speed_mode = LEDC_LOW_SPEED_MODE,
        },
    };
    
    usr_pwm_handle_t pwm_handle_b = NULL;
    usr_pwm_init(pwm_configs, 1, &pwm_handle_b);
    
    // 使用PWM...
    
    // 模块卸载时
    // usr_pwm_deinit(pwm_handle_b);
}
```

## 注意事项

1. **定时器共享**：
   - 多个通道可以共享同一个定时器，但共享定时器的通道必须使用相同的频率
   - 如果不同模块使用相同定时器但配置不同频率，后初始化的模块配置会覆盖先初始化的模块配置

2. **占空比分辨率与频率关系**：
   - 占空比分辨率越高，可设置的最大频率越低
   - 例如，使用13位分辨率时，最大频率约为19.5kHz (80MHz/2^13)

3. **GPIO选择**：
   - 不是所有GPIO都支持PWM输出，请参考ESP32系列芯片的技术规格书
   - 某些GPIO可能有特殊功能，使用前请确保不会与其他功能冲突

4. **资源释放**：
   - 每个模块应该在不再需要PWM时调用`usr_pwm_deinit`释放资源
   - 只有当最后一个使用PWM的模块释放资源时，PWM硬件才会被真正释放

5. **速度模式**：
   - ESP32支持高速模式和低速模式，但建议使用低速模式，因为高速模式可能会与其他外设冲突

## 支持的芯片

- ESP32
- ESP32-S2
- ESP32-S3
- ESP32-C3
- 其他支持LEDC功能的ESP32系列芯片

## 许可证

此模块遵循 [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0) 开源许可证。