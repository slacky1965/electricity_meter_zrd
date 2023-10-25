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
    uint8_t     data[PKT_BUFF_MAX_LEN*2];   /* | HCS | Information | FCS | Flag |       */
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
    uint8_t     client_addr;
    uint16_t    server_lower_addr;
    uint16_t    server_upper_addr;
    uint8_t     password[8];
    format_t    format;
    uint16_t    max_info_field_tx;
    uint16_t    max_info_field_rx;
    uint32_t    window_tx;
    uint32_t    window_rx;
    package_t   package;
} meter_t;


#endif /* SRC_DEVICES_INCLUDE_NARTIS_100_H_ */
