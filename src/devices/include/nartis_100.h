#ifndef SRC_DEVICES_INCLUDE_NARTIS_100_H_
#define SRC_DEVICES_INCLUDE_NARTIS_100_H_

typedef enum _command_t {
    cmd_snrm = 0,
    cmd_disc,
} command_t;


typedef struct __attribute__((packed)) {
    uint8_t     address_lsb         :1;     /* 0 - use next byte, 1 - last byte         */
    uint8_t     address_digit       :7;
} address_t;

typedef struct __attribute__((packed)) {
    uint16_t    length              :11;    /* frame length                             */
    uint16_t    segmentation        :1;     /* 1 - segmentation, 0 - no segmentation    */
    uint16_t    type                :4;     /* 0x0A - type 3                            */
} format_t;

typedef struct __attribute__((packed)) {
    uint8_t     flag;                       /* 0x7E                                     */
    uint8_t     format[2];                  /* type, segmentation and length            */
    uint8_t     addr[3];                    /* addr0+addr1 addr2 or addr0 addr1+addr2   */
    uint8_t     control;                    /* control - srnm, disc, etc.               */
} header_t;

typedef struct __attribute__((packed)) {
    header_t    header;                     /* | FLAG | Format | Dest addr | Src addr | Control |   */
    uint8_t     data[PKT_BUFF_MAX_LEN*2];   /* | HCS | Information | FCS | Flag |                   */
} package_t;


typedef struct __attribute__((packed)) {
    uint8_t     format_id;
    uint8_t     group_id;
    uint8_t     length;
} group_t;

typedef struct __attribute__((packed)) {
    uint8_t     id;
    uint8_t     length;
    uint8_t     value[PKT_BUFF_MAX_LEN];
} parameter_t;

typedef struct __attribute__((packed)) {
    uint8_t     class[2];
    uint8_t     obis[6];
    uint8_t     attribute[2];
} request_t;

typedef struct __attribute__((packed)) {
    uint8_t     type;
    uint8_t     size;
    uint8_t     str;
} type_octet_string_t;

typedef struct __attribute__((packed)) {
    uint8_t     type;                       /* 0x05 or 0x06 */
    uint32_t    value;
} type_digit_t; /* int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t */

typedef struct __attribute__((packed)) {
    uint8_t     client_addr;
    uint16_t    server_lower_addr;
    uint16_t    server_upper_addr;
    m_password_t password;
    format_t    format;
    uint16_t    max_info_field_tx;
    uint16_t    max_info_field_rx;
    uint32_t    window_tx;
    uint32_t    window_rx;
    uint8_t     rrr;
    uint8_t     sss;
} meter_t;

typedef struct __attribute__((packed)) {
    size_t      size;
    uint8_t     complete;                   /* 1 - complete, 0 - not complete */
    uint8_t     buff[UART_BUFF_SIZE*2];
} result_package_t;


#endif /* SRC_DEVICES_INCLUDE_NARTIS_100_H_ */
