// Flash_SST25VF.h
// rev 1 - June 2024 - shabaz - first revision

#ifndef FLASH_SST25VF_H
#define FLASH_SST25VF_H

#include "Arduino.h"
#include "SPI.h"

// defines
#define FLASH_CS_DEFAULT 6
#define FLASH_READ 0x03
#define FLASH_ERASE_4K 0x20
#define FLASH_ERASE_FULL 0xC7
#define FLASH_WRITE_BYTE 0x02
#define FLASH_WRITE_EN 0x06
#define FLASH_WRITE_DIS 0x04
#define FLASH_READ_STATUS 0x05
#define FLASH_READ_ID 0x90
#define FLASH_WRITE_STATUS 0x01



#define DATA_BUF_SIZE 256


// class definition
class Flash_SST25VF
{
  public:
    Flash_SST25VF(); // default constructor assumes CS pin is FLASH_CS_DEFAULT
    ~Flash_SST25VF(); // destructor
    Flash_SST25VF(uint8_t pin_CS);
    // Used for checking that a Flash chip is present:
    uint8_t flash_read_id(); // should return 0x8e if SST25VF080B is present
    // Read data from Flash
    void flash_read(uint32_t addr, char *data, uint16_t len); 
    // Flash erase functions
    void flash_4k_erase(uint32_t addr); // erase 4k sector, takes about 25 msec
    void flash_full_erase(); // erase entire Flash, takes about 50 msec
    // three functions are called in sequence to write to Flash
    void prepare_write(uint32_t addr); // prepare to write to Flash starting at addr
    void byte_write(char data); // write a byte to Flash and increment the addr
    void write_complete(); // call after you are done writing byte(s) to Flash
    // accept data from Serial and write to Flash
    uint32_t serial_bin_to_flash(uint32_t addr, unsigned long initial_timeout_ms); 

    // CRC functions
    void crc32b_init(); // initialize CRC
    void crc32b_update(char single_byte); // update CRC with a byte
    uint32_t crc32b_get(); // get the CRC value

    // Since there's a chance SPI is being used for more than one
    // device, the following functions are provided to allow the user
    // to set any other CS pin to an input pulled high, or back to an output.
    // Used to set any default CS pin to an input pulled high if required:
    virtual void override_default_cs() = 0;
    // Used to set any default CS pin back to an output if required:
    virtual void release_default_cs() = 0;
    
  private:
    void spi_begin_transaction(); // configures the SPI speed and mode
    uint8_t flash_read_status(); // returns the Flash Status Register contents
    void flash_write_enable();  // enables writing to the Flash
    void flash_write_disable(); // disables writing to the Flash
    void flash_write_status(uint8_t status); // write to the Flash Status Register
    uint8_t mPin_CS;  // stores the Flash chip select (CS) pin
    uint32_t mWrite_addr; // stores the address to write to
    uint32_t mCRC; // stores CRC (non-inverted)
    SPIClass* mSPIptr; // stores the SPI object
};

#endif // FLASH_SST25VF_H
