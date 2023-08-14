#include <EEPROM.h>

int writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);

  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
  EEPROM.commit();
  return addrOffset+ 1 + len;
}

int readStringFromEEPROM(int addrOffset, String *strToRead)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];

  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0'; // !!! NOTE !!! Remove the space between the slash "/" and "0" (I've added a space because otherwise there is a display bug)

  *strToRead = String(data);
  return addrOffset + 1 +newStrLen;
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);

  int eepromOffset = 350;

  // Writing

  String str1 = "Today's tutorial:";
  String str2 = "Save String to EEPR";
  String str3 = "Thanks for reading!";

  int str1AddrOffset = writeStringToEEPROM(350, str1);
  int str2AddrOffset = writeStringToEEPROM(370, str2);
  writeStringToEEPROM(390, str3);

  // Reading

  String newStr1;
  String newStr2;
  String newStr3;

  int newStr1AddrOffset = readStringFromEEPROM(350, &newStr1);
  int newStr2AddrOffset = readStringFromEEPROM(370, &newStr2);
  readStringFromEEPROM(390, &newStr3);
  
  Serial.println(newStr1);
  Serial.println(newStr2);
  Serial.println(newStr3); 
}

void loop() {}
