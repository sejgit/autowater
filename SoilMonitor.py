#!/usr/bin/python2.7

# SoilMonitor.py

# sej 2016 11 07

import serial
ser = serial.Serial('/dev/ttyACM0', 115200)


while True :
    lineIn = ser.readline()
    print(lineIn)
    try:
        with open('SoilData.log', 'a') as f:
            f.write(lineIn)
    except IOError:
        print('IOError')

