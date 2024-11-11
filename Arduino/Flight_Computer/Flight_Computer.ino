#include <SPI.h>

const int SIGNAL_THRESHOLD    = 50;        // Min threshold to trigger on
const int RESET_THRESHOLD     = 25; 

const int LED_BRIGHTNESS      = 255;         // Brightness of the LED [0,255]

//Calibration fit data for 10k,10k,249,10pf; 20nF,100k,100k, 0,0,57.6k,  1 point
const long double cal[] = {-9.085681659276021e-27, 4.6790804314609205e-23, -1.0317125207013292e-19,
  1.2741066484319192e-16, -9.684460759517656e-14, 4.6937937442284284e-11, -1.4553498837275352e-08,
   2.8216624998078298e-06, -0.000323032620672037, 0.019538631135788468, -0.3774384056850066, 12.324891083404246};
   
const int cal_max = 1023;

//initialize variables
char detector_name[40];

unsigned long time_stamp                    = 0L;
unsigned long measurement_deadtime          = 0L;
unsigned long time_measurement              = 0L;      // Time stamp
unsigned long interrupt_timer               = 0L;      // Time stamp
int           start_time                    = 0L;      // Start time reference variable
long int      total_deadtime                = 0L;      // total time between signals

unsigned long measurement_t1;
unsigned long measurement_t2;

float temperatureC;


long int      count                         = 0L;         // A tally of the number of muon counts observed
float         last_adc_value                = 0;
char          filename[]                    = "File_000.txt";
int           Mode                          = 1;

byte SLAVE;
byte MASTER;
byte keep_pulse;


void setup() {
  analogReference (EXTERNAL);
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2));    // clear prescaler bits
  //ADCSRA |= bit (ADPS1);                                   // Set prescaler to 4  
  ADCSRA |= bit (ADPS0) | bit (ADPS1); // Set prescaler to 8
  
  get_detector_name(detector_name);
  pinMode(3, OUTPUT); 
  pinMode(6, INPUT);
  
  Serial.begin(9600);
  Serial.setTimeout(3000);

  if (digitalRead(6) == HIGH){
     filename[4] = 'S';
     SLAVE = 1;
     MASTER = 0;
  }
  
  else{
     //delay(10);
     filename[4] = 'M';
     MASTER = 1;
     SLAVE = 0;
     pinMode(6, OUTPUT);
     digitalWrite(6,HIGH);
     //delay(2000);
    }
    
  if (!SD.begin(SDPIN)) {
    Serial.println(F("SD initialization failed!"));
    Serial.println(F("Is there an SD card inserted?"));
    return;
  }
  
  get_Mode();
  if (Mode == 2) read_from_SD();
  else if (Mode == 3) remove_all_SD();
  else{setup_files();}
  
  if (MASTER == 1){digitalWrite(6,LOW);}
  analogRead(A0);
  
  
  start_time = millis();
}

void write_to_SD(){ 
  while (1){
    if (analogRead(A0) > SIGNAL_THRESHOLD){     //if pulse detected

    // Make a measurement of the pulse amplitude
      int adc = analogRead(A0);
      
      // If Master, send a signal to the Slave
      if (MASTER == 1) {digitalWrite(6, HIGH);
          count++;
          keep_pulse = 1;}
      
      analogRead(A3);
      
      if (SLAVE == 1){
          if (digitalRead(6) == HIGH){
              keep_pulse = 1;
              count++;}} 
      analogRead(A3);
      
      if (MASTER == 1){
            digitalWrite(6, LOW);}

      measurement_deadtime = total_deadtime;
      time_stamp = millis() - start_time;
      measurement_t1 = micros();  
      temperatureC = (((analogRead(A3)+analogRead(A3)+analogRead(A3))/3. * (3300./1024)) - 500)/10. ;

      if (MASTER == 1) {
          digitalWrite(6, LOW); 
          analogWrite(3, LED_BRIGHTNESS);
          Serial.println((String)count + " " + time_stamp+ " " + adc+ " " + get_sipm_voltage(adc)+ " " + measurement_deadtime+ " " + temperatureC);
          myFile.println((String)count + " " + time_stamp+ " " + adc+ " " + get_sipm_voltage(adc)+ " " + measurement_deadtime+ " " + temperatureC);
          myFile.flush();
          last_adc_value = adc;}
  
      if (SLAVE == 1) {
          if (keep_pulse == 1){   
              analogWrite(3, LED_BRIGHTNESS);
              Serial.println((String)count + " " + time_stamp+ " " + adc+ " " + get_sipm_voltage(adc)+ " " + measurement_deadtime+ " " + temperatureC);
              myFile.println((String)count + " " + time_stamp+ " " + adc+ " " + get_sipm_voltage(adc)+ " " + measurement_deadtime+ " " + temperatureC);
              myFile.flush();
              last_adc_value = adc;}}
              
      keep_pulse = 0;
      digitalWrite(3, LOW);
      while(analogRead(A0) > RESET_THRESHOLD){continue;}
      
      total_deadtime += (micros() - measurement_t1) / 1000.;}
    }
}


float get_sipm_voltage(float adc_value){
  float voltage = 0;
  for (int i = 0; i < (sizeof(cal)/sizeof(float)); i++) {
    voltage += cal[i] * pow(adc_value,(sizeof(cal)/sizeof(float)-i-1));
    }
    return voltage;
    }
