//IMPORTANT: RUN Initialize_EEPROM_before_first_run.ino FIRST BEFORE REUPLOADING THIS SKETCH TO ENSURE TIME INITIALIZATION
//Wiring: SDA to A4, SCL to A5
//I2C Addresses: 0x3F (LCD), 0x40 (HTU21D) 0x68 (DS3231 RTC & Temp), 0x57 (DS3231 EEPROM)

#include <Arduino.h>
#include <EEPROM.h>
#include <uRTCLib.h>
#include <Wire.h>
#include <SdFat.h>
#include <LiquidCrystal_I2C.h>
#include <SparkFunHTU21D.h>

//change values here (pin, target temperature, threshold)
int ledPin = 7; // set pin for LED, used to know whether write operation has been completed or not
int relayPin = 6;
int buzzerPin = 5;
float targetTemperature = 37.0;
float threshold = 0.2;
float temperatureCalibration = -0.8; //Calibrate the sensor temperature by this number, 
//end of values

bool inHeating = false;

// struct to check for time initialization
struct checkInitialized {
  bool firstRun;
};

checkInitialized initStruct;

int checkInitializedAddr = 0;  //address for checkInitialized struct

// setting the sd module for ExFAT type
SdExFat sd;
ExFile file;

const uint8_t SD_CS_PIN = 10;     // chip select (CS) pin
#define SPI_CLOCK SD_SCK_MHZ(50)  // define the sd card speed, I think 50 is the highest, dunno what does it translate to in terms of MB/s or IOPS

/* more formats:
at the mercy of SdFatConfig.h (check that one first, probably we won't use this)
SdFat sd;
File file;

FAT16/FAT32
SdFat32 sd;
File32 file;

exFAT/fat64
SdExFat sd;
ExFile file;

FAT16/FAT32 and exFAT
SdFs sd;
FsFile file; */

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)  // using shared SPI, gonna make sure no errors when we add more things

HTU21D myHumidity;

LiquidCrystal_I2C lcd(0x3F, 16, 2);  //Set I2C Address of the LCD, Character Length, and Available Row
uRTCLib rtc(0x68);                   //Set I2C Address of the DS3231 RTC

bool sdError = false;  //variable to know whether the SD card has error

char buffer6[6] = "";    //buffer for sprintf function, so we can print like printf. +1 because of string terminator
char buffer9[9] = "";    //another buffer, different size (hh:mm:ss)
char buffer14[14] = "";  //another buffer, different size (yyyy mmm dd) with +1 again because the month name is string (so total +2)
char buffer21[21] = "";  //buffer for dateTime format
float humd;              //set variable for humidity calculation, needed for sensor calibration
float temp;              //set variable for temperature calculation, needed for sensor calibration

const char* monthsName[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };  // Array of Months, just define once because it will be needed in void loop logging

int dayOfWeek(int y, int m, int d) {
  static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  y -= m < 3;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7 + 1;
}

char* yyyymmmdd(int rtcYear, int rtcMonth, int rtcDay) {
  sprintf(buffer14, "%04u %s %02u", yearPrint(rtcYear), monthPrint(rtcMonth), rtcDay);
  /*
  Serial.print("Month from RTC: ");
  Serial.println(rtcMonth);
  Serial.print("Month: ");
  Serial.println(monthPrint(rtcMonth));
  Serial.println(buffer14);
  */
  return buffer14;
}

char* hhmmss(int rtcHour, int rtcMinute, int rtcSeconds) {
  sprintf(buffer9, "%02u:%02u:%02u", rtcHour, rtcMinute, rtcSeconds);
  return buffer9;
}

char* printDateTime(int rtcYear, int rtcMonth, int rtcDay, int rtcHour, int rtcMinute, int rtcSeconds) {
  sprintf(buffer21, "%04u-%02u-%02u %02u:%02u:%02u", yearPrint(rtcYear), rtcMonth, rtcDay, rtcHour, rtcMinute, rtcSeconds);
  return buffer21;
}

// because these 2 will be called from setup and loop, and possibly any other
char* monthPrint(int monthNumber) {
  return monthsName[monthNumber - 1];
}

int yearPrint(int rtcYear) {
  return rtcYear + 2000;
}

void checkFirstRun(checkInitialized& initStruct) {  // need the & to make sure the original variable is changed
  if (initStruct.firstRun == true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("First Run");
    // Get compile time
    const char* compileTime = __TIME__;
    const char* compileDate = __DATE__;

    /*
    Serial.print("compileDate: ");
    Serial.println(compileDate);
    */

    // Parse hours, minutes and seconds from compileTime
    int hour = atoi(compileTime);
    int minute = atoi(compileTime + 3);
    int second = atoi(compileTime + 6);

    // Parse year, month and day from compileDate
    char month[4];
    int day, year;
    sscanf(compileDate, "%s %d %d", month, &day, &year);
    month[3] = '\0';
    /*
    Serial.print("Got month: ");
    Serial.print(month);
    Serial.println("|");
    */
    
    year = year - 2000;  //needed because the uRTCLib only support 2 letter year from 2000

    // Convert month to number
    int monthNumber;
    for (int i = 0; i < 12; i++) {
      if (strcmp(month, monthsName[i]) == 0) {
        monthNumber = i + 1;
        break;
      }
    }
    /*
    Serial.print("monthNumber: ");
    Serial.println(monthNumber);
    */

    // Manually adjust the time for upload delay
    int uploadDelayInSeconds = 10;  // Change this to the number of seconds you want to add
    second += uploadDelayInSeconds;

    // Calculate day of the week (1=Sunday, 7=Saturday)
    int weekday = dayOfWeek(year, monthNumber, day);

    // Set the time and date
    rtc.set(second, minute, hour, weekday, day, monthNumber, year);
    initStruct.firstRun = false;
    EEPROM.put(checkInitializedAddr, initStruct);
    lcd.clear();
    lcd.setCursor(0, 0);
    rtc.refresh();
    lcd.print(yyyymmmdd(rtc.year(), rtc.month(), rtc.day()));
    /*
    Serial.print("Month number from RTC: ");
    Serial.println(rtc.month());
    */
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.setCursor(6, 1);
    lcd.print(hhmmss(rtc.hour(), rtc.minute(), rtc.second()));
    delay(3000);
  }
}




void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin, OUTPUT); // set the red led pin on Arduino as output, used for file operation indicator
  //Serial.begin(9600);
  URTCLIB_WIRE.begin();

  // begin sd card, if fail type in LCD
  lcd.begin();
  if (!sd.begin(SD_CONFIG)) {
    lcd.setCursor(0, 0);
    lcd.print("-SD Init Failed-");
    while (1)
      ;
  }
  // check if board is first run after programming, flash the Initialize_EEPROM first!
  EEPROM.get(checkInitializedAddr, initStruct);
  checkFirstRun(initStruct);

  rtc.refresh();
  sprintf(buffer6, "%02u:%02u", rtc.hour(), rtc.minute());
  myHumidity.begin();
  //humd = myHumidity.readHumidity();
  temp = myHumidity.readTemperature() + temperatureCalibration;

  sd.cacheClear();  //clear the SD cache file, required because cache makes the file exists even when the card is removed
  // create the log file and always type timestamp after every boot, to make sure that power offs are known
  file = sd.open("tempLog.csv", FILE_WRITE);
  if (file) {
    digitalWrite(ledPin, HIGH);
    file.println("Date & Time;Temperature");
    file.print(printDateTime(rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second()));
    file.print(";");
    file.print(temp);
    file.print(";");
    file.println("Started");
    file.flush();
    file.close();
    delay(1500);
    digitalWrite(ledPin, LOW);
    sdError = false;
  } else {
    lcd.setCursor(0, 0);
    lcd.print("--Cannot write--");
    lcd.setCursor(0, 1);
    lcd.print("--to log file---");
    sdError = true;
  }

  if (sdError == false) {
    lcd.setBacklight(31);
    lcd.setCursor(0, 0);
    lcd.print("-----Hello!-----");
    lcd.setCursor(0, 1);
    lcd.print("--Initializing--");
    delay(5000);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    lcd.setCursor(6, 0);
    lcd.print(buffer6);
    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.setCursor(6, 1);
    lcd.print(temp);
    lcd.setCursor(11, 1);
    lcd.print((char)223);
    lcd.setCursor(12, 1);
    lcd.print("C");
  }
  if (temp < (targetTemperature-threshold)) {
    digitalWrite(relayPin, HIGH);
    inHeating = true;
  } else {
    digitalWrite(relayPin, LOW);
    inHeating = false;
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  rtc.refresh();
  temp = myHumidity.readTemperature() + temperatureCalibration;
  if (inHeating == true) {
    if (temp > (targetTemperature+threshold)) {
      digitalWrite(relayPin, LOW);
      inHeating = false;
    }
  } else if (temp < (targetTemperature-threshold)) {
    digitalWrite(relayPin, HIGH);
    inHeating = true;
  }
  if (rtc.second() % 30 == 0) {  //set interval every 30 seconds, because second mod 30x equals to 0
    temp = myHumidity.readTemperature() + temperatureCalibration;
    sd.cacheClear();  //clear the SD cache file, required because cache makes the file exists even when the card is removed
    file = sd.open("tempLog.csv", FILE_WRITE);
    if (file) {
      digitalWrite(ledPin, HIGH);
      file.print(printDateTime(rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute(), rtc.second()));
      file.print(";");
      if (sdError == true) {
        file.print(temp);
        file.print(";");
        file.println("Restarted");
      } else {
        file.println(temp);
      }
      file.flush();
      file.close();
      delay(1500);
      digitalWrite(ledPin, LOW);
      sdError = false;
    } else {
      lcd.setCursor(0, 0);
      lcd.print("--Cannot write--");
      lcd.setCursor(0, 1);
      lcd.print("--to log file---");
      sdError = true;
    }

    if (sdError == false) {
      lcd.clear();
      sprintf(buffer6, "%02u:%02u", rtc.hour(), rtc.minute());  //2 digits unsigned integer with 0 padding to the left (uint8_t is an unsigned integer, so using u and not d)
      lcd.setCursor(0, 0);
      lcd.print("Time: ");
      lcd.setCursor(6, 0);
      lcd.print(buffer6);
      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.setCursor(6, 1);
      //humd = myHumidity.readHumidity();
      lcd.print(temp);
      lcd.setCursor(11, 1);
      lcd.print((char)223);
      lcd.setCursor(12, 1);
      lcd.print("C");
      delay(1000);
    }

    if (sdError == true) {
      sd.begin(SD_CONFIG);
    }
  }
}
