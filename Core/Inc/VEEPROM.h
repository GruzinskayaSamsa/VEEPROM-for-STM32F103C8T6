/*
 * VEEPROM.h
 *
 *  Created on: Oct 21, 2025
 *      Author: matve
 */

#ifndef INC_VEEPROM_H_
#define INC_VEEPROM_H_

//#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal.h"
#include "StaticCircularBuffer.h"


#define PAGE_SIZE 0x400 		//page size for STM32F103C8T6 - 1 KB

#define PAGE_END 0x8000000
#define PAGE_START 0x800FC00


typedef enum {
	VEEPROM_OK = 0,
	VEEPROM_ERROR = 1
} VEEPROM_Result;

typedef enum: uint16_t {
	PAGE_ACTIVE = 0xF0F0,
	PAGE_CLEARED = 0x0F0F,
	PAGE_RECEIVING = 0xFF00,
	PAGE_FULL = 0x00FF
} PageStatus;


struct PageInfo {
	PageStatus status;
	uint16_t wear;
};


class VEEPROM {


public:
	VEEPROM_Result init(uint8_t pagesNum);

	VEEPROM_Result read(uint16_t varId, uint16_t* varValue);
	VEEPROM_Result write(uint16_t varId, uint8_t varValue);

private:

	PageInfo pages[128];
	uint8_t pagesNumber;
	StaticCircularBuffer<uint8_t> sequence;

	VEEPROM_Result format();
	VEEPROM_Result formatPage(uint8_t pageId, uint16_t* wear);
	VEEPROM_Result initPage(uint8_t pageId);
	VEEPROM_Result findValidPage(uint8_t* id);
	VEEPROM_Result transferPage(uint8_t pageFrom, uint8_t pageTo);

	VEEPROM_Result transferValue(uint16_t varId);

	PageInfo getPageInfo(uint8_t pageId);
	VEEPROM_Result setPageStatus(uint8_t pageId, PageStatus info);

	VEEPROM_Result findClearPlace(uint8_t pageId, uint8_t* varIndex);

	bool isPageCorrect(uint8_t pageId);

};

#endif /* INC_VEEPROM_H_ */
