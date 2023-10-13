/*
 * Bitfield
 * Copyright (c) 2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef BITFIELD_H
#define BITFIELD_H

#define BF_BYTE_ARRAY_SIZE(bits)	(((bits - 1U) >> 3U) + 1U)

struct bitfield
{
    uint8_t *bits;	//!< Pointer to the reserved array of bytes
	uint32_t max_bits;
};

void bitfield_init(struct bitfield *bf, uint8_t init_level);
void bitfield_set(struct bitfield *bf, uint32_t bit);
void bitfield_clear(struct bitfield *bf, uint32_t bit);
int bitfield_is_set(struct bitfield *bf, uint32_t bit);
int bitfield_get_first_zero(struct bitfield *bf);

#endif /* BITFIELD_H */
