/**
 * Implementation of a protected array with read/write pointers
 *
 * Copyright (c) 2016, Anthony Rabine
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the BSD license.
 * See LICENSE.txt for more details.
 *
 */

#include "csm_array.h"
#include "os_util.h"
//#include <string.h>

#define INDEX(array, i)         (i+array->offset)
#define WR_INDEX(array)         (array->wr_index+array->offset)
#define RD_INDEX(array)         (array->rd_index+array->offset)
#define TEST_INDEX(array, i)    (INDEX(array, i) < array->size)
#define TEST_WR_INDEX(array)    (TEST_INDEX(array, array->wr_index))

void csm_array_init(csm_array *array, uint8_t *buffer, uint32_t max_size, uint32_t used_size, uint32_t offset)
{
    CSM_ASSERT(used_size <= max_size);

    array->buff = buffer;
    array->rd_index = 0U;
    array->offset = offset;
    array->size = max_size;
    array->wr_index = (used_size+offset) >= max_size ? (max_size-offset) : used_size;
}

int csm_array_get(const csm_array *array, uint32_t index, uint8_t *byte)
{
    int ret = FALSE;
    if (TEST_INDEX(array, index))
    {
        *byte = array->buff[INDEX(array, index)];
        ret = TRUE;
    }
    else
    {
        *byte = 0U;
    }
    return ret;
}

int csm_array_set(csm_array *array, uint32_t index, uint8_t byte)
{
    int ret = FALSE;
    if (TEST_INDEX(array, index))
    {
        array->buff[INDEX(array, index)] = byte;
        ret = TRUE;
    }
    return ret;
}

int csm_array_write_u8(csm_array *array, uint8_t byte)
{
    int ret = FALSE;

    if (csm_array_free_size(array) >= 1U)
    {
        array->buff[WR_INDEX(array)] = byte;
        array->wr_index++;
        ret = TRUE;
    }
    else
    {
        CSM_ERR("[ARRAY] Full");
    }
    return ret;
}

int csm_array_write_u16(csm_array *array, uint16_t value)
{
    int ret = FALSE;

    if (csm_array_free_size(array) >= 2U)
    {
        uint8_t *data = csm_array_wr_data(array);
        PUT_BE16(data, value);
        ret = csm_array_writer_jump(array, 2U);
    }
    else
    {
        CSM_ERR("[ARRAY] Full");
    }
    return ret;
}

int csm_array_write_u32(csm_array *array, uint32_t value)
{
    int ret = FALSE;

    if (csm_array_free_size(array) >= 4U)
    {
        uint8_t *data = csm_array_wr_data(array);
        PUT_BE32(data, value);
        ret = csm_array_writer_jump(array, 4U);
    }
    else
    {
        CSM_ERR("[ARRAY] Full");
    }
    return ret;
}

void csm_array_dump(csm_array *array)
{
    for (uint32_t i = 0U; i < array->size; i++)
    {
        if (i > 0U)
        {
            CSM_TRACE(":");
        }
        CSM_TRACE("%02X", array->buff[i]);
    }
    CSM_TRACE("\r\n");
}

int csm_array_writer_jump(csm_array *array, uint32_t nb_bytes)
{
    int ret = TRUE;

    array->wr_index += nb_bytes;
    if (WR_INDEX(array) >= array->size)
    {
        // saturate
        array->wr_index = (array->size-array->offset); // Write index out of bound, it forbid any further write
        ret = FALSE;
    }

    return ret;
}

int csm_array_reader_jump(csm_array *array, uint32_t nb_bytes)
{
    int ret = TRUE;

    CSM_ASSERT(array != NULL);
    array->rd_index += nb_bytes;
    if (RD_INDEX(array) >= array->size)
    {
        // saturate
        array->rd_index = (array->size-array->offset);
        ret = FALSE;
    }

    return ret;
}

int csm_array_read_buff(csm_array *array, uint8_t *to_buff, uint32_t size)
{
    CSM_ASSERT(array != NULL);
    CSM_ASSERT(to_buff != NULL);

    (void) memcpy(to_buff, csm_array_rd_data(array), size);
    return csm_array_reader_jump(array, size);
}

int csm_array_write_buff(csm_array *array, const uint8_t *buff, uint32_t size)
{
    int ret = FALSE;
    CSM_ASSERT(array != NULL);

    if ((WR_INDEX(array) + size) <= array->size)
    {
        (void) memcpy(csm_array_wr_data(array), &buff[0], size);
        array->wr_index += size;
        ret = TRUE;
    }
    else
    {
        CSM_ERR("[ARRAY] Full");
    }
    return ret;
}

uint32_t csm_array_unread(csm_array *array)
{
    uint32_t unread = 0U;
    if (array->wr_index > array->rd_index)
    {
        unread = (array->wr_index - array->rd_index);
    }
    return unread;
}

uint32_t csm_array_free_size(csm_array *array)
{
    return (array->size - WR_INDEX(array));
}

uint32_t csm_array_written(csm_array *array)
{
    return (WR_INDEX(array));
}

uint8_t *csm_array_rd_data(csm_array *array)
{
    return (array->buff + RD_INDEX(array));
}

uint8_t *csm_array_wr_data(csm_array *array)
{
    return (array->buff + WR_INDEX(array));
}

int csm_array_read_u8(csm_array *array, uint8_t *byte)
{
    int ret = FALSE;
    if (csm_array_unread(array) >= 1U)
    {
        *byte = array->buff[RD_INDEX(array)];
        array->rd_index++;
        ret = TRUE;
    }
    else
    {
        CSM_ERR("[ARRAY] No more data");
    }
    return ret;
}

int csm_array_read_u32(csm_array *array, uint32_t *value)
{
    int ret = FALSE;
    if (csm_array_unread(array) >= 4U)
    {
        uint8_t *start = csm_array_rd_data(array);
        *value = GET_BE32(start);
        ret = csm_array_reader_jump(array, 4U);
    }
    return ret;
}

int csm_array_read_u16(csm_array *array, uint16_t *value)
{
    int ret = FALSE;
    if (csm_array_unread(array) >= 2U)
    {
        uint8_t *start = csm_array_rd_data(array);
        *value = GET_BE16(start);
        ret = csm_array_reader_jump(array, 2U);
    }
    return ret;
}
