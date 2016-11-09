#!/usr/bin/python2.7

# SoilMonitor.py

# sej 2016 11 07
# sej 2016 11 09 add datetime

import serial
import datetime
from os.path import expanduser

ser = serial.Serial('/dev/ttyACM0', 115200)

while True :
    lineIn = ser.readline()
    d = '{:%m-%d %H:%M:%S}  '.format(datetime.datetime.now())
    print(d + lineIn).rstrip('\n')
    try:
        path = expanduser("~/autowater/SoilData.log")
        with open(path , 'a') as f:
            f.write(d + lineIn)
    except IOError:
        print('IOError')

