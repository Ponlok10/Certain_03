#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#include "esp_adc_cal.h"
#include "math.h"
#include "blink_led.h"
//#include "rom/ets_sys.h"

#define topics_sub "v1/devices/me/rpc/request/+"
#define topics_pub "v1/devices/me/telemetry"
#define broker "mqtt://hiJZ0Z7f6Mk9GJgekOrA:b3377720-8884-11ec-8fad-d5a515836ddb@demo.thingsboard.io"

float R_max = 4095;
float R_low = 10;
float Light;
uint8_t L_max = 10;
uint8_t L_low = 1;

uint8_t perRevolution = 5;
uint16_t Speed = 1000;
uint16_t count = 0;
uint8_t BedRoom = -1;
uint8_t ButtonVal;
bool Dir;
uint8_t ButtonStop = -1;

char Datapub[64];
char DataSub[128];
static const char *TAG = "MQTT_Thingsboard";

esp_mqtt_client_config_t mqtt_cfg = {
			        .uri = broker,
			    };
static esp_mqtt_client_handle_t client;

#define NO_OF_SAMPLES   64

#if CONFIG_IDF_TARGET_ESP32
static const adc_channel_t channel = ADC_CHANNEL_0;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
#endif
static const adc_atten_t atten = ADC_ATTEN_DB_11;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, topics_sub, 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:

        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        strncat(DataSub,(event->data),event->data_len);
        printf("DataSub= %s\n",DataSub);
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void ReadingLight(void){
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
    float adc_reading = 0;
      for (int i = 0; i < NO_OF_SAMPLES; i++) {
              adc_reading += adc1_get_raw((adc1_channel_t)channel);
      }
      adc_reading /= NO_OF_SAMPLES;
  	  float m = (R_max - R_low)/(L_low - L_max);
      //Convert adc_reading to voltage in mV
      //float voltage = adc_reading * 3.3/4095;
      Light = (adc_reading-R_low + m*L_max)/m;
      if (ButtonVal == 1){
          if (Light<6){
  			 esp_mqtt_client_publish(client,topics_pub,"{\"BedRoomLed\":false}", 0, 1, 0);
  			 ESP_LOGI(TAG, "sent publish successful");
    		  led_off();
    		  Dir = 0;
          }
          else if (Light>=6){
  			esp_mqtt_client_publish(client,topics_pub,"{\"BedRoomLed\":true}", 0, 1, 0);
  			ESP_LOGI(TAG, "sent publish successful");
        	  led_on();
        	  Dir = 1;
          }
      }

      char buffer[10];

      sprintf(buffer,"%f",Light);
      strcpy(Datapub,"{\"Light\":");
      strcat(Datapub,buffer);
      strcat(Datapub,"}");
      printf("Photo_Resistor: %fV\t", adc_reading);
      printf("Data: %s\n", Datapub);
      vTaskDelay(pdMS_TO_TICKS(5000));
}
void PublishData(void *pvParameter){
  	while(1){
  		if (MQTT_EVENT_CONNECTED){
  		ReadingLight();
  		esp_mqtt_client_publish(client,topics_pub,Datapub, 0, 1, 0);
  	    ESP_LOGI(TAG, "sent publish successful");
  		vTaskDelay(pdMS_TO_TICKS(5000));
  		memset(Datapub, 0, sizeof Datapub);
  		}else{
  			esp_mqtt_client_reconnect(client);
  			esp_mqtt_client_start(client);
  			esp_mqtt_client_subscribe(client,topics_sub,1);
  		}
  }
}
void mqtt_Subscribe(void){
	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
	esp_mqtt_client_start(client);
}
void read_string(char *s){
	if(strstr(s,"BRoom")!=NULL){
		if (strstr(s, "true")!=NULL){
			printf("Bed Room LED is Turning ON\n");
			BedRoom = 1;
			ButtonStop = 0;
		}
		else if (strstr(s, "false")!=NULL){
			printf("Bed Room LED is Turning OFF\n");
			BedRoom = 0;
			ButtonStop = 0;
		}
	}
	if(strstr(s,"Light_Control")!=NULL){
		if(strstr(s,"true")!=NULL){
			printf("Set Automatic\n");
			ButtonVal = 1;
		}
		else if(strstr(s,"false")!=NULL){
			printf("Set Manual\n");
			ButtonVal = 0;
		}
	}
	if(strstr(s,"Stop")!=NULL){
		if(strstr(s,"true")!=NULL){
			ButtonStop = 1;
		}
	}
}

void Control(void *pvParameter){
	while(1){
		read_string(DataSub);
		memset(DataSub, 0, sizeof Datapub);
		if (BedRoom == 1 && ButtonVal == 0){
			esp_mqtt_client_publish(client,topics_pub,"{\"BedRoomLed\":true}", 0, 1, 0);
			ESP_LOGI(TAG, "sent publish successful");
			led_on();
			Dir = gpio_get_level(ONBOARD_LED);
			BedRoom = -1;
		}
		else if(BedRoom == 0 && ButtonVal == 0){
			esp_mqtt_client_publish(client,topics_pub,"{\"BedRoomLed\":false}", 0, 1, 0);
			ESP_LOGI(TAG, "sent publish successful");
			led_off();
			Dir = gpio_get_level(ONBOARD_LED);
			BedRoom = -1;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
void Count(void *pvParameter){
	while(1){
		if (ButtonStop == 0){
		    if (count<perRevolution && Dir == 1) {
		    	Relay_on();
				count++;
			    vTaskDelay(pdMS_TO_TICKS(1000));
			}
		    if (count>0 && Dir == 0) {
		    	Relay_on();
				count--;
			    vTaskDelay(pdMS_TO_TICKS(1000));
			}
		}

	    if (count == perRevolution || count == 0 || ButtonStop == 1) {
	    	Relay_off();
		}
	}
}
void app_main(void)
{
	example_ledc_init();
	led_init();
	Set_Resolution();
	Relay_off();
	gpio_set_level(D12, 1);
	gpio_set_level(D14, 0);
	gpio_set_level(D26, 0);
	//esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY));
        // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));

    mqtt_Subscribe();
    xTaskCreatePinnedToCore(&PublishData,"Publish Data to Thingsboard", 2048, NULL, 2,NULL, 1);
    xTaskCreatePinnedToCore(&Control,"Control The LED", 2048, NULL, 1,NULL, 1);
    xTaskCreatePinnedToCore(&Count,"Speed_Step", 2048, NULL, 1,NULL, 1);
}

