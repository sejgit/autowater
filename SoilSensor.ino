/*
  SoilSensor and Watering Valve

  05 Oct 2016 SeJ
  16 Oct 2016 SeJ update to add timed watering
  07 Nov 2016 SeJ/CsJ update to add PI monitoring & calibrate
  08 Nov 2016 SeJ/CsJ update to change error mode to self-reset

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
int sensorMapValue_old = 0; // output everytime mapvalue changes
int soilState = 0;          // variable for reading the digital soil status
int recentWater = false;    // flag for recent water
int watercount = 0;         // number of times watered during wateringcyle
int overWater = false;      // flag for overwatering within watercyle
int flash = false;          // flag for flashing error light
int hysteresis = false;     // flag for hysteresis watering


unsigned long prevWater = 0;        // previous watering timestamp
unsigned long waterMillis = 0;      // timestamp for watering
unsigned long flashMillis = 0;      // timestamp for led flash
unsigned long errorMillis = 0;      // timestamp to reset overwater condition
unsigned long outputMillis = 0;     // timestamp to throttle data output

const int hysLow = 40;         // hysteresis low
const int hysHigh = 60;       // hysteresis high
const int maxcount = 5;       // max watering in wateringcycle
//                          m *  s * mil
const int heartbeat      = 1000;    // blink timing & delay to not flood data output
const int watertime      =  7000;   //  7 * 1000 =  7s watering time
const long interval      =  120000; // 2 * 60 * 1000 = 2min watering interval
const long wateringcycle = 3600000; //60 * 60 * 1000 = 1hr wateringcylce duration

void SensorRead();
void SerialOutput();
void LedOutput(unsigned long currentMillis);

void setup() {
  Serial.begin(115200);         // init serial communications at 9600 bps
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

  if ((sensorMapValue != sensorMapValue_old) && (currentMillis - outputMillis >= heartbeat)) {
    outputMillis = currentMillis;
    sensorMapValue_old = sensorMapValue;
    SerialOutput();
  }

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
      errorMillis = currentMillis;
      Serial.println("!!!OverWatering!!!!");
    }

    if ((sensorMapValue < hysLow || hysteresis == true)  && recentWater == false) {
      Serial.print("Watering...");
      if (sensorMapValue < hysHigh)
        hysteresis = true;
      else
        hysteresis = false;
      Serial.print("hysteresis = ");
      Serial.println(hysteresis);
      // turn valve on as soil is dry:
      digitalWrite(valveOut, HIGH);
      delay(watertime);
      digitalWrite(valveOut, LOW);
      Serial.println("Done Watering.");
      recentWater = true;
      watercount++;
      Serial.println(watercount);
      if (watercount <= 1) {
        waterMillis = currentMillis;
      }
    } else {
      // failsafe turn off valve as soil is wet:
      digitalWrite(valveOut, LOW);
    }

  } else {
    if (currentMillis - errorMillis >= wateringcycle) {
      overWater = false;
      /*
       * PIR sensor tester
       */

      int ledPin = 13;                // choose the pin for the LED
      int inputPin = 2;               // choose the input pin (for PIR sensor)
      int pirState = LOW;             // we start, assuming no motion detected
      int val = 0;                    // variable for reading the pin status

      void setup() {
        pinMode(ledPin, OUTPUT);      // declare LED as output
        pinMode(inputPin, INPUT);     // declare sensor as input

        Serial.begin(9600);
      }

      void loop(){
        val = digitalRead(inputPin);  // read input value
        if (val == HIGH) {            // check if the input is HIGH
	  digitalWrite(ledPin, HIGH);  // turn LED ON
	  if (pirState == LOW) {
	    // we have just turned on
	    Serial.println("Motion detected!");
	    // We only want to print on the output change, not state
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
  sensorMapValue = map(sensorRawValue, 0, 1023, 100, 0);  // normalise
}


void SerialOutput() {
  // print the results to the serial monitor:
  Serial.print("raw: ");
  Serial.print(sensorRawValue);
  Serial.print("  map: ");
  Serial.print(sensorMapValue);
  Serial.print("  dig:");
  Serial.print(soilState);
  if (overWater == true) {
    Serial.print("  xWatered!!: ");
  } else {
    Serial.print("  xWatered  : ");
  }
  Serial.println(watercount);
}


void LedOutput(unsigned long currentMillis) {
  if (overWater == true) {
    if (currentMillis - flashMillis >= heartbeat) {
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

