#include <stdio.h>
#include "driver/gpio.h"
#include "blink_led.h"
#include "driver/ledc.h"
#include "esp_err.h"
void led_init(void){
	gpio_config_t io_config;
	io_config.mode = GPIO_MODE_INPUT_OUTPUT;
	io_config.pin_bit_mask = (1ULL<<ONBOARD_LED|1ULL<<Relay_Pin|1ULL<<D13|1ULL<<D12|1ULL<<D14|1ULL<< D26|1ULL<<D18|1ULL<<D19|1ULL<<D21);
	io_config.pull_down_en=false;
	io_config.pull_up_en=false;
	io_config.intr_type=GPIO_INTR_DISABLE;
	gpio_config(&io_config);
}
void led_on(void){

	gpio_set_level(ONBOARD_LED,1);
}
void led_off(void){
	gpio_set_level(ONBOARD_LED,0);
}
void Set_Resolution(void){
    gpio_set_level(D18, 1);
    gpio_set_level(D19, 0);
    gpio_set_level(D21, 0);
}
void Relay_on(void){
    gpio_set_level(Relay_Pin, 0);
    gpio_set_level(D13, 1);
}
void Relay_off(void){
    gpio_set_level(Relay_Pin, 1);
	gpio_set_level(D13, 0);
}
void example_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}


