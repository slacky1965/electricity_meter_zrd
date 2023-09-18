#ifndef SRC_INCLUDE_APP_TEMPERATURE_H_
#define SRC_INCLUDE_APP_TEMPERATURE_H_

// OneWire commands
#define SKIPROM         0xCC
#define STARTCONVO      0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define COPYSCRATCH     0x48  // Copy EEPROM
#define READSCRATCH     0xBE  // Read EEPROM
#define WRITESCRATCH    0x4E  // Write to EEPROM
#define RECALLSCRATCH   0xB8  // Reload from last known
#define READPOWERSUPPLY 0xB4  // Determine if device needs parasite power
#define ALARMSEARCH     0xEC  // Query bus for devices with an alarm condition

// Scratchpad locations
#define TEMP_LSB        0
#define TEMP_MSB        1
#define HIGH_ALARM_TEMP 2
#define LOW_ALARM_TEMP  3
#define CONFIGURATION   4
#define INTERNAL_BYTE   5
#define COUNT_REMAIN    6
#define COUNT_PER_C     7
#define SCRATCHPAD_CRC  8

void ds18b20_init();
s32 getTemperatureCb(void *arg);

//#if defined(MCU_CORE_8258)
//void adc_temp_init();
//s16 adc_temp_result();
//#endif

#endif /* SRC_INCLUDE_APP_TEMPERATURE_H_ */
