

/*
 * This is a custom firmware for the Polaroid LED Level
 * 
 * Features:
 * Better Acuracy (interpolates between leds)
 * No more flickering lights (measurement is filtered)
 * Two step calibration
 * Works in any direction not only horizontal (works vertical as well)
 * Three sensitivity settings
 * 
 * Calibration Procedure:
 * 1. Place the Camera flat on the table
 * 2. Press the Brightness Button and wait for the green LED
 * 3. Place the Camera floor facing (tripod or on lens)
 * 4. Press the Brightness Button again and wait for the green LED
 * 5. Done!
 * 
 * Flashing Procedure:
 * Pins on the bottom of the board are:
 * 1: SCK
 * 2: MISO
 * 3: MOSI
 * 4: Reset
 * 5: N/A?
 * 6: GND
 * 7: VCC
 * 
 * The board can be powered with 5v
 * 
 * ATTENTION: YOUR WARRANTY WILL BE LOST IF YOU WANT TO FLASH THE FIRMWARE! THERE CAN BE DAMAGES TO YOUR DEVICE! USE ON YOUR OWN RISK!!!
 */

#include <EEPROM.h>

#include <math.h> 

#define RAD2DEC 57.11399191265875

// Digital Pins for Rows
#define HORIZONTAL 8
#define VERTICAL 9

// LED Count per row
#define LEDCOUNT 7

// Analog Pins for Accelerometer
#define AX A2
#define AY A1
#define AZ A0

#define BTN1 18
#define BTN2 19

#define LIGHTSTEPS 10

// Current Led values. Range [-10, 10]
float ledH, ledV;

byte hVals[LEDCOUNT] = {0},
     vVals[LEDCOUNT] = {0};

byte currentLedCycle;

bool lastBtn1State, lastBtn2State;

double degH, degV, degZ;
double calibH, calibV, calibZ, calibV2;

double sensitivity;

// Calib State: 0 = off, 1 = waiting for button press
int calibState;

int displayMode;

void saveCalib() {
  EEPROM.put(0,calibH);
  EEPROM.put(sizeof(double),calibV);
  EEPROM.put(2*sizeof(double),calibZ);
  EEPROM.put(3*sizeof(double),calibV2);
}

void readCalib() {
  EEPROM.get(0,calibH);
  EEPROM.get(sizeof(double),calibV);
  EEPROM.get(2*sizeof(double),calibZ);
  EEPROM.get(3*sizeof(double),calibV2);
}

void saveSensitivity() {
  if(sensitivity != 4 && sensitivity != 2 && sensitivity != 1) {
    sensitivity = 1;
  }
  EEPROM.put(4*sizeof(double), sensitivity);
}

void readSensitivity() {
  EEPROM.get(4*sizeof(double), sensitivity);
}

// the setup function runs once when you press reset or power the board
void setup() {
  // Pins 0 to 6 are the LEDs
  for(byte i = 0; i <= 6; i++)
      pinMode(i, OUTPUT);

  // PD7 needs to be input, because AY collides here.
  pinMode(7, INPUT);

  // VERTICAL and HORIZONTAL selectors need to be output aswell
  pinMode(VERTICAL, OUTPUT);
  pinMode(HORIZONTAL, OUTPUT);

  // Pullups for Buttons
  pinMode(BTN1, INPUT);
  digitalWrite(BTN1, HIGH);
  pinMode(BTN2, INPUT);
  digitalWrite(BTN2, HIGH);
  
  // All LEDs off
  currentLedCycle = 0;
  ledH = 0;
  ledV = 0;
  digitalWrite(HORIZONTAL, HIGH);
  digitalWrite(VERTICAL, HIGH);

  for(byte i = 0; i < LEDCOUNT; i++)
    digitalWrite(i, HIGH);

  // Buttons not pressed
  lastBtn2State = lastBtn1State = true;  

  // Read calibration data
  readCalib();

  // We are currently not calibrating
  calibState = 0;

  // Reset the angles
  degH = 0;
  degV = 0;
  degZ = 0;

  // We are in normal display mode
  displayMode = 0;

  // Read the sensitivity settings
  readSensitivity();

  showSensitivtity();
}

// the loop function runs over and over again forever

void loop() {
  if(handleButtons())
    return;
  digitalWrite(HORIZONTAL, HIGH);
  digitalWrite(VERTICAL, HIGH);
  // Initialize hVals and vVals
  if(currentLedCycle == 0) {
    if(displayMode == 0) {
      // Stretched values map from [-10, 10] to [0, 6]
      float hStretched = constrain((6.0f * (ledH + 10.0f)) / 20.0f, 0.0f, 6.0f);
      float vStretched = constrain((6.0f * (ledV + 10.0f)) / 20.0f, 0.0f, 6.0f);
  
      for(int i = 0; i < LEDCOUNT; i++) {
        float hdiff = abs(hStretched - i);
        if(hdiff < 1.0f) {
          hVals[i] = 10 - hdiff*10;
        } else {
          hVals[i] = 0;
        }
        float vdiff = abs(vStretched - i);
        if(vdiff < 1.0f) {
          vVals[i] = 10 - vdiff*10;
        } else {
          vVals[i] = 0;
        }
  
        hVals[3] = max(hVals[3], vVals[3]);
      }
    } else {
      // Stretched values map from [-10, 10] to [0, 6]
      float hStretched = constrain((6.0f * (ledH + 10.0f)) / 20.0f, 0.0f, 6.0f);
      float vStretched = constrain((6.0f * (ledV + 10.0f)) / 20.0f, 0.0f, 6.0f);
  
      for(int i = 0; i < LEDCOUNT; i++) {
        if(round(hStretched) == i) {
          hVals[i] = 255;  
        } else {
          hVals[i] = 0;  
        }
        if(round(vStretched) == i && hVals[i] == 0) {
          hVals[i] = 2;
        }
        
        hVals[3] = max(hVals[3], vVals[3]);
      }
    }
  }

  if(currentLedCycle < LIGHTSTEPS) {
    if(currentLedCycle%2 == 0 || displayMode == 1) {
      for(int i = 0; i < LEDCOUNT; i++) {
        if(hVals[i] > 0) {
          digitalWrite(i, LOW);
          hVals[i]--;
        } else {
          digitalWrite(i, HIGH);
        }
      }
      digitalWrite(HORIZONTAL, LOW);
    } else {
      for(int i = 0; i < LEDCOUNT; i++) {
        if(vVals[i] > 0) {
          digitalWrite(i, LOW);
          vVals[i]--;
        } else {
          digitalWrite(i, HIGH);
        }
      }
      digitalWrite(VERTICAL, LOW);
    }
    delayMicroseconds(100);
  } else if(currentLedCycle == LIGHTSTEPS) {
    if(displayMode == 0) {
      for(int i = 0; i < LEDCOUNT; i++) {
          digitalWrite(i, HIGH);
      }
    }
    measure();
  }
  // %LIGHTSTEPS*2, because we want 2*LIGHTSTEPS steps to run before looping
  currentLedCycle = (currentLedCycle+1) % (LIGHTSTEPS*2);

}

void measure() {
  

  
  float vx = analogRead(AX) - 512.0f;
  float vy = analogRead(AY) - 512.0f;
  float vz = analogRead(AZ) - 512.0f;
  
  degH = .9*degH + .1* atan2(vy, vz) * RAD2DEC;
  degV = .9*degV + .1* atan2(vx, vz) * RAD2DEC;
  degZ = .9*degZ + .1* atan2(vx, vy) * RAD2DEC;

  float realDegV = degV + 45;

  if(abs(degV) > 45) {
    realDegV -= calibV2;
  } else {
    realDegV -= calibV;
  }
    
  realDegV -= floor(realDegV / 90.0f)*90.0f;
  ledV = constrain((realDegV - 45.0f)/sensitivity,-10.0f,10.0f);

  float realDegH = 0;
  if(abs(degV) > 45) {
    displayMode = 1;
    // We are tilted to the front or back
    realDegH = degZ + 45 - calibZ;
  } else {
    displayMode = 0;
    realDegH = degH + 45 - calibH;
  }
  
  realDegH -= floor(realDegH / 90.0f)*90.0f;
  ledH = constrain((realDegH - 45.0f)/sensitivity,-10.0f,10.0f);
}

bool handleButtons() {
  bool currentBtn1State = digitalRead(BTN1);
  bool currentBtn2State = digitalRead(BTN2);

  if(currentBtn2State && !lastBtn2State) {
    // Button1 pressed
    sensitivity *= 2;
    if(sensitivity >= 8) {
      sensitivity = 1;
    }

    saveSensitivity();
    
    showSensitivtity();


    lastBtn2State = currentBtn2State;
    return true;
  }

  if(currentBtn1State && !lastBtn1State) {
    if(calibState == 0) {
      showStartCalib(true);
      delay(1000);
      showCalibBusy(true);
      delay(100);
      float vx = analogRead(AX) - 512.0f;
      float vy = analogRead(AY) - 512.0f;
      float vz = analogRead(AZ) - 512.0f;
      
      degH = atan2(vy, vz) * RAD2DEC;
      degV = atan2(vx, vz) * RAD2DEC;
      
      for(int i = 0; i < 100; i++){
        vx = analogRead(AX) - 512.0f;
        vy = analogRead(AY) - 512.0f;
        vz = analogRead(AZ) - 512.0f;
        
        degH = .8*degH + .2* atan2(vy, vz) * RAD2DEC;
        degV = .8*degV + .2* atan2(vx, vz) * RAD2DEC;

        delay(10);
      }
  
      calibH = degH;
      calibV = degV;

      showCalibBusy(false);
      delay(500);
      calibState = 1;
      
      lastBtn1State = currentBtn1State;
      return true;
    } else {
      showCalibBusy(true);
      delay(100);
      float vx = analogRead(AX) - 512.0f;
      float vy = analogRead(AY) - 512.0f;
      float vz = analogRead(AZ) - 512.0f;
      
      degZ = atan2(vx, vy) * RAD2DEC;
      degV = atan2(vx, vz) * RAD2DEC;
      
      for(int i = 0; i < 100; i++){
        vx = analogRead(AX) - 512.0f;
        vy = analogRead(AY) - 512.0f;
        vz = analogRead(AZ) - 512.0f;
        
        degZ = .8*degZ + .2 * atan2(vx, vy) * RAD2DEC;
        degV = .8*degV + .2 * atan2(vx, vz) * RAD2DEC;
        delay(10);
      }
  
      calibZ = degZ;
      calibV2 = degV;

      saveCalib();
      
      showCalibBusy(false);
      delay(1000);
      calibState = 0;
      lastBtn1State = currentBtn1State;
      return true;
    }
  }

  lastBtn1State = currentBtn1State;
  lastBtn2State = currentBtn2State;

  // Prevent Loop from Running
  if(calibState == 1) {
    showStartCalib(false);
    return true;
  }

  return false;
}

void showCalibBusy(boolean isBusy) {
  digitalWrite(HORIZONTAL, LOW);
  digitalWrite(VERTICAL, LOW);

  if(isBusy) {
    digitalWrite(0, LOW);
    digitalWrite(6, LOW);
    digitalWrite(0, LOW);
    digitalWrite(6, LOW);
  } else {
    digitalWrite(3, LOW);
  }
}

void showStartCalib(bool horizontal) {
  if(horizontal) {
    digitalWrite(HORIZONTAL, LOW);
    digitalWrite(VERTICAL, LOW);
  } else {
    digitalWrite(HORIZONTAL, HIGH);
    digitalWrite(VERTICAL, LOW);
  }

  for(int j = 0; j <= 3; j++) {
    for(int i = 0; i <= 6; i++) {
      if(3-j == i || 3+j == i) {
        digitalWrite(i, LOW);
      } else {
        digitalWrite(i, HIGH);
      }
    }
    if(horizontal) {
      delay(100);
    } else {
      delay(50);
    }
  }
}

void showSensitivtity() {
  digitalWrite(HORIZONTAL, LOW);
  digitalWrite(VERTICAL, HIGH);

  for(int i = 0; i < 7; i++)
    digitalWrite(i, HIGH);

  if(sensitivity == 1) {
    digitalWrite(0, LOW);
    digitalWrite(6, LOW);
  } else if(sensitivity == 2) {
    digitalWrite(1, LOW);
    digitalWrite(5, LOW);
  } else if(sensitivity == 4) {
    digitalWrite(2, LOW);
    digitalWrite(4, LOW);
  }

  delay(150);
  for(int i = 0; i < 7; i++)
    digitalWrite(i, HIGH);
}
