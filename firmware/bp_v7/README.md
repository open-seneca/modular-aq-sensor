## Important: bp_v7 is an improved code (originally bp_v6). It adds support for an OLED display and GPS is now optional.

### New features
- SIM800L GSM wireless transmission
- HM10 Bluetooth transmission (not yet implemented in code)
- SHT31 Humidity / Temparature sensor
- SPS30 Particulate matter sensor
- Connector for UBLOX GPS
- Micro USB power plug
- 128x64 i2c OLED display

### Discontinued functions
- Accelerometer / Gyroscope
- LoRa wireless transmission


The measured power draw is around 70-90mA (while GSM is not trasmitting) at 5V supply, ~0.4W.
With a 10.000mAh powerbank you can keep the sensor running for about 5 days constantly (ideal conditions).
