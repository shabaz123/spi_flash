/***************************************************************************** 
 * Flash_SST25VF_Monitor_Programmer.ino
 * rev 1 - July 2024 - shabaz
 * Only tested with SST25VF080B Flash chip
 * This code provides simple Serial menu options to read/write/erase.
 * Optionally use with PC software (receive_bin.py and send_bin.py)
 * to read/write entire Flash memory using binary files.
 * Note: This code does not use the default SPI chip-select (CS or SS) pin;
 * it uses a separate user-definable pin, see the FLASH_CS definition below.
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
uint8_t dry_run = 0; // set to 1 to not actually write to Flash

// ********* function prototypes ***********
uint8_t menu(uint32_t* addr, uint32_t* len);  // display a menu and get user choice
void get_serial_string(char* sdata, int len); // allow user to enter a string from Serial

// ********* setup ***********
void setup()
{
    uint8_t flash_id;
    Serial.begin(115200);
    DELAY_MS(3000); // delay to allow the user to open the Serial Monitor

    Serial.println("*** Flash Monitor / Programmer ***");
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
        case 1:
            Serial.print("Flash ID: ");
            Serial.println(flash.flash_read_id(), HEX);
            break;
        case 2:
            // read 16 bytes max at a time and display them
            flash.crc32b_init();
            while(len > 0) {
                uint16_t read_len = len > 16 ? 16 : len;
                flash.flash_read(addr, data, read_len);
                Serial.print("0x");                
                sprintf(sdata, "%06X", addr);
                Serial.print(sdata);
                Serial.print(" : ");
                for (uint16_t i = 0; i < read_len; i++) {
                    sprintf(sdata, "%02X", data[i]);
                    Serial.print(sdata + strlen(sdata) - 2); // print last 2 characters
                    flash.crc32b_update(data[i]);
                    Serial.print(" ");
                }
                Serial.print(" : ");
                for (uint16_t i = 0; i < read_len; i++) {
                    if (data[i] >= 32 && data[i] <= 126) {
                        Serial.print(data[i]);
                    } else {
                        Serial.print(".");
                    }
                }
                Serial.println();
                addr += read_len;
                len -= read_len;
            }
            crc_val = flash.crc32b_get();
            Serial.print("Read complete, checksum is 0x");
            Serial.println(crc_val, HEX);
            break;
        case 3:
            if (dry_run == 0) {
                flash.flash_4k_erase(addr);
            }
            Serial.println("4k sector erased");
            break;
        case 4:
            flash.prepare_write(addr);
            num_done = 0;
            flash.crc32b_init();
            Serial.println("*** Ready for binary data ***");
            // read one byte from Serial at a time and write to Flash
            while(len > 0) {
                while (!Serial.available());
                byteval = Serial.read();
                if (dry_run == 0) {
                    flash.byte_write(byteval);
                }
                len--;
                flash.crc32b_update(byteval);
                num_done++;
            }
            flash.write_complete();
            crc_val = flash.crc32b_get();
            Serial.print("Write complete, checksum is 0x");
            Serial.println(crc_val, HEX);
            break;
        case 5:
            // read entire flash, 16 bytes at a time, and output to serial
            addr = 0;
            len = 0x100000; // 1MByte
            num_done = 0;
            flash.crc32b_init();
            Serial.println("[SENDING]");
            DELAY_MS(1000);
            while(len > 0) {
                uint16_t read_len = 16;
                flash.flash_read(addr, data, read_len);
                // write bytes to serial
                Serial.write(data, read_len);
                for (i=0; i<read_len; i++) {
                    flash.crc32b_update(data[i]);
                }
                addr += read_len;
                len -= read_len;
                num_done += read_len;
            }
            crc_val = flash.crc32b_get();
            Serial.print("Read complete, checksum is 0x");
            Serial.println(crc_val, HEX);
            break;
        case 6:
            if (dry_run == 0) {
                flash.flash_full_erase();
            }
            Serial.println("Flash fully erased");
            break;
        case 7:
            // write entire flash, 16 bytes at a time, from serial
            addr = 0;
            len = 0x100000;
            num_done = 0;
            flash.crc32b_init();
            flash.prepare_write(addr);
            Serial.println("*** Ready for binary data ***");
            // read one byte from Serial at a time and write to Flash
            // wait up to 10 seconds for the first byte
            current_millis = millis();
            while (!Serial.available()) {
                if (millis() - current_millis > 10000) {
                    Serial.println("Error, timeout no data received");
                    flash.write_complete();
                    return;
                }
                DELAY_MS(1);
            }
            // write first byte
            byteval = Serial.read();
            if (dry_run == 0) {
                flash.byte_write(byteval);
            }
            Serial.print("."); // every byte, send '.'
            flash.crc32b_update(byteval);
            num_done++;
            len--;
            // get the rest of the data and write it
            while(len > 0) {
                current_millis = millis();
                while (!Serial.available()) {
                    if (millis() - current_millis > 1000) {
                        Serial.print("Error, timeout after ");
                        Serial.print(num_done);
                        Serial.println(" bytes written");
                        flash.write_complete();
                        return;
                    }
                }
                byteval = Serial.read();
                if (dry_run == 0) {
                    flash.byte_write(byteval);
                }
                len--;
                Serial.print("."); // every byte, send '.'
                num_done++;
                flash.crc32b_update(byteval);
                
            }
            flash.write_complete();
            crc_val = flash.crc32b_get();
            Serial.print("Write complete, checksum is 0x");
            Serial.println(crc_val, HEX);
            break;
        case 8:
            if (dry_run == 1) {
                dry_run = 0;
                Serial.println("Flash writing is now enabled");
            } else {
                dry_run = 1;
                Serial.println("Flash writing is now disabled");
            }
            break;
        case 9:
            Serial.println("Exiting");
            return;
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

void get_serial_string(char* sdata, int len) {
    int i = 0;
    char c;
    while (1) {
        while (!Serial.available());
        c = Serial.read();
        if (c == '\n' || c == '\r') {
            sdata[i] = 0;
            Serial.println();
            return;
        }
        // check for backspace
        if (c == 0x08) {
            if (i > 0) {
                i--;
                Serial.print(c);
            }
            continue;
        }
        sdata[i] = c;
        Serial.print(c);
        i++;
        if (i >= len) {
            sdata[i] = 0;
            return;
        }
    }
}

uint8_t menu(uint32_t* addr, uint32_t* len) {
    uint8_t choice;
    char sdata[10];
    Serial.println();
    Serial.println("Menu:");
    Serial.println("1. Read Flash ID");
    Serial.println("2. Read Flash from address");
    Serial.println("3. Erase 4k sector");
    Serial.println("4. Write to Flash from address");
    Serial.println("5. Read entire Flash");
    Serial.println("6. Erase full Flash");
    Serial.println("7. Write entire Flash");
    if (dry_run == 1) {
        Serial.println("8. Exit dry-run mode");
    } else {
        Serial.println("8. Enter dry-run mode");
    }
    Serial.println("9. Exit");
    Serial.print("Enter choice: ");
    while (!Serial.available());
    choice = Serial.read() - '0';
    Serial.println(choice);
    
    if (choice < 1 || choice > 9) {
        Serial.println("Invalid choice");
        return 0;
    }

    Serial.setTimeout(3600000); // otherwise readString() will timeout!
    if (choice == 2 || choice == 3 || choice == 4) { // we need to get the address
        Serial.print("Enter address (hex): 0x");
        get_serial_string(sdata, 10);
        *addr = strtoul(sdata, NULL, 16); // convert the hex (base 16) string to a number
    }

    if (choice == 2 || choice == 4) { // we need to get number of bytes to read
        Serial.print("Enter number of bytes (decimal): ");
        get_serial_string(sdata, 10);
        *len = strtoul(sdata, NULL, 10);
    }
    Serial.setTimeout(1000);

    // get confirmation
    Serial.print("Choice:");
    Serial.print(choice);
    if (choice == 2 || choice == 3 || choice == 4) {
        Serial.print(" Address:0x");
        Serial.print(*addr, HEX);
    }
    if (choice == 2 || choice == 4) {
        Serial.print(" Length:");
        Serial.print(*len);
    }
    Serial.print(" OK? (y/n):");
    while (!Serial.available());
    sdata[0] = Serial.read();
    Serial.println(sdata[0]);
    if (sdata[0] != 'y') {
        Serial.println("Aborted");
        return 0;
    }
    return choice;
}
