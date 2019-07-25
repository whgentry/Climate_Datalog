/*
  DGS and SHT31-D SD card DataLogger for Feather M0 with SD card

  Required Hardware
  - RTC DS3231
  - SHT31-D
  - DGS
  
  Reads data from sensors and stores data in a csv file specified 
    by the current date and the string FILE_SUFFIX appended. For easy
    reading of file use .csv extention
  The ReadSDCard_FeatherM0_2.m MATLAB script will pull all the files 
    from the SD and save them to the computer. (The old script only 
    pulled the latest file)

  Functionality of Indicator LEDs
  DATA LED (Yellow)
  -Blinks Once when data collection entry begins
  -Blinks 3 times when the sensor reading is complete and the entry is written
   to the SD card
  -Remains on while files are being transfered from the SD card to the computer

  BATTERY LED (Red)
  -Blinks when battery is low. Low battery voltage set by macro BAT_LOW
  -Solid to indicate battery is full/done charging. set by macro BAT_FULL

  Serial Monitor (UART) Commands
  - P<samplePeriodValue>
    Ex. To set the period to 60 seconds send "P60" no quotes
  - B
    Ex. Send "B" no quotes and the battery voltage will be returned
  - D
    Ex. Send "D" no quotes and all the files from the SD card will be
      sent over serial. This can be used directly in the Serial Monitor,
      but the MATLAB script will handle this better and actually save the
      data to files on your computer.
*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include "DGS.h"
#include "RTClib.h"

#define SECONDS_PER_DAY 86400

#define SETUP false
#define FILE_SUFFIX ".csv"
#define FILE_HEADER "Time, Temp C, Humidity %, Concentration\r\n" 
#define SAMPLE_PERIOD_FILE_NAME "PERIOD"

#define BAT_LOW (3.5)
#define BAT_FULL (4.2)

#define CMD_SET_PERIOD 'P'
#define CMD_GET_DATA 'D'
#define CMD_GET_BAT 'B'

#define T_START "TRANSFER START"
#define T_END "TRANSFER END"
#define F_START "FILE START"
#define F_END "FILE END"

DGS mySensor(&Serial1);
Adafruit_SHT31 sht31 = Adafruit_SHT31();
RTC_DS3231 rtc;

File samplePeriodFile;

const int chipSelect = 4;
const int dataLED = 12; // 13 for on board, 12 for external PCB
const int batLED = 11; // 8 for on board, 11 for external PCB
const int batVoltPin = A0; // Battery Voltage Detection won't work unless the voltage division circuit is set up, system still operates otherwise
double batVolt = 0;
String fileName = "";
int sec;
int samplePeriod = 60;
int batPeriod = 3;

void setup() //Setup intended for first time setup of SDK. Must initialize both serial ports:
{
  while(!Init_All()) {
    blinkLED(dataLED,1,500);
    blinkLED(batLED,1,500);
  }
}

void loop()
{
  String dataString = "";
  File dataFile;
  File root;

  // UART commands
  if (Serial.available()) {
    switch(Serial.read()) {
      case CMD_SET_PERIOD:
        char str[16];
        Serial.readBytesUntil('\n',str,15);
        int temp;
        temp = String(str).toInt();
        samplePeriod = temp > 0 ? temp : samplePeriod;
        if (SD.exists(SAMPLE_PERIOD_FILE_NAME))
          SD.remove(SAMPLE_PERIOD_FILE_NAME);
        samplePeriodFile = SD.open(SAMPLE_PERIOD_FILE_NAME, FILE_WRITE);
        samplePeriodFile.write((char*)&samplePeriod,sizeof(int));
        samplePeriodFile.close();
        Serial.print("Sample Period: ");
        Serial.println(samplePeriod);
      break;
      
      case CMD_GET_DATA:
        Serial.println(T_START);
        blinkLED(dataLED,0,1);
        root = SD.open("/");
        // Keep sending past dates if they exist
        while(dataFile = root.openNextFile()) {
          if (dataFile.isDirectory())
            continue;
          // Send the file name
          Serial.println(dataFile.name());
          int8_t c;
          // Send the file data
          c = dataFile.read();
          while (c != -1) {
            Serial.write((char*)&c,1);
            c = dataFile.read();
          }
          Serial.println(F_END);
          dataFile.close();
        }
        Serial.println(T_END);
        blinkLED(dataLED,2,200);
      break;

      case CMD_GET_BAT:
        Serial.print(batVolt);
        Serial.println(" V");
      break;
      default:
        Serial.println("Command Not Recognized");
      break;
    }
    while(Serial.available()) Serial.read();
  }

  // This code will run every second
  if ( sec != now()) {
    sec = now();

    // Run with a period of samplePeriod
    if (sec%samplePeriod == 0) {
      blinkLED(dataLED,1,200);
      
      // Read DGS
      mySensor.getData('\r');
      int conc = mySensor.getConc();     //default is 'p' for temperature compensated ppb, any other cahracter for raw counts
    
      // Fill data String
      dataString += timeString() + ", ";
      dataString += String(sht31.readTemperature()) + ", ";
      dataString += String(sht31.readHumidity()) + ", ";
      dataString += String(conc);
    
      // Write out entry to file
      fileName = dateString(sec) + String(FILE_SUFFIX);
      // If new date, add headers before dataString
      if (!SD.exists(fileName)) {
        String headers = FILE_HEADER;
        dataString = headers + dataString;
      }
      dataFile = SD.open(fileName, FILE_WRITE);
      if (dataFile) {
        dataFile.println(dataString);
        blinkLED(dataLED,5,100);
      } else { // if the file isn't open, pop up an error:
        Serial.println(String("error opening ") + fileName);
      }
      dataFile.close();
    }

    // Check battery at period batPeriod
    if (sec%batPeriod == 0) {
      batVolt = 2 * 3.3 * analogRead(batVoltPin) / 1024;
      if (batVolt < BAT_LOW) {
        blinkLED(batLED,1,200);
      } else if (batVolt >= BAT_FULL) {
        blinkLED(batLED,0,1);
      } else
        blinkLED(batLED,0,0);
    }
  }
  
}


/***** Init Function *****/
bool Init_All(void) {
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial.flush();
  Serial1.flush();
  Serial.println("Begin Setup");

  // Pin Setup
  pinMode(dataLED, OUTPUT);
  pinMode(batLED, OUTPUT);
  digitalWrite(batLED, 0);
  pinMode(batVoltPin, INPUT);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    return false;
  }

  // Setup Current Time from RTC
  setTime(rtc.now().unixtime());
  sec = now();

  // Setup SD card
  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return false;
  }

  // Read Period Value from SD card
  if (samplePeriodFile = SD.open(SAMPLE_PERIOD_FILE_NAME, FILE_READ))
    samplePeriodFile.read(&samplePeriod,sizeof(int));
    samplePeriodFile.close();
  
  // Setup for DGS
  mySensor.DEBUG = false;
  if (SETUP) {
    Init_DGS();
  }

  // Setup for SHT32
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    return false;
  }

  Serial.println("Finished Setup");
  return true;
}

void Init_DGS(void) {
  Serial.println();
  Serial.println(mySensor.getFW());  // Retruns FW date

  if (mySensor.setBC("071615011247 110102 CO 1507 4.05")) Serial.println("Finished Setting BC");  //Enter your barcode string. Returns 1/0.

  mySensor.getEEPROM();  //Read eeprom into array and/or list view if DEBUG is TRUE. See end of sketch for outline of values.
  Serial.println();

  if (mySensor.getLMP()) { // Loads LMP values into variable returns 1/0
    Serial.println("LMP91000 Values loaded into array");
    if (mySensor.setLMP(mySensor.LMP[0], mySensor.LMP[1], mySensor.LMP[2])) Serial.println("Finished Setting New Values For LMP"); //Sets LMP variables into register returns 1/0
  }
  Serial.println();
  
  if (mySensor.setToff(2.3)) Serial.println("Finished Setting T Offset"); //Set T offset in deg C returns 1/0
  Serial.println();

  Serial.println("Enter 'Z' When Sensor is Stabilized");
  while (Serial.read() != 'Z') {}
  if (mySensor.zero()) Serial.println("Finished Setting Zero"); //zeros returns 1/0
  Serial.println();

  Serial.println("Delay for zeroing...");
  delay(5000);
  if (mySensor.zero()) Serial.println("Finished Setting Zero"); //zeros returns 1/0
  Serial.println();
}

/***** Extra Functions *****/
String dateString(time_t t) {
  String year_str(year(t));
  year_str.remove(0,2);
  return String(month(t)) + "_" + String(day(t)) + "_" + year_str;
}

String timeString(void) {
  return String(hour()) + ":" + String(minute()) + ":" + String(second());
}

void ErrorLoop(void) {
  while (true) {
      digitalWrite(dataLED,1);
      delay(1000);
      digitalWrite(dataLED,0);
      delay(1000);
  }
}

void blinkLED(int pinNum, int cycles, int ms) {
  digitalWrite(pinNum,ms);
  for (int i = 0; i < cycles; i++) {
    digitalWrite(pinNum,1);
    delay(ms);
    digitalWrite(pinNum,0);
    delay(ms);
  }
}
