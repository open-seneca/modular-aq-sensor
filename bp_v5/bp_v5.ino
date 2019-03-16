#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

#define LED PB12
#define CSPIN PA4
#define VINPIN PA0
#define RED PB1 // former LED1
#define GREEN PB0
#define BLUE PA1
#define SDA PB7
#define SCL PB6

#define SHT_addr 0x44
#define SPS30_addr 0x69

String code_version = "BlackPill_v5";

// PM sensor SPS30
float mc_1p0, mc_2p5, mc_4p0, mc_10p0, nc_0p5, nc_1p0, nc_2p5, nc_4p0, nc_10p0, typical_particle_size = 0;
byte w1, w2,w3;
byte ND[60];
long tmp;
float measure[10];

// The TinyGPS++ object
TinyGPSPlus gps;
File myFile;
int filename;
uint32_t mtime;

// Humidity and temperature sensor
Adafruit_SHT31 sht31 = Adafruit_SHT31();

//GSM module
String imei, cnum, payload = "";

float vin, temperature, humidity = 0;
bool status;
uint8_t fwdate[3], workmode, asleep;
int counter=0,ret, gpsUpdated = 0;

void setup() {
    // set up LEDs and turn on
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);
    pinMode(GREEN, OUTPUT);
    digitalWrite(GREEN, HIGH);
    pinMode(RED, OUTPUT);
    digitalWrite(RED, LOW);

    // Enabling internal pull ups resistors for I2C communication
    pinMode(SDA,INPUT_PULLUP);
    pinMode(SCL,INPUT_PULLUP);
    
    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    Serial1.begin(9600);
    Serial2.begin(9600); // GPS baudrate is 9600
    delay(50);

    // check if battery level is OK and output it to debug output
    /*if (!batteryOk()) {
        Serial.print(F("Battery voltage is low: "));
    } else {
        Serial.print(F("Battery voltage is ok: "));
    }
    Serial.println(vin);*/
 
    // initialise SD card
    Serial.println(F("Initialising SD card..."));
    if (!SD.begin(CSPIN)) {
        Serial.println(F("SD initialisation failed!"));
    }
    else {
        Serial.println(F("SD initialisation done."));
    }

    // print whether GPS data communication is enabled or not
    if (Serial2.available() > 0) {
        Serial.println(F("GPS communication enabled."));
    } else {
        Serial.println(F("GPS not available."));
    }

    // Start i2c communication
    Wire.begin();
   
   // Testing humidity sensor
   if (! sht31.begin(0x44)) Serial.println(F("Couldn't find SHT31"));  // Set to 0x45 for alternate i2c addr
   
   //Testing PM sensor
    SPS30_start_measurement();
    SPS30_cleaning();
 
    // open the file. Note that only one file can be open at a time,so you have to close this one before opening another.
    filename = getNextName();
    digitalWrite(LED, LOW);
    myFile = SD.open(String(filename) + ".csv", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
        myFile.println(F("Version of code: BlackPill_v5, including GPS, GSM, SPS30, SHT31, SD card"));
        myFile.println(F("Counter,Latitude,Longitude,mc_10p0,mc_2p5,mc_4p0,mc_1p0,nc_0p5,nc_1p0,nc_2p5,nc_4p0,nc_10p0,ParticleSize,Temperature,Humidity,gpsUpdated,Satellites,GPS_speed,Date,Time,Millis,BatteryVIN"));
        myFile.print(","); myFile.print(filename);
        
        // close the file:
        myFile.close();
    } else {
        // if the file didn't open, print an error:
        Serial.println(F("Error while writing SD"));
    }

    // print the data to debug output as well
    Serial.println(F("Counter,Latitude,Longitude,mc_10p0,mc_2p5,mc_4p0,mc_1p0,nc_0p5,nc_1p0,nc_2p5,nc_4p0,nc_10p0,ParticleSize,Temperature,Humidity,gpsUpdated,Satellites,GPS_speed,Date,Time,Millis,BatteryVIN"));

    // Initialise GSM
    //while (Serial1.available() > 0){ 
        initSIM();
        Serial.println(F("SIM card initialised"));
    
    //else Serial.println(F("GSM not available"));
    
    digitalWrite(LED, HIGH);
    mtime = millis();
}

void loop() {
    
    // wait until GPS is available
    while (Serial2.available() > 0) {
        // read & encode GPS data
        gps.encode(Serial2.read());

        // check that 3 seconds have passed, since this is our data collection timeframe
        if (millis() - mtime > 5000) {
            // turn on LED for user feedback
            digitalWrite(LED, LOW);

            // read in battery level
            batteryOk();

            // Read PM sensor SPS30
            SPS30_reading();

           // Read temperature and humidity 
           temperature = sht31.readTemperature();
           humidity = sht31.readHumidity();

            if (gps.location.isUpdated()) {
                gpsUpdated = 1;
            }
            
            payload = String(gps.location.lat()+180) + "," + String(gps.location.lng()+90)+ "," + String(mc_10p0)+ "," + String(mc_2p5)+ "," + String(mc_4p0)+ "," + String(mc_1p0)+ "," + String(nc_0p5)+ "," + String(nc_1p0)+ "," + String(nc_2p5)+ "," + String(nc_4p0)+ "," + String(nc_10p0)+ "," + String(typical_particle_size)+ "," + String(temperature)+ "," + String(humidity)+ "," + String(gpsUpdated)+ "," + String(gps.satellites.value())+ "," + String(gps.speed.kmph()) + "," + String(formatDate())+ "," + String(formatTime())+ "," + String(mtime)+ "," + String(vin) + "," + code_version;
            uploadSIM(payload);
            
            // open file to write sensor data
            myFile = SD.open(String(filename) + ".csv", FILE_WRITE);
            
            // if the file opened successfully, write the sensor data to it
            if (myFile) {
              myFile.print(payload);
              myFile.close();
            }

            // turn off LED for user feedback
            digitalWrite(LED, HIGH);

            // print data to serial output for debugging purposes
            Serial.println(payload);
            
            // set current time
            mtime = millis();
          
            counter++;
            gpsUpdated = 0;
        }
    }
    delay(10);
    
} //end loop

// FUNCTIONS

// format date in the following format:
// YYYYMMDD
String formatDate() {
    String out = "";
    out += gps.date.year();
    out += formatInt(gps.date.month(), 2);
    out += formatInt(gps.date.day(), 2);
    return out;
}

// format time in the following format:
// HHMMSS
String formatTime() {
    String out = "";
    out += formatInt(gps.time.hour(), 2);
    out += formatInt(gps.time.minute(), 2);
    out += formatInt(gps.time.second(), 2);
    return out;
}

// format an integer as a string with the required number of digits of precision
// if the number of digits in the integer is larger than precision, return the largest number
// possible using that precision ie. precision number of 9s
String formatInt(int num, int precision) {
  // find the maximum
  int maximum = 1;
  for (int i = 0; i < precision; i++) {
    maximum *= 10;
  }
  maximum--;

  // if the number is too big, return the maximum number that can be represented using precision digits
  if (num > maximum) {
    return String(maximum);
  }

  // otherwise convert the integer and prepend 0s until full
  String out = String(num); // fill up with leading zeroes if necessary
  String zero = "";
  for (int i = 0; i < (precision - out.length()); i++) {
    zero += "0";
  }
  
  return (zero + out);
}

// check filenames on SD card and increment by one
int getNextName() {
  File root = SD.open("/");
  int out = 10000;
  while (true) {
    // check file
    File entry = root.openNextFile();

    // if opening the file doesn't return a correct file, we are done
    if (!entry) {
      break;
    }

    // check if the filename is in the correct range. If it is, increment the counter
    String temp = entry.name();
    temp = temp.substring(0 ,5);
    int num = temp.toInt();
    if (num < 99999 && num >= out) {
      out = num + 1;
    }

    // close the file
    entry.close();
  }
  return out;
}


// checks if the battery voltage is OK and returns whether it is or not
boolean batteryOk() { // analog input can measure between 0 and supply voltage (3.3V)
    vin = 2 * 3.3 * analogRead(VINPIN) / 4095.0; // calculation based on 50:50 voltage divider to measure battery voltage between 0 and 6.6V
    if (vin < 3.5) {
        digitalWrite(RED, HIGH);
        return false;
    }
    else digitalWrite(RED, LOW);
    return true;
}

// Calculating checksum. Function provided in SPS30 datasheet
byte CalcCrc(byte data[2]) {
  byte crc = 0xFF;
  for(int i = 0; i < 2; i++) {
    crc ^= data[i];
    for(byte bit = 8; bit > 0; --bit) {
      if(crc & 0x80) {
      crc = (crc << 1) ^ 0x31u;
      } else {
        crc = (crc << 1);
       }
     }
   }
   //Serial.print("calcCrc: "); Serial.println(crc, HEX);
  return crc;
}

void SetPointer(byte P1, byte P2)
{
  Wire.beginTransmission(SPS30_addr);
  Wire.write(P1);
  Wire.write(P2);
  Wire.endTransmission();
  }

void SPS30_start_measurement(){
   Wire.beginTransmission(SPS30_addr);
   
   // Start measurement (command: 0x0010)
   Wire.write(0x00);
   Wire.write(0x10);
   
   // Read measurement (command: 0x0300)
   Wire.write(0x03);
   Wire.write(0x00);
   
   // Send checksum
   uint8_t data[2]={0x03, 0x00};
   Wire.write(CalcCrc(data));
   
   Wire.endTransmission();
   delay(10000);
}

void SPS30_cleaning(){
  Serial.println(F("clean"));
  //Start fan cleaning
  SetPointer(0x56, 0x07);
  delay(12000);
  Serial.println(F("clean end"));
  delay(100); 
}

void SPS30_reading(){
  SetPointer(0x02, 0x02);
  Wire.requestFrom(SPS30_addr, 3);
  w1=Wire.read();
  w2=Wire.read();
  w3=Wire.read();
  
  // Read measurements
  if (w2==0x01){              //0x01: new measurements ready to read
    SetPointer(0x03,0x00);
    Wire.requestFrom(SPS30_addr, 60);
    
    //for(int i=0;i<60;i++) ND[i]=Wire.read();                        
    for(int i=0;i<60;i++) { 
        ND[i]=Wire.read();     //without Wire.h lib modification only first 5 values
        //Serial.print(i);Serial.print(": ");Serial.println(ND[i],HEX);
    }

    int k=0;
    // Checksum
    for(int i=0;i<60;i++) { 
       if ((i+1)%3==0){
          byte datax[2]={ND[i-2], ND[i-1]};
          //Serial.print("crc: ");Serial.print(CalcCrc(datax),HEX);
          //Serial.print("  "); Serial.println(ND[i],HEX);
          if(tmp==0) {
           tmp= ND[i-2]; 
           tmp= (tmp<<8) + ND[i-1]; 
          }
          else{
           tmp= (tmp<<8)+ ND[i-2];
           tmp= (tmp<<8) + ND[i-1];
           //Serial.print(tmp,HEX);Serial.print(" ");
           measure[k]= (*(float*) &tmp);
           k++;
           tmp=0;
          }  
        }
      }
    mc_1p0 = measure[0];
    mc_2p5 = measure[1];
    mc_4p0 = measure[2];
    mc_10p0 = measure[3];
    nc_0p5 = measure[4];
    nc_1p0 = measure[5];
    nc_2p5 = measure[6];
    nc_4p0 = measure[7];
    nc_10p0 = measure[8];
    typical_particle_size = measure[9];
    
    }

  //  Stop Meaurement
  //  SetPointer(0x01, 0x04);
  }


  void initSIM() {
  runCommand("AT");
  imei = runCommand ("AT+CGSN").substring(0,15);
  cnum = runCommand ("AT+CNUM").substring(13,27);
  runCommand("AT+CREG?");
  runCommand("AT+CSQ");
  runCommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  runCommand("AT+SAPBR=3,1,\"APN\",\"TM\"");
  runCommand("AT+SAPBR=1,1");
  runCommand("AT+SAPBR=2,1");
  runCommand("AT+HTTPINIT");    
}

void uploadSIM(String payload) {
  runCommand("AT+HTTPPARA=\"URL\",\"http://app.cambikesensor.net/php/gsmUpload.php?imei=" + imei + "&simnumber=" + cnum + "&payload=" + payload + "\"");
  runCommand("AT+HTTPACTION=0");
  runCommand("AT+HTTPREAD");
}
  
String runCommand(String cmd) {
  Serial.println(cmd);
  Serial1.println(cmd);
  delay(1000);
  String resp = "";
  
  while(Serial1.available()) {
    resp += (char)Serial1.read();
  }

  resp.replace("\r","");
  resp.replace("\n","");

  String filteredResp = "";
  int i;
  for(i = cmd.length(); i < resp.length(); i++) {
      filteredResp += resp[i];
  }

  Serial.println("filtered: " + filteredResp);
  return filteredResp;
  
}

//String getRandomName() { // generate a "random" filename using sensor input
//  uint32_t ran = 0;
//  for (int i = 0; i < 7; i++) ran += ref[i];
//  while (ran > 99999) ran -= ref[0];
//  String out = String(ran);
//  return out;
//}

// encode a 28 digit number String into a 12 byte ASCII string
// this function was used to encode more data using fewer bytes
// this function is no longer needed
/*String encodeMsg28(String msg) {
    if (msg.length() != 28) {
        String ret = "" + msg.length();
        return ret;
    }

    unsigned char bytes[3];
    String out = "";

    for (int j = 0; j < 4; j++)
    {
        String temp = "";
        for (int i = 0; i < 7; i++)
            temp += msg[j * 7 + i];
        unsigned long int t = temp.toInt();

        bytes[0] = (t >> 16);
        bytes[1] = (t >> 8);
        bytes[2] = t;
        //out += (unsigned char) bytes[0] + (unsigned char) bytes[1] + (unsigned char) bytes[2];
        out += (char) bytes[0];
        out += (char) bytes[1];
        out += (char) bytes[2];
    }
    return out;
}*/

//String trimGPS(double lati) { // returns longitude/latitude trimmed to 5 relevant digits
//  int temp = ((int)(lati*100000))%100000; // also works but might cause loss of precision: =(int)((lati-(int)lati)*100000);
//  String out = (String)temp;
//  temp = 5-out.length(); // fill up with leading zeroes if necessary
//  for (int i=0; i<temp; i++) out="0"+out;
//  return out;
//}

            /*
             * m = sprintf(c, " %3d:  %5d    ", i,n);    // build integer string using C integer formatters  (m is length, and not used in this code)
               Serial.print(c);   

              dtostrf(x,6,3,f);                          // -n.nnn Use this for a consistent float format
              Serial.println(f); 
             */

            /*char latitude[9];
            char longitude[9];
            char mass_1[5];
            char mass_25[5];
            char mass_4[5];
            char mass_10[5];
            
            dtostrf(gps.location.lat()+180, 3,6,latitude);
            dtostrf(gps.location.lng()+90, 3,6,longitude);
            dtostrf(mc_1p0,3,2,mass_1);
            dtostrf(mc_2p5,3,2,mass_25);
            dtostrf(mc_4p0,3,2,mass_4);
            dtostrf(mc_10p0,3,2,mass_10);
            
            Serial.print("latitude: "); Serial.println(latitude);
            */

