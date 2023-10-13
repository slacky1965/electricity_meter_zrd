/**
 * Implementation of the Cosem ACSE services
 *
 * Copyright (c) 2016, Anthony Rabine
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the BSD license.
 * See LICENSE.txt for more details.
 *
 */

#ifndef CSM_ASSOCIATION_H
#define CSM_ASSOCIATION_H


#ifdef __cplusplus
extern "C" {
#endif


//#include <stdint.h>
#include "csm_config.h"
#include "csm_ber.h"
#include "csm_definitions.h"

// States machine of the Control Function
enum state_cf { CF_INACTIVE, CF_IDLE, CF_ASSOCIATION_PENDING, CF_ASSOCIATED, CF_ASSOCIATION_RELEASE_PENDING };

/* GreenBook 8
 * 9.4.2.2.2        The COSEM application context
 * Table 74 – COSEM application context names

    Logical_Name_Referencing_No_Ciphering ::= context_id(1)
    Short_Name_Referencing_No_Ciphering ::= context_id(2)
    Logical_Name_Referencing_With_Ciphering ::= context_id(3)
    Short_Name_Referencing_With_Ciphering ::= context_id(4)
*/
enum csm_referencing
{
    NO_REF = 0U,
    LN_REF = 1U,  // Logical Name
    SN_REF = 2U,  // Short Name
    LN_REF_WITH_CYPHERING = 3U,
    SN_REF_WITH_CYPHERING = 4U
};

/*
    COSEM_lowest_level_security_mechanism_name ::= mechanism_id(0)
    COSEM_low_level_security_mechanism_name ::= mechanism_id(1)
    COSEM_high_level_security_mechanism_name ::= mechanism_id(2)
    COSEM_high_level_security_mechanism_name_using_MD5 ::= mechanism_id(3)
    COSEM_high_level_security_mechanism_name_using_SHA-1 ::= mechanism_id(4)
    COSEM_High_Level_Security_Mechanism_Name_Using_GMAC ::= mechanism_id(5)
    COSEM_High_Level_Security_Mechanism_Name_Using_SHA-256 ::= mechanism_id(6)
    COSEM_High_Level_Security_Mechanism_Name_Using_ECDSA ::= mechanism_id(7)
    NOTE 1 With mechanism_id(2), the method of processing the challenge is secret.
    NOTE 2 The use of authentication mechanisms 3 and 4 are not recommended for new implementations.
*/
enum csm_auth_level
{
    CSM_AUTH_LOWEST_LEVEL       = 0U,
    CSM_AUTH_LOW_LEVEL          = 1U,
    CSM_AUTH_HIGH_LEVEL         = 2U,
    CSM_AUTH_HIGH_LEVEL_MD5     = 3U,
    CSM_AUTH_HIGH_LEVEL_SHA1    = 4U,
    CSM_AUTH_HIGH_LEVEL_GMAC    = 5U,
    CSM_AUTH_HIGH_LEVEL_SHA256  = 6U
};

typedef enum
{
    CSM_ASSO_AARQ                   = TAG_APPLICATION + TAG_CONSTRUCTED + 0U,   ///< Application number 0
    CSM_ASSO_AARE                   = TAG_APPLICATION + TAG_CONSTRUCTED + 1U,   ///< Application number 1
    CSM_ASSO_RLRQ                   = TAG_APPLICATION + TAG_CONSTRUCTED + 2U,   ///< Application number 2
    CSM_ASSO_RLRE                   = TAG_APPLICATION + TAG_CONSTRUCTED + 3U,   ///< Application number 3
    CSM_ASSO_PROTO_VER              = TAG_CONTEXT_SPECIFIC + TAG_PRIMITIVE,
    CSM_ASSO_APP_CONTEXT_NAME       = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  1U,

    // AARQ tags
    CSM_ASSO_CALLED_AP_TITLE        = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  2U,
    CSM_ASSO_CALLED_AE_QUALIFIER    = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  3U,
    CSM_ASSO_CALLED_AP_INVOC_ID     = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  4U,
    CSM_ASSO_CALLED_AE_INVOC_ID     = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  5U,
    CSM_ASSO_CALLING_AP_TITLE       = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  6U,
    CSM_ASSO_CALLING_AE_QUALIFIER   = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  7U,
    CSM_ASSO_CALLING_AP_INVOC_ID    = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  8U,
    CSM_ASSO_CALLING_AE_INVOC_ID    = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  9U,
    CSM_ASSO_SENDER_ACSE_REQU       = TAG_CONTEXT_SPECIFIC + TAG_PRIMITIVE   + 10U,
    CSM_ASSO_REQ_MECHANISM_NAME     = TAG_CONTEXT_SPECIFIC + TAG_PRIMITIVE   + 11U,
    CSM_ASSO_CALLING_AUTH_VALUE     = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED + 12U,
    CSM_ASSO_IMPLEMENTATION_INFO    = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED + 29U,
    CSM_ASSO_USER_INFORMATION       = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED + 30U,

    // AARE tags
    CSM_ASSO_RESP_AUTH_VALUE        = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  10U,
    CSM_ASSO_RESP_MECHANISM_NAME    = TAG_CONTEXT_SPECIFIC + TAG_PRIMITIVE   +  9U,
    CSM_ASSO_RESPONDER_ACSE_REQ     = TAG_CONTEXT_SPECIFIC + TAG_PRIMITIVE   +  8U,
    CSM_ASSO_RESP_AP_TITLE          = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  4U,
    CSM_ASSO_RESULT_FIELD           = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  2U,
    CSM_ASSO_RESULT_SRC_DIAG        = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  3U,
    CSM_ASSO_RESULT_SERVICE_USER    = TAG_CONTEXT_SPECIFIC + TAG_CONSTRUCTED +  1U,
} csm_asso_tag;


/*

    acse-service-user                  [1] INTEGER
    {
        null                                             (0),
        no-reason-given                                  (1),
        application-context-name-not-supported           (2),
        calling-AP-title-not-recognized                  (3),
        calling-AP-invocation-identifier-not-recognized  (4),
        calling-AE-qualifier-not-recognized              (5),
        calling-AE-invocation-identifier-not-recognized  (6),
        called-AP-title-not-recognized                   (7),
        called-AP-invocation-identifier-not-recognized   (8),
        called-AE-qualifier-not-recognized               (9),
        called-AE-invocation-identifier-not-recognized   (10),
        authentication-mechanism-name-not-recognized     (11),
        authentication-mechanism-name-required           (12),
        authentication-failure                           (13),
        authentication-required                          (14)
    }

 */
enum csm_asso_result
{
    CSM_ASSO_ERR_NULL                           = 0U,   //!< No error
    CSM_ASSO_NO_REASON_GIVEN                    = 1U,
    CSM_ASSO_AUTH_NOT_RECOGNIZED                = 11U,
    CSM_ASSO_AUTH_MECANISM_NAME_REQUIRED        = 12U,
    CSM_ASSO_ERR_AUTH_FAILURE                   = 13U,
    CSM_ASSO_AUTH_REQUIRED                      = 14U,
};

/**
 * @brief Configuration structure of one association, should be fixed in ROM at compile time
 */
typedef struct
{
    csm_llc llc;
    uint32_t conformance;          ///< All services and functionalities authorized.
    uint8_t  is_auto_connected;    ///< Boolean to indicate if the association is auto connected or not;
} csm_asso_config;

typedef struct
{
    uint8_t value[CSM_DEF_CHALLENGE_SIZE];
    uint8_t size;
} csm_challenge;


/**
 * @brief Temporary structure valid during the ACSE
 */
typedef struct
{
    csm_challenge ctos; // CtoS: Client to Server Challenge
    csm_challenge stoc; // StoC: Server to Client challenge
    uint32_t proposed_conformance;
    uint16_t client_max_receive_pdu_size;
    uint16_t server_max_receive_pdu_size;
    uint8_t accepted;
    enum csm_asso_result result;
} csm_asso_handshake;

/**
 * @brief State and information of the current association
 *
 * This state structure contains valid data for the life of one association
 *
 */
typedef struct
{
    // Current state and parameters of the association
    enum state_cf state_cf;
    enum csm_referencing ref;
    enum csm_auth_level auth_level;
    uint8_t client_app_title[CSM_DEF_APP_TITLE_SIZE];
    uint8_t server_app_title[CSM_DEF_APP_TITLE_SIZE];

    // Valid for the ACSE session establishment, for security reasons it should be erased after all
    csm_asso_handshake handshake;

    // Pointer to the configuration structure in ROM
    const csm_asso_config *config;
} csm_asso_state;

void csm_asso_init(csm_asso_state *state);
int csm_asso_server_execute(csm_asso_state *state, csm_array *packet);
int csm_asso_encoder(csm_asso_state *state, csm_array *array, uint8_t tag);
int csm_asso_decoder(csm_asso_state *state, csm_array *array, uint8_t tag);

#ifdef __cplusplus
}
#endif

#endif // CSM_ASSOCIATION_H
