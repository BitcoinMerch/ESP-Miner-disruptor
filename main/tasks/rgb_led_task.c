#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h" // Include esp_timer for esp_timer_get_time
#include "driver/gpio.h"
#include "esp_log.h"
#include "connect.h"
#include "led_strip.h"

#define LED_STRIP_LENGTH 8U
#define LED_STRIP_RMT_INTR_NUM 19U // <- how is this determined?

static const char *TAG = "RGB_LED_task";

static struct led_color_t led_strip_buf_1[LED_STRIP_LENGTH];
static struct led_color_t led_strip_buf_2[LED_STRIP_LENGTH];

struct led_strip_t led_strip = {
    .rgb_led_type = RGB_LED_TYPE_WS2812,
    .rmt_channel = RMT_CHANNEL_1,
    .rmt_interrupt_num = LED_STRIP_RMT_INTR_NUM,
    .gpio = GPIO_NUM_21,
    .led_strip_buf_1 = led_strip_buf_1,
    .led_strip_buf_2 = led_strip_buf_2,
    .led_strip_length = LED_STRIP_LENGTH
};

void set_all_leds(uint8_t rval, uint8_t gval, uint8_t bval, uint8_t delaytime)
{
    int i,j;

    for(i=0; i<8; i++) {
        for(j=0; j<i+1; j++) {
            //turn on all the LEDs up to [i]
            led_strip_set_pixel_rgb(&led_strip, j, rval, gval, bval);
        }
        led_strip_show(&led_strip);
        vTaskDelay(delaytime / portTICK_PERIOD_MS);
    }
}

void RGB_LED_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Started RGB_LED_task");

    // Initialize the led strip
    led_strip.access_semaphore = xSemaphoreCreateBinary();
    bool led_init_ok = led_strip_init(&led_strip);
    if (!led_init_ok) {
        ESP_LOGI(TAG, "ERROR initializing LED Strip");
    } else {
        ESP_LOGI(TAG, "LED Strip initialized");

        //led_strip_set_pixel_rgb(&led_strip, 0, 0xFF, 0xFF, 0xFF);
        //led_strip_set_pixel_rgb(&led_strip, 1, 0xFF, 0x00, 0x00);
        //led_strip_set_pixel_rgb(&led_strip, 2, 0x00, 0xFF, 0x00);
        //led_strip_set_pixel_rgb(&led_strip, 3, 0x00, 0x00, 0xFF);
        //led_strip_show(&led_strip);

        //set_all_leds(0xFF, 0x00, 0x00, 500);

        //led_strip_set_pixel_color(&led_strip, 1, struct led_color_t *color);

        while (1)
        {
            //int64_t duration = esp_timer_get_time() - time;

            set_all_leds(0xFF, 0x00, 0x00, 75);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            set_all_leds(0x00, 0xFF, 0x00, 75);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            set_all_leds(0x00, 0x00, 0xFF, 75);
            vTaskDelay(250 / portTICK_PERIOD_MS);

            vTaskDelay(30 / portTICK_PERIOD_MS); // don't starve idle task and trigger watchdog timer
        }
    }
}
