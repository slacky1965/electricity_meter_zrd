#include "tl_common.h"
#include "zb_api.h"
#include "zcl_include.h"
#include "gp.h"

int32_t delayedMcuResetCb(void *arg) {

    //printf("mcu reset\r\n");
    zb_resetDevice();
    return -1;
}

int32_t delayedFactoryResetCb(void *arg) {

    //printf("factory reset\r\n");
    zb_factoryReset();
//    zb_resetDevice();
    return -1;
}

int32_t delayedFullResetCb(void *arg) {

    //printf("full reset\r\n");
    return -1;
}

/* Function return size of string and convert signed  *
 * integer to ascii value and store them in array of  *
 * character with NULL at the end of the array        */

uint32_t itoa(uint32_t value, uint8_t *ptr) {
    uint32_t count = 0, temp;
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

uint32_t from24to32(const uint8_t *str) {

    uint32_t value;

    value = str[0] & 0xff;
    value |= (str[1] << 8) & 0xff00;
    value |= (str[2] << 16) & 0xff0000;

    return value;
}

uint64_t fromPtoInteger(uint16_t len, uint8_t *data) {

    uint64_t value = 0;

    if (len > 8) len = 8;

    uint16_t v_len = len-1;

    for (uint8_t i = 0; i < len; i++, v_len--) {
        value |= (uint64_t)data[i] << (i*8);
    }

    return value;
}

uint8_t set_zcl_str(uint8_t *str_in, uint8_t *str_out, uint8_t len) {
    uint8_t *data = str_out;
    uint8_t *str_len = data;
    uint8_t *str_data = data+1;

    for (uint8_t i = 0; *(str_in+i) != 0 && i < (len-1); i++) {
        *(str_data+i) = *(str_in+i);
        (*str_len)++;
    }

    return *str_len;
}

