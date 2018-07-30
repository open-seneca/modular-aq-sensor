# CambikeSensor

https://www.youtube.com/watch?v=J7ctdFaBZ20

a) To burn bootloader on Black Pill using USB2TTL adapter:
- jumper B0+ to center pin, B1- to center pin
- RX to PA9, TX to PA10, GND to G on the ST-Link connector, 3.3V to V3 on the ST-Link connector
- use bootloader software from this folder and burn the .bin file (follow screenshots)

b) To flash software:
- install Arduino Due in IDE board manager if you do not have it already
- put Arduino_STM32 library (Github) in "My Documents/Arduino/hardware" (Note: if the hardware folder doesn't exist you will need to create it)
- restart IDE
- under "Tools -> Board" select "Generic STM32F103C series"
- under "Tools -> Upload method" select Serial
- connect everything exactly as descibed in a)
- press reset button to enter flash mode
- flash your code

c) To execute the code (board enters execution mode automatically after each flash)
- plug both header pins on low (-)
- connect to power/onboard micro usb

To flash code via onboard micro USB you need to add a resistor, see video.