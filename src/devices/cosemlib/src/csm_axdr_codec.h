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

#ifndef AXDR_CODEC_H
#define AXDR_CODEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csm_array.h"
#include "csm_definitions.h"

/*
Data ::= CHOICE
{
null-data                          [0]   IMPLICIT   NULL,
array                              [1]   IMPLICIT   SEQUENCE OF Data,
structure                          [2]   IMPLICIT   SEQUENCE OF Data,
boolean                            [3]   IMPLICIT   BOOLEAN,
bit-string                         [4]   IMPLICIT   BIT STRING,
double-long                        [5]   IMPLICIT   Integer32,
double-long-unsigned               [6]   IMPLICIT   Unsigned32,
octet-string                       [9]   IMPLICIT   OCTET STRING,
visible-string                     [10]  IMPLICIT   VisibleString,
utf8-string                        [12]  IMPLICIT   UTF8String,
bcd                                [13]  IMPLICIT   Integer8,
integer                            [15]  IMPLICIT   Integer8,
long                               [16]  IMPLICIT   Integer16,
unsigned                           [17]  IMPLICIT   Unsigned8,
long-unsigned                       [18]  IMPLICIT   Unsigned16,
compact-array                      [19]  IMPLICIT   SEQUENCE
{
contents-description                [0]              TypeDescription,
array-contents                      [1]   IMPLICIT   OCTET STRING
},
long64                             [20]  IMPLICIT   Integer64,
long64-unsigned                    [21]  IMPLICIT   Unsigned64,
enum                               [22]  IMPLICIT   Unsigned8,
float32                            [23]  IMPLICIT   OCTET STRING (SIZE(4)),
float64                            [24]  IMPLICIT   OCTET STRING (SIZE(8)),
date-time                          [25]  IMPLICIT   OCTET STRING (SIZE(12)),
date                               [26]  IMPLICIT   OCTET STRING (SIZE(5)),
time                               [27]  IMPLICIT   OCTET STRING (SIZE(4)),
dont-care                          [255] IMPLICIT   NULL
}
*/

enum axdr_tag
{
    AXDR_TAG_NULL           = 0U,
    AXDR_TAG_ARRAY          = 1U,
    AXDR_TAG_STRUCTURE      = 2U,
    AXDR_TAG_BOOLEAN        = 3U,
    AXDR_TAG_BITSTRING      = 4U,
    AXDR_TAG_INTEGER32      = 5U,
    AXDR_TAG_UNSIGNED32     = 6U,
    AXDR_TAG_OCTETSTRING    = 9U,
    AXDR_TAG_VISIBLESTRING  = 10U,
    AXDR_TAG_UTF8_STRING    = 12U,
    AXDR_TAG_BCD            = 13U,
    AXDR_TAG_INTEGER8       = 15U,
    AXDR_TAG_INTEGER16      = 16U,
    AXDR_TAG_UNSIGNED8      = 17U,
    AXDR_TAG_UNSIGNED16     = 18U,
    AXDR_TAG_INTEGER64      = 20U,
    AXDR_TAG_UNSIGNED64     = 21U,
    AXDR_TAG_ENUM           = 22U,
    AXDR_TAG_UNKNOWN        = 255U

};

// Compute how many bytes are needed to store a bit field
#define BITFIELD_BYTES(bits)    (((bits - 1U) >> 3U) + 1U)

typedef void (*axdr_data_cb)(uint8_t type, uint32_t size, uint8_t *data);

// Decoders
int csm_axdr_rd_null(csm_array *array);
int csm_axdr_rd_octetstring(csm_array *array, uint32_t *size);

int csm_axdr_decode_tags(csm_array *array, axdr_data_cb callback);
int csm_axdr_decode_block(csm_array *array, uint32_t *size);

// Encoders
int csm_axdr_wr_octetstring(csm_array *array, const uint8_t *buffer, uint32_t size);
int csm_axdr_wr_i8(csm_array *array, int8_t value);
int csm_axdr_wr_u16(csm_array *array, uint16_t value);
int csm_axdr_wr_boolean(csm_array *array, uint8_t value);
int csm_axdr_wr_capture_object(csm_array *array, csm_object_t *data);

#ifdef __cplusplus
}
#endif

#endif // AXDR_CODEC_H
