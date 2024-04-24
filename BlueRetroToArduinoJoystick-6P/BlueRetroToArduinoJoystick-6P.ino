// Using the Arduino Joystick Library creates multiple 
// Joystick objects on a single Arduino Leonardo or Arduino Micro.
// Joystick objects are controlled based on serial data received 
// from BlueRetro device.
//
// In order to have 6 Joysticks enabled at the same time CDC must be disabled
// for Arduino Pro Micro this can be done by adding -DCDC_DISABLED to the extra_flags line in the boards.txt file
// example:  leonardo.build.extra_flags={build.usb_flags}  -DCDC_DISABLED
// note this will disable serial communication through usb so you will need to program the board another way
// sometimes holding reset and waiting for the Arduino IDE to say uploading before releasing the reset button will work

// If you do not want to disable CDC you can do up to 4 players without disabling CDC
// take a look at BlueRetroToArduinoJoystick-4P.ino

// For better serial communication also modified these lines in cores/arduino/USBAPI.h
// #define SERIAL_BUFFER_SIZE 256//Modified from 64

//requires Arduino Joystick Library created by Matthew Heironimus
#include <Joystick.h>
#define JOYSTICK_COUNT 6
#define BUTTON_COUNT 11
int minAxisRange = -128;
int maxAxisRange = 127;
int minTriggerRange = -255;
int maxTriggerRange = 255;
Joystick_ Joystick[JOYSTICK_COUNT] = {//btn hat  LX    LY     Z    Rx    Ry    Rz  Rudder Throttle Accel Brake Steering
  Joystick_(0x03, JOYSTICK_TYPE_GAMEPAD, 11, 1, true, true, true, true, true, true, false, false, false, false, false),
  Joystick_(0x04, JOYSTICK_TYPE_GAMEPAD, 11, 1, true, true, true, true, true, true, false, false, false, false, false),
  Joystick_(0x05, JOYSTICK_TYPE_GAMEPAD, 11, 1, true, true, true, true, true, true, false, false, false, false, false),
  Joystick_(0x06, JOYSTICK_TYPE_GAMEPAD, 11, 1, true, true, true, true, true, true, false, false, false, false, false),
  Joystick_(0x07, JOYSTICK_TYPE_GAMEPAD, 11, 1, true, true, true, true, true, true, false, false, false, false, false),
  Joystick_(0x08, JOYSTICK_TYPE_GAMEPAD, 11, 1, true, true, true, true, true, true, false, false, false, false, false)
};

int dPadlastState[JOYSTICK_COUNT];
bool lastButtonState[JOYSTICK_COUNT][BUTTON_COUNT];
char inputString[100];        // a String to hold incoming serial data from BlueRetro
bool stringComplete = false;
enum button { A, B, X, Y, L1, R1, SELECT, START, L3, R3, HOME };

// Button bitflags
#define BTN_LEFT  0x00000001  // 1
#define BTN_RIGHT  0x00000002  // 2
#define BTN_DOWN  0x00000004  // 4
#define BTN_UP  0x00000008  // 8
#define BTN_X  0x00000010  // 16
#define BTN_B  0x00000020  // 32
#define BTN_A  0x00000040  // 64
#define BTN_Y  0x00000080  // 128
#define BTN_START  0x00000100  // 256
#define BTN_SELECT  0x00000200  // 512
#define BTN_HOME 0x00000400  // 1024
#define BTN_L1 0x00000800  // 2048
#define BTN_L3 0x00001000  // 4096
#define BTN_R1 0x00002000  // 8192
#define BTN_R3 0x00004000  // 16384
#define ALL_VALID_BUTTON_FLAGS (BTN_LEFT | BTN_RIGHT | BTN_DOWN | BTN_UP | BTN_X | BTN_B | BTN_A | BTN_Y | BTN_START | BTN_SELECT | BTN_HOME | BTN_L1 | BTN_L3 | BTN_R1 | BTN_R3)

void setup() {
  for (int joystickIndex = 0; joystickIndex < JOYSTICK_COUNT; joystickIndex++)
  {
    Joystick[joystickIndex].begin(false);
    Joystick[joystickIndex].setXAxisRange(minAxisRange, maxAxisRange);
    Joystick[joystickIndex].setYAxisRange(minAxisRange, maxAxisRange);
    Joystick[joystickIndex].setRxAxisRange(minAxisRange, maxAxisRange);
    Joystick[joystickIndex].setRyAxisRange(minAxisRange, maxAxisRange);
    Joystick[joystickIndex].setZAxisRange(minTriggerRange, maxTriggerRange);
    Joystick[joystickIndex].setRzAxisRange(minTriggerRange, maxTriggerRange);    
    Joystick[joystickIndex].setXAxis(0);
    Joystick[joystickIndex].setYAxis(0);
    Joystick[joystickIndex].setZAxis(0);
    Joystick[joystickIndex].setRxAxis(0);
    Joystick[joystickIndex].setRyAxis(0);
    Joystick[joystickIndex].setRzAxis(0);
    dPadlastState[joystickIndex] = -1;
    for (int buttonindex = 0; buttonindex < BUTTON_COUNT; buttonindex++)
    {
      lastButtonState[joystickIndex][buttonindex] = false;
    }
  }
  delay(3000); //delay to allow BlueRetro adapter to finish booting
  Serial1.begin(1000000);
  while (!Serial1) {
    delay(10);
  }
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
void serialEvent1() {
  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();

    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
      break;
    }
    else {
      // add it to the inputString:
      int index = strlen(inputString);
      inputString[index++]=inChar;
      inputString[index]='\0';  // always terminate strings
    }
  }
}

void loop() {
  // process received data
  if (stringComplete) {
    //joystick data will be a fixed 30 characters
    int inputStringLength = strlen(inputString);
    if (inputStringLength >= 30){
      parseJoystickData();
    }
    // clear the string:
    strcpy(inputString,"");
    stringComplete = false;
  }
}

void parseJoystickData(){
  int inputStringLength = strlen(inputString);
  // Calculate the starting index for the last 30 characters
  int startIndex = inputStringLength - 30;
  // Shift the last 30 characters to the beginning of the array
  memmove(inputString, inputString + startIndex, 30);
  // Null-terminate the array after the last 30 characters
  inputString[30] = '\0';

  if (inputString[0] != '[' || inputString[29] != ']') return;
  
  char deviceId = inputString[1];
  int joystickIndex = inputString[1] - '0';
  if (isValidDeviceId(joystickIndex)) return;
  
  int32_t buttonValue = getInt32Value(2, 5);
  int32_t lAxesX = getInt32Value(7, 4);
  int32_t lAxesY = getInt32Value(11, 4);
  int32_t rAxesX = getInt32Value(15, 4);
  int32_t rAxesY = getInt32Value(19, 4);
  int32_t triggerL2 = getInt32Value(23, 3);
  int32_t triggerR2 = getInt32Value(26, 3);

  if (!isValidButtonFlags(buttonValue)) return;
  if (lAxesX < minAxisRange || lAxesX > maxAxisRange) return;
  if (lAxesY < minAxisRange || lAxesY > maxAxisRange) return;
  if (rAxesX < minAxisRange || rAxesX > maxAxisRange) return;
  if (rAxesY < minAxisRange || rAxesY > maxAxisRange) return;
  if (triggerL2 < minTriggerRange || triggerL2 > maxTriggerRange) return;
  if (triggerR2 < minTriggerRange || triggerR2 > maxTriggerRange) return;
  
  Joystick[joystickIndex].setXAxis(lAxesX);
  Joystick[joystickIndex].setYAxis(lAxesY);
  Joystick[joystickIndex].setZAxis(triggerL2);
  Joystick[joystickIndex].setRzAxis(triggerR2);
  Joystick[joystickIndex].setRxAxis(rAxesX);
  Joystick[joystickIndex].setRyAxis(rAxesY);

  processButton(joystickIndex, A, isButtonPressed(buttonValue, BTN_A));
  processButton(joystickIndex, B, isButtonPressed(buttonValue, BTN_B));
  processButton(joystickIndex, X, isButtonPressed(buttonValue, BTN_X));
  processButton(joystickIndex, Y, isButtonPressed(buttonValue, BTN_Y));
  processButton(joystickIndex, L1, isButtonPressed(buttonValue, BTN_L1));
  processButton(joystickIndex, R1, isButtonPressed(buttonValue, BTN_R1));
  processButton(joystickIndex, SELECT, isButtonPressed(buttonValue, BTN_SELECT));
  processButton(joystickIndex, START, isButtonPressed(buttonValue, BTN_START));
  processButton(joystickIndex, L3, isButtonPressed(buttonValue, BTN_L3));
  processButton(joystickIndex, R3, isButtonPressed(buttonValue, BTN_R3));
  processButton(joystickIndex, HOME, isButtonPressed(buttonValue, BTN_HOME));
  processDPad(joystickIndex, isButtonPressed(buttonValue, BTN_UP), isButtonPressed(buttonValue, BTN_DOWN), isButtonPressed(buttonValue, BTN_LEFT), isButtonPressed(buttonValue, BTN_RIGHT));
  Joystick[joystickIndex].sendState();
}

bool isButtonPressed(uint32_t flags, uint32_t flag) {
  return (flags & flag) != 0;
}

bool isValidDeviceId(int joystickIndex){
  return (joystickIndex < 0 || joystickIndex > JOYSTICK_COUNT - 1);
}

bool isValidButtonFlags(uint32_t flags) {
  return (flags & ~ALL_VALID_BUTTON_FLAGS) == 0;
}

void processButton(int joystickIndex, int buttonIndex, bool currentButtonState){
  if (currentButtonState == lastButtonState[joystickIndex][buttonIndex]) return;
  if (currentButtonState == 1) {
    Joystick[joystickIndex].pressButton(buttonIndex);
  }
  else {
    Joystick[joystickIndex].releaseButton(buttonIndex); 
  }    
  lastButtonState[joystickIndex][buttonIndex] = currentButtonState;
}

void processDPad(int joystickIndex, bool padUp, bool padDown, bool padLeft, bool padRight){
  int newButtonState = -1;
  if (padUp && padLeft) {
    newButtonState = 315;
  }
  else if (padUp && padRight) {
    newButtonState = 45;
  }  
  else if (padDown && padLeft) {
    newButtonState = 225;
  }    
  else if (padDown && padRight) {
    newButtonState = 135;
  }
  else if (padUp) {
    newButtonState = 360;
  }  
  else if (padDown) {
    newButtonState = 180;
  }
  else if (padLeft) {
    newButtonState = 270;
  }
  else if (padRight) {
    newButtonState = 90;
  }
  else {
    newButtonState = -1;
  }
  if (newButtonState != dPadlastState[joystickIndex]) {
    Joystick[joystickIndex].setHatSwitch(0, newButtonState);
    dPadlastState[joystickIndex] = newButtonState;
  }
}

int32_t getInt32Value(int startPosition, int numDigits){
  char returnValue[numDigits];
  for (int index = 0; index < numDigits; index++){
    returnValue[index] = inputString[index + startPosition];
  }

  int32_t axisValue = atol(returnValue);
  return axisValue;  
}
