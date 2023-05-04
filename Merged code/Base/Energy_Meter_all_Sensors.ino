/*
pin configuration
-------------------------- 

Battery Voltage sensor --> A2 
Battery Current sensor --> A3, optional (8,2)
AC power module        --> 6,7
ESP12F communication   --> 4,5
SD card                --> 10,11,12,13
RTC module             --> A4,A5
Buzzer                 --> 3
AC Power control Relay --> 9
DHT sensor             --> A1 // digital only

Reserved pins          --> 0,1,A0

 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10 (for MKRZero SD: SDCARD_SS_PIN)

*/

#include <PZEM004Tv30.h> //AC power module
#include <Wire.h>
#include <SPI.h>
#include <SD.h> //sd card
#include <dht.h>
#include <SoftwareSerial.h>
#include <Robojax_WCS.h>  // DC current sensor A3 , 8, 2
PZEM004Tv30 pzem(6, 7); // Software Serial pin 6 (RX) & 7 (TX)
SoftwareSerial mySerial(4, 5); //ESP12F communication 4 RX,5 TX
#include <ErriezDS1307.h>  //RTC
#include "ErriezDS1307_Alarm.h"  // RTC
#define DHT11PIN A1
dht1wire DHT(DHT11PIN, dht::DHT11);
File myFile; // sd card
ErriezDS1307 rtc;
#define Voltage_12V_Sensor A2
#define BUZZER 3
#define AC_Relay 9
#define MODEL 10 //see list above 10
#define SENSOR_PIN A3 //pin for reading sensor
#define SENSOR_VCC_PIN 8 //pin for powring up the sensor
#define ZERO_CURRENT_LED_PIN 2 //zero current LED pin
#define ZERO_CURRENT_WAIT_TIME 5000 //wait for 5 seconds to allow zero current measurement
#define CORRECTION_VLALUE 164 //mA
#define MEASUREMENT_ITERATION 100
#define VOLTAGE_REFERENCE  5000.0 //5000mv is for 5V
#define BIT_RESOLUTION 10
#define DEBUT_ONCE true

Robojax_WCS sensor(
          MODEL, SENSOR_PIN, SENSOR_VCC_PIN, 
          ZERO_CURRENT_WAIT_TIME, ZERO_CURRENT_LED_PIN,
          CORRECTION_VLALUE, MEASUREMENT_ITERATION, VOLTAGE_REFERENCE,
          BIT_RESOLUTION, DEBUT_ONCE           
          );
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t mday;
    uint8_t mon;
    uint16_t year;
    uint8_t wday;          
    int RTC_error;  // if it is 0 no error
void alarmOff(void);
void alarmOn(void);
// Define at least one alarm (hour, minute, second, handler)
Alarm alarms[] = {
    Alarm(12, 0, 5, &alarmOn),
    Alarm(12, 0, 15, &alarmOff),
    Alarm(12, 0, 30, &alarmOn),
    Alarm(12, 1, 0, &alarmOff)
};
          
float adc_voltage = 0.0;
float in_voltage = 0.0, in_voltage1 = 0.0, in_voltage2 = 0.0, in_voltage3 = 0.0, in_voltage4 = 0.0, in_voltage5 = 0.0;
// 
float Battery_Volts = 0.0;
float Battery_Current = 0.0;
float AC_Volts = 0.0;
float AC_Current = 0.0;
float AC_pf = 0.0;
float AC_Power = 0.0;
float AC_Energy = 0.0;
float AC_frequency = 0.0;

int count = 0; 
// Floats for resistor values in divider (in ohms)
float R1 = 30000.0;
float R2 = 7500.0; 
 
// Float for Reference Voltage
float ref_voltage = 5.0;
 
// Integer for ADC value
int adc_value = 0;

#define INTERVAL_MESSAGE1 5000  //AC power module
#define INTERVAL_MESSAGE2 7000  // DC voltage
#define INTERVAL_MESSAGE3 11000
#define INTERVAL_MESSAGE4 13000
 
unsigned long time_1 = 0;
unsigned long time_2 = 0;
unsigned long time_3 = 0;
unsigned long time_4 = 0;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600); // for ESP12F communication
  sensor.start();  // Current sensor
  SD.begin(10); // sd card begin
   Wire.begin();
    Wire.setClock(400000);
    if (!rtc.begin()) {
        Serial.println(F("RTC not found"));
        }
    // Enable RTC clock
    rtc.clockEnable(true);
   /*
     // Set date/time: 12:34:56 31 December 2020 Sunday
    if (!rtc.setDateTime(12, 34, 56,  31, 12, 2020, 0)) {
        Serial.println(F("Set date/time failed"));
    }
     */   
  pinMode(BUZZER,OUTPUT);
  pinMode(AC_Relay,OUTPUT);

  digitalWrite(BUZZER,LOW);  // buzzer is off
  digitalWrite(AC_Relay,LOW); // relay was off
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

// Alarm on handler
void alarmOn()
{
    Serial.println("Alarm ON");
}

// Alarm off handler
void alarmOff()
{
    Serial.println("Alarm OFF");
}

void printTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    char buf[10];

    // Print time
    snprintf(buf, sizeof(buf), "%d:%02d:%02d", hour, minute, second);
    Serial.println(buf);
    SD_card();
}

void CLOCK()
{
    static uint8_t secondLast = 0xff;
    
    // Read date/time
    if (!rtc.getDateTime(&hour, &min, &sec, &mday, &mon, &year, &wday)) {
        Serial.println(F("Read date/time failed"));
        RTC_error = 1;
        return;
    } else {
        // Print RTC time every second
        if (sec != secondLast) {
            secondLast = sec;

            // Print RTC time
            printTime(hour, min, sec);  // above function will print

            // Handle alarms
            for (uint8_t i = 0; i < sizeof(alarms) / sizeof(Alarm); i++) {
                alarms[i].tick(hour, min, sec);
                RTC_error = 0;
            }
        }
    }
}

void SD_card() // called at time print
{
   // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  CLOCK();
  //sprintf(buffer, "Sum of %d and %d is %d", a, b, c);
  char Filename[20];
  char Time[20];
  char Data[100];
  sprintf(Filename, "%d%d%d%s", mday, mon, year,".txt");
  sprintf(Time, "%s%d%c%d%c","TIME ", hour, ':', min,' ');
  myFile = SD.open(Filename, FILE_WRITE);  // file name must below 8 char

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to 13052023.txt...");
    myFile.print(Time);
    sprintf(Data, "%s%f%s%f%s%f%s%f%s%f%s%f","AC_V = ",AC_Volts,"AC_A = ",AC_Current,"AC_P = ",AC_Power,"AC_pf = ",AC_pf,"DC_V = ",Battery_Volts,"DC_A = ",Battery_Current);
    myFile.print(Data);
    //myFile.println("AC_V = 230;AC_A = 10;AC_P = 2000;AC_pf = 1;DC_V = 12;DC_A = 150;Water = 1000L; Motor = OFF bore OFF; MCU ; tank = ok; motor=ok ;clock=ok;lora=ok;RTC=ok; ");
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.print("error opening File");
    Serial.println(Filename);
  }

  // re-open the file for reading:
  myFile = SD.open(Filename);
  if (myFile) {
    Serial.println(Filename);

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.print("error opening File");
    Serial.println(Filename);
  }
}

void DTH_sensor()
{
   DHT.read();

  Serial.print(F("Humidity (%): "));
  Serial.println(DHT.getHumidity()/10);

  Serial.print(F("Temperature (°C): "));
  Serial.println(DHT.getTemperature()/10);
}

void Battery_Current_sensor()  // called at battery voltage fun
{
  sensor.readCurrent();//this must be inside loop
  sensor.printCurrent(); // it will print in serial Monitor

  Battery_Current = sensor.getCurrent();

  //does somethig when current is equal or greator than 12.3A
  if(sensor.getCurrent() >= 12.3)
  {
    // does somethig here
  }
  
}
void ESP12F_communication()
{
  if (mySerial.available())
    Serial.write(mySerial.read());
      char i = mySerial.read();
    if(i=='0'){
    mySerial.write('1');
   // digitalWrite(LED,LOW);
    }
    else {
    mySerial.write('0');
   // digitalWrite(LED,HIGH);
    }
    delay(1000);
}

void Battery_Voltage()
{
// Read the Analog Input
   adc_value = analogRead(Voltage_12V_Sensor);
   
   // Determine voltage at ADC input
   adc_voltage  = (adc_value * ref_voltage) / 1024.0; 
   
   // Calculate voltage at divider input
   in_voltage = adc_voltage / (R2/(R1+R2)) ; 
    count = count + 1;
    if(count == 1)
    {
      in_voltage1 = in_voltage; 
    }
    if(count == 2)
    {
      in_voltage2 = in_voltage; 
    }
    if(count == 3)
    {
      in_voltage3 = in_voltage; 
    }
    if(count == 4)
    {
      in_voltage4 = in_voltage; 
    }
    if(count == 5)
    {
      in_voltage5 = in_voltage; 
    }
   if(count == 5)
     {
      Battery_Volts = (in_voltage1 + in_voltage2 + in_voltage3 + in_voltage4 + in_voltage5)/5;
   // Print results to Serial Monitor to 2 decimal places
     Serial.print("Input Voltage = ");
     Serial.println(Battery_Volts, 3);
     Battery_Current_sensor();
     count = 0;
     }
  
}

void AC_POWER_Panel()
{
  float voltage = pzem.voltage();
  if (voltage != NAN) {
    AC_Volts = pzem.voltage();
    Serial.print("Voltage: ");
    Serial.print(AC_Volts);
    Serial.println("V");
  } else {
    Serial.println("Error reading voltage");
  }

  float current = pzem.current();
  if (current != NAN) {
    AC_Current = pzem.current();
    Serial.print("Current: ");
    Serial.print(AC_Current);
    Serial.println("A");
  } else {
    Serial.println("Error reading current");
  }

  float power = pzem.power();
  if (current != NAN) {
    AC_Power = pzem.power();
    Serial.print("Power: ");
    Serial.print(AC_Power);
    Serial.println("W");
  } else {
    Serial.println("Error reading power");
  }

  float energy = pzem.energy();
  if (current != NAN) {
    AC_Energy = pzem.energy();
    Serial.print("Energy: ");
    Serial.print(AC_Energy, 3);
    Serial.println("kWh");

  } else {
    Serial.println("Error reading energy");
  }

  float frequency = pzem.frequency();
  if (current != NAN) {
    AC_frequency = pzem.frequency();
    Serial.print("Frequency: ");
    Serial.print(AC_frequency, 1);
    Serial.println("Hz");
  } else {
    Serial.println("Error reading frequency");
  }

  float pf = pzem.pf();
  if (current != NAN) {
    AC_pf = pzem.pf();
    Serial.print("PF: ");
    Serial.println(AC_pf);
  } else {
    Serial.println("Error reading power factor");
  }

  Serial.println();
  
}

void loop() {

CLOCK();
if(min == 0) // every 1 hr update the log in SD card
{
  SD_card();
}

if(millis() >= time_1 + INTERVAL_MESSAGE1){
  time_1 +=INTERVAL_MESSAGE1;
  AC_POWER_Panel();
  }
   
if(millis() >= time_2 + INTERVAL_MESSAGE2){
  time_2 +=INTERVAL_MESSAGE2;
  Battery_Voltage();
  }
   
if(millis() >= time_3 + INTERVAL_MESSAGE3){
  time_3 +=INTERVAL_MESSAGE3;
  CLOCK();
  }
   
if(millis() >= time_4 + INTERVAL_MESSAGE4){
  time_4 += INTERVAL_MESSAGE4;
  ESP12F_communication();
  } 
if(millis() >= time_4 + INTERVAL_MESSAGE4){
  time_4 += INTERVAL_MESSAGE4;
  ESP12F_communication();
  }


  sensor.readCurrent();  // Battery current must in loop
//resetFunc();  //it will reset the arduino
}
