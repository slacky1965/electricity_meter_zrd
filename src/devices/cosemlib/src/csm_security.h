/**
 * Cosem security layer functions to (de)cipher and authenticate packets
 *
 * Copyright (c) 2016, Anthony Rabine
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the BSD license.
 * See LICENSE.txt for more details.
 *
 */

#ifndef CSM_SECURITY_H
#define CSM_SECURITY_H

#include "csm_array.h"
#include "csm_definitions.h"

#define CSM_DEF_SEC_HDR_SIZE    5U


// A Cosem secure packet has the following form:
//     SC || IC || Information || T

/*
9.2.7.2.4.2         The security header
The security header SH includes the security control byte concatenated with the invocation counter:
SH = SC II IC. The security control byte is shown in Table 37 where:
     Bit 3-0: Security_Suite_Id, see 9.2.3.7;
     Bit 4: â€œAâ€� subfield: indicates that authentication is applied;
     Bit 5: â€œEâ€� subfield: indicates that encryption is applied;
     Bit 6: Key_Set subfield: 0 = Unicast, 1 = Broadcast;
     Bit 7: Indicates the use of compression.
*/
typedef union
{
    uint8_t sh_byte;
    struct
    {
        uint8_t security_suite:4;
        uint8_t authentication:1;
        uint8_t encryption:1;
        uint8_t key_set:1;
        uint8_t compression:1;
    } sh_bit_field;
} csm_sec_control_byte;

typedef struct
{
    uint32_t client_ic; //!< Invocation counter of the client
    uint32_t server_ic; //!< Invocation counter of the server
} csm_sec_context;

typedef enum
{
    CSM_SEC_OK = 0,
    CSM_SEC_AUTH_FAILURE = 1U,
    CSM_SEC_CRYPT_FAILURE = 2U, //!< Either ciphering or deciphering
    CSM_SEC_ERROR = 3U,
} csm_sec_result;


/**
 * @brief Authenticate and/or decipher the packet, following the control byte
 * @param array
 * @return
 */
csm_sec_result csm_sec_auth_decrypt(csm_array *array, csm_request *request, const uint8_t *system_title);
csm_sec_result csm_sec_auth_encrypt(csm_array *array, csm_request *request, const uint8_t *system_title, csm_sec_control_byte sc, uint32_t ic);


#endif // CSM_SECURITY_H
