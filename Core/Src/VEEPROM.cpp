/*
 * VEEPROM.cpp
 *
 *  Created on: Oct 21, 2025
 *      Author: matve
 */

#include "VEEPROM.h"



VEEPROM_Result VEEPROM::init(uint8_t pagesNum) {

	VEEPROM_Result res = VEEPROM_Result::VEEPROM_OK;
	this->pagesNumber = pagesNum+1;
	this->sequence.init(pagesNum);

	uint8_t activeNumber = 0;
	uint8_t actives[127];
	uint16_t activeMinWear = 0xFFFF;
	uint8_t activeMinWearId;

	uint8_t receivingNumber = 0;
	uint8_t receivings[127];
	uint16_t receivingMinWear = 0xFFFF;
	uint8_t receivingMinWeatId;


	uint16_t minWear = 0xFFFF;
	uint8_t minWearId;


	for (uint8_t i = 0; i < this->pagesNumber; i++) {
		if (this->isPageCorrect(i)) {
			this->pages[i] = this->getPageInfo(i);

			if (this->pages[i].status == PageStatus::PAGE_ACTIVE) {
				actives[activeNumber] = i;
				activeNumber++;
				if (this->pages[i].wear <= activeMinWear) {
					activeMinWear = this->pages[i].wear;
					activeMinWearId = i;
				}
			}

			if (this->pages[i].status == PageStatus::PAGE_RECEIVING) {
				receivings[receivingNumber] = i;
				receivingNumber++;
				if (this->pages[i].wear <= receivingMinWear) {
					receivingMinWear = this->pages[i].wear;
					receivingMinWeatId = i;
				}
			}

			if (this->pages[i].status == PageStatus::PAGE_FULL || this->pages[i].status == PageStatus::PAGE_CLEARED) {
				this->sequence.push_overwrite(i);
			}

			if (this->pages[i].wear < minWear) {
				minWear = this->pages[i].wear;
				minWearId = i;
			}

		} else {
			this->initPage(i);
			this->pages[i] = this->getPageInfo(i);
		}
	}


	if (activeNumber > 0) {
		if (activeNumber > 1) {
			for (uint8_t i = 0; i < activeNumber; i++) this->setPageStatus(actives[i], PageStatus::PAGE_CLEARED);
			for (uint8_t i = 0; i < receivingNumber; i++) this->setPageStatus(receivings[i], PageStatus::PAGE_CLEARED);
			this->pages[minWearId].status = PageStatus::PAGE_ACTIVE;
			this->setPageStatus(minWearId, PageStatus::PAGE_ACTIVE);
			this->sequence.push_overwrite(minWearId);
		} else {
			if (receivingNumber > 0) {
				if (receivingNumber > 1) {
					for (uint8_t i = 0; i < receivingNumber; i++) this->setPageStatus(receivings[i], PageStatus::PAGE_CLEARED);
					this->setPageStatus(actives[0], PageStatus::PAGE_ACTIVE);
					this->sequence.push_overwrite(actives[0]);
				} else {
					this->sequence.push_overwrite(actives[0]);

					this->transferPage(actives[0], receivings[0]);
					this->sequence.pop(actives[0]);
					this->setPageStatus(actives[0], PageStatus::PAGE_CLEARED);

					this->pages[receivings[0]].status = PageStatus::PAGE_ACTIVE;
					this->setPageStatus(receivings[0], PageStatus::PAGE_ACTIVE);
					this->sequence.push_overwrite(receivings[0]);
				}
			} else {
				this->sequence.push_overwrite(actives[0]);
			}
		}

	} else {
		if (receivingNumber > 0) {
			if (receivingNumber > 1) {
				for (uint8_t i = 0; i < receivingNumber; i++) this->setPageStatus(receivings[i], PageStatus::PAGE_CLEARED);
				this->pages[minWearId].status = PageStatus::PAGE_ACTIVE;
//				this->formatPage(minWearId);
				this->setPageStatus(minWearId, PageStatus::PAGE_ACTIVE);
				this->sequence.push_overwrite(minWearId);
			} else {
				this->pages[receivings[0]].status = PageStatus::PAGE_ACTIVE;
//				this->formatPage(receivings[0]);
				this->setPageStatus(receivings[0], PageStatus::PAGE_ACTIVE);
				this->sequence.push_overwrite(receivings[0]);
			}
		} else {
			this->pages[minWearId].status = PageStatus::PAGE_ACTIVE;
//			this->formatPage(minWearId);
			this->setPageStatus(minWearId, PageStatus::PAGE_ACTIVE);
			this->sequence.push_overwrite(minWearId);
		}
	}




	return res;
}


VEEPROM_Result VEEPROM::read(uint16_t varId, uint16_t* varValue) {

	HAL_FLASH_Unlock();

	for (int i = 1; i < this->pagesNumber; i++){
		uint8_t pageId;
		this->sequence.peek_at(pagesNumber-i, &pageId);
		uint32_t addr = PAGE_START - pageId * PAGE_SIZE + 0x3E0;

		for (int j = 0; j < 255; j++) {
			uint32_t flashInfo = *(__IO uint32_t*)(addr - j * 4);
			if ((uint16_t)(flashInfo >> 16) == varId) {
				if (((uint8_t)(flashInfo) ^ (uint8_t)(flashInfo >> 8)) == 0) {
					*varValue = (uint16_t)(flashInfo >> 8);
					return VEEPROM_Result::VEEPROM_OK;
				} else {
					return VEEPROM_Result::VEEPROM_ERROR;
				}
			}
		}
	}

	return VEEPROM_Result::VEEPROM_ERROR;
}


VEEPROM_Result VEEPROM::findClearPlace(uint8_t pageId, uint8_t* varIndex) {
	uint32_t addr = PAGE_START - pageId * PAGE_SIZE;
	for (uint8_t i = 1; i < 256; i++) {
		uint32_t flashInfo = *(__IO uint32_t*)(addr + i);
		if (flashInfo == 0x00000000) {
			*varIndex = i;
			return VEEPROM_Result::VEEPROM_OK;
		}
	}
	return VEEPROM_Result::VEEPROM_ERROR;
}


VEEPROM_Result VEEPROM::write(uint16_t varId, uint8_t varValue) {
//	__disable_irq();
	HAL_FLASH_Unlock();

	uint8_t pageId;
	this->sequence.peek_at(pagesNumber-1, &pageId);
	uint32_t addr = PAGE_START - pageId * PAGE_SIZE;

	uint8_t varIndex;
	this->findClearPlace(pageId, &varIndex);

	uint8_t checksum = (((uint32_t)varId) << 16 | ((uint32_t)varValue) << 8) ^ 0x00000000;

	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + varIndex, varId);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + varIndex + 2, ((uint16_t)varId) << 8 | checksum);

//	__enable_irq();
	HAL_FLASH_Lock();

	return VEEPROM_Result::VEEPROM_OK;
}


VEEPROM_Result VEEPROM::transferValue(uint16_t varId) {
	for (int i = 0; i<this->pagesNumber; i++) {
		uint8_t pageId;
		this->sequence.peek_at(this->pagesNumber-i, &pageId);
		for (int j = 0; j< 255; j++) {
			uint32_t flashInfo = *(__IO uint32_t*)(PAGE_START - (pageId + 1) * PAGE_SIZE - (j+1) * 4);
			if ((uint16_t)flashInfo >> 16 == varId) {		// TODO: checksum
				this->write(varId, (uint8_t)flashInfo>>8);
			}
		}
	}
}


bool contains(uint16_t arr[], uint16_t arrSize, uint16_t value) {
	for (int i=0; i<arrSize; i++) {
		if (arr[i] == value) return true;
	}
	return false;
}


VEEPROM_Result VEEPROM::transferPage(uint8_t pageFrom, uint8_t pageTo) {

	uint32_t addrFrom = PAGE_START - pageFrom * PAGESIZE;
	uint32_t addrTo = PAGE_START - pageTo * PAGESIZE;

	uint16_t vaddrs[255];
	uint8_t vaadrsNum = 0;

	for (int i = 0; i < 255; i++) {
		uint32_t flashInfo = *(__IO uint32_t*)(addrFrom + PAGE_SIZE - (i+1) * 4); // TODO: checksum
		if (!(contains(vaddrs, vaadrsNum, (uint16_t)flashInfo>>16))) {
			vaddrs[vaadrsNum] = (uint16_t)flashInfo>>16;
			vaadrsNum++;
			this->transferValue(flashInfo >> 16);
		}
	}
}


VEEPROM_Result VEEPROM::formatPage(uint8_t pageId, uint16_t* wear) {
	FLASH_EraseInitTypeDef FlashErase;
	uint32_t pageError = 0;
	HAL_StatusTypeDef status;
	uint32_t addr = PAGE_START - pageId * PAGE_SIZE;
	*wear = (uint16_t)((*(__IO uint32_t*)(addr)>>16) + 1);

//	HAL_Delay(10);

	__disable_irq();
	status =  HAL_FLASH_Unlock();
	FlashErase.TypeErase = FLASH_TYPEERASE_PAGES;
	FlashErase.PageAddress = addr;
	FlashErase.NbPages = 1;
	if (HAL_FLASHEx_Erase(&FlashErase, &pageError) != HAL_OK)
	{
		HAL_FLASH_Lock();
		__enable_irq();
		return VEEPROM_Result::VEEPROM_ERROR;
	}

//	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, PageStatus::PAGE_CLEARED);
//	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr+2, uses+1);
	HAL_FLASH_Lock();
	__enable_irq();

	return VEEPROM_Result::VEEPROM_OK;
}


VEEPROM_Result VEEPROM::initPage(uint8_t pageId) {
	uint32_t addr = PAGE_START - pageId * PAGE_SIZE;

	FLASH_EraseInitTypeDef FlashErase;
	uint32_t pageError = 0;

//	__disable_irq();
	HAL_FLASH_Unlock();
	FlashErase.TypeErase = FLASH_TYPEERASE_PAGES;
	FlashErase.PageAddress = addr;
	FlashErase.NbPages = 1;
	if (HAL_FLASHEx_Erase(&FlashErase, &pageError) != HAL_OK)
	{
		HAL_FLASH_Lock();
//		__enable_irq();
		return VEEPROM_Result::VEEPROM_ERROR;
	}

	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, PageStatus::PAGE_CLEARED);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr+2, 0x0001);
//	__enable_irq();
	HAL_FLASH_Lock();

	return VEEPROM_Result::VEEPROM_OK;
}


bool VEEPROM::isPageCorrect(uint8_t pageId) {
	uint32_t flashInfo = *(__IO uint32_t*)(PAGE_START - pageId * PAGE_SIZE);
	if (((uint16_t)flashInfo == 0x00FF) || ((uint16_t)flashInfo == 0xFF00) ||
			((uint16_t)flashInfo == 0x0F0F) || ((uint16_t)flashInfo == 0xF0F0)) {
		return true;
	}
	return false;
}


PageInfo VEEPROM::getPageInfo(uint8_t pageId) {

	PageInfo info;
	uint32_t flashInfo = *(__IO uint32_t*)(PAGE_START - pageId * PAGE_SIZE);

	info.status = (PageStatus)flashInfo;
	info.wear = (uint16_t)(flashInfo >> 16);

	return info;
}


VEEPROM_Result VEEPROM::setPageStatus(uint8_t pageId, PageStatus status) {
	uint32_t addr = PAGE_START - pageId * PAGE_SIZE;

	uint16_t* wear;
	this->formatPage(pageId, wear);

//	__disable_irq();
	HAL_FLASH_Unlock();

	HAL_StatusTypeDef res;
	res = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, (uint16_t)status);
	res = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr+2, *wear);

	HAL_FLASH_Lock();
//	__enable_irq();

	return VEEPROM_Result::VEEPROM_OK;
}




