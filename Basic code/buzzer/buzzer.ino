int buzzer=A1;
void setup() {
  // put your setup code here, to run once:
pinMode(buzzer,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
tone(buzzer,2500,5);
delay(500);
tone(buzzer,0,0);
delay(500);
}
