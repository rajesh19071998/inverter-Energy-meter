// Example Arduino/ESP8266 code to upload data to Google Sheets
// Follow setup instructions found here:
// https://github.com/StorageB/Google-Sheets-Logging
// reddit: u/StorageB107
// email: StorageUnitB@gmail.com


#include <Arduino.h>
#include "HTTPSRedirect.h"
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ErriezDS1307.h>  //RTC  5 , 4
#include <ESP8266WiFi.h>
#include <ModbusMasterPzem017.h>
#include <SoftwareSerial.h>
#include <espnow.h>
static uint8_t pzemSlaveAddr = 0x01; //PZem Address
static uint16_t NewshuntAddr = 0x0000;      // Declare your external shunt value. Default is 100A, replace to "0x0001" if using 50A shunt, 0x0002 is for 200A, 0x0003 is for 300A
ModbusMaster node;
SoftwareSerial DC_Serial(D5, D6); //(14 Rx , 12 TX)D5,D6
bool flag = false;

const char WiFiPassword[] = "123456789";
const char AP_NameChar[] = "UPS";

/********************************************************************/
// Enter network credentialsfor Internet connection MIT App Hotspot:
//const char* ssid     = "Rajesh_CCTV_Camera";
//const char* password = "Rajesh@1234";

const char* ssid     = "Redmi";
const char* password = "123456788";
/***************************************************************/

ErriezDS1307 rtc;
PZEM004Tv30 pzem(D3, D4); // Software Serial pin 0 (RX) & 2 (TX)  D3,D4   : 3 volt
int Buzzer = 16; //D0 Buzzer pin
int AC_in = 13; // D7 AC input
int Relay = 15; // D8 Relay
int DEBUG=1;
int MIT_flag = 0;  
int P_cut_flag = 0; // 0 is Power in 1 is Power cut
int Last_P_cut_flag = 0; // Hold previous P_cut_flag
int H, M, S,  d, m, y, w , val; // for mit req decoding 

int AC_buzzer_count = 0;
int AC_buzzer_count_off = 0;

/*****************Readings  *********/
 
uint8_t UPS_hour=5, UPS_min=53, UPS_sec=43, UPS_mday=30, UPS_mon=6, UPS_wday=5;
uint8_t UPS_Last_min = 0 , temp_hour=0, temp_min= 0, temp_sec=0, temp_mday=0, temp_mon=0, temp_wday=0;

int Rtc=0; // RTC working
uint16_t UPS_year=2023, temp_year=2023;
int temp_Time_flag = 1; // rtc was working

float AC_voltage=230, AC_current=5, AC_power=1254, AC_energy =125447, AC_frequency=60, AC_pf=0.99;
char Last_Pin[8], Last_Pcut[8];
float DC_voltage=12.55, DC_current=15.99, DC_power, DC_energy;


WiFiServer server(80);
 String request = "";
 String DATA;
 char DATA_1[25];

String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t TANK[] = {0xEA, 0xDB, 0x84, 0x99, 0x43, 0x55};  // TANK board MAC
uint8_t MOTOR_MAC[] = {0xEA, 0xDB, 0x84, 0x98, 0x81, 0xBB}; //MOTOR board MAC
uint8_t OLD[] = {0xF4, 0xCF, 0xA2, 0xD7, 0xCA, 0x2D};       // old NODEMCU board MAC

// Updates DHT readings every 10 seconds
const long interval = 10000; 
unsigned long previousMillis = 0;    // MIT APP was updated every 10 sec

const long interval1 = 20000; 
unsigned long previousMillis1 = 0;    // AC and DC module data update every 20 sec 

const long interval2 = 60000; 
unsigned long previousMillis2 = 0;    // Clock check was done every 1 min = 60 sec 

// Variable to store if sending data was successful
String success;

/****************************************************************************/



// Enter Google Script Deployment ID:
const char *GScriptId = "AKfycbyXTHUXPVlsiwOu8LMy4KQGZJSRlI4iIT9fmri6aV7-nkKQaHMV1fHVNWTsD4zuiVk";//"AKfycpwCtWuN3BNIO5CR9BtZJAMaRNUgusldcAz6V7rtGCg1L5srZ5iAEZumVeb-1ea5Y6mg3Q";

// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
//String payload_base =  "{\"command\": \"append_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

// Google Sheets setup (do not edit)
const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;

// Declare variables that will be published to Google Sheets
int value0 = 0;
int value1 = 0;
int value2 = 0;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t mday;
    uint8_t mon;
    uint16_t year;
    uint8_t wday;
    int Rtc;
    float AC_voltage;
    float AC_current;
    float AC_power;
    float AC_energy;
    float AC_frequency;
    float AC_pf;
    float DC_voltage;
    float DC_current;
    float DC_power;
    float DC_energy;
    char Last_Pcut[8];
    char Last_Pin[8];
} struct_message;

// Create a struct_message called DHTReadings to hold sensor readings
struct_message UPS_DATA;

// Create a struct_message to hold incoming sensor readings
struct_message incoming_DATA;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if(DEBUG)
  {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
  }
}

// Callback when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    
   if(mac[0] == TANK[0] && mac[1] == TANK[1] && mac[2] == TANK[2] && mac[3] == TANK[3] && mac[4] == TANK[4] && mac[5] == TANK[5])
    {
  memcpy(&incoming_DATA, incomingData, sizeof(incoming_DATA));
  if(DEBUG)
  {
  Serial.print("Bytes received: ");
  Serial.println(len);

   Serial.print("TANK Mac address: ");
       for (int i = 0; i < 6; i++) {
        Serial.print("0x");
        Serial.print(mac[i], HEX);
         Serial.print(":");
        }
       Serial.println();
  }
  
   }
else
 {
  if(DEBUG)
  {
   Serial.print(" OTHER Mac address: ");
       for (int i = 0; i < 6; i++) {
        Serial.print("0x");
        Serial.print(mac[i], HEX);
         Serial.print(":");
        }
       Serial.println();
   }  
 }

  
}



void Battery_Status()
{
  Serial.println();
  if(DC_voltage >= 14.4 && DC_current < 0.800)
  {
    Serial.println("Battery was fully charged");        
  }
  if(DC_voltage < 10.5)
  {
    Serial.println("Battery was Very Low");        
  }
  if(DC_voltage < 13.5)
  {
   // Serial.println("Power Cut......!");
   // P_cut_flag = 1;
  }
  if(DC_voltage >= 13.5)
  {
   // Serial.println("Power In......!");
   // P_cut_flag = 0;
  }
} 

void Power_Cut_Status()
{ 
  if(!digitalRead(AC_in))
   {
   P_cut_flag = 0; // Power in
   //Serial.println("Power IN......!");
   }
  else
   {
   P_cut_flag = 1; // power cut
   //Serial.println("Power Cut......!");
  }

  if(Last_P_cut_flag != P_cut_flag)
  {
    char P_time[8];
    if(P_cut_flag) // Power cut
    {
      //Last_Pcut
      sprintf(P_time,"%d%c%d",UPS_hour,':',UPS_min);
      strcpy(Last_Pcut , P_time);
      strcpy(UPS_DATA.Last_Pcut , Last_Pcut);
      Serial.println("Power Cut......!");
    }
    else  // Power In
    {
      //Last_Pin
      sprintf(P_time,"%d%c%d",UPS_hour,':',UPS_min);
      strcpy(Last_Pin , P_time);
      strcpy(UPS_DATA.Last_Pin , Last_Pin);
      Serial.println("Power IN......!");
    }
    Last_P_cut_flag = P_cut_flag;   // Update Last_P_cut_flag
  }

}

void setShunt(uint8_t slaveAddr) {
  static uint8_t SlaveParameter = 0x06;                                                             /* Write command code to PZEM */
  static uint16_t registerAddress = 0x0003;                                                         /* change shunt register address command code */
  
  uint16_t u16CRC = 0xFFFF;                                                                         /* declare CRC check 16 bits*/
  u16CRC = crc16_update(u16CRC, slaveAddr);                                                         // Calculate the crc16 over the 6bytes to be send
  u16CRC = crc16_update(u16CRC, SlaveParameter);
  u16CRC = crc16_update(u16CRC, highByte(registerAddress));
  u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
  u16CRC = crc16_update(u16CRC, highByte(NewshuntAddr));
  u16CRC = crc16_update(u16CRC, lowByte(NewshuntAddr));
      
  Serial.println("Change shunt address");
  DC_Serial.write(slaveAddr); //these whole process code sequence refer to manual
  DC_Serial.write(SlaveParameter);
  DC_Serial.write(highByte(registerAddress));
  DC_Serial.write(lowByte(registerAddress));
  DC_Serial.write(highByte(NewshuntAddr));
  DC_Serial.write(lowByte(NewshuntAddr));
  DC_Serial.write(lowByte(u16CRC));
  DC_Serial.write(highByte(u16CRC));
    delay(10); delay(100);
    while (DC_Serial.available()) {
      Serial.print(char(DC_Serial.read()), HEX); //Prints the response and display on Serial Monitor (Serial)
      Serial.print(" ");
   }
} //setShunt Ends

void DC_Module()
{
 uint8_t result;
    result = node.readInputRegisters(0x0000, 6);
      if (result == node.ku8MBSuccess) {
        uint32_t tempdouble = 0x00000000;
          DC_voltage = node.getResponseBuffer(0x0000) / 100.0;
          DC_current = node.getResponseBuffer(0x0001) / 100.0;
        tempdouble =  (node.getResponseBuffer(0x0003) << 16) + node.getResponseBuffer(0x0002); // get the power value. Power value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit 
          DC_power = tempdouble / 10.0; //Divide the value by 10 to get actual power value (as per manual)
        tempdouble =  (node.getResponseBuffer(0x0005) << 16) + node.getResponseBuffer(0x0004);  //get the energy value. Energy value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit
          DC_energy = tempdouble;
            Serial.print(DC_voltage, 1); //Print Voltage value on Serial Monitor with 1 decimal*/
            Serial.print("V   ");
            Serial.print(DC_current, 3); Serial.print("A   ");
            Serial.print(DC_power, 1); Serial.print("W  ");
            Serial.print(DC_energy, 0); Serial.print("Wh  ");
              Serial.println();

              UPS_DATA.DC_voltage = DC_voltage;
              UPS_DATA.DC_current = DC_current;
              UPS_DATA.DC_power = DC_power;
              UPS_DATA.DC_energy = DC_energy;
    
    } else { Serial.println("DC Module Not connected Failed to read modbus");
              UPS_DATA.DC_voltage = 999;
              UPS_DATA.DC_current = 999;
              UPS_DATA.DC_power = 999;
              UPS_DATA.DC_energy = 999;
    }

    Battery_Status(); 
}

void setup() {
  pinMode(Buzzer,OUTPUT);
  tone(Buzzer,4000,5000);// pin,4000 is louder freq, 5000 is duration
  delay(500);
  noTone(Buzzer);

  pinMode(AC_in,INPUT_PULLUP);
  
  Serial.begin(9600);
  DC_Serial.begin(9600);
  setShunt(pzemSlaveAddr);
  node.begin(pzemSlaveAddr, DC_Serial);
          
 
     
    // Connect to WiFi
  WiFi.begin(ssid, password);
  if(DEBUG) {             
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ..."); }
 for (int i=0; i<10; i++) 
  {
  if(WiFi.status() != WL_CONNECTED ){ 
    delay(1000);
    if(DEBUG)
    Serial.print(".");
    flag = false;
  }
  else{
 flag = true;
 if(DEBUG)
  Serial.println("WIFI Connected....!");
  break;
  }
  }
  if(DEBUG){
  Serial.print("flag = ");
  Serial.println(flag);  }
  if(flag == true){
   if(DEBUG){ 
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); }

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  if(DEBUG){
  Serial.print("Connecting to ");
  Serial.println(host);  }

  // Try to connect for a maximum of 5 times
  bool flag1 = false;
  for (int i=0; i<5; i++){ 
    int retval = client->connect(host, httpsPort);
    if (retval == 1){
       flag1 = true;
       if(DEBUG)
       Serial.println("Connected");
       break;
    }
    else
    {
      if(DEBUG)
      Serial.println("Connection failed. Retrying...");
    }
  }
  if (!flag1){
    if(DEBUG){
    Serial.print("Could not connect to server: ");
    Serial.println(host); }
    return;
  }
  delete client;    // delete HTTPSRedirect object
  client = nullptr; // delete HTTPSRedirect object
  }
 else{
// call mit app 
  // Set device as a Wi-Fi Station
   WiFi.mode(WIFI_STA);
   WiFi.disconnect();

    boolean conn = WiFi.softAP(AP_NameChar, WiFiPassword);
    server.begin();
    if(DEBUG){
    Serial.println("Hotspot was created for IT app");
    Serial.println("Username : UPS");
    Serial.println("Pass : 123456789"); }
  }
    
  // Init ESP-NOW
  if (esp_now_init() != 0) {
    if(DEBUG)
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
       
    delay(1000);
int RTC_count_flag = 0;
    
     Wire.begin();
     delay(1000);
    Wire.setClock(100000);
    R:
    if (!rtc.begin()) {
        if(DEBUG)
        Serial.println(F("RTC not found"));
        Rtc =1;  // RTC not working flag
        delay(3000);
        if(RTC_count_flag < 6){
        RTC_count_flag = RTC_count_flag + 1;
        goto R;
        }
        }
    // Enable RTC clock
    rtc.clockEnable(true);
   
     // Set date/time: 12:34:56 31 December 2020 Sunday
     /*
    if (!rtc.setDateTime(14, 43, 1,  25, 7, 2023, 2)) {
        Serial.println(F("Set date/time failed"));
    }
    */
    rtc.setSquareWave(SquareWaveDisable); // comment this line  
}

void AC_buzzer()
{
  if(!digitalRead(AC_in))  // if AC_in pin is LOW Power is on
  {
    if(AC_buzzer_count)
    {
    tone(Buzzer,4000,6000);
    delay(400);
    noTone(Buzzer);
    delay(400);
    AC_buzzer_count = AC_buzzer_count - 1;
    }
    else
    {
      AC_buzzer_count_off = 3;
    }
  }
  else
  {
    if(AC_buzzer_count_off)
    {
     tone(Buzzer,3500,6000);
    delay(500);
    noTone(Buzzer);
    delay(500);
    AC_buzzer_count_off = AC_buzzer_count_off - 1;
    }
    else
    { 
      AC_buzzer_count = 6;
    }
    
  }
}

void ESP_Now()
{
    unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the UPS data values
    previousMillis = currentMillis;
    
    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &UPS_DATA, sizeof(UPS_DATA));

  }
}

void MIT_APP() {
  // Check if a client has connected
    WiFiClient client = server.available();
    if (!client)  {  return;  }
 
    // Read the first line of the request
    request = client.readStringUntil('\r');
   // Serial.println(request);

   if ( request.indexOf("ACV") > 0 )  {MIT_flag = 1; sprintf(DATA_1, "%f%s", AC_voltage," V");}
    else if  ( request.indexOf("ACC") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%f%s", AC_current," A"); }
    else if  ( request.indexOf("ACP") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%f%s", AC_power," W"); }
    else if  ( request.indexOf("ACE") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%f%s", AC_energy," kwh"); }
    else if  ( request.indexOf("ACF") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%f%s", AC_frequency," Hz"); }
    else if  ( request.indexOf("ACPF") > 0 ) {MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%f%s", AC_pf," %"); }
    
    else if  ( request.indexOf("DCV") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%f%s", DC_voltage," V"); }
    else if  ( request.indexOf("DCC") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%f%s", DC_current," A"); }
    else if  ( request.indexOf("DCP") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%f%s", DC_power," W"); }
    else if  ( request.indexOf("DCE") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%f%s", DC_energy," kwh"); }
    else if  ( request.indexOf("LPC") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); strcpy(DATA_1 , Last_Pcut); }
    else if  ( request.indexOf("LPI") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); strcpy(DATA_1 , Last_Pin); }
    else if ( request.indexOf("GTIME") > 0 ) { MIT_flag = 1;if(DEBUG) Serial.println(request); sprintf(DATA_1, "%d%c%d%c%d%c%d%c%d%c%d%c%d", UPS_hour,':', UPS_min,':', UPS_sec,' ', UPS_mday,'/', UPS_mon,'/', UPS_year,' ', UPS_wday);}
    
    else if(request.indexOf("TIME") > 0 )  {MIT_flag = 0; Serial.println("TIME");if(DEBUG) Serial.println(request);TIME(request); }
    else if(request.indexOf("DATE") > 0 )  {MIT_flag = 0; Serial.println("DATE");if(DEBUG) Serial.println(request);DATE(request); }
    else if(request.indexOf("WEEK") > 0 )  {MIT_flag = 0; Serial.println("WEEK");if(DEBUG) Serial.println(request);WEEK(request); }
    else if(request.indexOf("SET") > 0 )  {MIT_flag = 0; Serial.println("SET");if(DEBUG) Serial.println(request);
              if(DEBUG) {
              Serial.print("Hour : ");
              Serial.println(H);
              Serial.print("Minute : ");
              Serial.println(M);
              Serial.print("Seconds : ");
              Serial.println(S);
              Serial.print("Day : ");
              Serial.println(d);
              Serial.print("Month : ");
              Serial.println(m);
              Serial.print("Year : ");
              Serial.println(y);
              Serial.print("Week : ");
              Serial.println(w);
              }
             if (!rtc.setDateTime(H, M,S , d, m, y, w))
              {
                if(DEBUG)
                Serial.println(F("Set date/time failed"));Rtc = 1;
                DATA = "TIME SET Failed";
              }
             else
              {
                Rtc = 0;
                DATA = "TIME SETED successfully";
              }
          }
      
  // if (!write(H, M, S,  d, m, y, w)){Serial.println(F("Set date/time failed"));}
                                                                                       //    else {Serial.println(F("Set date/time Updated"));}
    client.flush();
 
    client.print( header );
if(MIT_flag)
   client.print(DATA_1);
else
   client.print(DATA);   
 
    delay(5);
}



void G_sheet_update()
{
    // create some fake data to publish
  value0 ++;
  value1 = random(0,1000);
  value2 = random(0,100000);

 static bool flag2 = false;
  if (!flag2){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag2 = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr){
    if (!client->connected()){
      client->connect(host, httpsPort);
    }
  }
  else{
    if(DEBUG)
    Serial.println("Error creating client object!");
  }
  
  // Create json object string to send to Google Sheets
  payload = payload_base + "\"" + value0 + "," + value1 + "," + value2 + "\"}";
  
  // Publish data to Google Sheets
  if(DEBUG){
  Serial.println("Publishing data...");
  Serial.println(payload); }
  if(client->POST(url, host, payload)){ 
    // do stuff here if publish was successful
  }
  else{
    // do stuff here if publish was not successful
    if(DEBUG)
    Serial.println("Error while connecting");
    delay(5000);
    ESP.restart();
  }
}

void set_time()
{

 // Set date/time: 12:34:56 31 December 2020 Sunday
    if (!rtc.setDateTime(12, 59, 55,  31, 12, 2020, 0)) {
        if(DEBUG)
        Serial.println(F("Set date/time failed"));
        Rtc = 1;
    }

}
 int Decode(char *T,int i,char e)
 {
  char S[4];
  int c=0;
   memset(S,'\0',4);
  for(;T[i];i++)
 {
  if(T[i] == e){i++; break;}
   S[c] = T[i];
   c++;
  val = atoi(S);
 }
return i;
 }

 void printTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    char buf[10];

    // Print time
    snprintf(buf, sizeof(buf), "%d:%02d:%02d", hour, minute, second);
    Serial.println(buf);
  
}

void CLOCK()
{
    static uint8_t secondLast = 0xff;
    
    // Read date/time
    if (!rtc.getDateTime(&UPS_hour, &UPS_min, &UPS_sec, &UPS_mday, &UPS_mon, &UPS_year, &UPS_wday)) {
       // if(DEBUG)
       // Serial.println(F("Read date/time failed"));
        Rtc = 1;
    } else {
        // Print RTC time every second
        if (UPS_sec != secondLast) {
            secondLast = UPS_sec;

            // Print RTC time
            printTime(UPS_hour, UPS_min, UPS_sec);  // above function will print

        }
        if(temp_Time_flag)
        {
        Rtc = 0;
        UPS_DATA.hour = UPS_hour;
        UPS_DATA.min = UPS_min;
        UPS_DATA.sec = UPS_sec;
        UPS_DATA.mday = UPS_mday;
        UPS_DATA.mon = UPS_mon;
        UPS_DATA.year = UPS_year;
        UPS_DATA.wday = UPS_wday;
        
        UPS_Last_min = UPS_min;
        }
    }

    
    UPS_DATA.Rtc = Rtc;  
    
}

void clock_check()
{
    if((UPS_Last_min == UPS_min) )
    {
      temp_Time_flag = 0;
      Rtc = 1;
      UPS_DATA.Rtc = Rtc;

      temp_min = temp_min + 1;
     
    if(temp_min > 60)
    {
      temp_hour = temp_hour + 1;
      temp_min = 0;
    }
    if(temp_hour > 24)
    {
      temp_hour = 0;
      temp_mday = temp_mday + 1;
    }
    if(temp_mday > 30)
    {
      temp_mday = 0;
      temp_mon = temp_mon + 1;
    }
    if(temp_mon > 12)
    {
      temp_mon = 0;
      temp_year = temp_year + 1;
    }
    
        UPS_DATA.hour = temp_hour;
        UPS_DATA.min = temp_min;
        UPS_DATA.sec = temp_sec;
        UPS_DATA.mday = temp_mday;
        UPS_DATA.mon = temp_mon;
        UPS_DATA.year = temp_year;
        UPS_DATA.wday = temp_wday;  
      
    }
    else
    {
      temp_Time_flag = 1;
      
      temp_sec = UPS_sec;
      temp_hour = UPS_hour;
      temp_min = UPS_min;
      temp_mday = UPS_mday;
      temp_mon = UPS_mon;
      temp_year = UPS_year;
      
    }
}

void AC_Module()
{
  
  AC_voltage = pzem.voltage();
  AC_current = pzem.current();
  AC_power = pzem.power();
  AC_energy = pzem.energy();
  AC_frequency = pzem.frequency();
  AC_pf = pzem.pf();

    UPS_DATA.AC_voltage = AC_voltage;
    UPS_DATA.AC_current = AC_current;
    UPS_DATA.AC_power = AC_power;
    UPS_DATA.AC_energy = AC_energy;
    UPS_DATA.AC_frequency = AC_frequency;
    UPS_DATA.AC_pf = AC_pf;

  if(DEBUG)
  {
  if (AC_voltage != NAN) {
    Serial.print("AC_voltage: ");
    Serial.print(AC_voltage);
    Serial.println("V");
  } else {
    Serial.println("Error reading voltage");
  }
  if (AC_current != NAN) {
    Serial.print("AC_current: ");
    Serial.print(AC_current);
    Serial.println("A");
  } else {
    Serial.println("Error reading current");
  }
  if (AC_current != NAN) {
    Serial.print("AC_power: ");
    Serial.print(AC_power);
    Serial.println("W");
  } else {
    Serial.println("Error reading power");
  }
  if (AC_current != NAN) {
    Serial.print("AC_energy: ");
    Serial.print(AC_energy, 3);
    Serial.println("kWh");

  } else {
    Serial.println("Error reading energy");
  }
  if (AC_current != NAN) {
    Serial.print("AC_frequency: ");
    Serial.print(AC_frequency, 1);
    Serial.println("Hz");
  } else {
    Serial.println("Error reading frequency");
  }
  if (AC_current != NAN) {
    Serial.print("AC_pf: ");
    Serial.println(AC_pf);
  } else {
    Serial.println("Error reading power factor");
  }
  Serial.println();
}
}







void WEEK(String Week){
  int n = Week.length();
 char W2[n+1];
 int i=9; // pointing to week

 strcpy(W2 , Week.c_str());

 i = Decode(W2,i,' ');
 if(DEBUG)
 Serial.println(val);
 w = val;
}
void DATE(String Date){
int n = Date.length();
 char D2[n+1];
 int i=9; // pointing to date

 strcpy(D2 , Date.c_str());

 i = Decode(D2,i,'/');
 d = val;
 if(DEBUG)
  Serial.println(val);
 i = Decode(D2,i,'/');
 m = val;
 if(DEBUG)
  Serial.println(val);
 i = Decode(D2,i,' ');
 //y = val-44;
 y = val-100;  // +56
 //y = val;
 if(DEBUG)
  Serial.println(val);  
}
void TIME(String Time){

 int n = Time.length();
 char T2[n+1];
 int i=9; // pointing to time

 strcpy(T2 , Time.c_str());

 i = Decode(T2,i,':');
 H = val;
 if(DEBUG)
 Serial.println(val);
 i = Decode(T2,i,' ');
 M = val;
 if(DEBUG)
 Serial.println(val);

 }


void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis1 >= interval1) {
    AC_Module();
    DC_Module();
    previousMillis1 = currentMillis;
  }
 if (currentMillis - previousMillis2 >= interval2) {
  clock_check();  // update every 1 min
  previousMillis2 = currentMillis;
 }

if(flag == true )
 G_sheet_update();
else
 MIT_APP();
    
  CLOCK();
  ESP_Now();

  AC_buzzer();
  Power_Cut_Status();
  

}
