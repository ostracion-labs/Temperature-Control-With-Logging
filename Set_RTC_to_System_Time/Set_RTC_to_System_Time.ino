#include <Arduino.h>
#include <uRTCLib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2); //Set I2C Address of the LCD, Character Length, and Available Row
uRTCLib rtc(0x68); //Set I2C Address of the DS3231 RTC

char buffer[9]=""; //buffer for sprintf function, so we can print like printf
float temp; //set variable for temperature calculation, needed for sensor calibration

void setup() {
  // put your setup code here, to run once:
  //Serial.begin(9600);
  URTCLIB_WIRE.begin();

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
  int uploadDelayInSeconds = 30; // Change this to the number of seconds you want to add
  second += uploadDelayInSeconds;

  // Calculate day of the week (1=Sunday, 7=Saturday)
  int weekday = dayOfWeek(year, monthNumber, day);

  // Set the time and date
  rtc.set(second, minute, hour, weekday, day, monthNumber, year);

  lcd.begin();
  lcd.setBacklight(31);
  lcd.setCursor(0,0);
  lcd.print("-----Hello!-----");
  lcd.setCursor(0,1);
  lcd.print("--Initializing--");
  delay(7000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Time: ");
  lcd.setCursor(0,1);
  lcd.print("Temp: ");
}

int dayOfWeek(int y, int m, int d) {
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  y -= m < 3;
  return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7 + 1;
}

void loop() {
  // put your main code here, to run repeatedly:
  rtc.refresh();
  sprintf(buffer, "%02u:%02u:%02u", rtc.hour(), rtc.minute(), rtc.second()); //2 digits unsigned integer with 0 padding to the left (uint8_t is an unsigned integer, so using u and not d)
  lcd.setCursor (6,0);
  lcd.print(buffer);
  temp = (rtc.temp() / 100);
  lcd.setCursor(6,1);
  lcd.print(temp);
  delay(5000);
}
