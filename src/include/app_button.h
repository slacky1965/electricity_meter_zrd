#ifndef SRC_INCLUDE_APP_BUTTON_H_
#define SRC_INCLUDE_APP_BUTTON_H_

typedef struct {
    u8  released :1;
    u8  pressed  :1;
    u8  counter  :6;
    u8  debounce;
    u32 pressed_time;
    u32 released_time;
} button_t;

void init_button();
void button_handler();
u8 button_idle();

#endif /* SRC_INCLUDE_APP_BUTTON_H_ */
