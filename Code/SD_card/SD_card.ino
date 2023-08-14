#include <Wire.h>
#include "SdFat.h" //sd card
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#define DATE_STRING_SHORT           3
int DEBUG = 1;

char line[100];
char Buf[100];

char UPSFilename[30];
char TankFilename[30];
char MotorFilename[30];
char PcutFilename[30];

void SD_CARD_Read(void);

const char WiFiPassword[] = "123456789";
const char AP_NameChar[] = "SD_LOGGER" ;

// Try max SPI clock for an SD. Reduce SPI_CLOCK if errors occur.
#define SPI_CLOCK SD_SCK_MHZ(50)

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(D8, DEDICATED_SPI, SPI_CLOCK)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(D8, SHARED_SPI, SPI_CLOCK)
#endif  // HAS_SDIO_CLASS


SdFat32 sd;
File32 file;




//------------------------------------------------------------------------------
// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))
//------------------------------------------------------------------------------
/*stucture variables*/

int water_level = 0, over_flow = 0, water_in = 0,motor = 0,bore = 0,button = 0,sender_ack = 0,receiver_ack = 0;
int m_motor=0,m_over_flow=0,m_Auto=0;
int motor_sel = 2, motor_sel_R = 0;// Both motors selected for input

float AC_voltage=230.0, AC_current=5.6, AC_power=1254.9, AC_energy =125.4, AC_frequency=60.8, AC_pf=0.99;
char Last_Pin[8], Last_Pcut[8];
float DC_voltage=12.55, DC_current=15.99, DC_power, DC_energy;

int sd_Read_flag = 0;

/********************************************************************/
int log_number;
int hour_count;
int H, M, S,  d, m, y, w ;

int flag = 0;  // for copy values to structure
 int val =0;

uint8_t UPS_hour=5, UPS_min=53, UPS_sec=43, UPS_mday=30, UPS_mon=6, UPS_wday=5;
uint8_t UPS_Last_min = 0 , temp_hour=0, temp_min= 0, temp_sec=0, temp_mday=0, temp_mon=0, temp_wday=0;
int Rtc=0; // RTC working
uint16_t UPS_year=2023 , temp_year=2023;
int temp_Time_flag = 0;

WiFiServer server(80);
 String request = "";
 String DATA;
 char DATA_1[20];
 int MIT_flag = 0;

String header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t TANK_MAC[] = {0xEA, 0xDB, 0x84, 0x99, 0x43, 0x55};  // TANK board MAC
uint8_t MOTOR_MAC[] = {0xEA, 0xDB, 0x84, 0x98, 0x81, 0xBB}; //MOTOR board MAC

uint8_t UPS_MAC[] = {0xF6, 0xCF, 0xA2, 0xD7, 0xCA, 0x2D};       // old NODEMCU board MAC

// Updates MIT app and ESP NOW data exchange readings every 10 seconds
const long interval = 10000; 
unsigned long previousMillis = 0;    // will store last time MIT APP and ESP NOW was updated 

const long interval1 = 60000;  // SD card update every 60 seconds = 1 min  //60000
unsigned long previousMillis1 = 0;    // will store last time DHT was updated 

const long interval2 = 1000; 
unsigned long previousMillis2 = 0;    // will store last time DHT was updated 

// Variable to store if sending data was successful
String success;

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

// Create a struct_message to hold incoming sensor readings
struct_message UPS_DATA;

typedef struct struct_message_tank {
    int water_level;
    int over_flow;
    int water_in;
} struct_message_tank;

// Create a struct_message called Tank to hold sensor readings
struct_message_tank TANK_data;

typedef struct struct_message_motor {
    int m_motor;
    int m_over_flow;
    int m_water_in;
    int m_Auto;
    int motor_sel;
    int Receiver_ack;
} struct_message_motor;

struct_message_motor MOTOR_data;

typedef struct struct_message_display {
    int motor_switch;
    int over_flow_flag;
    int sender_ack;
    int motor_sel;
    int mode_Auto;
} struct_message_display;
 struct_message_display DISPLAY_data; 


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
    
if(mac[0] == TANK_MAC[0] && mac[1] == TANK_MAC[1] && mac[2] == TANK_MAC[2] && mac[3] == TANK_MAC[3] && mac[4] == TANK_MAC[4] && mac[5] == TANK_MAC[5])
    {
     
      memcpy(&TANK_data, incomingData, sizeof(TANK_data));
      if(DEBUG)
      {
       Serial.print("TANK Bytes received: ");
       Serial.println(len);
       Serial.print("TANK Mac address: ");
       for (int i = 0; i < 6; i++) {
        Serial.print("0x");
        Serial.print(mac[i], HEX);
         Serial.print(":");
        }
       Serial.println();
      }
       water_level = TANK_data.water_level;
       over_flow = TANK_data.over_flow;
       water_in = TANK_data.water_in;
       if(DEBUG)
       Tank_data_print();
      }

if(mac[0] == MOTOR_MAC[0] && mac[1] == MOTOR_MAC[1] && mac[2] == MOTOR_MAC[2] && mac[3] == MOTOR_MAC[3] && mac[4] == MOTOR_MAC[4] && mac[5] == MOTOR_MAC[5])
    {
     
      memcpy(&MOTOR_data, incomingData, sizeof(MOTOR_data));
      if(DEBUG)
      {
       Serial.print("MOTOR Bytes received: ");
       Serial.println(len);
       Serial.print("MOTOR Mac address: ");
       for (int i = 0; i < 6; i++) {
        Serial.print("0x");
        Serial.print(mac[i], HEX);
         Serial.print(":");
        }
       Serial.println();
      }
       m_motor = MOTOR_data.m_motor;
       m_over_flow = MOTOR_data.m_over_flow;
       bore = MOTOR_data.m_water_in;
       m_Auto = MOTOR_data.m_Auto;
       motor_sel_R = MOTOR_data.motor_sel;
       receiver_ack = MOTOR_data.Receiver_ack;
       if(DEBUG)
       Motor_data_print();
       //flag_updating();
      } 
if(mac[0] == UPS_MAC[0] && mac[1] == UPS_MAC[1] && mac[2] == UPS_MAC[2] && mac[3] == UPS_MAC[3] && mac[4] == UPS_MAC[4] && mac[5] == UPS_MAC[5])
    {     
      memcpy(&UPS_DATA, incomingData, sizeof(UPS_DATA));
      if(DEBUG)
      {
       Serial.print("MOTOR Bytes received: ");
       Serial.println(len);
       Serial.print("UPS Mac address: ");
       for (int i = 0; i < 6; i++) {
        Serial.print("0x");
        Serial.print(mac[i], HEX);
         Serial.print(":");
        }
       Serial.println();
    } 
    UPS_hour = UPS_DATA.hour;
    UPS_min = UPS_DATA.min;
    UPS_sec = UPS_DATA.sec;
    UPS_mday = UPS_DATA.mday;
    UPS_mon = UPS_DATA.mon;
    UPS_year = UPS_DATA.year;
    UPS_wday = UPS_DATA.wday;
    Rtc = UPS_DATA.Rtc;
    AC_voltage = UPS_DATA.AC_voltage;
    AC_current = UPS_DATA.AC_current;
    AC_power = UPS_DATA.AC_power;
    AC_energy = UPS_DATA.AC_energy;
    AC_frequency = UPS_DATA.AC_frequency;
    AC_pf = UPS_DATA.AC_pf;
    DC_voltage = UPS_DATA.DC_voltage;
    DC_current = UPS_DATA.DC_current;
    DC_power = UPS_DATA.DC_power;
    DC_energy = UPS_DATA.DC_energy;
    strcpy(Last_Pcut,UPS_DATA.Last_Pcut);
    strcpy(Last_Pin,UPS_DATA.Last_Pin);
     if(DEBUG)
     UPS_DATA_print();    
    }
      else
      {
       if(DEBUG)
       { 
        Serial.print("OTHER BOARD Mac address: ");
       for (int i = 0; i < 6; i++) {
        Serial.print("0x");
        Serial.print(mac[i], HEX);
         Serial.print(":");
        }
       Serial.println();
       }
      } 
}


void printIncomingReadings(){
         
}   

void Tank_data_print()
{ 
  Serial.print("water_level : ");
  Serial.println(water_level);
  Serial.print("over_flow : ");
  Serial.println(over_flow);
  Serial.print("water_in : ");
  Serial.println(water_in);
  Serial.println();
  
}  
void Motor_data_print()
{
 Serial.print("m_motor : ");
 Serial.println(m_motor);
 Serial.print("m_over_flow : ");
 Serial.println(m_over_flow);
 Serial.print("bore : ");
 Serial.println(bore);
 Serial.print("m_Auto : ");
 Serial.println(m_Auto);
  Serial.print("motor_sel_R : ");
 Serial.println(motor_sel_R);
  Serial.print("receiver_ack : ");
 Serial.println(receiver_ack);
     
}       
void UPS_DATA_print()
{
char Time[25];  
char Date[25];
sprintf(Time, "Time %d:%d:%d Day : %d", UPS_hour, UPS_min, UPS_sec,UPS_wday);  
sprintf(Date, "Date %d/%d/%d", UPS_mday, UPS_mon ,UPS_year);  
Serial.println(Time);
Serial.println(Date);
Serial.print("Rtc flag : ");
Serial.println(Rtc);
Serial.println();
Serial.print("AC_voltage : ");
Serial.println(AC_voltage);
Serial.print("AC_current : ");
Serial.println(AC_current);
Serial.print("AC_power : ");
Serial.println(AC_power);
Serial.print("AC_energy : ");
Serial.println(AC_energy);
Serial.print("AC_frequency : ");
Serial.println(AC_frequency);
Serial.print("AC_pf : ");
Serial.println(AC_pf);
Serial.print("DC_voltage : ");
Serial.println(DC_voltage);
Serial.print("DC_current : ");
Serial.println(DC_current);
Serial.print("DC_power : ");
Serial.println(DC_power);
Serial.print("DC_energy : ");
Serial.println(DC_energy);
Serial.print("Last_Pcut : ");
Serial.println(Last_Pcut);
Serial.print("Last_Pin : ");
Serial.println(Last_Pin);
Serial.println();
   
}


// Check for extra characters in field or find minus sign.
char* skipSpace(char* str) {
  while (isspace(*str)) str++;
  return str;
}
//------------------------------------------------------------------------------
bool parseLine(char* str) {
  char* ptr;  
Serial.println(str);

  str = strtok(str, ",");
  if (!str) return false;

  // strtoul accepts a leading minus with unexpected results.
  if (*skipSpace(str) == '-') return false;

  // Convert string to unsigned long integer.
  uint32_t H = strtoul(str, &ptr, 0);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("H : ");
  Serial.println(H);

  str = strtok(nullptr, ",");
  if (!str) return false;

  // strtoul accepts a leading minus with unexpected results.
  if (*skipSpace(str) == '-') return false;

  // Convert string to unsigned long integer.
  uint32_t M = strtoul(str, &ptr, 0);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("M : ");
  Serial.println(M);

  if(sd_Read_flag == 3)  // only for Water tank
  {
   // Convert string to unsigned long integer.
  uint32_t W_Level = strtoul(str, &ptr, 0);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("W_Level : ");
  Serial.println(W_Level);

   // Convert string to unsigned long integer.
  uint32_t W_OvFlow = strtoul(str, &ptr, 0);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("W_OvFlow : ");
  Serial.println(W_OvFlow);

   // Convert string to unsigned long integer.
  uint32_t W_In = strtoul(str, &ptr, 0);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("W_In : ");
  Serial.println(W_In);
  }
  
 if(sd_Read_flag == 1 || sd_Read_flag == 2)  // for AC and DC reading variables
 {
  str = strtok(nullptr, ",");
  if (!str) return false;
  // Convert string to double.
  double AV = strtod(str, &ptr);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("AC_V : ");
  Serial.println(AV);

  str = strtok(nullptr, ",");
  if (!str) return false;
  // Convert string to double.
  double AC = strtod(str, &ptr);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("AC_C : ");
  Serial.println(AC);

  str = strtok(nullptr, ",");
  if (!str) return false;
  // Convert string to double.
  double AP = strtod(str, &ptr);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("AC_P : ");
  Serial.println(AP);

  str = strtok(nullptr, ",");
  if (!str) return false;
  // Convert string to double.
  double AE = strtod(str, &ptr);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("AC_E : ");
  Serial.println(AE);
 }

 if(sd_Read_flag == 1)  // Only AC
 {
  str = strtok(nullptr, ",");
  if (!str) return false;
  // Convert string to double.
  double AF = strtod(str, &ptr);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("AC_Freq : ");
  Serial.println(AF);

  str = strtok(nullptr, ",");
  if (!str) return false;
  // Convert string to double.
  double pf = strtod(str, &ptr);
  if (str == ptr || *skipSpace(ptr)) return false;
  Serial.print("pf : ");
  Serial.println(pf);
 }
   
  // Check for extra fields.
  return strtok(nullptr, ",") == nullptr;
}
/*****************************************************************************************************************************/


/****************************************************************     CLEARED         ********************************************/
void setup() {
  Serial.begin(9600);
   // Initialize the SD.
  if (!sd.begin(SD_CONFIG)) {
    sd.initErrorHalt(&Serial);
    return;
     }
   Wire.begin();
   log_number = EEPROM.read(1);

    // Set device as a Wi-Fi Station
   WiFi.mode(WIFI_STA);
   WiFi.disconnect();

    boolean conn = WiFi.softAP(AP_NameChar, WiFiPassword);
    server.begin();
    
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


void MIT_APP() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;

    //Get DHT readings
  

    
    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &UPS_DATA, sizeof(UPS_DATA));

    // Print incoming readings
    printIncomingReadings();
  }

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
    
    else if(request.indexOf("TIME") > 0 )  { MIT_flag = 0; Serial.println("TIME");if(DEBUG) Serial.println(request);TIME(request); }
    else if(request.indexOf("DATE") > 0 )  {MIT_flag = 0; Serial.println("DATE"); if(DEBUG) Serial.println(request);DATE(request); }
    else if(request.indexOf("WEEK") > 0 )  {MIT_flag = 0; Serial.println("WEEK");if(DEBUG) Serial.println(request);WEEK(request); }
    else if(request.indexOf("SET") > 0 )  {MIT_flag = 0; Serial.println("SET");if(DEBUG) Serial.println(request);  }

   // else if ( request.indexOf("AC") > 0 )  {MIT_flag = 1; sprintf(DATA_1, "%d%s", water_level," Ltrs");  }

    else if ( request.indexOf("WATER") > 0 )  {MIT_flag = 1; sprintf(DATA_1, "%d%s", water_level," Ltrs");  }
    else if  ( request.indexOf("OVERFLOW") > 0 ) {MIT_flag = 1; sprintf(DATA_1, "%s", over_flow?"YES":"NO"); }
    else if  ( request.indexOf("IN") > 0 ) {MIT_flag = 1; sprintf(DATA_1, "%s", water_in?"YES":"NO"); }
      

                                                                                       //    else {Serial.println(F("Set date/time Updated"));}
    client.flush();
 
    client.print( header );
    if (MIT_flag)
     client.print(DATA_1);
    else
     client.print(DATA); 
 
    delay(5);
}
/*****************************************************   START   ******************************************************************************************************/
void clear_Buf()
{
  int n = strlen(Buf)+1;
  int i=0;
  for (i=0;i<=n;i++)
  {
    Buf[i] = '\0';
  }
  n = strlen(UPSFilename)+1;
  for (i=0;i<=n;i++)
  {
    UPSFilename[i] = '\0';
  }
  memset(&UPSFilename[0],'\0',sizeof(UPSFilename));
}
void SD_card_data()
{
  if((UPS_Last_min != UPS_min) )  // || (!Rtc) )
  {
  char c = ',';
  clear_Buf();
  sprintf(Buf,"%d%c%d%c%f%c%f%c%f%c%f%c%f%c%f%s",UPS_hour,c,UPS_min,c,AC_voltage,c,AC_current,c,AC_power,c,AC_energy,c,AC_frequency,c,AC_pf,"\r\n");
  sprintf(UPSFilename, "%s%s%d%d%d%s","UPS/","AC_", UPS_mday, UPS_mon, UPS_year,".csv");
  SD_card_Write(1);
  sd_Read_flag = 1;
  SD_CARD_Read();
  clear_Buf();
  sprintf(Buf,"%d%c%d%c%f%c%f%c%f%c%f%s",UPS_hour,c,UPS_min,c,DC_voltage,c,DC_current,c,DC_power,c,DC_energy,"\r\n"); // double values in parseLine function
  sprintf(UPSFilename, "%s%s%d%d%d%s","UPS/","DC_", UPS_mday, UPS_mon, UPS_year,".csv");
  SD_card_Write(2);
  clear_Buf();
  sprintf(Buf,"%d%c%d%c%d%c%d%c%d%s",UPS_hour,c,UPS_min,c,water_level,c,over_flow,c,water_in,"\r\n"); // double values in parseLine function
  sprintf(UPSFilename, "%s%s%d%d%d%s","TANK/","Tank_", UPS_mday, UPS_mon, UPS_year,".csv");
  SD_card_Write(3);
  clear_Buf();
  temp_Time_flag = 0;
  UPS_Last_min = UPS_min;
  }

  else
  {
    if(temp_Time_flag == 0)
    {
      temp_sec = UPS_sec;
      temp_hour = UPS_hour;
      temp_min = UPS_min;
      temp_mday = UPS_mday;
      temp_mon = UPS_mon;
      temp_year = UPS_year;
    }
    
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
    
    
  char c = ',';
  clear_Buf();
  sprintf(Buf,"%d%c%d%c%f%c%f%c%f%c%f%c%f%c%f%s",temp_hour,c,temp_min,c,AC_voltage,c,AC_current,c,AC_power,c,AC_energy,c,AC_frequency,c,AC_pf,"\r\n");
  sprintf(UPSFilename, "%s%s%d%d%d%s","UPS/","AC_", temp_mday, temp_mon, temp_year,".csv");
  SD_card_Write(1);
  sd_Read_flag = 1;
  SD_CARD_Read();
  clear_Buf();
  sprintf(Buf,"%d%c%d%c%f%c%f%c%f%c%f%s",temp_hour,c,temp_min,c,DC_voltage,c,DC_current,c,DC_power,c,DC_energy,"\r\n"); // double values in parseLine function
  sprintf(UPSFilename, "%s%s%d%d%d%s","UPS/","DC_", temp_mday, temp_mon, temp_year,".csv");
  SD_card_Write(2);
  clear_Buf();
  sprintf(Buf,"%d%c%d%c%d%c%d%c%d%s",temp_hour,c,temp_min,c,water_level,c,over_flow,c,water_in,"\r\n"); // double values in parseLine function
  sprintf(UPSFilename, "%s%s%d%d%d%s","TANK/","Tank_", temp_mday, temp_mon, temp_year,".csv");
  SD_card_Write(3);
  clear_Buf();
    

     temp_Time_flag = 1;
  }
  
  
}



void SD_card_Write(int option) // called at time print
{
  L1:
  char c = ',';
  char BUF[100];
  //char Data[100];
  Serial.println();
  Serial.println("SD Card Write Function------------    ");
  Serial.println();
  if(!Rtc)  // if RTC working
  {
  //sprintf(UPSFilename, "%s%s%d%d%d%s","UPS/","UPS", UPS_mday, UPS_mon, UPS_year,".csv");
  // if the file opened okay, write to it:
    
    /*
    Rtc = UPS_DATA.Rtc;
    strcpy(Last_Pcut,UPS_DATA.Last_Pcut);
    strcpy(Last_Pin,UPS_DATA.Last_Pin);   */

   // if existing apped else create file.
  if (sd.exists(UPSFilename)) {
  // Create the file.
     if (!file.open(UPSFilename, O_APPEND | O_WRITE)) { //for append:   O_APPEND | O_WRITE  // for create file: FILE_WRITE
        error("Append open failed");
       }
       Serial.print("Writing to ");
       Serial.println(UPSFilename);
    
  file.print(Buf);
   Serial.println("Appended to file");     

  // Rewind file for read.
  file.rewind();
  file.close();
  }
  else
  {
   
   L:
   if (!file.open(UPSFilename, FILE_WRITE)) { //for Create file:   O_APPEND | O_WRITE  // for create file: FILE_WRITE
        Serial.println(" Create file in Directory Was failed");
        char dir[10];
        int i=0;
        for (i = 0;UPSFilename[i] != '/';i++)
        {
          dir[i] = UPSFilename[i];
        }
         dir[i] = '\0';
        if (!sd.exists(dir)) {
          if (!sd.mkdir(dir)) {
               Serial.print(dir);
               Serial.println("  Directory Creation was failed");
                 return;
             }
             else
             goto L;  // after creating Directory need to create file.
          }
       }
       Serial.print(UPSFilename);
       Serial.println(" File was Created.....");
       char Header[100];    
       if(option == 1) 
          sprintf(Header,"%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%s","Hour",c,"Min",c,"AC_voltage",c,"AC_current",c,"AC_power",c,"AC_energy",c,"AC_frequency",c,"AC_pf","\r\n");
       if(option == 2)  
          sprintf(Header,"%s%c%s%c%s%c%s%c%s%c%s%s","Hour",c,"Min",c,"DC_voltage",c,"DC_current",c,"DC_power",c,"DC_energy","\r\n");
       if(option == 3)
          sprintf(Header,"%s%c%s%c%s%c%s%c%s%s","Hour",c,"Min",c,"Water_level",c,"Over_flow",c,"Water_in","\r\n");    



       
   
        file.print(Header);
        file.close();
    goto L1;
  }

  Serial.println("***************************************************************************************");
  Serial.println();
  Serial.println();
  
  } 
}



void SD_CARD_Read()
{
int i = 0;
  Serial.println(UPSFilename);
if (!file.open(UPSFilename, FILE_READ)) {
    error("Read open failed");
  }

  while (file.available()) {
    int n = file.fgets(line, sizeof(line)); // line
    if (n <= 0) {
      error("fgets failed");
    }
    if (line[n - 1] != '\n' && n == (sizeof(line) - 1)) { // line
      error("line too long");
    }
    if(i != 0){  // only first line will skip. because first line has Header string.
    if (!parseLine(line)) {   //1.text , 2.i32, 3.u32, 4.double values in parseLine function ( change the order for new data types) 
      error("parseLine failed");
      i = i+1;
    } }
    Serial.println();
  }
  file.close();
  Serial.println(F("File reading was Done Successfully.............! "));


  
}


void loop() 
{

unsigned long currentMillis = millis();

MIT_APP();

if (currentMillis - previousMillis1 >= interval1) {
    // save the last time you updated the SD card values
    previousMillis1 = currentMillis;
    SD_card_data();
  }
}

 
