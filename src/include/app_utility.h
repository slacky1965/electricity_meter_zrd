#ifndef SRC_INCLUDE_APP_UTILITY_H_
#define SRC_INCLUDE_APP_UTILITY_H_

#define BUILD_U48(b0, b1, b2, b3, b4, b5)   ( (u64)((((u64)(b5) & 0x0000000000ff) << 40) + (((u64)(b4) & 0x0000000000ff) << 32) + (((u64)(b3) & 0x0000000000ff) << 24) + (((u64)(b2) & 0x0000000000ff) << 16) + (((u64)(b1) & 0x0000000000ff) << 8) + ((u64)(b0) & 0x00000000FF)) )

u32 itoa(u32 value, u8 *ptr);
u32 from24to32(const u8 *str);
u64 fromPtoInteger(u16 len, u8 *data);
u8 set_zcl_str(u8 *str_in, u8 *str_out, u8 len);


#endif /* SRC_INCLUDE_APP_UTILITY_H_ */
