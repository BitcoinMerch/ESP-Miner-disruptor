#ifndef RGB_LED_TASK_H_
#define RGB_LED_TASK_H_

void set_one_led(int lednum, uint8_t rval, uint8_t gval, uint8_t bval);
void set_all_leds(uint8_t rval, uint8_t gval, uint8_t bval, uint8_t delaytime);
void RGB_LED_task(void *pvParameters);

#endif
