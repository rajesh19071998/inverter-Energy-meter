int LED1 = 2;      // Assign LED1 to pin GPIO2
int Time = 1000;

void setup() {

  // initialize GPIO2 and GPIO16 as an output

  pinMode(LED1, OUTPUT);

}

// the loop function runs forever

void loop() {

  digitalWrite(LED1, LOW);     // turn the LED off

  delay(Time);                // wait for a second

  digitalWrite(LED1, HIGH);  // turn the LED on

  delay(Time);               // wait for a second

}
