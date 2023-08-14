// For MIT APP
#include <Arduino.h>
#include "HTTPSRedirect.h"
//for OTA
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

#ifndef STASSID
#define STASSID "Redmi"
#define STAPSK  "123456789"
#endif
// For OTA
const char* ssid = STASSID;
const char* password = STAPSK;
// For MIT App
const char WiFiPassword[] = "123456789";
const char AP_NameChar[] = "OTA_Led" ;
const long interval_OTA = 600000; //60000 * 10 = 60sec or 1 min * 10 = 600000  10 min it willgive control OTA to APPL
unsigned long previousMillis_OTA = 0; 


WiFiServer server(80);
 String request = "";
 String DATA;
 char DATA_1[20];
 int MIT_flag = 0;
String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";



int OTA_flag;
int OTA_addr = 0;
int LED1 = 2;      // Assign LED1 to pin GPIO2
int Delay_Time = 1000;
int DEBUG = 1;

void OTA_Setup()
{
 Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
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
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}  

void MIT_APP_Setup()
{
  // Set device as a Wi-Fi Station
   WiFi.mode(WIFI_STA);
   WiFi.disconnect();

    boolean conn = WiFi.softAP(AP_NameChar, WiFiPassword);
    server.begin();
}

void MIT_APP() {

    WiFiClient client = server.available();
    if (!client)  {  return;  }
 
    // Read the first line of the request
    request = client.readStringUntil('\r');
   // Serial.println(request);

   if ( request.indexOf("SET") > 0 )  {OTA_flag?OTA_flag = 0:OTA_flag = 1;
   
 
            EEPROM.write(OTA_addr, OTA_flag);
            EEPROM.commit();
            delay(5000);
            ESP.restart();
   }
    else if  ( request.indexOf("ACC") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%s%s", "AC_current"," A"); }
    else if  ( request.indexOf("ACP") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%s%s", "AC_power"," W"); }

      

    client.flush();
 
    client.print( header );
    if (MIT_flag)
     client.print(DATA_1);
    else
     client.print(DATA); 
 
    delay(5);
}

void setup() {

  // initialize GPIO2 and GPIO16 as an output

  pinMode(LED1, OUTPUT);

EEPROM.begin(512); 
  
  OTA_flag = EEPROM.read(OTA_addr);
 if(OTA_flag)
  OTA_Setup();
 else
  MIT_APP_Setup();
   
/*
            OTA_flag = 1;
            EEPROM.write(OTA_addr, OTA_flag);
            EEPROM.commit();
*/
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
