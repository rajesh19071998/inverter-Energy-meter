#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX
#define LED 13
void setup()  
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  Serial.println("Goodnight moon!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  mySerial.println("Hello, USHA");
  mySerial.print('0');
}

void loop() // run over and over
{
  if (mySerial.available())
    Serial.write(mySerial.read());
      char i = mySerial.read();
    if(i=='0'){
    mySerial.write('1');
    digitalWrite(LED,LOW);
    }
    else {
    mySerial.write('0');
    digitalWrite(LED,HIGH);
    }
    delay(1000);
  
}
