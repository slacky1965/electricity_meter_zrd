#ifndef HDLC_H
#define HDLC_H

#ifdef __cplusplus
extern "C" {
#endif

//#include <stdint.h>

//#include "transports.h"
//
//typedef struct
//{
//    uint16_t max_rcv_info_size;
//    uint16_t max_snd_info_size;
//    uint16_t address;
//    data_handler handler;
//
//} hdlc_channel;


#define HDLC_OK         0
#define HDLC_ERR        -1
#define HDLC_ERR_7E     -2
#define HDLC_ERR_FORMAT -3
#define HDLC_ERR_SIZE   -4 // bad packet size
#define HDLC_ERR_ADDR   -5 // address format invalid
#define HDLC_ERR_FCS    -6
#define HDLC_ERR_HCS    -7
#define HDLC_ERR_I_FORMAT    -8     // Frame type I bad format
#define HDLC_ERR_NEGO   -9


// Packet types
#define HDLC_PACKET_TYPE_BAD    (0)
#define HDLC_PACKET_TYPE_I      (1)
#define HDLC_PACKET_TYPE_RR     (2)
#define HDLC_PACKET_TYPE_RNR    (3)
#define HDLC_PACKET_TYPE_SNRM   (4)
#define HDLC_PACKET_TYPE_DISC   (5)
#define HDLC_PACKET_TYPE_UA     (6)
#define HDLC_PACKET_TYPE_DM     (7)
#define HDLC_PACKET_TYPE_FRMR   (8)
#define HDLC_PACKET_TYPE_UI     (9)

typedef enum
{
    HDLC_CLIENT = 0,
    HDLC_SERVER = 1
} HDLC_SENDER;

typedef struct
{
    uint16_t data_index; // index to the information data
    uint16_t data_size;   // HDLC packet len, without start and end flags (7E)
    uint16_t logical_device;
    uint16_t phy_address;
    uint16_t client_addr; // Client is always one byte length, but store it on 16 bytes for Cosem LLC compatibility
    uint16_t frame_size; // index to the next HDLC frame in the packet
    uint16_t max_info_field_tx;
    uint16_t max_info_field_rx;
    uint8_t window_tx;
    uint8_t window_rx;
    uint8_t addr_len; // server addressing scheme (1, 2 or 4 bytes)
    uint8_t segmentation;
    uint8_t rrr;
    uint8_t sss;
    uint8_t type; // packet type (SNRM, UA, I...)
    uint8_t poll_final;
    uint8_t cmd_resp; // Command (0) or response (1) LLC  (E6 E6 00 for a command and E6 E7 00 for a response)
    uint8_t sender; // 0=client, 1=server

} hdlc_t;

void hdlc_init(hdlc_t *hdlc);
int hdlc_decode_info_field(hdlc_t *hdlc, const uint8_t *buf, uint16_t info_field_size);
int hdlc_encode_snrm(hdlc_t *hdlc, uint8_t *buf, uint16_t size);
int hdlc_encode_rr(hdlc_t *hdlc, uint8_t *buf, uint16_t size);
int hdlc_encode_data(hdlc_t *hdlc, uint8_t *buf, uint16_t size, const uint8_t *data, uint16_t data_size);
int hdlc_encode(hdlc_t *hdlc, uint8_t *buf, uint16_t size, uint8_t frame_type, const uint8_t *data, uint16_t data_size);
int hdlc_decode(hdlc_t *hdlc, const uint8_t *buf, uint16_t size);
void hdlc_print_result(hdlc_t *hdlc, int code);

//void hdlc_init(hdlc_channel *chan);
//int hdlc_handle(hdlc_channel *chan, uint8_t *data, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif // HDLC_H
