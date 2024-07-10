##################################
# send_bin.py
# rev 1 - shabaz - july 2024
##################################

import serial
import time
import sys
portname = 'COM18'
fname = 'data.bin'

ser = serial.Serial()
ser.port = portname
ser.baudrate = 115200
ser.timeout = 1
# catch serial port open errors
try:
    ser.open()
except:
    print(f'Error: could not open serial port {portname}')
    print("Check that you don't have a serial console session open!")
    print("Check the port number!")
    sys.exit()

# check if a filename has been provided
if len(sys.argv) > 1:
    fname = sys.argv[1]


# check if the file exists
try:
    with open(fname, 'rb') as f:
        pass
except:
    print(f'Error: file {fname} does not exist')
    sys.exit()

print(f'This program will transfer file {fname} to 1 MByte of Flash memory')
print('WARNING - all existing contents of Flash memory will be erased!')
print('Press Enter to start, or Ctrl-C to exit')
input()
print('Erasing Flash memory...')
ser.write(b'6')
ser.write(b'y')
ser.flush()
# wait for the text 'Flash fully erased\r\n'
while True:
    line = ser.readline()
    if line == b'Flash fully erased\r\n':
        break
# read from serial until the text 'Enter choice:' is received
while True:
    line = ser.readline()
    if line == b'Enter choice: ':
        break
time.sleep(0.5)
print('Preparing to write to Flash...')
ser.write(b'7')
ser.write(b'y')
ser.flush()
# wait for the text *** Ready for binary data ***\r\n
while True:
    line = ser.readline()
    if line == b'*** Ready for binary data ***\r\n':
        break
bytes_sent = 0
print('Sending, this will take several minutes...')
# now send the file
with open(fname, 'rb') as f:
    while True:
        data = f.read(1)
        if len(data) == 0:
            if (bytes_sent < 1024*1024):
                print('Sending padding...')
            # no more data in the source file.
            # we should send a total of 1 Mbyte
            # so send 0xff in chunks of 16 bytes max
            while bytes_sent < 1024*1024:
                ser.write(b'\xff')
                # wait until a '.' is received
                while True:
                    c = ser.read(1)
                    if c == b'.':
                        break
                bytes_sent += 1
                # every 10240 bytes, print a dot
                if bytes_sent % 10240 == 0:
                    print('.', end='', flush=True)
            break
        ser.write(data)
        # every byte, wait until a '.' is received
        while True:
            c = ser.read(1)
            if c == b'.':
                break
        bytes_sent += len(data)
        # every 10240 bytes, print a dot
        if bytes_sent % 10240 == 0:
            print('.', end='', flush=True)
print()
print('Done')
ser.close()

