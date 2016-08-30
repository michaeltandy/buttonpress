extern "C" {
#include "spi_flash.h" // Provides SPI_FLASH_SEC_SIZE (usually 4096)
}

#ifdef EEPROM_h
/* By default we use the same eeprom region as EEPROM.cpp so if that's loaded
 * this shouldn't be and vice-versa. */
typedef char appendqueue_is_incompatible_with_eeprom_h[-1];
#endif

//#define COMPILE_TIME_ASSERT(test) typedef char compile_time_assert[(test)?0:-1]

extern "C" uint32_t _SPIFFS_end; // Usually points to 0x405FB000

const uint32_t MAGIC_HEADER = 71449302;
const uint32_t START_ADDRESS_BYTES = (uint32_t)&_SPIFFS_end - 0x40200000;
const uint32_t START_ADDRESS_FLASH_SECTORS = START_ADDRESS_BYTES/SPI_FLASH_SEC_SIZE;

/**
 * Tests the behaviour of the esp8266 flash chip.
 * Aim: Confirm we're using an interface where the NAND flash behaves like NAND flash.
 * Output is:
 * Result of zeroth read: 0xFFFFFFFF
 * Result of first read: 0xF0F0F0F0
 * Result of second read: 0xF0F0
 */
void testFlash() {
  Serial.println("Testing flash behaviour...");

  /*Serial.print("SPI1CLK: 0x");
  Serial.println(SPI1CLK, HEX);
  Serial.print("&SPI1CLK: 0x");
  Serial.println((int)&SPI1CLK, HEX);*/

  Serial.print("SPI0CLK: 0x");
  Serial.println(SPI0CLK, HEX);
  Serial.print("&SPI0CLK: 0x");
  Serial.println((int)&SPI0CLK, HEX);
  SPI0CLK = 0x00D43002;
  Serial.print("SPI0CLK set to: 0x");
  Serial.println(SPI0CLK, HEX);
  
  if(spi_flash_erase_sector_nointerrupts(START_ADDRESS_FLASH_SECTORS) != SPI_FLASH_RESULT_OK) {
    Serial.println("Problem erasing sector?");
    return;
  }

  {
    uint32_t readBuf[1] = {0};
    if (spi_flash_read_nointerrupts(START_ADDRESS_BYTES, readBuf, sizeof(readBuf)) != SPI_FLASH_RESULT_OK) {
      Serial.println("Problem in zeroth read?");
      return;
    }
    Serial.print("Result of zeroth read: 0x");
    Serial.println(readBuf[0], HEX);
  }
  
  {
    uint32_t writeBuf[1] = {0xF0F0F0F0};
    if (spi_flash_write_nointerrupts(START_ADDRESS_BYTES, writeBuf, sizeof(writeBuf)) != SPI_FLASH_RESULT_OK) {
      Serial.println("Problem in first write?");
      return;
    }
  }

  /*{
    uint32_t readBuf[1] = {0};
    if (spi_flash_read_nointerrupts(START_ADDRESS_BYTES, readBuf, sizeof(readBuf)) != SPI_FLASH_RESULT_OK) {
      Serial.println("Problem in first read?");
      return;
    }
    Serial.print("Result of first read: 0x");
    Serial.println(readBuf[0], HEX);
  }

  {
    uint32_t writeBuf[1] = {0x0000FFFF};
    if (spi_flash_write_nointerrupts(START_ADDRESS_BYTES, writeBuf, sizeof(writeBuf)) != SPI_FLASH_RESULT_OK) {
      Serial.println("Problem in second write?");
      return;
    }
  }

  {
    uint32_t readBuf[1] = {0};
    if (spi_flash_read_nointerrupts(START_ADDRESS_BYTES, readBuf, sizeof(readBuf)) != SPI_FLASH_RESULT_OK) {
      Serial.println("Problem in second read?");
      return;
    }
    Serial.print("Result of second read: 0x");
    Serial.println(readBuf[0], HEX);
  }

  {
    uint32_t writeBuf[2] = {0b10101010, 0xFF00FF00};
    if (spi_flash_write_nointerrupts(START_ADDRESS_BYTES, writeBuf, sizeof(writeBuf)) != SPI_FLASH_RESULT_OK) {
      Serial.println("Problem in third write?");
      return;
    }
  }

  {
    uint32_t readBuf[2] = {0};
    if (spi_flash_read_nointerrupts(START_ADDRESS_BYTES, readBuf, sizeof(readBuf)) != SPI_FLASH_RESULT_OK) {
      Serial.println("Problem in third read?");
      return;
    }
    Serial.print("Result of third read: 0b");
    Serial.print(readBuf[0], BIN);
    Serial.print(", 0b");
    Serial.println(readBuf[1], BIN);
  }*/
  
}

static SpiFlashOpResult spi_flash_erase_sector_nointerrupts(uint16 sec) {
  noInterrupts();
  SpiFlashOpResult result = spi_flash_erase_sector(sec);
  interrupts();
  return result;
}

static SpiFlashOpResult spi_flash_write_nointerrupts(uint32 des_addr, uint32 *src_addr, uint32 size) {
  noInterrupts();
  SpiFlashOpResult result = spi_flash_write(des_addr, src_addr, size);
  interrupts();
  return result;
}

static SpiFlashOpResult spi_flash_read_nointerrupts(uint32 src_addr, uint32 *des_addr, uint32 size) {
  noInterrupts();
  SpiFlashOpResult result = spi_flash_read(src_addr, des_addr, size);
  interrupts();
  return result;
}

