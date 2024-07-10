/***************************************************************************** 
 * Flash_SST25VF_SimpleDemo.ino
 * rev 1 - July 2024 - shabaz
 * Only tested with SST25VF080B Flash chip
 * ***************************************************************************/

#include <Flash_SST25VF.h>

// ********* defines ***********
// ESP32 example: Use A0 as the CS pin for the Flash chip
// Arduino Uno R4 (modified for 3.3V logic) example: Use 6 as the CS pin
#ifdef ARDUINO_ARCH_ESP32
#define FLASH_CS A0
#else
#define FLASH_CS 6
#endif
// comment the line below if the default SPI CS pin is unused by any device
//#define DEFAULT_CS SS // default SPI CS pin
// misc
#define DELAY_MS delay

// ***** class based on Flash_SST25VF ********
class Flash : public Flash_SST25VF
{
  public:
    Flash(uint8_t pin_CS) : Flash_SST25VF(pin_CS) {};
    // override/release functions to play nice with
    // other devices on the SPI bus that may be using
    // the default CS pin
    void override_default_cs();  // override the default CS pin if required
    void release_default_cs();  // release the default CS pin if required
};

// ********* global variables ***********
Flash flash(FLASH_CS); // create an instance of the Flash class

// ********* function prototypes ***********
uint8_t menu(uint32_t* addr, uint32_t* len);  // display a menu and get user choice
void print_addr_and_bytes(uint32_t addr, char* data, uint16_t len); // print address and bytes

// ********* setup ***********
void setup()
{
    uint8_t flash_id;
    Serial.begin(115200);
    DELAY_MS(3000); // delay to allow the user to open the Serial Monitor

    Serial.println("*** Flash Simple Demo ***");
    //print the SPI pins
    Serial.println(" ");
    Serial.println("SPI Pin Configuration:");
    Serial.print("MISO: ");
    Serial.println(MISO);
    Serial.print("MOSI: ");
    Serial.println(MOSI);
    Serial.print("SCK: ");
    Serial.println(SCK);
    Serial.print("SS: ");
    Serial.println(FLASH_CS);

    // check the Flash chip is present
    Serial.println("Checking for Flash...");
    flash_id = flash.flash_read_id();
    if (flash_id == 0x8e) {
        Serial.println("Flash chip is present");
    } else {
        Serial.print("Flash chip is not present val = ");
        Serial.println(flash_id);
    }
    Serial.println(" ");
}

// ********* loop function ***********
void loop()
{
    uint8_t choice;
    uint32_t addr = 0x00000;
    uint32_t len = 0;
    uint32_t num_done;
    char byteval;
    char data[16];
    char sdata[10]; // temp string buffer
    uint32_t crc_val;
    int i;
    unsigned long current_millis;

    choice = menu(&addr, &len);
    switch(choice) {
        case 0:
            // invalid choice
            return;
        case 1: // Read Flash ID, should return 0x8e if SST25VF080B is present
            Serial.print("Flash ID: ");
            Serial.println(flash.flash_read_id(), HEX);
            break;
        case 2: // Read Demo
            Serial.println("Reading 16 bytes starting from address 0x0000 0000");
            addr = 0x00000;
            flash.flash_read(addr, data, 16);
            print_addr_and_bytes(addr, data, 16);

            Serial.println("Reading 16 bytes starting from address 0x000f fff0");
            addr = 0xffff0;
            flash.flash_read(addr, data, 16);
            print_addr_and_bytes(addr, data, 16);
            break;
        case 3: // Write Demo
            Serial.println("Erasing 4kbyte (0x1000) sector at address 0x0000 0000");
            addr = 0x00000;
            flash.flash_4k_erase(addr);
            Serial.println("Writing 16 bytes ['A' to 'P'] starting at address 0x0000 0000");
            flash.prepare_write(addr);
            for (i=0; i<16; i++) {
                byteval = i + 'A';
                flash.byte_write(byteval);
            }
            flash.write_complete();

            Serial.println("Erasing 4kbyte (0x1000) sector at address 0x000F F000");
            addr = 0xff000;
            flash.flash_4k_erase(addr);
            Serial.println("Writing 16 bytes ['K' to 'Z'] starting at address 0x000F FFF0");
            addr = 0xffff0;
            flash.prepare_write(addr);
            for (i=0; i<16; i++) {
                byteval = i + 'K';
                flash.byte_write(byteval);
            }
            flash.write_complete();
            break;
        case 4: // Erase entire Flash
            Serial.println("Erasing entire Flash");
            flash.flash_full_erase();
            break;
        default:
            Serial.print("Error, unexpected invalid choice ");
            Serial.println(choice);
            break;
    }
    DELAY_MS(1000);
}


// ********* other functions ***********
void Flash::override_default_cs()
{
    // comment the line below if the default SPI CS pin is unused by any device
    //pinMode(DEFAULT_CS, INPUT_PULLUP);
}

void Flash::release_default_cs()
{
    // comment the line below if the default SPI CS pin is unused by any device
    //pinMode(DEFAULT_CS, OUTPUT);
}

void print_addr_and_bytes(uint32_t addr, char* data, uint16_t len) {
    char sdata[10];
    Serial.print("0x");                
    sprintf(sdata, "%06X", addr);
    Serial.print(sdata);
    Serial.print(" : ");
    for (uint16_t i = 0; i < len; i++) {
        sprintf(sdata, "%02X", data[i]);
        Serial.print(sdata + strlen(sdata) - 2); // print last two characters
        Serial.print(" ");
    }
    Serial.print(" : ");
    for (uint16_t i = 0; i < len; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            Serial.print(data[i]);
        } else {
            Serial.print(".");
        }
    }
    Serial.println();
}

uint8_t menu(uint32_t* addr, uint32_t* len) {
    uint8_t choice;
    char sdata[10];
    Serial.println();
    Serial.println("Menu:");
    Serial.println("1. Read Flash ID");
    Serial.println("2. Read Demo");
    Serial.println("3. Write Demo");
    Serial.println("4. Erase Entire Flash");
    Serial.print("Enter choice: ");
    while (!Serial.available());
    choice = Serial.read() - '0';
    Serial.println(choice);
    
    if (choice < 1 || choice > 4) {
        Serial.println("Invalid choice");
        return 0;
    }

    // get confirmation
    switch(choice) {
        case 1:
            Serial.print("Choice: Read Flash ID. OK? (y/n):");
            break;
        case 2:
            Serial.print("Choice: Read demonstration. OK? (y/n):");
            break;
        case 3:
            Serial.print("Choice: WRITE demonstration (will overwrite content!). OK? (y/n):");
            break;
        case 4:
            Serial.print("Choice: Erase entire Flash (will erase all content!). OK? (y/n):");
            break;
    }
    while (!Serial.available());
    sdata[0] = Serial.read();
    Serial.println(sdata[0]);
    if (sdata[0] != 'y') {
        Serial.println("Aborted");
        return 0;
    }
    return choice;
}
