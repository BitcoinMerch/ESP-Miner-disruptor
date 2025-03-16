#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h" // Include esp_timer for esp_timer_get_time
#include "driver/gpio.h"
#include "esp_log.h"
#include "connect.h"
#include "led_strip.h"
#include "rgb_led_task.h"

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

/* An LED 'state' holds the LED number, and brightness levels for RED, GREEN, and BLUE values of one LED */
/* An LED 'frame' consists of one led_state for each LED, and a duration for that frame */
/* An LED 'pattern' consists of an array of LED frames */
/* Each LED frame is displayed for its assigned duration one by one, and then the pattern is repeated */

/* Pattern 0 - alternating blue LEDs */
#define PATTERN_0_LEN 2
led_frame led_pattern_0[PATTERN_0_LEN] = {
    {{{1, 0x00, 0x00, 0x030}, {2, 0x00, 0x00, 0x00}, {3, 0x00, 0x00, 0x30}, {4, 0x00, 0x00, 0x00},
    {5, 0x00, 0x00, 0x30}, {6, 0x00, 0x00, 0x00}, {7, 0x00, 0x00, 0x30}, {8, 0x00, 0x00, 0x00}},
    700},
    {{{1, 0x00, 0x00, 0x00}, {2, 0x00, 0x00, 0x30}, {3, 0x00, 0x00, 0x00}, {4, 0x00, 0x00, 0x30},
    {5, 0x00, 0x00, 0x00}, {6, 0x00, 0x00, 0x30}, {7, 0x00, 0x00, 0x00}, {8, 0x00, 0x00, 0x30}},
    700},
};

/* Pattern 1 - Cycle all LEDs through 3 main colors R, G, B */
#define PATTERN_1_LEN 3
led_frame led_pattern_1[PATTERN_1_LEN] = {
    {{{1, 0x40, 0x00, 0x00}, {2, 0x40, 0x00, 0x00}, {3, 0x40, 0x00, 0x00}, {4, 0x40, 0x00, 0x00},
    {5, 0x40, 0x00, 0x00}, {6, 0x40, 0x00, 0x00}, {7, 0x40, 0x00, 0x00}, {8, 0x40, 0x00, 0x00}},
    500},
    {{{1, 0x00, 0x40, 0x00}, {2, 0x00, 0x40, 0x00}, {3, 0x00, 0x40, 0x00}, {4, 0x00, 0x40, 0x00},
    {5, 0x00, 0x40, 0x00}, {6, 0x00, 0x40, 0x00}, {7, 0x00, 0x40, 0x00}, {8, 0x00, 0x40, 0x00}},
    500},
    {{{1, 0x00, 0x00, 0x40}, {2, 0x00, 0x00, 0x40}, {3, 0x00, 0x00, 0x40}, {4, 0x00, 0x00, 0x40},
    {5, 0x00, 0x00, 0x40}, {6, 0x00, 0x00, 0x40}, {7, 0x00, 0x00, 0x40}, {8, 0x00, 0x00, 0x40}},
    500},
};

#define PATTERN_2_LEN 7
led_frame led_pattern_2[PATTERN_2_LEN] = {
    {{{1, 0x40, 0x00, 0x00}, {2, 0x40, 0x00, 0x00}, {3, 0x40, 0x00, 0x00}, {4, 0x40, 0x00, 0x00},
    {5, 0x40, 0x00, 0x00}, {6, 0x40, 0x00, 0x00}, {7, 0x40, 0x00, 0x00}, {8, 0x40, 0x00, 0x00}},
    500},
    {{{1, 0x00, 0x40, 0x00}, {2, 0x00, 0x40, 0x00}, {3, 0x00, 0x40, 0x00}, {4, 0x00, 0x40, 0x00},
    {5, 0x00, 0x40, 0x00}, {6, 0x00, 0x40, 0x00}, {7, 0x00, 0x40, 0x00}, {8, 0x00, 0x40, 0x00}},
    500},
    {{{1, 0x40, 0x40, 0x00}, {2, 0x40, 0x40, 0x00}, {3, 0x40, 0x40, 0x00}, {4, 0x40, 0x40, 0x00},
    {5, 0x40, 0x40, 0x00}, {6, 0x40, 0x40, 0x00}, {7, 0x40, 0x40, 0x00}, {8, 0x40, 0x40, 0x00}},
    500},
    {{{1, 0x00, 0x00, 0x40}, {2, 0x00, 0x00, 0x40}, {3, 0x00, 0x00, 0x40}, {4, 0x00, 0x00, 0x40},
    {5, 0x00, 0x00, 0x40}, {6, 0x00, 0x00, 0x40}, {7, 0x00, 0x00, 0x40}, {8, 0x00, 0x00, 0x40}},
    500},
    {{{1, 0x40, 0x00, 0x40}, {2, 0x40, 0x00, 0x40}, {3, 0x40, 0x00, 0x40}, {4, 0x40, 0x00, 0x40},
    {5, 0x40, 0x00, 0x40}, {6, 0x40, 0x00, 0x40}, {7, 0x40, 0x00, 0x40}, {8, 0x40, 0x00, 0x40}},
    500},
    {{{1, 0x00, 0x40, 0x40}, {2, 0x00, 0x40, 0x40}, {3, 0x00, 0x40, 0x40}, {4, 0x00, 0x40, 0x40},
    {5, 0x00, 0x40, 0x40}, {6, 0x00, 0x40, 0x40}, {7, 0x00, 0x40, 0x40}, {8, 0x00, 0x40, 0x40}},
    500},
    {{{1, 0x40, 0x40, 0x40}, {2, 0x40, 0x40, 0x40}, {3, 0x40, 0x40, 0x40}, {4, 0x40, 0x40, 0x40},
    {5, 0x40, 0x40, 0x40}, {6, 0x40, 0x40, 0x40}, {7, 0x40, 0x40, 0x40}, {8, 0x40, 0x40, 0x40}},
    500},
};

void set_one_led(int lednum, uint8_t rval, uint8_t gval, uint8_t bval)
{
    int i;

    for(i=0; i<8; i++) {
        if (lednum == i) {
            //turn on the LEDs
            led_strip_set_pixel_rgb(&led_strip, i, rval, gval, bval);
        } else {
            led_strip_set_pixel_rgb(&led_strip, i, 0, 0, 0);
        }
    }
    led_strip_show(&led_strip);
}

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


void RGB_Init()
{
    led_strip.access_semaphore = xSemaphoreCreateBinary();
    bool led_init_ok = led_strip_init(&led_strip);
    if (!led_init_ok) {
        ESP_LOGI(TAG, "ERROR initializing LED Strip");
    } else {
        ESP_LOGI(TAG, "LED Strip initialized");
    }
}

void set_leds_to_color(int LED_COLOR)
{
    switch (LED_COLOR) {
        case LED_COLOR_RED:
            set_all_leds(0x40, 0x00, 0x00, 0);
            break;
        case LED_COLOR_GREEN:
            set_all_leds(0x00, 0x40, 0x00, 0);
            break;
        case LED_COLOR_BLUE:
            set_all_leds(0x00, 0x00, 0x40, 0);
            break;
        case LED_COLOR_WHITE:
            set_all_leds(0x040, 0x40, 0x40, 0);
            break;
        default:
            ESP_LOGE(TAG, "ERROR- unknown LED color");
    }
}

void RGB_LED_task(void *pvParameters)
{
    led_frame *ptn;
    int pattern_len;

    int framenum, i, j;

    ESP_LOGI(TAG, "Started RGB_LED_task");

    // Initialize the led strip
    led_strip.access_semaphore = xSemaphoreCreateBinary();
    bool led_init_ok = led_strip_init(&led_strip);
    if (!led_init_ok) {
        ESP_LOGI(TAG, "ERROR initializing LED Strip");
    } else {
        ESP_LOGI(TAG, "LED Strip initialized");

        //set_all_leds(0x30, 0x30, 0x30, 2000);
        //led_strip_show(&led_strip);

        //set_one_led(0, 0x30, 0x30, 0x30);
        //vTaskDelay(3000 / portTICK_PERIOD_MS);

        //set_one_led(7, 0x30, 0x30, 0x30);
        //vTaskDelay(3000 / portTICK_PERIOD_MS);

        ptn = led_pattern_0;
        pattern_len = PATTERN_0_LEN;
        for (j=0; j<10; j++)
        {
            /* display each frame one by one */
            for(framenum=0; framenum < pattern_len; framenum++) {

                //ESP_LOGI(TAG, "Frame: %d", framenum);

                /* set all the LEDs in this frame */
                for(i=0; i<8; i++) {
                    //turn on all the LEDs up to [i]
                    //ESP_LOGI(TAG, "r: %d  g: %d  b: %d", 
                    //    ptn[framenum].leds[i].level_red, 
                    //    ptn[framenum].leds[i].level_green, 
                    //    ptn[framenum].leds[i].level_blue);
                    led_strip_set_pixel_rgb(&led_strip, i, ptn[framenum].leds[i].level_red, 
                                            ptn[framenum].leds[i].level_green, 
                                            ptn[framenum].leds[i].level_blue);
                }
                /* display this frame */
                led_strip_show(&led_strip);
                /* delay for the time specified */
                vTaskDelay(ptn->duration / portTICK_PERIOD_MS);
            }
        }

        ptn = led_pattern_2;
        pattern_len = PATTERN_2_LEN;
        while (1)
        {
            /* display each frame one by one */
            for(framenum=0; framenum < pattern_len; framenum++) {

                //ESP_LOGI(TAG, "Frame: %d", framenum);

                /* set all the LEDs in this frame */
                for(i=0; i<=8; i++) {
                    //turn on all the LEDs up to [i]
                    //ESP_LOGI(TAG, "r: %d  g: %d  b: %d", 
                    //    ptn[framenum].leds[i].level_red, 
                    //    ptn[framenum].leds[i].level_green, 
                    //    ptn[framenum].leds[i].level_blue);
                    led_strip_set_pixel_rgb(&led_strip, i, ptn[framenum].leds[i].level_red, 
                                            ptn[framenum].leds[i].level_green, 
                                            ptn[framenum].leds[i].level_blue);
                }
                /* display this frame */
                led_strip_show(&led_strip);
                /* delay for the time specified */
                vTaskDelay(ptn->duration / portTICK_PERIOD_MS);
            }

            //vTaskDelay(30 / portTICK_PERIOD_MS); // don't starve idle task and trigger watchdog timer
        }
    }
}
