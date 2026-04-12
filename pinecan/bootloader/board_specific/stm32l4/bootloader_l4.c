#include "bootloader_l4.h"

void programBootloader(struct uavcan_protocol_file_ReadResponse pkt){

	HAL_FLASH_Unlock();

	uint32_t bytes_written = 0;
	while(bytes_written < pkt.data.len){
		uint8_t chunk[8] = {0xFF}; //Default erased value
		uint32_t remain = pkt.data.len - bytes_written;
		uint32_t to_copy = (remain >= 8U) ? 8U : remain;

		memcpy(chunk, &pkt.data.data[bytes_written], to_copy);

		uint64_t double_word;

		memcpy(&double_word, chunk, 8); // Makes a 64 bit object for the HAL_FLASH

		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, fwupdate.flash_address, double_word) != HAL_OK){
			HAL_FLASH_Lock();
			fwupdate.in_progress = false;
			fwupdate.node_id = 0;
			return;
		}
		//Double word means it writes 64 bits at a time (a word is 32 bits)
		//Might have to change around the values
		fwupdate.flash_address += 8U; //8 bytes long = 64 bits
		bytes_written += to_copy;
	}
	HAL_FLASH_Lock();

	fwupdate.offset += pkt.data.len;
	fwupdate.last_read_ms = 0; // makes it so fwupdate check in sendFirmwareRead will go through right away
}


void eraseStagingFlash(void){

	HAL_FLASH_Unlock();

	FLASH_EraseInitTypeDef erase = {0};
	uint32_t page_error = 0;
	// This assumes a 32kb bootloader size. (From 0x08000000 - 0x08008000)
	uint32_t page_num = (APP_START_ADDR - FLASH_BASE_ADDR)/FLASH_PAGE_SIZE; //(0x08020000-0x08000000)/0x800; //0x800 is 2kb which is 1 page length according to data sheet
	// Line above shows starting page for memory wipe
	fwupdate.flash_address =  APP_START_ADDR;//0x08020000; // Need to change this maybe to make sure that bootloader is not affected
	erase.Page = page_num;
	erase.NbPages = 16; //Assuming 32kb for staging slot
	erase.TypeErase = FLASH_TYPEERASE_PAGES;
	//erase.Banks     = FLASH_BANK_1; // Always specify on STM32L4/F4
	if(HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK){
		//TODO: handle error
	}

	HAL_FLASH_Lock();

}

uint32_t bootloaderGetTime(void){
	return HAL_GetTick();
}