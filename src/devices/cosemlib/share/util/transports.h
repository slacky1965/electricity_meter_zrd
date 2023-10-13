#ifndef TRANSPORTS_H
#define TRANSPORTS_H

//#include <stdint.h>


typedef struct
{
    uint8_t *data;
    uint32_t offset; //!< Where to start storing data
    uint32_t max_size;
} memory_t;

/**
 * @brief Cosem handler called from the transport layer upon reception of data
 */
typedef int (*data_handler)(uint8_t channel, memory_t *buffer, uint32_t payload_size);

enum conn_event
{
    CONN_DISCONNECTED,
    CONN_NEW
};

/**
 * @brief Connection handler called from the transport
 * @return 0 if connection is rejected, otherwise returns the channel identifier
 */
typedef uint8_t (*conn_handler)(uint8_t channel, enum conn_event event);

#endif // TRANSPORTS_H

