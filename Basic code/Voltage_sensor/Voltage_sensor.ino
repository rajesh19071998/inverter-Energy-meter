// Define analog input
#define Voltage_12V_Sensor A0
 
// Floats for ADC voltage & Input voltage
float adc_voltage = 0.0;
float in_voltage = 0.0, in_voltage1 = 0.0, in_voltage2 = 0.0, in_voltage3 = 0.0, in_voltage4 = 0.0, in_voltage5 = 0.0, in_voltageF = 0.0;
int count = 0; 
// Floats for resistor values in divider (in ohms)
float R1 = 30000.0;
float R2 = 7500.0; 
 
// Float for Reference Voltage
float ref_voltage = 5.0;
 
// Integer for ADC value
int adc_value = 0;
 
void setup()
{
   // Setup Serial Monitor
   Serial.begin(9600);
   Serial.println("DC Voltage Test");
}
 
void loop(){
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
      in_voltageF = (in_voltage1 + in_voltage2 + in_voltage3 + in_voltage4 + in_voltage5)/5;
   // Print results to Serial Monitor to 2 decimal places
     Serial.print("Input Voltage = ");
     Serial.println(in_voltageF, 3);
     count = 0;
     }
 
  // Short delay
  delay(500);
}
