/*
  SD card read/write

 This example shows how to read and write data to and from an SD card file
 The circuit:
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

 created   Nov 2010
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe

 This example code is in the public domain.

 */

#include <SPI.h>
#include <SD.h>

File myFile;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.print("Initializing SD card...");

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("30042022.txt", FILE_WRITE); //8 char ony file name

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to 30042022_log.txt...");
    myFile.println("AC_V = 230;AC_A = 10;AC_P = 2000;AC_pf = 1;DC_V = 12;DC_A = 150;Water = 1000L; Motor = OFF bore OFF; MCU ; tank = ok; motor=ok ;clock=ok;lora=ok;RTC=ok;  ");
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error Writing test.txt");
  }

  // re-open the file for reading:
  myFile = SD.open("30042022.txt"); //8 char only file name
  if (myFile) {
    Serial.println("Reading 30042022.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}

void loop() {
  // nothing happens after setup
}
