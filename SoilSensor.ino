/*
  SoilSensor and Watering Valve

  05 Oct 2016 SeJ 
  16 Oct 2016 SeJ update to add timed watering

  using digital / analog soil sensor and outputing level to five digital leds
  sends the value to console
  Valve operation have somee safeguards to not over water.
  
  todo:
  Will perhaps add a level sensor for the filling water tank.

 */

const int analogInPin = A0; // Analog input pin that the potentiometer is attached to
const int soilPin = 12;     // soilsensor digital output -> 12 input
const int valveOut =  13;   // valve output

const int ledG1 = 6;  // indicator leds
const int ledG2 = 5;
const int ledG3 = 4;
const int ledYel = 3;
const int ledRed = 2;

int sensorRawValue = 0;     // value read from the pot
int sensorMapValue = 0;     // value normalised
int soilState = 0;          // variable for reading the digital soil status
int recentWater = false;    // flag for recent water
int watercount = 0;         // number of times watered during wateringcyle
int overWater = false;      // flag for overwatering within watercyle
int flash = false;          // flag for flashing error light

unsigned long prevWater = 0;        // previous watering timestamp
unsigned long waterMillis = 0;      // timestamp for watering
unsigned long flashMillis = 0;      // timestamp for led flash
const int maxcount = 6;             // max watering in wateringcycle
//const long interval = 600000;       // watering interval
//const long wateringcycle = 3600000; // wateringcylce duration
const long interval = 60000;       // watering interval
const long wateringcycle = 360000; // wateringcylce duration

void setup() {
  Serial.begin(9600);         // init serial communications at 9600 bps
  pinMode(valveOut, OUTPUT);  // init valve
  pinMode(soilPin, INPUT);    // init soil sensor output
  pinMode(ledG1, OUTPUT);     // init indicator leds
  pinMode(ledG2, OUTPUT);
  pinMode(ledG3, OUTPUT);
  pinMode(ledYel, OUTPUT);
  pinMode(ledRed, OUTPUT);
  
  watercount = 0;  // reset watercount
  overWater = false;  // reset overWatering

  SensorRead();
  SerialOutput();
  LedOutput(0);
  
}


void loop() {

  unsigned long currentMillis = millis();

  SensorRead();
  LedOutput(currentMillis);
  
  if (currentMillis - prevWater >= interval) {
    prevWater = currentMillis;
    recentWater = false;
    SerialOutput();
  }

  if (overWater == false) {
    
    if (currentMillis - waterMillis >= wateringcycle) {  
      watercount = 0;
      waterMillis = currentMillis;  
    }

    if (watercount >= maxcount) {
      overWater = true;
      Serial.println("!!!OverWatering!!!!");
    }
  
    if (sensorMapValue <20  && recentWater == false) {
      Serial.println("Watering...");
      // turn valve on as soil is dry:
      digitalWrite(valveOut, HIGH);
      delay(7000);
      digitalWrite(valveOut, LOW);
      Serial.println("Done Watering.");
      recentWater = true;
      watercount++;
      Serial.println(watercount);
      if (watercount <= 1) {
        unsigned long waterMillis = currentMillis;
      }
    } else {
      // failsafe turn off valve as soil is wet:
      digitalWrite(valveOut, LOW);
    }
    
  }


  // wait 2 milliseconds before the next loop
  // for the analog-to-digital converter to settle
  // after the last reading:
  delay(2);
}


void SensorRead() {  
  soilState = digitalRead(soilPin);
  sensorRawValue = analogRead(analogInPin);  // read analog soil value
  sensorMapValue = map(sensorRawValue, 200, 1023, 100, 0);  // normalise
}


void SerialOutput() {
  // print the results to the serial monitor:
  Serial.print("sensor = ");
  Serial.print(sensorRawValue);
  Serial.print("\t output = ");
  Serial.print(sensorMapValue);
  Serial.print("\t digital output = ");
  Serial.print(soilState);
  Serial.print("\t watercount = ");
  Serial.println(watercount);
  if (overWater == true) {
    Serial.print("!!OverWater Flag is true!!");
  }
}


void LedOutput(unsigned long currentMillis) {
  if (overWater == true) {
    if (currentMillis - flashMillis >= 1000) {
      flashMillis = currentMillis;
      if(flash == true) {
        digitalWrite(ledG1, HIGH);
        digitalWrite(ledG2, HIGH);
        digitalWrite(ledG3, HIGH);
        digitalWrite(ledYel, HIGH);
        digitalWrite(ledRed, HIGH);
        flash = false;
      } else {
        digitalWrite(ledG1, HIGH);
        digitalWrite(ledG2, HIGH);
        digitalWrite(ledG3, HIGH);
        digitalWrite(ledYel, HIGH);
        digitalWrite(ledRed, LOW);
        flash = true;
      }
    }
  } else {
    if (sensorMapValue >= 80) {
      digitalWrite(ledG1, LOW);
      digitalWrite(ledG2, LOW);
      digitalWrite(ledG3, LOW);
      digitalWrite(ledYel, LOW);
      digitalWrite(ledRed, LOW);
    }
    if (sensorMapValue >= 60 && sensorMapValue < 80) {
      digitalWrite(ledG1, HIGH);
      digitalWrite(ledG2, LOW);
      digitalWrite(ledG3, LOW);
      digitalWrite(ledYel, LOW);
      digitalWrite(ledRed, LOW);
    }
    if (sensorMapValue >= 40 && sensorMapValue < 60) {
      digitalWrite(ledG1, HIGH);
      digitalWrite(ledG2, HIGH);
      digitalWrite(ledG3, LOW);
      digitalWrite(ledYel, LOW);
      digitalWrite(ledRed, LOW);
    }
    if (sensorMapValue >= 20 && sensorMapValue < 40) {
      digitalWrite(ledG1, HIGH);
      digitalWrite(ledG2, HIGH);
      digitalWrite(ledG3, HIGH);
      digitalWrite(ledYel, LOW);
      digitalWrite(ledRed, LOW);
    }
    if (sensorMapValue < 20) {
      digitalWrite(ledG1, HIGH);
      digitalWrite(ledG2, HIGH);
      digitalWrite(ledG3, HIGH);
      digitalWrite(ledYel, HIGH);
      digitalWrite(ledRed, LOW);
    }
  }
}

