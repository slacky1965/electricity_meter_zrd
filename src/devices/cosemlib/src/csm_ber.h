/**
 * BER decoder/encoder/helper
 *
 * Copyright (c) 2016, Anthony Rabine
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the BSD license.
 * See LICENSE.txt for more details.
 *
 */

#ifndef CSM_BER_H
#define CSM_BER_H

//#include <stdint.h>
#include "csm_array.h"

enum
{
    CLASS_MASK	= 0xC0,
    TYPE_MASK	= 0x20, //< Bit 6 indicates if it is a Primitive (0) of Constructed (1)
    TAG_MASK	= 0x1F,
    LEN_XTND	= 0x80,
    LEN_MASK	= 0x7F
};

/*
Universal Class Tags Name 	P/C 	Number (decimal) 	Number (hexadecimal)
EOC (End-of-Content) 	P 	0 	0
BOOLEAN 	P 	1 	1
INTEGER 	P 	2 	2
BIT STRING 	P/C 	3 	3
OCTET STRING 	P/C 	4 	4
NULL 	P 	5 	5
OBJECT IDENTIFIER 	P 	6 	6
Object Descriptor 	P/C 	7 	7
EXTERNAL 	C 	8 	8
REAL (float) 	P 	9 	9
ENUMERATED 	P 	10 	A
EMBEDDED PDV 	C 	11 	B
UTF8String 	P/C 	12 	C
RELATIVE-OID 	P 	13 	D
(reserved) 	- 	14 	E
(reserved) 	- 	15 	F
SEQUENCE and SEQUENCE OF 	C 	16 	10
SET and SET OF 	C 	17 	11
NumericString 	P/C 	18 	12
PrintableString 	P/C 	19 	13
T61String 	P/C 	20 	14
VideotexString 	P/C 	21 	15
IA5String 	P/C 	22 	16
UTCTime 	P/C 	23 	17
GeneralizedTime 	P/C 	24 	18
GraphicString 	P/C 	25 	19
VisibleString 	P/C 	26 	1A
GeneralString 	P/C 	27 	1B
UniversalString 	P/C 	28 	1C
CHARACTER STRING 	P/C 	29 	1D
BMPString 	P/C 	30 	1E
(use long-form) 	- 	31 	1F
*/
enum
{
    CSM_BER_TYPE_EOC                = 0U,
    CSM_BER_TYPE_BOOLEAN            = 1U,
    CSM_BER_TYPE_INTEGER            = 2U,
    CSM_BER_TYPE_BIT_STRING         = 3U,
    CSM_BER_TYPE_OCTET_STRING       = 4U,
    CSM_BER_TYPE_NULL               = 5U,
    CSM_BER_TYPE_OBJECT_IDENTIFIER  = 6U,
};

/*
    Class           bit 8 	bit 7 	description
Universal           0       0       The type is native to ASN.1
Application         0       1   	The type is only valid for one specific application
Context-specific 	1       0       Meaning of this type depends on the context (such as within a sequence, set or choice)
Private             1       1       Defined in private specifications
*/
enum tag_class
{
    TAG_UNIVERSAL           = 0x00U,
    TAG_APPLICATION         = 0x40U,
    TAG_CONTEXT_SPECIFIC    = 0x80U,
    TAG_PRIVATE             = 0xC0U
};

enum
{
    TAG_PRIMITIVE = 0U,
    TAG_CONSTRUCTED	= TYPE_MASK
};

typedef struct
{
    uint8_t cls;
    uint8_t isPrimitive;
    uint8_t id;
    uint8_t tag;    //< We limit to one-byte tag
    uint8_t ext;    //< 2 bytes encoded TAG, extension (store tags > 31)
    uint8_t nbytes;
} ber_tag;

typedef struct
{
    uint16_t length; //< Cosem APDU size is coded on Unsigned16
    uint8_t nbytes;
} ber_length;

typedef struct
{
    ber_tag tag;
    ber_length length;
} csm_ber;


typedef struct
{
    const uint8_t *header;   //< 5 bytes values representing the Organisation ID, always  60 85 74 05 08 for Cosem (see chapter 11.4 page 398)
    uint8_t size;   // Size of
    uint8_t name;       //< name object
    uint8_t id;         //< id object
} ber_object_identifier;


void csm_ber_dump(csm_ber *i_ber);
int csm_ber_decode_object_identifier(ber_object_identifier *oid, csm_array *array);
int csm_ber_decode(csm_ber *ber, csm_array *array);

int csm_ber_read_len(csm_array *array, ber_length *o_len);
int csm_ber_write_len(csm_array *array, uint16_t len);

int csm_ber_write_u8(csm_array *array, uint8_t value);
int csm_ber_read_u8(csm_array *array, uint8_t *value);

#endif // CSM_BER_H
