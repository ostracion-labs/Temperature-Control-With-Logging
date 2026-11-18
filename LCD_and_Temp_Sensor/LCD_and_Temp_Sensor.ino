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

// struct to check for time initialization
struct checkInitialized {
  bool firstRun;
};

checkInitialized initStruct;

int checkInitializedAddr = 0;  //address for checkInitialized struct

HTU21D myHumidity;

LiquidCrystal_I2C lcd(0x3F, 16, 2);  //Set I2C Address of the LCD, Character Length, and Available Row
uRTCLib rtc(0x68);                   //Set I2C Address of the DS3231 RTC

char buffer[6] = "";  //buffer for sprintf function, so we can print like printf
float humd;           //set variable for humidity calculation, needed for sensor calibration
float temp;           //set variable for temperature calculation, needed for sensor calibration

void setup() {
  // put your setup code here, to run once:
  //Serial.begin(9600);
  URTCLIB_WIRE.begin();
  lcd.begin();
  EEPROM.get(checkInitializedAddr, initStruct);
  if (initStruct.firstRun == true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("First Run");
    // Get compile time
    const char* compileTime = __TIME__;
    const char* compileDate = __DATE__;

    // Parse hours, minutes and seconds from compileTime
    int hour = atoi(compileTime);
    int minute = atoi(compileTime + 3);
    int second = atoi(compileTime + 6);

    // Parse year, month and day from compileDate
    char month[4];
    int day, year;
    sscanf(compileDate, "%s %d %d", month, &day, &year);
    year = year-2000; //needed because the uRTCLib only support 2 letter year from 2000

    // Convert month to number
    int monthNumber;
    if (strcmp(month, "Jan") == 0) monthNumber = 1;
    else if (strcmp(month, "Feb") == 0) monthNumber = 2;
    else if (strcmp(month, "Mar") == 0) monthNumber = 3;
    else if (strcmp(month, "Apr") == 0) monthNumber = 4;
    else if (strcmp(month, "May") == 0) monthNumber = 5;
    else if (strcmp(month, "Jun") == 0) monthNumber = 6;
    else if (strcmp(month, "Jul") == 0) monthNumber = 7;
    else if (strcmp(month, "Aug") == 0) monthNumber = 8;
    else if (strcmp(month, "Sep") == 0) monthNumber = 9;
    else if (strcmp(month, "Oct") == 0) monthNumber = 10;
    else if (strcmp(month, "Nov") == 0) monthNumber = 11;
    else if (strcmp(month, "Dec") == 0) monthNumber = 12;

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
    lcd.print("Time Initialized");
    rtc.refresh();
    sprintf(buffer, "%02u:%02u:%02u", rtc.hour(), rtc.minute(), rtc.second());
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.setCursor(6, 1);
    lcd.print(buffer);
    delay(3000);
  }
  rtc.refresh();
  sprintf(buffer, "%02u:%02u", rtc.hour(), rtc.minute());

  myHumidity.begin();
  humd = myHumidity.readHumidity();
  temp = myHumidity.readTemperature();

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
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.setCursor(6, 1);
  lcd.print(temp);
  lcd.setCursor(11,1);
  lcd.print((char)223);
  lcd.setCursor(12,1);
  lcd.print("C");
}

int dayOfWeek(int y, int m, int d) {
  static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  y -= m < 3;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7 + 1;
}

void loop() {
  // put your main code here, to run repeatedly:
  rtc.refresh();
  if (rtc.second() % 15 == 0) { //set interval every 15 seconds, because second mod 15 equals to 0
    sprintf(buffer, "%02u:%02u", rtc.hour(), rtc.minute());  //2 digits unsigned integer with 0 padding to the left (uint8_t is an unsigned integer, so using u and not d)
    lcd.setCursor(6, 0);
    lcd.print(buffer);
    //temp = bmp.readTemperature()-1.5;
    //temp = (rtc.temp() / 100);
    lcd.setCursor(6, 1);

    humd = myHumidity.readHumidity();
    temp = myHumidity.readTemperature();
    lcd.print(temp);
    lcd.setCursor(11,1);
    lcd.print((char)223);
    lcd.setCursor(12,1);
    lcd.print("C");
    /*Serial.print(buffer);
    Serial.print(" ");
    Serial.println(temp);
    Serial.println(""); */
  }
}