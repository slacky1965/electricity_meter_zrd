//#include <stdio.h>
//#include <stdint.h>
//#include <string.h>
//#include <stdlib.h>
#include "tl_common.h"

#include "hdlc.h"
#include "os_util.h"

#ifndef BIT
#define BIT(x) (1U << (x))
#endif

/*
 === SNRM (CONNECTION) EXAMPLE WITHOUT ANY PARAMETER NEGOCIATION

7EA00A00020023F193232E7E


7E A00A : frame type=3, size=10
   0002 0023 // dest address (server)
      ^    ^
      |  	|___ physical address (0x23 / 2 = 17)
	  |_________ logical device (2 / 2 = 1)
   F1 : client address (0xF1 / 2 = 120)
    
   93 : control field (here: SNRM)
         100 1    0011
		     ^
			 P/F
   
   
   232E : FCS (no info field)
7E

*/

/*
 === SNRM (CONNECTION) EXAMPLE WITHOUT PARAMETER NEGOCIATION

 7EA0210002002321931964818012050180060180070400000001080400000007655E7E
 
 7EA021 
       0002002321
	   
	   93 CF
	   
	   1964 HCS
	   
	   818012050180060180070400000001080400000007
	   
	   
655E7E

*/

static const uint16_t fcstab[256] = { 
     0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 
     0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 
     0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 
     0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 
     0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 
     0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 
     0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 
     0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 
     0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 
     0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 
     0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 
     0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 
	 0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 
     0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 
     0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 
     0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 
     0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 
     0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 
     0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 
     0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 
     0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 
     0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 
     0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 
     0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 
     0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 
     0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 
     0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 
     0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 
     0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 
     0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 
     0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 
     0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78 
}; 

#define PPPINITFCS16    0xffff  /* Initial FCS value */ 
/* 
* Calculate a new fcs given the current fcs and the new data. 
*/ 
uint16_t pppfcs16(uint16_t fcs, const uint8_t* cp, uint32_t len) 
{ 
	while (len--)
	{
		fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff]; 
	}
	return (fcs); 
} 


// MASKS
#define HDLC_FORMAT_TYPE	(0xA0)
#define HDLC_LEN_HI         (0x03)

// BITS
#define HDLC_SEGMENTATION_BIT	(3)
#define HDLC_POLL_FINAL_BIT	    (4)

static uint16_t hdlc_get_len(const uint8_t *buf)
{
	uint16_t len = (buf[0] & HDLC_LEN_HI) << 11;
	return (len + buf[1]);
}

/*
8.4.1.1  HDLC frame format type 3 
The MAC sublayer uses the HDLC frame format type 3 as defined in Annex H.4 of ISO/IEC 13239. It 
is shown on Figure 49: 
Flag  Frame format  Dest. address  Src. address  Control  HCS  Information  FCS  Flag 
*/

void hdlc_init(hdlc_t *hdlc)
{
	hdlc->logical_device = 0U;
	hdlc->phy_address = 0U;
	hdlc->client_addr = 0U;
	hdlc->addr_len = 4U;
	hdlc->segmentation = 0U;
	hdlc->frame_size = 0U;
	hdlc->rrr = 0U;
	hdlc->sss = 0U;
	hdlc->type = 0U;
	hdlc->poll_final = 0U;
	hdlc->cmd_resp = 0U;
	hdlc->data_index = 0U;
	hdlc->data_size = 0U;
	hdlc->sender = HDLC_SERVER;
	hdlc->max_info_field_tx = 128U;
	hdlc->max_info_field_rx = 128U;
	hdlc->window_rx = 1U;
	hdlc->window_tx = 1U;
}

/**

The client address shall always be expressed on one byte. 
The  server  address  �  to  enable  addressing  more  than  one  logical  device  within  a  single  physical
device and to support the multi-drop configuration � may be divided into two parts:
the  upper  HDLC  address  is  used  to  address  a  Logical  Device  (a  separately  addressable  entity within a physical device);
the  lower  HDLC  address  is  used  to  address  a  Physical  Device  (a  physical  device  on  the  multi-drop).
The upper HDLC address shall always be present. The  lower HDLC  address may  be omitted if it is 
not required. 

*/

static const uint8_t cMAX_SIZE = 4U; // address size is 4 bytes max

uint8_t hdlc_decode_addr_size(const uint8_t *buf, uint16_t size)
{
	uint8_t addr_size = 0U;
	uint8_t i = 0U;
	
	// Compute limit of the loop
	uint8_t max = (size < cMAX_SIZE) ? size : cMAX_SIZE;
	
	for (i = 0U; i < max; i++)
	{
		addr_size += 1;
		if ((buf[i] % 2U) != 0)
		{
			break;
		}
	}
	return addr_size;
}

uint16_t hdlc_getuint16_addr(const uint8_t *buf)
{
	uint16_t data = (uint16_t)(buf[0]>>1);
	data = data << 8U;
	data = data + (buf[1]>>1);
	return data;
}


void hdlc_set_uint16_low_first(uint8_t *buf, uint16_t val)
{
    buf[0] = (uint8_t)(val & 0x00FFU);
    buf[1U] = (uint8_t)((val >> 8U) & 0x00FFU);
}

int hdlc_get_addr(const uint8_t *buf, uint8_t addr_size, uint16_t *upper, uint16_t *lower)
{
	int ret = HDLC_OK;
	switch(addr_size)
	{
	case 1:
		*upper = (uint16_t)(buf[0]>>1);
		break;
	case 2:
		*upper = (uint16_t)(buf[0]>>1);
		*lower = (uint16_t)(buf[1]>>1);
		break;
	case 4:
		*upper = hdlc_getuint16_addr(&buf[0]);
		*lower = hdlc_getuint16_addr(&buf[2]);
		break;
	 default:
	   ret = HDLC_ERR_ADDR;
	}
	return ret;
}

int hdlc_set_addr(uint8_t *buf, uint8_t addr_size, uint16_t upper, uint16_t lower)
{
    int ret = HDLC_OK;
    switch(addr_size)
    {
    case 1:
        buf[0] = ((uint8_t)(upper << 1U)) | 0x01U;
        break;
    case 2:
        buf[0] = ((uint8_t)(upper << 1U)) & 0x7EU;
        buf[1] = ((uint8_t)(lower << 1U)) | 0x01U;
        break;
    case 4:
        buf[0] = ((uint8_t)(upper >> 7U)) & 0x7EU;
        buf[1] = ((uint8_t)(upper << 1U)) & 0x7EU;
        buf[2] = ((uint8_t)(lower >> 7U)) & 0x7EU;
        buf[3] = ((uint8_t)(lower << 1U)) | 0x01U;
        break;
     default:
       ret = HDLC_ERR_ADDR;
    }
    return ret;
}

#define HDLC_CF_POLL	0
#define HDLC_CF_FINAL	1

const char* hdlc_packet_to_string(hdlc_t *hdlc)
{
	if (hdlc->type == HDLC_PACKET_TYPE_I)
	{
		return "Information";
	}
	else if (hdlc->type == HDLC_PACKET_TYPE_RR)
	{
		return "RR";
	}
	else if (hdlc->type == HDLC_PACKET_TYPE_RNR)
	{
		return "RNR";
	}
	else if (hdlc->type == HDLC_PACKET_TYPE_SNRM)
	{
		return "SNRM";
	}
	else if (hdlc->type == HDLC_PACKET_TYPE_DISC)
	{
		return "DISC";
	}
	else if (hdlc->type == HDLC_PACKET_TYPE_UA)
	{
		return "UA";
	}
	else if (hdlc->type == HDLC_PACKET_TYPE_DM)
	{
		return "DM";
	}
	else if (hdlc->type == HDLC_PACKET_TYPE_FRMR)
	{
		return "FRMR";
	}
	else if (hdlc->type == HDLC_PACKET_TYPE_UI)
	{
		return "UI";
	}
	else
	{
		return "Bad packet";
	}
}


typedef struct
{
	uint8_t type;	// HDLC packet type
	uint8_t mask;	// Mask to apply to detect the packet type
	uint8_t value;	// Expected value that must have after the mask has been applied
} hdlc_packet_t;

/**
 *
                       MSB             LSB 
  Command    Response   
	I  			I  		R R R P/F S S S 0 
	RR  		RR  	R R R P/F 0 0 0 1 
	RNR  		RNR  	R R R P/F 0 1 0 1 
	SNRM    			1 0 0  P  0 0 1 1 
	DISC    			0 1 0  P  0 0 1 1 
				UA 		0 1 1  F  0 0 1 1 
				DM  	0 0 0  F  1 1 1 1 
				FRMR  	1 0 0  F  0 1 1 1 
	UI  		UI  	0 0 0 P/F 0 0 1 1 
*/

static const hdlc_packet_t cPACKET_TYPES[] = {
	{ HDLC_PACKET_TYPE_I, 		0x01U, 0x00U },
	{ HDLC_PACKET_TYPE_RR, 		0x0FU, 0x01U },
	{ HDLC_PACKET_TYPE_RNR, 	0x0FU, 0x05U },
	{ HDLC_PACKET_TYPE_SNRM, 	0xEFU, 0x83U },
	{ HDLC_PACKET_TYPE_DISC, 	0xEFU, 0x43U },
	{ HDLC_PACKET_TYPE_UA, 		0xEFU, 0x63U },
	{ HDLC_PACKET_TYPE_DM, 		0xEFU, 0x0FU },
	{ HDLC_PACKET_TYPE_FRMR, 	0xEFU, 0x87U },
	{ HDLC_PACKET_TYPE_UI, 		0xEFU, 0x03U }
};

static const uint32_t cNUMBER_OF_PACKETS = sizeof(cPACKET_TYPES)/sizeof(cPACKET_TYPES[0]);

int hdlc_decode_control_field(hdlc_t *hdlc, const uint8_t cf)
{
	int ret = HDLC_ERR;
	
	for (uint32_t i = 0U; i < cNUMBER_OF_PACKETS; i++)
	{
		if ((cf & cPACKET_TYPES[i].mask) == cPACKET_TYPES[i].value)
		{
			hdlc->type = cPACKET_TYPES[i].type;
			ret = HDLC_OK;
			break;
		}
	}
	
	hdlc->poll_final = is_bit_set(cf, HDLC_POLL_FINAL_BIT);
	// always extract rrr and sss even if it is not the right packet
	hdlc->rrr = (cf >> 5) & 0x07U;
	hdlc->sss = (cf >> 1) & 0x07U;
	
	return ret;
}

static uint32_t hdlc_read_option(const uint8_t *buf, uint8_t size)
{
    uint32_t option = 0U;
    if (size == 1U)
    {
        option = buf[0];
    }
    else if (size == 2U)
    {
        option = GET_BE16(&buf[0]);
    }
    else if (size == 4U)
    {
        option = GET_BE32(&buf[0]);
    }
    else
    {
        option = 0U;
    }

    return option;
}

int hdlc_decode_info_field(hdlc_t *hdlc, const uint8_t *buf, uint16_t info_field_size)
{
	int ret = HDLC_OK;

	if (info_field_size > 0U)
	{
        switch(hdlc->type)
        {
            case HDLC_PACKET_TYPE_SNRM:
            case HDLC_PACKET_TYPE_UA:
            {
                if ((buf[0] == 0x81U) &&
                    (buf[1] == 0x80U) &&
                    ((buf[2] + 3U) == info_field_size))
                {
                    // Decode framing options
                    uint8_t index = 3U;
                    uint8_t number_of_tags = 0U;
                    while (ret == HDLC_OK)
                    {
                        uint8_t tag = buf[index];
                        index++;
                        uint8_t size = buf[index];
                        index++;

                        if (tag == 0x05U)
                        {
                            uint32_t opt = hdlc_read_option(&buf[index], size);
                            if (opt)
                            {
                                hdlc->max_info_field_tx = opt;
                                number_of_tags++;
                                index += size;
                            }
                            else
                            {
                                ret = HDLC_ERR_NEGO;
                            }
                        }
                        else if (tag == 0x06U)
                        {
                            uint32_t opt = hdlc_read_option(&buf[index], size);
                            if (opt)
                            {
                                hdlc->max_info_field_rx = opt;
                                number_of_tags++;
                                index += size;
                            }
                            else
                            {
                                ret = HDLC_ERR_NEGO;
                            }
                        }
                        else if (tag == 0x07U)
                        {
                            uint32_t opt = hdlc_read_option(&buf[index], size);
                            if (opt)
                            {
                                hdlc->window_tx = opt;
                                number_of_tags++;
                                index += size;
                            }
                            else
                            {
                                ret = HDLC_ERR_NEGO;
                            }
                        }
                        else if (tag == 0x08U)
                        {
                            uint32_t opt = hdlc_read_option(&buf[index], size);
                            if (opt)
                            {
                                hdlc->window_rx = opt;
                                number_of_tags++;
                                index += size;
                            }
                            else
                            {
                                ret = HDLC_ERR_NEGO;
                            }
                        }

                        if (number_of_tags >= 4U)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    ret = HDLC_ERR_NEGO;
                }
                break;
            }

            default :
                break;
        }
	}
		
	return ret;
}

int hdlc_check_fcs(const uint8_t* buf, uint16_t size)
{
	uint16_t fcs = pppfcs16(PPPINITFCS16, &buf[1], size-4);
	fcs ^= 0xffff;
	
	// check last two bytes before the last 7E
	uint16_t expected = buf[size-2];
	expected = (expected << 8) + buf[size-3];
	
//	debug_print("FCS calculated: 0x%.4X\r\n", fcs);
//	debug_print("FCS expected: 0x%.4X\r\n", expected);
	
	return (expected == fcs) ? HDLC_OK : HDLC_ERR_FCS;
}

// buf: pointer to the start of the frame
// size: header size, including HCS and 7E
int hdlc_check_hcs(const uint8_t* buf, uint16_t size)
{
	uint16_t hcs = pppfcs16(PPPINITFCS16, &buf[1], size-3); // 3 = minus 7E and HCS
	hcs ^= 0xffff;
	
	// check last two bytes before the last 7E
	uint16_t expected = buf[size-1];
	expected = (expected << 8) + buf[size-2];
	
//	debug_print("HCS calculated: 0x%.4X\r\n", hcs);
//	debug_print("HCS expected: 0x%.4X\r\n", expected);
	
	return (expected == hcs) ? HDLC_OK : HDLC_ERR_HCS;
}


static const uint8_t snrm_nego[] = { 0x81U, 0x80U, 0x12U, 0x05U, 0x01U, 0x80U, 0x06U, 0x01U, 0x80U, 0x07U, 0x04U, 0x00U, 0x00U, 0x00U, 0x01U,
                                     0x08U, 0x04U, 0x00U, 0x00U, 0x00U, 0x07 };
static const uint16_t snrm_size = sizeof(snrm_nego);

int hdlc_encode_snrm(hdlc_t *hdlc, uint8_t *buf, uint16_t size)
{
    hdlc->type = HDLC_PACKET_TYPE_SNRM;
    return hdlc_encode(hdlc, buf, size, 0x93U, snrm_nego, snrm_size);
}

int hdlc_encode_data(hdlc_t *hdlc, uint8_t *buf, uint16_t size, const uint8_t *data, uint16_t data_size)
{
    uint8_t iframe = (((hdlc->rrr << 5U) + (hdlc->sss << 1U)) | 0x10U) & 0xFEU;
    hdlc->type = HDLC_PACKET_TYPE_I;
    return hdlc_encode(hdlc, buf, size, iframe, data, data_size);
}

int hdlc_encode_rr(hdlc_t *hdlc, uint8_t *buf, uint16_t size)
{
    uint8_t iframe = ((hdlc->rrr << 5U) & 0xF0U) | 0x11U; // Final bit = 1
    hdlc->type = HDLC_PACKET_TYPE_RR;
    return hdlc_encode(hdlc, buf, size, iframe, NULL, 0U);
}


// Size is the max size of the buffer
int hdlc_encode(hdlc_t *hdlc, uint8_t *buf, uint16_t size, uint8_t frame_type, const uint8_t *data, uint16_t data_size)
{
    uint16_t index = 0U;
    uint16_t info_field_start;
    int ret = -1;

    (void) size;

    buf[index] = 0x7EU;

    // Bytes 1 and 2 contains frame length, will be encoded at the end
    index += 3U; // Jump 7E + frame type/length

    // Encode destination address

    if (hdlc->sender == HDLC_SERVER)
    {
        // Destination is client, always 1 byte
        hdlc_set_addr(&buf[index], 1U, hdlc->client_addr, 0U); // lower not used
        index++;
        // source is server
        hdlc_set_addr(&buf[index], hdlc->addr_len, hdlc->logical_device, hdlc->phy_address);
        index += hdlc->addr_len;
    }
    else
    {
        // Destination is server, encode address
        hdlc_set_addr(&buf[index], hdlc->addr_len, hdlc->logical_device, hdlc->phy_address);
        index += hdlc->addr_len;
        // source client, always 1 byte
        hdlc_set_addr(&buf[index], 1U, hdlc->client_addr, 0U); // lower not used
        index++;
    }

    // Encode UA
    buf[index] = frame_type;
    index++; // jump over frame type

    if ((data_size > 0U) &&
        (data != NULL))
    {
        index += 2U; // jump over HCS
        // memorize info field location
        info_field_start = index;

        // Copy info field
        memcpy(&buf[info_field_start], &data[0], data_size);
        index += data_size;
    }

    uint16_t frame_size = index + 1U; // Total size including FCS, without both 7E

    // Insert frame type, contains size
    buf[1] = 0xA0 + ((frame_size >> 8U) & 0x07U);
    buf[2] = (uint8_t)(frame_size & 0xFFU);

    if ((data_size > 0U) &&
            (data != NULL))
    {
        // Compute HCS
        uint16_t hcs = pppfcs16(PPPINITFCS16, &buf[1], (info_field_start - 3U)); // minus 7E and HCS
        hcs ^= 0xffff;

        // insert HCS
        hdlc_set_uint16_low_first(&buf[info_field_start - 2U], hcs);
    }

    // Now compute FCS
    uint16_t fcs = pppfcs16(PPPINITFCS16, &buf[1], frame_size - 2U); // without FCS itself
    fcs ^= 0xffff;

    // Insert FCS
    hdlc_set_uint16_low_first(&buf[index], fcs);

    index += 2U; // jump over FCS
    buf[index] = 0x7EU; // final
    index++;

    if (index <= size)
    {
        ret = index;
    }

    return ret;
}


int hdlc_decode(hdlc_t *hdlc, const uint8_t *buf, uint16_t size)
{
	int ret = HDLC_ERR;

	// test packet structure
	if (buf[0] == 0x7E)
	{
        // next byte is the frame format
        uint8_t format = buf[1] & HDLC_FORMAT_TYPE;

        if (format == HDLC_FORMAT_TYPE)
        {
            hdlc->segmentation = is_bit_set(buf[1], HDLC_SEGMENTATION_BIT);

            // We have the real length of the frame, now we can test the whole frame structure
            hdlc->frame_size = hdlc_get_len(&buf[1]) + 2U; // The value of the frame length subfield is the count of octets in the frame excluding the opening and
                                                        // closing frame flag sequences.

            if ((hdlc->frame_size <= size) && (buf[hdlc->frame_size-1] == 0x7E))
            {
                // Test FCS, always present
                ret = hdlc_check_fcs(buf, hdlc->frame_size);

                // Sanity check:
                if (ret == HDLC_OK)
                {
                    uint16_t dummy; // stub temporary variable

                    // FIXME: Sanity check: test a minimal size
                    // 7E + frame format + dest + src +         + FCS + 7E
                    //  1        2           1      1              2     1

                    const uint8_t* ptr = &buf[3];

                    // Destination address decoder
                    uint8_t addr_size = hdlc_decode_addr_size(ptr, hdlc->frame_size);

                    if (hdlc->sender == HDLC_CLIENT)
                    {
                        ret = hdlc_get_addr(ptr, addr_size, &hdlc->logical_device, &hdlc->phy_address);
                        hdlc->addr_len = addr_size; // update server address size
                    }
                    else
                    {
                        // client address is always transmitted on 1 byte
                        if (addr_size == 1)
                        {
                            ret = hdlc_get_addr(ptr, addr_size, &hdlc->client_addr, &dummy);
                        }
                        else
                        {
                            ret = HDLC_ERR_ADDR;
                        }
                    }

                    if (!ret)
                    {
                        // advance to source address
                        ptr += addr_size;
                        addr_size = hdlc_decode_addr_size(ptr, hdlc->frame_size);

                        if (hdlc->sender == HDLC_CLIENT)
                        {
                            // client address is always transmitted on 1 byte
                            if (addr_size == 1U)
                            {
                                ret = hdlc_get_addr(ptr, addr_size, &hdlc->client_addr, &dummy);
                            }
                            else
                            {
                                ret = HDLC_ERR_ADDR;
                            }
                        }
                        else
                        {
                            ret = hdlc_get_addr(ptr, addr_size, &hdlc->logical_device, &hdlc->phy_address);
                            hdlc->addr_len = addr_size; // update server address size
                        }

                        if (!ret)
                        {
                            // Advance to next frame part
                            ptr += addr_size;
                            // now decode the control field
                            ret = hdlc_decode_control_field(hdlc, *ptr);

                            if (!ret)
                            {
                                ptr += 1; // skip control field

                                // Now check the user information, if any.
                                // compute the remaining data size
                                // We remove FCS and last 7E
                                uint16_t remaining_size = (uint16_t)(&buf[hdlc->frame_size-3] - ptr);

                                // If there is an HCS, then there is data. HCS is 2 bytes length
                                if (remaining_size >= 2U)
                                {
                                    ptr += 2U; // jump over HCS
                                    uint16_t header_size = (uint16_t)(ptr - &buf[0]); // include the HCS to get it and test it
                                    // Compute Header checksum
                                    ret = hdlc_check_hcs(buf, header_size);

                                    if (!ret)
                                    {
                                        // Info field size
                                        hdlc->data_index = (uint16_t)(ptr - &buf[0]);
                                        hdlc->data_size = remaining_size - 2U;
                                    }
                                }
                            }
                        }
                        else
                        {
                            ret = HDLC_ERR_ADDR;
                        }
                    }
                    // return error code is already set here
                }
                // return error code is already set here
            }
            else
            {
                ret = HDLC_ERR_SIZE;
            }
        }
        else
        {
            ret = HDLC_ERR_FORMAT;
        }
	}
	else
	{
		ret = HDLC_ERR_7E;
	}
	return ret;
}

void hdlc_print_result(hdlc_t *hdlc, int code)
{
	if (code == HDLC_OK)
	{
		printf("Success!\r\n");
		
		// Then print HDLC parameters:
		printf("Segmentation: %s\r\n", hdlc->segmentation ? "yes" : "no");
		printf("Frame size: %d\r\n", hdlc->frame_size);
		printf("Data size: %d\r\n", hdlc->data_size);
		printf("Data index: %d\r\n", hdlc->data_index);
		printf("Physical address: %d\r\n", hdlc->phy_address);
		printf("Logical device: %d\r\n", hdlc->logical_device);
		printf("Client SAP: %d\r\n", hdlc->client_addr);
		printf("Packet type: %d (%s)\r\n", hdlc->type, hdlc_packet_to_string(hdlc));
		printf("N(Sender): %d\r\n", hdlc->sss);
		printf("N(Receiver): %d\r\n", hdlc->rrr);
		printf("Poll/Final bit: %d\r\n", hdlc->poll_final);
		printf("Max info field TX: %d\r\n", hdlc->max_info_field_tx);
		printf("Max info field RX: %d\r\n", hdlc->max_info_field_rx);
		printf("Window size TX: %d\r\n", hdlc->window_tx);
		printf("Window size RX: %d\r\n", hdlc->window_rx);
	}
	else if (code == HDLC_ERR_7E)
	{
		printf("Packet must start and finish with 0x7E\r\n");
	}
	else if (code == HDLC_ERR)
	{
		printf("Generic error\r\n");
	}
	else if (code == HDLC_ERR_FCS)
	{
		printf("Bad checksum\r\n");
	}
	else
	{
		printf("Unknown error code: %d\r\n", code);
	}
}



