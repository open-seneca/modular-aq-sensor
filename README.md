# CambikeSensor

A cyclist’s mobile sensor hub for big data citizen science.

If you want to build our sensor hub based on the Black Pill board, you need to flash some code using the Arduino IDE. Here is how:

a) Wiring and onboard jumpers:
- RX on the USB2TTL adapter to PA9, TX to PA10, GND to G, 3.3V to V3
- jumper B0+ to center pin, B1- to center pin

b) To flash the code:
(- install Arduino Due in IDE board manager)
- put Arduino_STM32 library (from the "hardware" folder in this repo) in "My Documents/Arduino/hardware" (Note: if the hardware folder doesn't exist you will need to create it)
- start Arduino IDE and open the latest bp_vX from this repository
- under "Tools -> Board" select "Generic STM32F103C series"
- under "Tools -> Upload method" select Serial
- press reset button on the Black Pill to enter its flash mode
- click "Upload" to flash your code

c) To execute the code:
- after flashing code should run
- plug both header pins on low (B1- should be low already if your followed the steps correctly, move other jumper to B0-)
- the board will now keep its flash memory, every reset will restart execution
