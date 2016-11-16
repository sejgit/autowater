#!/bin/bash

# clean log file to reasonable length

# 2016 11 16 sej


mv --force $HOME/autowater/SoilData.log $HOME/autowater/SoilData.old
tail -n 100 $HOME/autowater/SoilData.old > $HOME/autowater/SoilData.log


