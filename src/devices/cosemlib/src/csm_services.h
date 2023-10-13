/**
 * Cosem services coder/decoder
 *
 * Copyright (c) 2016, Anthony Rabine
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the BSD license.
 * See LICENSE.txt for more details.
 *
 */

#ifndef CSM_SERVICES_H
#define CSM_SERVICES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "csm_definitions.h"
#include "csm_association.h"

enum csm_req_type { RQ_NORMAL, RQ_NEXT, RQ_WITH_LIST };

/**
 * @brief Generic Codec error codes
 **/
typedef enum
{
    // These two error codes can be used in priority
    CSM_OK,                 //!< Request OK
    CSM_OK_BLOCK,           //!< Request OK, ask for a block transfer (not enough space to store the data)
    CSM_ERR_OBJECT_ERROR,   //!< Generic error coming from the object

    // Some more specific errors
    CSM_ERR_OBJECT_NOT_FOUND,    //!< Not found in the database, check obis code!
    CSM_ERR_BAD_ENCODING,        //!< Bad encoding of codec
    CSM_ERR_UNAUTHORIZED_ACCESS, //!< Attribute access problem
    CSM_ERR_TEMPORARY_FAILURE,   ///< Temporary failure
    CSM_ERR_DATA_CONTENT_NOT_OK, ///< Data content is not accepted.
} csm_db_code;



typedef csm_db_code (*csm_db_access_handler)(csm_array *in, csm_array *out, csm_request *request);




// ----------------------------------- CLIENT SERVICES -----------------------------------

void csm_client_init(csm_request *request, csm_response *response);
int csm_client_has_more_data(csm_response *response);
int csm_client_decode(csm_response *response, csm_array *array);
int svc_request_encoder(csm_request *request, csm_array *array);
int csm_client_encode_selective_access_by_range(csm_array *array, csm_object_t *restricting_object, csm_array *start, csm_array *end);

// ----------------------------------- SERVER SERVICES -----------------------------------

void csm_services_init(const csm_db_access_handler db_access);

// Return he number of bytes to transfer back, 0 if no response
int csm_server_services_execute(csm_asso_state *state, csm_request *request, csm_array *array);

// Specific method in case of HLS authentication
int csm_services_hls_execute(csm_asso_state *state, csm_request *request, csm_array *array);

#ifdef __cplusplus
}
#endif

#endif // CSM_SERVICES_H
