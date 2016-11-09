#!/usr/bin/bash

# SoilBg.sh

# sej 2016 11 09 script to launch SoilMonitor.py in the background

nohup @reboot /home/pi/autowater/./SoilMonitor.py > /home/pi/autowater/error.log 2>&1 &


