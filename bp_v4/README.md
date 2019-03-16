## bp_v4 is our Team Challenge version (supporting the accelerometer). Flash this code if you were following the GitHub documentation.

### Features

Microcontroller:
STM32F103C8T6, also known as STM32 Black Pill.

GPS:
NEO-6M V2 GPS, for geo-localisation.

Particulate Matter (PM) sensor:
SDS011, which measures PM10 and PM2.5. If you are interested in knowing more about why it is important to measure particulate matter and about how this sensor works, have a look to the Particulate Matter section!

Accelerometer and Gyroscope:
Arduino 6-axis GY-521, to infer road quality.

SD card module:
Arduino-compatible SD card module to record all the data.

LoRaWAN radio:
NiceRF Lora1276, for low-power wireless transmission of data. If you would like to know more about LoRaWAN and Low-Power Wide-Area Networks (LPWAN) check out this section!

Battery:
Lithium polymer flat 3.7V 1200 mAh battery with protective circuits against over-charge, over-discharge, short-circuit and over-current.

Step-up voltage regulator:
Boost from 1-5 V to 5 V, to increase the battery voltage from 3.7 V to 5 V. It is needed to power some of the components, such as the PM sensor.

Step-down voltage regulator:
Buck from 6-3.3 V to 3.3 V, to decrease the battery voltage from 3.7 V to 3.3 V and to power most of the components.

Button LED:
RGB LED self-latching push button, to be used as a status LED and master switch of the hub.