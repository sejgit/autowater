#!/usr/bin/python2.7

# SoilMonitor.py

# sej 2016 11 07

import serial
ser = serial.Serial('/dev/ttyACM0', 9600)

while True :
    ser.readline()

