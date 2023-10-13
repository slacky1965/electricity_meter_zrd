/**
 * A virtual channel of communication with the logical device
 *
 * Copyright (c) 2016, Anthony Rabine
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the BSD license.
 * See LICENSE.txt for more details.
 *
 */

#ifndef CSM_CHANNEL_H
#define CSM_CHANNEL_H

#include "csm_association.h"
#include "csm_services.h"

#define INVALID_CHANNEL_ID 0U


typedef struct
{
    csm_request request;
    csm_asso_state *asso;   //!< Association used for that channel

} csm_channel;


void csm_channel_init(csm_channel *channels, uint8_t chan_size, csm_asso_state *assos, const csm_asso_config *assos_config, uint8_t asso_size);
void csm_channel_disconnect(uint8_t channel);
int csm_channel_hls_pass3(csm_array *array, csm_request *request);
int csm_channel_hls_pass4(csm_array *array, csm_request *request);
int csm_channel_execute(uint8_t channel, csm_array *packet);
uint8_t csm_channel_new(void);

#endif // CSM_CHANNEL_H
