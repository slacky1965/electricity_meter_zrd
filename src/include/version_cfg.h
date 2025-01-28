#if (CHIP_FLASH_SIZE == 512)
#include "version_cfg_512k.h"
#elif (CHIP_FLASH_SIZE == 1024)
#include "version_cfg_1m.h"
#else
#error CHIP_TYPE must be TLSR_8258_512K or TLSR_8258_1M
#endif
