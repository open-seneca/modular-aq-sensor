![alt text](https://raw.githubusercontent.com/sh969/Open-Seneca/master/logo.png)


If you already have a sensor and want to upload and visualise your data you can create an account here:

http://app.open-seneca.org

Please refer to our wiki page to learn about the assembly of the hardware to build a CamBike sensor youself:

http://wiki.cambikesensor.net

![alt text](https://raw.githubusercontent.com/sh969/Open-Seneca/master/photo.png)

If you have already built your kit and want to flash the code to the Black Pill board, you are exactly right here. 
Here is how:

a) Wiring and onboard jumpers:
- RX on the USB2TTL adapter to PA9, TX to PA10, GND to G, 3.3V to V3
- jumper B0+ to center pin, B1- to center pin

b) To flash the code:
- (install Arduino Due in IDE board manager)
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

We are an open-source project, please feel free to extend and improve our code, just post a commit!
