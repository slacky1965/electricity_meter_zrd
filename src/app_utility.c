#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#include "gp.h"

s32 delayedMcuResetCb(void *arg) {

    //printf("mcu reset\r\n");
    zb_resetDevice();
    return -1;
}

s32 delayedFactoryResetCb(void *arg) {

    //printf("factory reset\r\n");
    zb_factoryReset();
//    zb_resetDevice();
    return -1;
}

s32 delayedFullResetCb(void *arg) {

    //printf("full reset\r\n");
    return -1;
}

/* Function return size of string and convert signed  *
 * integer to ascii value and store them in array of  *
 * character with NULL at the end of the array        */

u32 itoa(u32 value, u8 *ptr) {
    u32 count = 0, temp;
    if(ptr == NULL)
        return 0;
    if(value == 0)
    {
        *ptr = '0';
        return 1;
    }

    if(value < 0)
    {
        value *= (-1);
        *ptr++ = '-';
        count++;
    }

    for(temp=value; temp>0; temp/=10, ptr++);
        *ptr = '\0';

    for(temp=value; temp>0; temp/=10)
    {
        *--ptr = temp%10 + '0';
        count++;
    }
    return count;
}

u32 from24to32(const u8 *str) {

    u32 value;

    value = str[0] & 0xff;
    value |= (str[1] << 8) & 0xff00;
    value |= (str[2] << 16) & 0xff0000;

    return value;
}

u64 fromPtoInteger(u16 len, u8 *data) {

    u64 value = 0;

    if (len > 8) len = 8;

    u16 v_len = len-1;

    for (u8 i = 0; i < len; i++, v_len--) {
        value |= (u64)data[i] << (i*8);
    }

    return value;
}

u8 set_zcl_str(u8 *str_in, u8 *str_out, u8 len) {
    u8 *data = str_out;
    u8 *str_len = data;
    u8 *str_data = data+1;

    for (u8 i = 0; *(str_in+i) != 0 && i < (len-1); i++) {
        *(str_data+i) = *(str_in+i);
        (*str_len)++;
    }

    return *str_len;
}

