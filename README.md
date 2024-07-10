spi_flash
=========
SPI Flash library for SST25VF. Only tested with SST25VF080B 3.3V 1 Mbyte SPI Flash chip.

Tested on ESP32 board (Adafruit Qualia) and an Arduino Uno R4 board that was modified for 3.3V logic level operation.

The connections were:

|Flash Chip|ESP32 Board|Uno R4 Minima|Description|
|:----|:----|:----|:----|
|1 (*CE)|A0 (IO17)|D6|Chip Select, can be any pin that works with DigitalOut|
|2 (SCK)|SCK (IO5)|D13|SPI Clock|
|3 (SI)|MOSI (IO7)|D11|SPI MOSI (COPI)|
|2 (SO)|MISO (IO6)|D12|SPI MISO (CIPO)|
|3, 7, 8 |3.3V Power|3.3V Power|WP, HOLD, VDD Pins|
|4 |GND |GND | 0V (GND) pin|

How to Use
----------
Download the **flash_sst25vf_library-1.0.0.zip** file to your PC.

To install it, within the Arduino IDE, click on **Sketch->Include Library->Add .ZIP File**.

That’s it!

There are a couple of example programs.

Simple Demo
-----------
In the Arduino IDE, go to **File->Examples->Flash_SST25VF_Library->Flash_SST25VF_SimpleDemo** and run it. The Arduino Serial Monitor can be used to run a simple read and write demonstration, it is self-explanatory from the menu options.

Monitor/Programmer
------------------
In the Arduino IDE, go to **Examples->Flash_SST25VF_Library->Flash_SST25VF_Monitor_Programmer** and run it. You can access it in two ways; the first is using the Arduino Serial Monitor as above, and it will provide a menu to be able to read any area of memory and dump it to the serial monitor display. The second way is to control it via Python programs, which are discussed next.

**Writing a Binary File to Flash**

On your PC, install pyserial (**pip install pyserial**) and then type:
```
python ./send_bin.py mybinaryfile.bin
```
It will take about 5 minutes to program the Flash memory

**Reading the Flash into a Binary File**

On your PC, install pyserial (**pip install pyserial**) and then type:
```
python ./receive_bin.py
```
It will take about a minute to read the Flash memory and save it to a file called data.bin on your PC.

Memory Map
----------
There is a memory map (**mem_map_1Mbyte.svg**) that may be useful for organizing content. To use it, download it to your PC, then open it in a web browser (e.g. Chrome). Then, print it in landscape format.


