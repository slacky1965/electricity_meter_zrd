/**
 * AXDR utility function to serialize data
 *
 * Copyright (c) 2016, Anthony Rabine
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the BSD license.
 * See LICENSE.txt for more details.
 *
 */

#include "csm_axdr_codec.h"
#include "csm_ber.h"

// -------------------------------   DECODERS   ------------------------------------------

int csm_axdr_rd_null(csm_array *array)
{
    int ret = FALSE;
    uint8_t byte = 0xFFU;
    if (csm_array_read_u8(array, &byte))
    {
        if (byte == AXDR_TAG_NULL)
        {
            ret = TRUE;
        }
    }
    return ret;
}

int csm_axdr_size(csm_array *array, uint32_t *size)
{
    int ret = FALSE;

    ber_length len;
    csm_ber_read_len(array, &len);

    // Check if size is somewhat possible
    if (len.length <= csm_array_unread(array))
    {
        *size = len.length;
        ret = TRUE;
    }
    return ret;
}

int csm_axdr_rd_octetstring(csm_array *array, uint32_t *size)
{
    int ret = FALSE;
    uint8_t byte = 0xFFU;
    if (csm_array_read_u8(array, &byte))
    {
        if (byte == AXDR_TAG_OCTETSTRING)
        {
            ret = csm_axdr_size(array, size);
        }
    }
    return ret;
}

typedef enum
{
    AXDR_SIZE_NONE = 0,
    AXDR_SIZE_1 = 1,
    AXDR_SIZE_2 = 2,
    AXDR_SIZE_4 = 4,
    AXDR_SIZE_8 = 8,
    AXDR_SIZE_CODED,
} axdr_size_t;

typedef struct
{
    uint8_t tag;
    uint8_t is_struct;
    axdr_size_t size;
} tag_t;

static const tag_t tags[] = {
        { AXDR_TAG_NULL,            0, AXDR_SIZE_NONE},
        { AXDR_TAG_ARRAY,           1, AXDR_SIZE_CODED},
        { AXDR_TAG_STRUCTURE,       1, AXDR_SIZE_CODED},
        { AXDR_TAG_BOOLEAN,         0, AXDR_SIZE_1},
        { AXDR_TAG_BITSTRING,       0, AXDR_SIZE_CODED},
        { AXDR_TAG_INTEGER32,       0, AXDR_SIZE_4},
        { AXDR_TAG_UNSIGNED32,      0, AXDR_SIZE_4},
        { AXDR_TAG_OCTETSTRING,     0, AXDR_SIZE_CODED},
        { AXDR_TAG_VISIBLESTRING,   0, AXDR_SIZE_CODED},
        { AXDR_TAG_UTF8_STRING,     0, AXDR_SIZE_CODED},
        { AXDR_TAG_BCD,             0, AXDR_SIZE_1},
        { AXDR_TAG_INTEGER8,        0, AXDR_SIZE_1},
        { AXDR_TAG_INTEGER16,       0, AXDR_SIZE_2},
        { AXDR_TAG_UNSIGNED8,       0, AXDR_SIZE_1} ,
        { AXDR_TAG_UNSIGNED16,      0, AXDR_SIZE_2},
        { AXDR_TAG_INTEGER64,       0, AXDR_SIZE_8},
        { AXDR_TAG_UNSIGNED64,      0, AXDR_SIZE_8},
        { AXDR_TAG_ENUM,            0, AXDR_SIZE_1}
};

static const uint32_t tags_size = sizeof(tags) / sizeof(tags[0]);


int csm_axdr_decode_tags(csm_array *array, axdr_data_cb callback)
{
    int ret = FALSE;
    uint8_t tag = 0xFFU;

    int error = 0;

    while (csm_array_read_u8(array, &tag) && !error)
    {
    	uint32_t i;
        for (i = 0; (i < tags_size) && !error; i++)
        {
            if (tags[i].tag == tag)
            {
                uint32_t size = tags[i].size;

                if (tags[i].size == AXDR_SIZE_CODED)
                {
                    if (!csm_axdr_size(array, &size))
                    {
                        error = 1;
                    }
                }

                callback(tag, size, csm_array_rd_data(array));

                // jump over the data, if any
                if ((!tags[i].is_struct) && size)
                {
                    // Special case: transform the size in bytes
                    if (tag == AXDR_TAG_BITSTRING)
                    {
                        size = BITFIELD_BYTES(size);
                    }
                    csm_array_reader_jump(array, size);
                }
                break; // enough
            }
        }

        // tag not found?
        if (i >= tags_size)
        {
        	error = 1;
        }
    }

    if (error)
    {
        ret = FALSE;
    }

    return ret;
}

int csm_axdr_decode_block(csm_array *array, uint32_t *size)
{
    int ret = FALSE;
    uint8_t byte = 0xFFU;
    if (csm_array_read_u8(array, &byte))
    {
        if (byte == 0x00U)
        {
            // begin of the block
            ber_length len;
            csm_ber_read_len(array, &len);

            // Check if size is somewhat possible
            if (len.length <= csm_array_unread(array))
            {
                *size = len.length;
                ret = TRUE;
            }
        }
    }
    return ret;
}


// -------------------------------   ENCODERS ------------------------------------------
int csm_axdr_wr_octetstring(csm_array *array, const uint8_t *buffer, uint32_t size)
{
    int valid = csm_array_write_u8(array, AXDR_TAG_OCTETSTRING);
    valid = valid && csm_ber_write_len(array, size);
    valid = valid && csm_array_write_buff(array, buffer, size);
    return valid;
}

int csm_axdr_wr_i8(csm_array *array, int8_t value)
{
    int valid = csm_array_write_u8(array, AXDR_TAG_INTEGER8);
    valid = valid && csm_array_write_u8(array, value);
    return valid;
}

int csm_axdr_wr_u16(csm_array *array, uint16_t value)
{
    int valid = csm_array_write_u8(array, AXDR_TAG_UNSIGNED16);
    valid = valid && csm_array_write_u16(array, value);
    return valid;
}

int csm_axdr_wr_boolean(csm_array *array, uint8_t value)
{
    int valid = csm_array_write_u8(array, AXDR_TAG_OCTETSTRING);
    valid = valid && csm_array_write_u8(array, value);
    return valid;
}

int csm_axdr_wr_capture_object(csm_array *array, csm_object_t *data)
{
    int valid = csm_array_write_u8(array, AXDR_TAG_STRUCTURE);
    valid = valid && csm_ber_write_len(array, 4U);

    // 1.
    valid = valid && csm_axdr_wr_u16(array, data->class_id);
    // 2.
    valid = valid && csm_axdr_wr_octetstring(array, (const uint8_t *)&data->obis.A, 6U);
    // 3.
    valid = valid && csm_axdr_wr_i8(array, data->id);
    // 4.
    valid = valid && csm_axdr_wr_u16(array, data->data_index);
    return valid;
}

