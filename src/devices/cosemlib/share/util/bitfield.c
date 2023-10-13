/*
 * Bitfield
 * Copyright (c) 2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "os_util.h"
#include "bitfield.h"

void bitfield_init(struct bitfield *bf, uint8_t init_level)
{
    uint8_t level = (init_level == 0U) ? 0x00U : 0xFFU;
    for (uint32_t i = 0U; i < BF_BYTE_ARRAY_SIZE(bf->max_bits); i++)
    {
        bf->bits[i] = level;
    }
}

void bitfield_set(struct bitfield *bf, uint32_t bit)
{
	if (bit >= bf->max_bits)
	{
		return;
	}
	bf->bits[bit / 8] |= BIT(bit % 8);
}


void bitfield_clear(struct bitfield *bf, uint32_t bit)
{
	if (bit >= bf->max_bits)
	{
		return;
	}
	bf->bits[bit / 8] &= ~BIT(bit % 8);
}


int bitfield_is_set(struct bitfield *bf, uint32_t bit)
{
	if (bit >= bf->max_bits)
	{
		return 0;
	}
	return !!(bf->bits[bit / 8] & BIT(bit % 8));
}


static int first_zero(uint8_t val)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		if (!(val & 0x01))
		{
			return i;
		}
		val >>= 1;
	}
	return -1;
}


int bitfield_get_first_zero(struct bitfield *bf)
{
	uint32_t i;
    for (i = 0; i < BF_BYTE_ARRAY_SIZE(bf->max_bits); i++)
    {
		if (bf->bits[i] != 0xff)
		{
			break;
		}
	}
	if (i == (bf->max_bits + 7) / 8)
	{
		return -1;
	}
	i = i * 8 + first_zero(bf->bits[i]);
	if (i >= bf->max_bits)
	{
		return -1;
	}
	return i;
}
