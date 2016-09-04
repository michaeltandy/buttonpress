extern "C" {
#include "spi_flash.h" // Provides SPI_FLASH_SEC_SIZE (usually 4096)
}

extern "C" uint32_t _SPIFFS_end; // Usually points to 0x405FB000
const uint32_t MAGIC_HEADER = 71449302;
const uint32_t START_ADDRESS_BYTES = (uint32_t)&_SPIFFS_end - 0x40200000;
const uint32_t START_ADDRESS_FLASH_SECTORS = START_ADDRESS_BYTES/SPI_FLASH_SEC_SIZE;

const int led = 2;
const int LED_ON = LOW;
const int LED_OFF = HIGH;

void setup() {
  pinMode(led, OUTPUT);
  digitalWrite(led, LED_OFF);
  Serial.begin(115200);
  Serial.println("Started.");
}

void loop() {
  delay(500);
  Serial.println("Testing SPI flash...");

  noInterrupts();
  digitalWrite(led, LED_ON);
  uint32_t clkbefore = SPI0CLK;
  
  SPI0CLK = 0x00D43002;
  
  spi_flash_erase_sector(START_ADDRESS_FLASH_SECTORS);
  uint32_t readBuf[1] = {0};
  spi_flash_read(START_ADDRESS_BYTES, readBuf, sizeof(readBuf));
  uint32_t writeBuf[1] = {0xF0F0F0F0};
  spi_flash_write(START_ADDRESS_BYTES, writeBuf, sizeof(writeBuf));

  SPI0CLK = clkbefore;
  digitalWrite(led, LED_OFF);
  interrupts();
}



