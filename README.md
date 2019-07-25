# DataLogger for Feather M0 with SD card

## Required Hardware
- Feather M0
- RTC DS3231
- SHT31-D
- DGS

## Overview
Reads data from sensors and stores data in a csv file specified by the current date and the string FILE_SUFFIX appended. For easy reading of file use .csv extention

## Companion MATLAB script
The ReadSDCard_FeatherM0_2.m MATLAB script will pull all the files from the SD and save them to the computer.

## Functionality of Indicator LEDs
### DATA LED (Yellow)
- Blinks 1 when data collection entry begins
- Blinks 3 times when the sensor reading is complete and the entry is written
to the SD card
- Remains on while files are being transfered from the SD card to the computer

### BATTERY LED (Red)
- Blinks when battery is low. Low battery voltage set by macro BAT_LOW
- Solid to indicate battery is full/done charging. set by macro BAT_FULL

## Serial Monitor (UART) Commands
### P **samplePeriodValue**
Ex. To set the period to 60 seconds send "P60" no quotes
### B
Ex. Send "B" no quotes and the battery voltage will be returned
### D
Ex. Send "D" no quotes and all the files from the SD card will be sent over serial. This can be used directly in the Serial Monitor, but the MATLAB script will handle this better and actually save the data to files on your computer.