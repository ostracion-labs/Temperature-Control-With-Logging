#include <SdFat.h>

#define SD_FAT_TYPE 2 // 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT

const uint8_t SD_CS_PIN = 10; // chip select (CS) pin

#define SPI_CLOCK SD_SCK_MHZ(50) // define the sd card speed, I think 50 is the highest

#if SD_FAT_TYPE == 0
SdFat sd;
File file;
#elif SD_FAT_TYPE == 1
SdFat32 sd;
File32 file;
#elif SD_FAT_TYPE == 2
SdExFat sd;
ExFile file;
#elif SD_FAT_TYPE == 3
SdFs sd;
FsFile file;
#else  // SD_FAT_TYPE
#error Invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK) // using shared SPI, gonna make sure no errors when we add more things

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  // wait for serial to be ready
  while (!Serial){

  }

  Serial.print("Initializing SD card...");
  if (!sd.begin(SD_CONFIG)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("Initialization done.");
  file = sd.open("test.txt", FILE_WRITE); // opening the file "test.txt" or creating and opening if the file doesn't exist

  if (file) {
    Serial.print("Writing to test.txt...");
    file.println("testing 1, 2, 3.");
    file.close();
    Serial.println("done.");
  } else {
    Serial.println("error opening test.txt");
  }

  file = sd.open("test.txt", FILE_WRITE);
  if (file) {
    Serial.print("Writing another line to test.txt...");
    file.println("testing 4, 5, 6.");
    file.close();
    Serial.println("done.");
  } else  {
    Serial.println("error opening test.txt");
  }

  file = sd.open("test.txt");
  if (file) {
    Serial.println("test.txt:");
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
  } else {
    Serial.println("error opening test.txt");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
