#ifndef LED_CONTROL_H_
#define LED_CONTROL_H_

#define ONBOARD_LED 2
//#define PWM_Pin 4
#define Relay_Pin 15
#define D13 13
#define D12 12
#define D14 14
#define D18 18
#define D19 19
#define D21 21
#define D26 26

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (4) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (400) // Frequency in Hertz. Set frequency at 5 kHz

void led_init(void);
void led_on(void);
void led_off(void);
void Set_Resolution(void);
void Relay_on(void);
void Relay_off(void);
void example_ledc_init(void);
#endif
