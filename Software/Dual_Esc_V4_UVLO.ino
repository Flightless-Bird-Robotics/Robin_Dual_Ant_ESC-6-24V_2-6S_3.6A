
#include <EEPROM.h>

//Input pins
const int PIN_VIN = 8;

const int PIN_RC_THROTTLE = 14;
const int PIN_RC_STEERING = 15;
const int PIN_RC_AUX      = 11;

//Output pins
const int PIN_LED = 9;

const int PIN_MOTOR_L_A = 0;
const int PIN_MOTOR_L_B = 1;
const int PIN_MOTOR_R_A = 7;
const int PIN_MOTOR_R_B = 16;

const int MAX_PWM = 255;
const int SOUND_PWM_POWER = 60;

const float MIXING_LEVELS[] = {0.0, 0.25, 0.50, 0.75, 1.0};
float currentMix = 0.0;
int mixingIndex = 0;
const int EEPROM_ADDR_MIX = 0;
const int EEPROM_ADDR_BRAKE_EN = 1;
bool brakingEN = false;
const int EEPROM_ADDR_FLIP_CHANNEL_A_B = 2;
bool flipChannel = false;
const int EEPROM_ADDR_INVERT_A = 3;
bool invertA = false;
const int EEPROM_ADDR_INVERT_B = 4;
bool invertB = false;
const int EEPROM_ADDR_UVLO = 5;
bool UVLO = false;
const int EEPROM_ADDR_DEADZONE = 6;
int DEADZONE = 10;

bool inDrive = false;
bool lastAuxStateHigh = false;
bool auxStateChange = false;
bool setupMode = false;
int counter1 = 0;
int counter2 = 0;
int counter3 = 0;
int UVLOcells = 0;
int counterUVLO = 0;
int speedL = 0;
int speedR = 0;

// Variables for interrupts
volatile uint16_t tcb_start_steering = 0;
volatile uint16_t rc_steering_ticks = 0;
volatile uint16_t tcb_start_throttle = 0;
volatile uint16_t rc_throttle_ticks = 0;
volatile uint16_t tcb_start_aux = 0;
volatile uint16_t rc_aux_ticks = 0;
volatile uint16_t throttle_timer = 0;
volatile uint16_t steering_timer = 0;
volatile uint16_t aux_timer = 0;

void setup() {//start setup
  //enable WDT timers
  _PROTECTED_WRITE(WDT.CTRLA,WDT_PERIOD_1KCLK_gc);

//Initialize pins
  pinMode(PIN_MOTOR_L_A, OUTPUT); pinMode(PIN_MOTOR_L_B, OUTPUT);
  pinMode(PIN_MOTOR_R_A, OUTPUT); pinMode(PIN_MOTOR_R_B, OUTPUT);
  pinMode(PIN_RC_STEERING, INPUT); pinMode(PIN_RC_THROTTLE, INPUT); pinMode(PIN_RC_AUX, INPUT);
  pinMode(PIN_LED, OUTPUT);pinMode(PIN_VIN,INPUT);

//Stop motors 
  stopMotors();

//Flash LED
  digitalWrite(PIN_LED, HIGH);                                                                                      

//Reading eeprom settings
  mixingIndex = EEPROM.read(EEPROM_ADDR_MIX);
  if (mixingIndex > 4){mixingIndex = 0;}
  currentMix = MIXING_LEVELS[mixingIndex];
  brakingEN = EEPROM.read(EEPROM_ADDR_BRAKE_EN);
  if(EEPROM.read(EEPROM_ADDR_BRAKE_EN) > 4){brakingEN = 0;}
  flipChannel = EEPROM.read(EEPROM_ADDR_FLIP_CHANNEL_A_B);
  if(EEPROM.read(EEPROM_ADDR_FLIP_CHANNEL_A_B) > 4){flipChannel = 0;}
  invertA = EEPROM.read(EEPROM_ADDR_INVERT_A);
  if (EEPROM.read(EEPROM_ADDR_INVERT_A) > 4){invertA = 0;}
  invertB = EEPROM.read(EEPROM_ADDR_INVERT_B);
  if (EEPROM.read(EEPROM_ADDR_INVERT_B) > 4){invertB = 0;}
  UVLO = EEPROM.read(EEPROM_ADDR_UVLO);   
  if (EEPROM.read(EEPROM_ADDR_UVLO) > 4){UVLO = 0;}
  if (EEPROM.read(EEPROM_ADDR_DEADZONE == 0)){
      DEADZONE = 40;
  }
  else {DEADZONE = 10;}

//Set the ADC voltage reference to 2.5V
  analogReference(2);
  delay(100);
//Set the ADC to 200kHz
  analogClockSpeed(200);
//Reading startup voltages for UVLO with 8 bit resolution and setting the detected number of cells/turning UVLO off
  if(analogReadEnh(PIN_VIN,8) >=  74  && analogReadEnh(PIN_VIN,8) <= 80){//8-8.6V                                        
    UVLOcells = 2;
  }
  else if(analogReadEnh(PIN_VIN,8) >= 111 && analogReadEnh(PIN_VIN,8) <= 120){//12-12.9V
    UVLOcells = 3;
  }
  else if (analogReadEnh(PIN_VIN,8) >= 148 && analogReadEnh(PIN_VIN,8) <= 160){//16-17.2V
    UVLOcells = 4;
  }
  else if (analogReadEnh(PIN_VIN,8) >= 185 && analogReadEnh(PIN_VIN,8) <= 200){//20-21.5V
    UVLOcells = 5;
  }
  else if (analogReadEnh(PIN_VIN,8) >= 222 && analogReadEnh(PIN_VIN,8) <= 240){//24-25.9V
    UVLOcells = 6;
  }
  else{
    UVLO = 0;
  }

       
  //Indicating the UVLO level   
  if (UVLO){                                                                  
    indicateMode(UVLOcells + 1);
  }
  else{
    indicateMode(1);
  }

//Setting registers for Interrrupts
  TCB0.CTRLB = 0;
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;

//Interrupts fot Input reading
  attachInterrupt(digitalPinToInterrupt(PIN_RC_STEERING), isrSteering, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_RC_THROTTLE), isrThrottle, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_RC_AUX), isrAux, CHANGE);


//Flash LED END
  digitalWrite(PIN_LED, LOW);
}//End setup

//Start main loop                                                                 LOOP
void loop() {
    //resetting WDT
  __asm__ __volatile__ ("wdr"::);
  
  uint16_t rawA, rawB, rawAx;


//Disabling interrupts                                                          reading inputs
  noInterrupts();
//Reading the pwm signal values
  rawB = rc_steering_ticks;
  rawA = rc_throttle_ticks;
  rawAx = rc_aux_ticks;
//Enabling interrupts
  interrupts();
//Resizing the input signals to 255
  int inA = ticksToPWM(rawA);
  int inB = ticksToPWM(rawB);
  int AUX = ticksToPWM(rawAx);
  bool auxIsHigh = (AUX > 200);

  //preventing aloss of signal to give any input and setting the inputs to zero
  steering_timer++;
  throttle_timer++;
  aux_timer++;

  //counting up if the UVLO threshold is reached 
  if(UVLOcells != 0 && UVLO) {
    if (analogReadEnh(PIN_VIN,8) <= (UVLOcells * 28)) {
        if (counterUVLO < 12000) {
          counterUVLO++;
          }
    }
    else if(analogReadEnh(PIN_VIN,8) >= (UVLOcells * 29)){
        counterUVLO = 0;
    }
  }
  //Setting motor outputs to 0 if signal lost or UVLO
  if(steering_timer >= 12000 || counterUVLO >= 12000 ){ inB = 0;rc_steering_ticks = 0;}
  if(throttle_timer >= 12000 || counterUVLO >= 12000){ inA = 0;rc_throttle_ticks = 0;}
  if(aux_timer >= 12000){ AUX = 0;rc_aux_ticks = 0;}


  if (!inDrive){                                                              // Settings
      if(auxIsHigh != lastAuxStateHigh){
      auxStateChange = true;
      }
      else{
        auxStateChange = false;
      }
    lastAuxStateHigh = auxIsHigh;
    //Leaving loop/entering driving mode
    if(inA > DEADZONE || inB > DEADZONE || inA < -DEADZONE || inB < -DEADZONE){
      inDrive = true;
    }
    counter1++;
    delay(10);
//enter setup mode
    if (counter1 > 300){
      counter1 = 400;
      if (!setupMode){
      digitalWrite(PIN_LED, true);
      setupMode = true;
      delay(200);
      digitalWrite(PIN_LED, false);
      }
    if (auxIsHigh && auxStateChange){
      counter2++;
      counter3 = 0;
      digitalWrite(PIN_LED,true);
      delay(50);
      digitalWrite(PIN_LED,false);
    }
    if(counter2 > 0){
      counter3++;
      if(counter3 > 300){
        if (counter2 == 1){
          counter2 = 0;
          counter3 = 0;
          digitalWrite(PIN_LED,true);delay(400);digitalWrite(PIN_LED,false);
        }
        else if(counter2 == 2){
          counter2 = 0;
                counter3 = 0;
            currentMix = 0.00;
            EEPROM.update(EEPROM_ADDR_MIX, 0);
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
        }
          else if (counter2 == 3){
            counter2 = 0;
                  counter3 = 0;
            currentMix = 0.25;
            EEPROM.update(EEPROM_ADDR_MIX, 1);
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 4){
            counter2 = 0;
            counter3 = 0;
            currentMix = 0.50;
            EEPROM.update(EEPROM_ADDR_MIX, 2);
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 5){
            counter2 = 0;
            counter3 = 0;
            currentMix = 0.75;
            EEPROM.update(EEPROM_ADDR_MIX, 3);
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 6){
            counter2 = 0;
            counter3 = 0;
            currentMix = 1.00;
            EEPROM.update(EEPROM_ADDR_MIX, 4);
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 7){
            counter2 = 0;
            counter3 = 0;
            if (brakingEN){
              brakingEN = false;
              EEPROM.update(EEPROM_ADDR_BRAKE_EN, 0);
            }
            else {
              brakingEN = true;
              EEPROM.update(EEPROM_ADDR_BRAKE_EN, 1);
            }
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 8){
            counter2 = 0;
            counter3 = 0;
            if (invertA){
              invertA = false;
              EEPROM.update(EEPROM_ADDR_INVERT_A, 0);
            } 
            else{
             invertA = true;
              EEPROM.update(EEPROM_ADDR_INVERT_A, 1);
            }
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 9){
            counter2 = 0;
            counter3 = 0;
            if (invertB){
              invertB = false;
              EEPROM.update(EEPROM_ADDR_INVERT_B, 0);
            } 
            else{
             invertB = true;
              EEPROM.update(EEPROM_ADDR_INVERT_B, 1);
            }
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 10){
            counter2 = 0;
            counter3 = 0;
            if (flipChannel){
              flipChannel = false;
              EEPROM.update(EEPROM_ADDR_FLIP_CHANNEL_A_B, 0);
            } 
            else{
             flipChannel = true;
              EEPROM.update(EEPROM_ADDR_FLIP_CHANNEL_A_B, 1);
            }
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 11){
            counter2 = 0;
            counter3 = 0;
            if (UVLO){
              UVLO = false;
              EEPROM.update(EEPROM_ADDR_UVLO, 0);
            } 
            else{
             UVLO = true;
              EEPROM.update(EEPROM_ADDR_UVLO, 1);
            }
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 12){
            counter2 = 0;
            counter3 = 0;
            if (DEADZONE == 10){
              DEADZONE = 40;
              EEPROM.update(EEPROM_ADDR_DEADZONE, 1);
            } 
            else{
             DEADZONE = 10;
              EEPROM.update(EEPROM_ADDR_DEADZONE, 0);
            }
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          }
          else if (counter2 == 13){
            counter2 = 0;
            counter3 = 0;
            currentMix = 0.00;
            EEPROM.update(EEPROM_ADDR_MIX, 0);
            brakingEN = false;
            EEPROM.update(EEPROM_ADDR_BRAKE_EN, 0);
            invertA = false;
            EEPROM.update(EEPROM_ADDR_INVERT_A, 0);
            invertB = false;
            EEPROM.update(EEPROM_ADDR_INVERT_B, 0);
            flipChannel = false;
            EEPROM.update(EEPROM_ADDR_FLIP_CHANNEL_A_B, 0);
            UVLO = false;
            EEPROM.update(EEPROM_ADDR_UVLO, 0);
            DEADZONE = 10;
            EEPROM.update(EEPROM_ADDR_DEADZONE, 0);
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
            digitalWrite(PIN_LED,true);delay(200);digitalWrite(PIN_LED,false);
          } 
          else{
            counter2 = 0;
            counter3 = 0;
            digitalWrite(PIN_LED,true);delay(2000);digitalWrite(PIN_LED,false);
          }
      }
      delay(10);
    }
    }
  }
  //Inverting and channel flipping
  if (!invertA){
    inA = -inA;
  }
  if (!invertB){
    inB = -inB;
  }
  if(flipChannel){
    int temp;
    temp = inA;
    inA = inB;
    inB = temp;
  }
  //Overhead Driving
  if(AUX > 200){                                                                                      
    inB = -inB;
  }  
  // mixing
  if(currentMix != 0.00){
    inA =(int)(inA*currentMix);
      speedL = inB +inA;                                                                             
      speedR = inB -inA;
  }
  else{
    speedL = inA;
    speedR = inB;
  }
  speedL = constrain(speedL, -MAX_PWM, MAX_PWM);
  speedR = constrain(speedR, -MAX_PWM, MAX_PWM);
  if (inDrive){                                                                     //Drive                                                                  
    digitalWrite(PIN_LED,true);
    drivePWM(speedL,speedR,brakingEN);
  }
}
 //                                                                                END LOOP

//Plays a number of beeps
void indicateMode(int beeps) {
  for (int i = 0; i < beeps; i++) {
    //resetting WDT
  __asm__ __volatile__ ("wdr"::);
    playTone(2000, 100);
    delay(150);
  }
}

//Play a tone/beep
void playTone(int freq, int durationMs) {
  long periodMicro = 1000000L / freq;
  long halfPeriod = periodMicro / 2;
  long cycles = (long)freq * durationMs / 1000L;

  for (long i = 0; i < cycles; i++) {
    analogWrite(PIN_MOTOR_L_A, SOUND_PWM_POWER); digitalWrite(PIN_MOTOR_L_B, LOW);
    analogWrite(PIN_MOTOR_R_A, SOUND_PWM_POWER); digitalWrite(PIN_MOTOR_R_B, LOW);
    delayMicroseconds(halfPeriod);
    digitalWrite(PIN_MOTOR_L_A, LOW); analogWrite(PIN_MOTOR_L_B, SOUND_PWM_POWER);
    digitalWrite(PIN_MOTOR_R_A, LOW); analogWrite(PIN_MOTOR_R_B, SOUND_PWM_POWER);
    delayMicroseconds(halfPeriod);
  }
  stopMotors();
}

//Converting the input values to -255 to 255
int ticksToPWM(uint16_t ticks) {
  if (ticks < 5000 || ticks > 25000) return 0;
  if (ticks > 14500 && ticks < 15500) return 0;
  long val = (long)ticks - 15000;
  val = map(val, -4000, 4000, -255, 255);
  return constrain(val, -255, 255);
}

void drivePWM(int left, int right,bool brakingEN) {
  setMotor(PIN_MOTOR_L_A, PIN_MOTOR_L_B, left, brakingEN);
  setMotor(PIN_MOTOR_R_A, PIN_MOTOR_R_B, right, brakingEN);
}

void setMotor(int pinA, int pinB, int speed,bool brakingEN) {
  if (speed > DEADZONE) {
    digitalWrite(pinB, LOW); analogWrite(pinA, speed);
  } else if (speed < -DEADZONE) {
    digitalWrite(pinA, LOW); analogWrite(pinB, abs(speed));
  } else {
    if (!brakingEN){
    digitalWrite(pinA, true); digitalWrite(pinB, true);
    }
    else{
    digitalWrite(pinA, LOW); digitalWrite(pinB, LOW);
    }
  }
}

//Stopping motors
void stopMotors() {
  digitalWrite(PIN_MOTOR_L_A, LOW); digitalWrite(PIN_MOTOR_L_B, LOW);
  digitalWrite(PIN_MOTOR_R_A, LOW); digitalWrite(PIN_MOTOR_R_B, LOW);
}

// Interrupt controller
void isrSteering() {
  steering_timer = 0;
  uint16_t now = TCB0.CNT;
  if (digitalRead(PIN_RC_STEERING)) tcb_start_steering = now;
  else rc_steering_ticks = now - tcb_start_steering;
}
//Interrupt controller
void isrThrottle() {
  throttle_timer = 0;
  uint16_t now = TCB0.CNT;
  if (digitalRead(PIN_RC_THROTTLE)) tcb_start_throttle = now;
  else rc_throttle_ticks = now - tcb_start_throttle;
}
//Interrupt controller
void isrAux() {
  aux_timer = 0;
  uint16_t now = TCB0.CNT;
  if (digitalRead(PIN_RC_AUX)) tcb_start_aux = now;
  else rc_aux_ticks = now - tcb_start_aux;
}