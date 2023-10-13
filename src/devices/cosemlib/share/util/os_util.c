/*
 * wpa_supplicant/hostapd / Internal implementation of OS specific functions
 * Copyright (c) 2005-2006, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 *
 * This file is an example of operating system specific  wrapper functions.
 * This version implements many of the functions internally, so it can be used
 * to fill in missing functions from the target system C libraries.
 *
 * Some of the functions are using standard C library calls in order to keep
 * this file in working condition to allow the functions to be tested on a
 * Linux target. Please note that OS_NO_C_LIB_DEFINES needs to be defined for
 * this file to work correctly. Note that these implementations are only
 * examples and are not optimized for speed.
 */

#include "os_util.h"
//#include <stdio.h>

int memcmp_const(const void *a, const void *b, size_t len)
{
	const uint8_t *aa = a;
	const uint8_t *bb = b;
	size_t i;
	uint8_t res;

	for (res = 0, i = 0; i < len; i++)
	{
		res |= aa[i] ^ bb[i];
	}

	return res;
}

void byte_to_hex(const char byte, char *out)
{
    int i = 0U;
    static const char binHex[] = "0123456789ABCDEF";

    out[2*i] = binHex[(byte >> 4) & 0x0F];
    out[2*i + 1] = binHex[byte & 0x0F];
}


void print_hex(const char *buf, int size)
{
    int i = 0U;
    char out[2];

    for (i = 0U; i < size; i++)
    {
        byte_to_hex(buf[i], &out[0]);

        printf("%c", out[0]);
        printf("%c", out[1]);
    }

//    fflush(stdout);
}


void hex2bin(const char *in, char* out, int size)
{
    int i = 0;
    int j = 0;

    int final_len = size / 2U;

    for (i = 0, j = 0; j<final_len; i+=2, j++)
    {
        out[j] = (in[i] % 32 + 9) % 25 * 16 + (in[i+1] % 32 + 9) % 25;
    }
}

