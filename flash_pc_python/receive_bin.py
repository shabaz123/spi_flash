##################################
# receive_bin.py
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

print(f'This program will read 1 Mbyte of Flash memory and save it to {fname}')
print('Press Enter to start, or Ctrl-C to exit')
input()
print('Starting...')
ser.write(b'5')
time.sleep(0.1)
ser.write(b'y')
ser.flush()
while True: # ignore everything up to [SENDING]\r\n
    line = ser.readline()
    if line == b'[SENDING]\r\n':
        break
num_bytes = 0
# read 1 Mbyte in 1024 byte chunks
with open(fname, 'wb') as f:
    for i in range(8192):
        data = ser.read(128)
        num_bytes += len(data)
        f.write(data)
        # progress dot:
        if i % 100 == 0:
            print('.', end='', flush=True)
print()
if (num_bytes != 1024*1024):
    print(f'ERROR: expected {1024*1024} bytes, received {num_bytes}')
    print(f'Discrepancy of {1024*1024 - num_bytes} bytes')
else:
    print(f'Success, saved to {fname}')
ser.close()


