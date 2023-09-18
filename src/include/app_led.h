#ifndef SRC_INCLUDE_APP_LED_H_
#define SRC_INCLUDE_APP_LED_H_

#define LED_ON                      1
#define LED_OFF                     0

void led_init(void);
void led_on(u32 pin);
void led_off(u32 pin);

void light_blink_start(u8 times, u16 ledOnTime, u16 ledOffTime);
void light_blink_stop(void);

void light_init(void);
void light_on(void);
void light_off(void);
s32 flashLedStatusCb(void *arg);


#endif /* SRC_INCLUDE_APP_LED_H_ */
