/*
  SoilSensor and Watering Valve

  05 Oct 2016 SeJ
  16 Oct 2016 SeJ update to add timed watering
  07 Nov 2016 SeJ/CsJ update to add PI monitoring & calibrate
  08 Nov 2016 SeJ/CsJ update to change error mode to self-reset
  13 Nov 2016 SeJ hysteresis, comments, reporting improvements

  not using digital output on sensor for now
  using analog soil sensor and outputing level to five digital leds
  sends the value to console (have a receiving python on rasp-pi)
  Valve operation have somee safeguards to not over water.

  todo:
  - perhaps add a level sensor for the filling water tank.
  - sensor at base for overfilling flower pot
  - picture from pi to check out status
 */

const int analogInPin = A0; // Analog input pin that the potentiometer is attached to
const int soilPin = 12;     // soilsensor digital output -> 12 input
const int valveOut =  13;   // valve output
const int manButton = 9;    // manual water button

const int ledG1 = 6;  // indicator leds
const int ledG2 = 5;
const int ledG3 = 4;
const int ledYel = 3;
const int ledRed = 2;

int sensorRawValue = 0;     // value read from the pot
int sensorMapValue = 0;     // value normalised
int sensorMapValue_old = 0; // output everytime mapvalue changes
int sensorMapValue_old2 = 0;// reduce flip-flop in datafile
int soilState = 0;          // variable for reading the digital soil status
int recentWater = false;    // flag for recent water
int watercount = 0;         // number of times watered during wateringcyle
int overWater = false;      // flag for overwatering within watercyle
int flash = false;          // flag for flashing error light
int hysteresis = false;     // flag for hysteresis watering

int manSet = false;        // manual button set
int manReset = false;      // manual button reset
int manWater = false;      // manual water flag

unsigned long currentMillis = 0;    // current timestamp
unsigned long prevWater = 0;        // previous watering timestamp
unsigned long waterMillis = 0;      // timestamp for watering
unsigned long flashMillis = 0;      // timestamp for led flash
unsigned long errorMillis = 0;      // timestamp to reset overwater condition
unsigned long outputMillis = 0;     // timestamp to throttle data output
unsigned long manMillis = 0;        // timestamp to delay manual watering
unsigned long overDeltaMillis = 0;  // delta until overwater is done

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
void LedOutput();
void HysteresisCheck();

void setup() {
  Serial.begin(115200);         // init serial communications at 9600 bps
  pinMode(valveOut, OUTPUT);  // init valve
  pinMode(soilPin, INPUT);    // init soil sensor output
  pinMode(ledG1, OUTPUT);     // init indicator leds
  pinMode(ledG2, OUTPUT);
  pinMode(ledG3, OUTPUT);
  pinMode(ledYel, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(manButton, INPUT_PULLUP);  // init manual watering button

  watercount = 0;       // reset watercount
  overWater = false;    // reset overWatering
  recentWater = false;  // flag for recent water
  hysteresis = false;   // flag for hysteresis watering
  manSet = false;        // manual button set
  manReset = false;      // manual button reset
  manWater = false;      // manual water flag

  prevWater = 0;        // previous watering timestamp
  waterMillis = 0;      // timestamp for watering
  errorMillis = 0;      // timestamp to reset overwater condition
  outputMillis = 0;     // timestamp to throttle data output
  manMillis = 0;        // timestamp to delay manual watering

  delay(10);
  Serial.println("...");
  Serial.println("Resetting System...");
  Serial.println("...");
  delay(10);

  SensorRead();
  SerialOutput();
  LedOutput();

}


void loop() {

  // set now
  currentMillis = millis();

  // read soilSensor update leds & update hysteresis
  SensorRead();
  LedOutput();
  HysteresisCheck();

  // update data output every so often but not too often
  if ((sensorMapValue != sensorMapValue_old) && (sensorMapValue != sensorMapValue_old2) && (currentMillis - outputMillis >= heartbeat)) {
    outputMillis = currentMillis;
    sensorMapValue_old2 = sensorMapValue_old;
    sensorMapValue_old = sensorMapValue;
    SerialOutput();
  }

  // set up timing for waterings based on interval
  if (currentMillis - prevWater >= interval) {
    recentWater = false;
  }

  // manual watering LOW=pushed make sure only once per push & only watertime + heartbeat so often
  if (digitalRead(manButton) == LOW) {
    if (manSet == false && manReset == false) manSet = true;
    if (currentMillis - manMillis >= (watertime + heartbeat)) manReset = false;
  } else {
    if (manSet == true && manReset == false) {
    manSet = false;
    manReset = true;
    manWater = true;
    manMillis = currentMillis;
    }
  }

  // main watering only if not overwater or if manual
  if (overWater == false || manWater == true) {
    // reset watercount after wateringcycle elapsed
    if (currentMillis - waterMillis >= wateringcycle) {
      watercount = 0;
      waterMillis = currentMillis;
    }

    // ready to water if all is ok
    if (manWater == true || (hysteresis == true && recentWater == false)) {
      // turn on valve for watering time and set recent flag
      Serial.print("Watering...");
      digitalWrite(valveOut, HIGH);
      delay(watertime);
      digitalWrite(valveOut, LOW);
      recentWater = true;
      prevWater = currentMillis;

      // if manual reset some flages if auto increase the count
      if (manWater == true) {
	manWater = false;
	hysteresis = false;
	overWater = false;
        Serial.println("Done Manual Watering.");
      } else {
	watercount++;
        Serial.println("Done Auto Watering.");
      }

      // set timing of first watering in cycle or set overwatering or pass
      if (watercount <= 1) {
        waterMillis = currentMillis;
      } else if (watercount >= maxcount) {
	overWater = true;
	hysteresis = false;
	errorMillis = currentMillis;
      }

      // update the data stream
      SerialOutput();

    } else {
      // failsafe turn off valve if we are not turning it on
      digitalWrite(valveOut, LOW);
    }

  } else {
    // reset overwater after watering cycle has timed out
    if (currentMillis - errorMillis >= wateringcycle) {
      overWater = false;
      }
  }

  // wait 4 mills for the analog-to-digital converter to settle after the last reading:
  delay(4);
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
  // time to reset overWater or hysteresis on or ok
  if (overWater == true) {
    overDeltaMillis = ((wateringcycle - (currentMillis - errorMillis)) / 60000);
    Serial.print("  xWat(OVER=");
    Serial.print(overDeltaMillis);
    Serial.print("m): ");
  } else if (hysteresis == true) {
    Serial.print("  xWat(hysteres): ");
  } else {
    Serial.print("  xWat(...ok...): ");
  }
  // finally print the watercount
  Serial.println(watercount);
}


void HysteresisCheck() {
  // setting of hysteresis when moisture is below hysLow, reset above hysHigh
  if ((sensorMapValue < hysLow) && (hysteresis == false)) {
    hysteresis = true;
  }
  else if ((sensorMapValue > hysHigh) && (hysteresis ==true)) {
    hysteresis = false;
  }
}


void LedOutput() {
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

