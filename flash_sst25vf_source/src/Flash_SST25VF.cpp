// Flash_SST25VF.cpp
// rev 1 - June 2024 - shabaz - first revision
//

#include "Flash_SST25VF.h"

// defines
#define FLASH_CS_LOW digitalWrite(mPin_CS, LOW)
#define FLASH_CS_HIGH digitalWrite(mPin_CS, HIGH)
#ifndef DELAY_MS
#define DELAY_MS delay
#endif
#define SERIAL_MAX_WAIT 1000
#define CRC_POLY 0xEDB88320
#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define VSPI FSPI
#define VSPI_MISO   MISO
#define VSPI_MOSI   MOSI
#define VSPI_SCLK   SCK
#define VSPI_SS     SS
#else
#define NOT_ESP32
#endif


Flash_SST25VF::Flash_SST25VF()
{
    mPin_CS = FLASH_CS_DEFAULT;
    pinMode(mPin_CS, OUTPUT);
    FLASH_CS_HIGH;
#ifdef NOT_ESP32
    mSPIptr = &SPI;
#else
    mSPIptr = new SPIClass(VSPI);
    mSPIptr->begin();
#endif
}

Flash_SST25VF::Flash_SST25VF(uint8_t pin_CS)
{
    mPin_CS = pin_CS;
    pinMode(mPin_CS, OUTPUT);
    FLASH_CS_HIGH;
#ifdef NOT_ESP32
    mSPIptr = &SPI;
#else
    mSPIptr = new SPIClass(VSPI);
    mSPIptr->begin();
#endif
}

Flash_SST25VF::~Flash_SST25VF()
{
#ifdef NOT_ESP32
#else
    delete mSPIptr;
#endif
}

uint8_t Flash_SST25VF::flash_read_id()
{
    uint8_t mnfr_id, dev_id;
    override_default_cs();
    spi_begin_transaction();
    FLASH_CS_LOW; // select the Flash chip
    mSPIptr->transfer(FLASH_READ_ID);
    mSPIptr->transfer(0);
    mSPIptr->transfer(0);
    mSPIptr->transfer(0);
    mnfr_id = mSPIptr->transfer(0);
    dev_id = mSPIptr->transfer(0);
    FLASH_CS_HIGH; // deselect the Flash chip
    mSPIptr->endTransaction();
    release_default_cs();
    return dev_id;
}

uint8_t Flash_SST25VF::flash_read_status()
{
    uint8_t status;
    //override_default_cs();
    //spi_begin_transaction();
    FLASH_CS_LOW; // select the Flash chip
    mSPIptr->transfer(FLASH_READ_STATUS);
    status = mSPIptr->transfer(0);
    FLASH_CS_HIGH; // deselect the Flash chip
    //mSPIptr->endTransaction();
    //release_default_cs();
    return status;
}

void Flash_SST25VF::flash_write_enable()
{
    FLASH_CS_LOW; // select the Flash chip
    mSPIptr->transfer(FLASH_WRITE_EN);
    FLASH_CS_HIGH; // deselect the Flash chip
}

void Flash_SST25VF::flash_write_disable()
{
    FLASH_CS_LOW; // select the Flash chip
    mSPIptr->transfer(FLASH_WRITE_DIS);
    FLASH_CS_HIGH; // deselect the Flash chip
}

void Flash_SST25VF::flash_4k_erase(uint32_t addr)
{
    flash_write_status(0x00); // clear the status register (disable write protect)
    override_default_cs();
    spi_begin_transaction();
    flash_write_enable();
    FLASH_CS_LOW; // select the Flash chip
    mSPIptr->transfer(FLASH_ERASE_4K);
    mSPIptr->transfer((addr >> 16) & 0xFF);
    mSPIptr->transfer((addr >> 8) & 0xFF);
    mSPIptr->transfer(addr & 0xFF);
    FLASH_CS_HIGH; // deselect the Flash chip
    DELAY_MS(25); // wait for the erase to complete
    while (flash_read_status() & 0x01) // confirm the erase is complete
    {
        DELAY_MS(1);
    }
    flash_write_disable();
    mSPIptr->endTransaction();
    release_default_cs();
    flash_write_status(0x1c); // enable write protect
}

void Flash_SST25VF::flash_full_erase()
{
    flash_write_status(0x00); // clear the status register (disable write protect)
    override_default_cs();
    spi_begin_transaction();
    flash_write_enable();
    FLASH_CS_LOW; // select the Flash chip
    mSPIptr->transfer(FLASH_ERASE_FULL);
    FLASH_CS_HIGH; // deselect the Flash chip
    DELAY_MS(50); // wait for the erase to complete
    while (flash_read_status() & 0x01) // confirm the erase is complete
    {
        DELAY_MS(1);
    }
    flash_write_disable();
    mSPIptr->endTransaction();
    release_default_cs();
    flash_write_status(0x1c); // enable write protect
}

void Flash_SST25VF::prepare_write(uint32_t addr)
{
    mWrite_addr = addr;
    flash_write_status(0x00); // clear the status register (disable write protect)
    override_default_cs();
    spi_begin_transaction();
    flash_write_enable();
}

void Flash_SST25VF::byte_write(char data)
{
    while (flash_read_status() & 0x01) // check that the device is not busy
    {
        DELAY_MS(1);
    }
    flash_write_enable(); // needed per byte it seems..
    FLASH_CS_LOW; // select the Flash chip
    mSPIptr->transfer(FLASH_WRITE_BYTE);
    mSPIptr->transfer((mWrite_addr >> 16) & 0xFF);
    mSPIptr->transfer((mWrite_addr >> 8) & 0xFF);
    mSPIptr->transfer(mWrite_addr & 0xFF);
    mSPIptr->transfer(data);
    FLASH_CS_HIGH; // deselect the Flash chip
    mWrite_addr++;    
}

void Flash_SST25VF::write_complete()
{
    while (flash_read_status() & 0x01) // check that the device is not busy
    {
        DELAY_MS(1);
    }
    flash_write_disable();
    mSPIptr->endTransaction();
    release_default_cs();
    flash_write_status(0x1c); // enable write protect
}

void Flash_SST25VF::flash_write_status(uint8_t status)
{
    override_default_cs();
    spi_begin_transaction();
    flash_write_enable();
    FLASH_CS_LOW; // select the Flash chip
    mSPIptr->transfer(FLASH_WRITE_STATUS);
    mSPIptr->transfer(status);
    FLASH_CS_HIGH; // deselect the Flash chip
    flash_write_disable();
    mSPIptr->endTransaction();
    release_default_cs();
}

void Flash_SST25VF::flash_read(uint32_t addr, char *data, uint16_t len)
{
  uint16_t i;
  override_default_cs();
  spi_begin_transaction();
  FLASH_CS_LOW; // select the Flash chip
  mSPIptr->transfer(FLASH_READ);
  mSPIptr->transfer((addr >> 16) & 0xFF);
  mSPIptr->transfer((addr >> 8) & 0xFF);
  mSPIptr->transfer(addr & 0xFF);
  for (i = 0; i < len; i++)
  {
      data[i] = mSPIptr->transfer(0);
  }
  FLASH_CS_HIGH; // deselect the Flash chip
  mSPIptr->endTransaction();
  release_default_cs();
}

void Flash_SST25VF::spi_begin_transaction()
{
    SPISettings spi_setting(1000000, MSBFIRST, SPI_MODE0);
    mSPIptr->beginTransaction(spi_setting);
}

void Flash_SST25VF::crc32b_init() {
    mCRC = 0xFFFFFFFF;
}

void Flash_SST25VF::crc32b_update(char single_byte) {
   int j;
   uint32_t mask;

    mCRC = mCRC ^ single_byte;
    for (j = 7; j >= 0; j--) {    // Do eight times.
        mask = -(mCRC & 1);
        mCRC = (mCRC >> 1) ^ (CRC_POLY & mask);
    }
}

uint32_t Flash_SST25VF::crc32b_get() {
    mCRC = ~mCRC;
    return mCRC;
}


uint32_t Flash_SST25VF::serial_bin_to_flash(uint32_t addr, unsigned long initial_timeout_ms)
{
    char data;
    uint32_t bytes_written = 0;
    unsigned long start_time;
    uint32_t crc_result;
    crc32b_init();
    prepare_write(addr);
    // wait up to SERIAL_MAX_WAIT_INITIAL milliseconds for the first byte
    start_time = millis();
    while (!Serial.available()) {
        if (millis() - start_time > initial_timeout_ms) {
            write_complete();
            Serial.println("Timeout, no data received");
            return 0;
        }
    }
    data = Serial.read();
    crc32b_update(data);
    byte_write(data);

    // loop to read data from Serial and write to Flash.
    // If no data is received for SERIAL_MAX_WAIT milliseconds, then
    // the write is considered complete.
    start_time = millis();
    while (1) {
        if (Serial.available()) {
            data = Serial.read();
            crc32b_update(data);
            byte_write(data);
            bytes_written++;
            start_time = millis();
            // every 1024 bytes, print a message
            if ((bytes_written & 0x3FF) == 0) {
                Serial.print("Bytes written: ");
                // print in kbytes
                Serial.print(bytes_written >> 10);
                Serial.println("k");
            }
        } else {
            if (millis() - start_time > SERIAL_MAX_WAIT) {
                break;
            } else {
                DELAY_MS(1);
            }
        }
    }
    write_complete();
    Serial.print("Total written: ");
    Serial.print(bytes_written);
    Serial.println(" bytes");
    Serial.print("crc32b: ");
    crc_result = crc32b_get();
    //print in decimal and hex
    Serial.print(crc_result, DEC);
    Serial.print(" ");
    Serial.println(crc_result, HEX);
    return bytes_written;
}
