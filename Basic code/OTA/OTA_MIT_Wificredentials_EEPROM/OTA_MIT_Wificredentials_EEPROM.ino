// For MIT APP
#include <Arduino.h>
#include "HTTPSRedirect.h"
//for OTA
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <string.h>

// For OTA
String ssid = "Redmi";
String password = "123456789";
// For MIT App
String AP_NameChar = "OTA_Led" ;
String WiFiPassword = "123456789";
String Wifi_Pass, Wifi_Name, Hotspot_Name, Hotspot_Pass;

const long interval_OTA = 600000; //60000 * 10 = 60sec or 1 min * 10 = 600000  10 min it willgive control OTA to APPL
unsigned long previousMillis_OTA = 0; 


WiFiServer server(80);
 String request = "";
 String DATA;
 char DATA_1[20];
 int MIT_flag = 0;
String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

int OTA_flag , OTA_addr = 0, Wifi_addr = 350, Last_OTA_update;

int LED1 = 2;      // Assign LED1 to pin GPIO2
int Delay_Time = 1000;
int DEBUG = 1;




void OTA_Setup()
{
 Serial.println("Booting");
// update_Hotspot_credentials();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print("ssid : ");
    Serial.println(ssid);
    Serial.print("password : ");
    Serial.println(password);
    Serial.println("Connection Failed! Rebooting...");
    delay(500);
    Last_OTA_update = 6;
    EEPROM.write(4, Last_OTA_update);
    EEPROM.commit();
    
    int OTA_Reset_count = EEPROM.read(8);
  if(OTA_Reset_count > 2)
   {
     OTA_flag = 0;OTA_Reset_count=0; // give cntrol to APPL
     EEPROM.write(OTA_addr, OTA_flag);
     EEPROM.write(8, OTA_Reset_count);
     EEPROM.commit();
     delay(5000);
     ESP.restart();    
   }
   OTA_Reset_count = OTA_Reset_count + 1;
   EEPROM.write(8, OTA_Reset_count);
   EEPROM.commit();
   delay(5000);
   ESP.restart(); 
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
            OTA_flag = 0;
            EEPROM.write(OTA_addr, OTA_flag);
            Last_OTA_update = 0;
            EEPROM.write(4, Last_OTA_update);
            EEPROM.commit();
            delay(5000);
            ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Last_OTA_update = 1;
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Last_OTA_update = 2;
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Last_OTA_update = 3;
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Last_OTA_update = 4;
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Last_OTA_update = 5;
      Serial.println("End Failed");
    }
    EEPROM.write(4, Last_OTA_update);
    EEPROM.commit();
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}  

void Last_OTA_Status()
{
  if(Last_OTA_update == 0)
   DATA = "OTA Flash_Successfull";
  else if(Last_OTA_update == 1)
   DATA = "OTA Auth_Failed";
  else if(Last_OTA_update == 2)
   DATA = "OTA Begin_Failed"; 
  else if(Last_OTA_update == 3)
   DATA = "OTA Connect_Failed";
  else if(Last_OTA_update == 4)
   DATA = "OTA Receive_Failed";  
  else if(Last_OTA_update == 5)
   DATA = "OTA End_Failed";
  else if(Last_OTA_update == 6)
   DATA = "OTA Hotspot_Connect_Failed";    
   
}

void MIT_APP_Setup()
{
  // Set device as a Wi-Fi Station
   WiFi.mode(WIFI_STA);
   WiFi.disconnect();

    boolean conn = WiFi.softAP(AP_NameChar, WiFiPassword);
    server.begin();
}

int writeStringToEEPROM(int addrOffset, String &strToWrite)
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

 int Wifi_Decode(char *T,int op,char e)
 {
  char S[20];
  int c=0,i = 7; // pointing to Data
   memset(S,'\0',20);
  for(;T[i];i++)
 {
  if(T[i] == e){i++; break;}
   S[c] = T[i];
   c++;
 }
   if(op == 1)
    {
      Serial.println(S);
    Wifi_Name = String(S);
    Serial.print("WIFI Name from MIT : ");
    Serial.println(Wifi_Name);
    writeStringToEEPROM(390, Wifi_Name);
    }
   else if (op == 2)
    {
    Wifi_Pass =  String(S);
    Serial.print("WIFI Pass from MIT : ");
    Serial.println(Wifi_Pass);
    writeStringToEEPROM(410, Wifi_Pass);    
    }
   else if (op == 3)
    {
    Hotspot_Name =  String(S);
    Serial.print("Hotspot Name from MIT : ");
    Serial.println(Hotspot_Name);
    writeStringToEEPROM(350, Hotspot_Name);
    }
    else if (op == 4)
    {
    Hotspot_Pass =  String(S);
    Serial.print("Hotspot Pass from MIT : ");
    Serial.println(Hotspot_Pass);
    writeStringToEEPROM(370, Hotspot_Pass);
    } 
return i;
 }

void Wifi_update(String Name, int op)
{
 int n = Name.length();
 char T2[n+1];
 int i=7; // pointing to Wifi Name

strcpy(T2 , Name.c_str());
Serial.println(T2);
 i = Wifi_Decode(T2,op,':'); // 1 for Wifi Name
}


void MIT_APP() {

    WiFiClient client = server.available();
    if (!client)  {  return;  }
 
    // Read the first line of the request
    request = client.readStringUntil('\r');
   // Serial.println(request);

   if ( request.indexOf("OTA") > 0 )  {OTA_flag?OTA_flag = 0:OTA_flag = 1;
   
            EEPROM.write(OTA_addr, OTA_flag);
            EEPROM.commit();
            delay(5000);
            ESP.restart();
   }
    else if  ( request.indexOf("WNQ") > 0 ) { MIT_flag = 1; Serial.println(request);sprintf(DATA_1, "%s%s","WNA ",AP_NameChar);}
    else if  ( request.indexOf("WPQ") > 0 ) { MIT_flag = 1; Serial.println(request);sprintf(DATA_1, "%s%s","WPA ",WiFiPassword); }
    else if  ( request.indexOf("HNQ") > 0 ) { MIT_flag = 1; Serial.println(request);sprintf(DATA_1, "%s%s","HNA ",ssid); }
    else if  ( request.indexOf("HPQ") > 0 ) { MIT_flag = 1; Serial.println(request);sprintf(DATA_1, "%s%s","HPA ",password); }
    else if  ( request.indexOf("STA") > 0 ) { MIT_flag = 3;Serial.println(request); Last_OTA_Status(); }
    
    else if  ( request.indexOf("WN") > 0 ) { MIT_flag = 2;if(DEBUG) Serial.println(request); Wifi_update(request,1); }
    else if  ( request.indexOf("WP") > 0 ) { MIT_flag = 2;if(DEBUG) Serial.println(request); Wifi_update(request,2); }
    else if  ( request.indexOf("HN") > 0 ) { MIT_flag = 2;if(DEBUG) Serial.println(request); Wifi_update(request,3); }
    else if  ( request.indexOf("HP") > 0 ) { MIT_flag = 2;if(DEBUG) Serial.println(request); Wifi_update(request,4); }
    else if  ( request.indexOf("RST") > 0 ) { MIT_flag = 2;if(DEBUG) Serial.println(request);delay(5000);ESP.restart(); }
    
    client.flush();
 
    client.print( header );
    if (MIT_flag == 1)
    {
     client.print(DATA_1);
     Serial.println(DATA_1);
    }
    else if(MIT_flag == 3)
    {
     client.print(DATA);
     Serial.println(DATA); 
    }
    delay(5);
}

void setup() {

  // initialize GPIO2 and GPIO16 as an output
Serial.begin(115200);
  pinMode(LED1, OUTPUT);

EEPROM.begin(512); 

  readStringFromEEPROM(390, &AP_NameChar);  // 19 char reserved for 
  readStringFromEEPROM(410, &WiFiPassword);

  readStringFromEEPROM(350, &ssid);  // 19 char reserved for 
  readStringFromEEPROM(370, &password); 
 
 OTA_flag = EEPROM.read(OTA_addr);
 Last_OTA_update = EEPROM.read(4);
 if(OTA_flag)
  OTA_Setup();
 else
  MIT_APP_Setup();
}

void Led_Blink()
{
    digitalWrite(LED1, LOW);     // turn the LED off

  delay(Delay_Time);                // wait for a second

  digitalWrite(LED1, HIGH);  // turn the LED on

  delay(Delay_Time); 
}

// the loop function runs forever

void loop() {
   unsigned long currentMillis = millis();
   
  if(OTA_flag){
   ArduinoOTA.handle();
   
   if (currentMillis - previousMillis_OTA >= interval_OTA) {
    previousMillis_OTA = currentMillis;
    OTA_flag = 0;
    EEPROM.write(OTA_addr, OTA_flag);
    EEPROM.commit();
    delay(5000);
    ESP.restart();
   }
   
  }
  else
  {
    MIT_APP();
    Led_Blink();
  }
}
