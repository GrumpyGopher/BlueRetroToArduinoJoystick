# BlueRetroToArduinoJoystick
Arduino sketches for connecting Arduino pro micro to BlueRetro using Arduino Joystick Library

There are two versions of this sketch for 4 player and 6 player. 

In order to have 6 Joysticks enabled at the same time CDC must be disabled
for Arduino Pro Micro this can be done by adding -DCDC_DISABLED to the extra_flags line in the boards.txt file
example:  leonardo.build.extra_flags={build.usb_flags}  -DCDC_DISABLED

**Note:** this will disable serial communication through usb so you will need to program the board another way sometimes holding reset and waiting for the Arduino IDE to say uploading before releasing the reset button will work

If you do not want to disable CDC you can do up to 4 players without disabling CDC
take a look at BlueRetroToArduinoJoystick-4P.ino

For better serial communication I also recommend updating the serial buffer size to 256.  This can be done by modifying this line cores/arduino/USBAPI.h
#define SERIAL_BUFFER_SIZE 256