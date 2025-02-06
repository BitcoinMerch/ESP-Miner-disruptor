#ifndef RGB_LED_TASK_H_
#define RGB_LED_TASK_H_

typedef struct led_state_t {
    uint8_t led_number;
    uint8_t level_red;
    uint8_t level_green;
    uint8_t level_blue;
} led_state;

typedef struct led_frame_t {
    led_state leds[8];
    int duration;
} led_frame;

void set_one_led(int lednum, uint8_t rval, uint8_t gval, uint8_t bval);
void set_all_leds(uint8_t rval, uint8_t gval, uint8_t bval, uint8_t delaytime);
void RGB_LED_task(void *pvParameters);

#endif
