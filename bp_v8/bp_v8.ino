#include <TinyGPS++.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h> // BUFFER_LENGTH 32 in WireBase.h and Wire_slave.cpp has been increased to 64 to read full SPS30 buffer (download from our Github)
#include "Adafruit_SHT31.h"
#include <Adafruit_SSD1306.h>

#define LED PB12
#define CSPIN PA4
#define VINPIN PA0
#define RED PB1 // former LED1
#define GREEN PB0
#define BLUE PA1
#define SDA PB7
#define SCL PB6

#define SHT31_addr 0x44
#define SPS30_addr 0x69
#define SSD1306_addr 0x3C
#define SGP30_addr 0x58
#define SGP30_FEATURESET 0x0020    ///< The required set for this library
#define SGP30_CRC8_POLYNOMIAL 0x31 ///< Seed for SGP30's CRC polynomial
#define SGP30_CRC8_INIT 0xFF       ///< Init value for CRC
#define SGP30_WORD_LEN 2           ///< 2 bytes per word

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
//#define LANDSCAPE // OLED display rotation

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool display_connected = true; // does not seem to work yet, implement i2c address check at bootup

String code_version = "BlackPill_v8";
String header = "Counter,Latitude,Longitude,gpsUpdated,Speed,Altitude,Satellites,Date,Time,Millis,PM1.0,PM2.5,PM4.0,PM10,Temperature,Humidity,NC0.5,NC1.0,NC2.5,NC4.0,NC10,TypicalParticleSize,TVOC,eCO2,BatteryVIN";


// The TinyGPS++ object
TinyGPSPlus gps;
File myFile;
String filename;
uint32_t mtime;

// Humidity and temperature sensor SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// VOC sensor SGP30
uint16_t sgp30_sn[3];
uint16_t TVOC = 0; // Total Volatile Organic Compounds in ppb
uint16_t eCO2 = 0; // Equivalent CO2 in ppm
bool sgp_connected = true;

// PM sensor SPS30
float mc_1p0, mc_2p5, mc_4p0, mc_10p0, nc_0p5, nc_1p0, nc_2p5, nc_4p0, nc_10p0, typical_particle_size = -1;
byte w1, w2,w3;
byte ND[60];
long tmp;
float measure[10];
String sps30_sn;

//GSM module
String imei = "", cnum = "", payload = "";
bool sim_connected = false;
bool gps_connected = true;

float vin = 0, temperature = 20.0f, humidity = 50.0f;
int counter=0, gpsUpdated=0, vsat=0;
uint32_t abs_hum = 0.0f;

void setup() {
    // set up LEDs and turn on
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH);
    // flash the PCB LED
    // pins tested -> works
    // Buenos Aires PCB LEDs do not flash
//    pinMode(GREEN, OUTPUT);
//    pinMode(RED, OUTPUT);
//    pinMode(BLUE, OUTPUT);
//    for (int i=0; i<10; i++) {
//      digitalWrite(BLUE, HIGH);
//      digitalWrite(GREEN, LOW);
//      digitalWrite(RED, LOW);
//      delay(300);    
//      digitalWrite(BLUE, LOW);
//      digitalWrite(GREEN, HIGH);
//      digitalWrite(RED, LOW);
//      delay(300);
//      digitalWrite(BLUE, LOW);
//      digitalWrite(GREEN, LOW);
//      digitalWrite(RED, HIGH);
//      delay(300);
//    }   
    
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
    if (!SD.begin(CSPIN)) Serial.println(F("SD card not available."));
    else Serial.println(F("SD card found."));
   
  // Testing humidity sensor
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println(F("SHT31 sensor not available."));
//    while (1) delay(1);
  }
  else Serial.println(F("SHT31 sensor initiated."));

  // Testing VOC sensor
  if (!SGP30_init(SGP30_addr)) {
    Serial.println(F("SGP30 sensor not available."));
    sgp_connected = false;
  }
  else {
    Serial.println(F("SGP30 sensor initiated."));  
    Serial.print("SGP30 serial #: ");
    Serial.print(sgp30_sn[0], HEX);
    Serial.print(sgp30_sn[1], HEX);
    Serial.println(sgp30_sn[2], HEX);
  }

    // OLED display intialisation
    if(!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_addr, false, false)) { // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
      Serial.println(F("SSD1306 allocation failed")); // doesn't work, method never returns false
      display_connected = false;
    }  
    if (display_connected) {
      Serial.println(F("SSD1306 display initiated."));
      // Show initial display buffer contents on the screen --
      // the library initializes this with an Adafruit splash screen.
  //    display.display();
  //    delay(2000); // Pause for 2 seconds  
      // Clear the buffer
      display.clearDisplay();  
      display.setTextColor(WHITE);
      display.setTextSize(2); // Draw 2X-scale text
      display.setCursor(0, 0);
      display.println(F("openseneca"));
      display.setTextSize(1);
      display.println(F("air quality sensing"));
      display.println(F("powered by citizen"));
      display.println(F("science"));
      display.println(); display.println();
      display.println(code_version);
      display.display();      // Show initial text
      Serial.println(F("SSD1306 display updated."));
    }

   //Testing PM sensor
    SPS30_start_measurement();
    SPS30_cleaning();
    sps30_sn = SPS30_getSerialNumber();

    // Initialise GSM
    //while (Serial1.available() > 0){ 
    if (!initSIM()) Serial.println("GSM module not available.");
    else {    
      Serial.print(F("GSM module found, IMEI: "));
      Serial.println(imei);
      if (cnum != "") {
        Serial.print(F("SIM card number: "));
        Serial.println(cnum);
      }
      else Serial.println(F("No SIM card."));
    }
    
    delay(50);
    // print whether GPS data communication is enabled or not
    if (Serial2.available() > 0) {
        Serial.println(F("GPS module found."));
    } else {
        gps_connected = false;
        Serial.println(F("GPS module not available."));
    }
 
    // open the file. Note that only one file can be open at a time,so you have to close this one before opening another.
    filename = getNextName();
    delay(50);
    Serial.print(F("Filename: ")); Serial.print(filename); Serial.println(F(".csv"));
    delay(50);
    digitalWrite(LED, LOW);
    myFile = SD.open(filename + ".csv", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
        myFile.print(code_version); myFile.print(", IMEI: "); myFile.print(imei); myFile.print(", SIM card number: "); myFile.print(cnum); myFile.print(", SPS30 SN: "); myFile.println(sps30_sn); //myFile.print(", SGP30 SN: "); Serial.print(sgp30_sn[0], HEX); Serial.print(sgp30_sn[1], HEX); Serial.print(sgp30_sn[2], HEX);
        delay(50);
        myFile.println(header);
        
        // close the file:
        myFile.close();
    } else {
        Serial.println(F("Could not write to file."));
    }

    // print the data to debug output as well
    Serial.println(header);
            
    digitalWrite(LED, HIGH);
    mtime = millis();
}

void loop() {    
    // wait until GPS is available
    while (gps_connected && Serial2.available() > 0) {
        // read & encode GPS data
        gps.encode(Serial2.read());
    }
    // check that 3 seconds have passed, since this is our data collection timeframe
    if (millis() - mtime > 1000) {
      routine();
    }
} //end loop

// FUNCTIONS

void routine() {
    // turn on LED for user feedback
    digitalWrite(LED, LOW);

    // read in battery level
    batteryOk();

    // Read PM sensor SPS30
    SPS30_reading();

    // Read temperature and humidity 
    temperature = sht31.readTemperature();
    humidity = sht31.readHumidity();
    
    // run humidity correction on VOC sensor
//    abs_hum = getAbsoluteHumidity(temperature, humidity);//2167 * humidity * 6112 / (273150 + temperature) * exp(1762 * temperature / (24312 + 10*temperature)); // [mg/m^3]
//    if (!SGP30_setHumidity(abs_hum)) Serial.println(F("SGP humidity correction failed."));
//    else Serial.println(F("SGP humidity corrected."));
   
    if (sgp_connected) SGP30_measure();
//    Serial.print("TVOC "); Serial.print(TVOC); Serial.print(" ppb\t");
//    Serial.print("eCO2 "); Serial.print(eCO2); Serial.println(" ppm");

    // optional: set baseline recorded or saved in non-volatile memory
//    if (counter%6 == 0) {
//      uint16_t TVOC_base, eCO2_base;
//      if (!SGP30_getIAQBaseline(&eCO2_base, &TVOC_base)) {
//        Serial.println("Failed to get baseline readings");
//        return;
//      }
//      Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
//      Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
//    }

    if (gps.location.isUpdated()) {
        gpsUpdated = 1;
    }
    if (String(gps.location.lat(), 6).substring(0,4) == "0.00") vsat = gps.satellites.value();
    else vsat = 10;
    
    payload = String(counter) + "," +
              String(gps.location.lat(), 6) + "," + 
              String(gps.location.lng(), 6) + "," + 
              String(gpsUpdated)+ "," + 
              String(gps.speed.kmph()) + "," + 
              String(gps.altitude.meters()) + "," + 
              String(gps.satellites.value())+ "," + 
              String(formatDate())+ "," + 
              String(formatTime())+ "," + 
              String(mtime)+ "," + 
              String(mc_1p0)+ "," + 
              String(mc_2p5)+ "," + 
              String(mc_4p0)+ "," + 
              String(mc_10p0)+ "," + 
              String(temperature)+ "," + 
              String(humidity)+ "," + 
              String(nc_0p5)+ "," + 
              String(nc_1p0)+ "," + 
              String(nc_2p5)+ "," + 
              String(nc_4p0)+ "," +
              String(nc_10p0)+ "," + 
              String(typical_particle_size)+ "," + 
              String(TVOC)+ "," + 
              String(eCO2)+ "," + 
              String(vin);
    if (imei != "") uploadSIM(payload);
    
    // open file to write sensor data
    myFile = SD.open(String(filename) + ".csv", FILE_WRITE);
    
    // if the file opened successfully, write the sensor data to it
    if (myFile) {
      myFile.println(payload);
      myFile.close();
      // turn off LED for user feedback
      digitalWrite(LED, HIGH);
    }

    // print data to serial output for debugging purposes
    delay(100);
    Serial.println(payload);

    // update data on OLED screen
    if (display_connected) {
      updateDisplay();
    }
    
    // set current time
    mtime = millis();
  
    counter++;
    gpsUpdated = 0;
}

void updateDisplay() {  
    // show on OLED
    // row 0-15 are yellow, 16-63 blue
    display.clearDisplay(); 
    #ifdef LANDSCAPE
      display.setTextSize(1); 
//      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.print(String(gps.time.hour())+F(":")+String(gps.time.minute()));
      display.setCursor(52, 0);
      display.print(String(round(humidity))+F(" %"));
      display.setCursor(100, 0);
      display.println(String(round(temperature))+F(" C"));
      display.setCursor(0, 16); //display.println(); 
      display.setTextSize(3); display.print(String(vsat).substring(0,5)); //mc_2p5
      display.setTextSize(1); display.println("ug/m3");    
      display.setCursor(0, 42);  
      display.setTextSize(3); display.print(String(TVOC).substring(0,5));
      display.setTextSize(1); display.println("ppb");    
    #else
      display.setRotation(1);
      display.setCursor(0, 0); display.setTextSize(1); display.println("PM2");
      display.setCursor(16, 0); display.setTextSize(2); display.print(String(mc_2p5).substring(0,4)); //mc_2p5
      display.setCursor(16, 16); display.setTextSize(1); display.println("ug/m3");
      
      display.setCursor(0, 28); display.setTextSize(1); display.print("VOC");
      display.setCursor(16, 28); display.setTextSize(2); display.print(String(TVOC).substring(0,4));
      display.setCursor(16, 44); display.setTextSize(1); display.println("ppb");
      
      display.setCursor(0, 60); display.setTextSize(1); display.print("CO2");
      display.setCursor(16, 60); display.setTextSize(2); display.print(String(eCO2).substring(0,4));
      display.setCursor(16, 76); display.setTextSize(1); display.println("ppm");
      
      display.setCursor(0, 88); display.setTextSize(1); display.print("T");
      display.setCursor(16, 88); display.setTextSize(2); display.print(String(round(temperature))+" C");
      display.setCursor(0, 106); display.setTextSize(1); display.print("RH");
      display.setCursor(16, 106); display.setTextSize(2); display.print(String(round(humidity))+" %");

      display.setCursor(0, 120); display.setTextSize(1); 
      for (int i=0; i<vsat; i++) display.print(".");
    #endif
    display.display();
}

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
String getNextName() {
  File root = SD.open("/");
  String output;
  if (sps30_sn == "") output = "000";
  else output = sps30_sn.substring(0, 3); // filenames longer than 6 digits are not being read so we only take 3 digits of the serial number
  int out = 100;
  while (true) {
    // check file
    File entry = root.openNextFile();

    // if opening the file doesn't return a correct file, we are done
    if (!entry) {
      break;
    }

    // check if the filename is in the correct range. If it is, increment the counter
    String temp = entry.name();
    temp = temp.substring(3 ,6);
    int num = temp.toInt();
    if (num < 999 && num >= out) {
      out = num + 1;
    }

    // close the file
    entry.close();
  }
  output = output+String(out);
  return output;
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

boolean initSIM() {
  long sim_start = millis();
  bool module_present = 0;
  
  while ( millis() - sim_start < 5000 && module_present == 0) {
      module_present = sendATcommand("AT+CFUN=1", "OK", 500);
  }
  
  if (module_present == 0) return false; 
  sendATcommand("AT+COPS=2", "OK", 2000);
  sendATcommand("AT+COPS=0", "OK", 2000);
  
  while ( millis() - sim_start < 20000 && sim_connected == 0) {
      if ( (sendATcommand("AT+CREG?", "+CREG: 1,1", 500) == 1 || sendATcommand("AT+CREG?", "+CREG: 1,5", 500) == 1 ||
           sendATcommand("AT+CREG?", "+CREG: 0,1", 500) == 1 || sendATcommand("AT+CREG?", "+CREG: 0,5", 500)) == 1 ){
            sim_connected = 1;
          } else {
            sim_connected = 0;
          }
//      Serial.println(millis() - sim_start);
//      Serial.println(sim_connected);
  }
  sendATcommand("AT+CGATT?", "+CGATT: 1", 1000);

  delay(100);
  imei = runCommand("AT+CGSN").substring(0,15);
  delay(100);
  cnum = runCommand("AT+CNUM").substring(13,27);
  
  sendATcommand("AT+CSQ", "OK", 1000);
  sendATcommand("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK", 5000);
  sendATcommand("AT+SAPBR=3,1,\"APN\",\"TM\"", "OK", 5000);
  sendATcommand("AT+SAPBR=1,1", "OK", 1000);
  sendATcommand("AT+SAPBR=2,1", "OK", 1000);
  sendATcommand("AT+HTTPINIT", "OK", 1000); 
  return true;
}

void uploadSIM(String payload) {
  runCommand("AT+HTTPPARA=\"URL\",\"http://app.open-seneca.org/php/gsmUpload.php?imei=" + imei + "&simnumber=" + cnum + "&payload=" + payload + "\"");
  if ( runCommand("AT+HTTPACTION=0") == "ERROR" && sim_connected == 1){
    initSIM();
  }
  runCommand("AT+HTTPREAD");
}
  
String runCommand(String cmd) {
//  Serial.println(cmd);
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

//  Serial.println("filtered: " + filteredResp);
  return filteredResp;
}

int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout) {
    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);    // Initialice the string    
    delay(100);
    
    while( Serial1.available() > 0) Serial1.read();    // Clean the input buffer
    if (ATcommand[0] != '\0') {
        Serial1.println(ATcommand);    // Send the AT command 
    }
    
    x = 0;
    previous = millis();

    // this loop waits for the answer
    do{
        if(Serial1.available() != 0){    // if there are data in the UART input buffer, reads it and checks for the asnwer
            response[x] = Serial1.read();
//            Serial.print(response[x]);
            x++;
            if (strstr(response, expected_answer) != NULL)    // check if the desired answer (OK) is in the response of the module
            {
                answer = 1;
            }
        }
    }while((answer == 0) && ((millis() - previous) < timeout));    // Waits for the asnwer with time out

    return answer;
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

void SetPointer(byte addr, byte P1, byte P2)
{
  Wire.beginTransmission(addr);
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
   delay(1000);
}

void SPS30_cleaning(){
  Serial.println(F("Attempting to clean SPS30 sensor..."));
  //Start fan cleaning
  SetPointer(SPS30_addr, 0x56, 0x07);
  delay(12000);
  Serial.println(F("Done!"));
  delay(50); 
}

void SPS30_reading(){
  SetPointer(SPS30_addr, 0x02, 0x02);
  Wire.requestFrom(SPS30_addr, 3);
  w1=Wire.read();
  w2=Wire.read();
  w3=Wire.read();
  
  // Read measurements
  if (w2==0x01){              //0x01: new measurements ready to read
    SetPointer(SPS30_addr, 0x03,0x00);
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
  //  SetPointer(SPS30_addr, 0x01, 0x04);
  }

String SPS30_getSerialNumber() {
  byte SN[48]; // SN is stored in 48 bytes, each representing 1 ASCII character
  SetPointer(SPS30_addr, 0xD0,0x33); // set to mode
  Wire.requestFrom(SPS30_addr, 48); // request SN
  for (int i=0; i<48; i++) SN[i]=Wire.read(); // read answer  
  String output = "";
  for (int i=0; i<48; i++) {
//    Serial.print(i); Serial.print(": "); Serial.print(SN[i]); Serial.print(" = "); Serial.println(char(SN[i]));
    if (i%3==0) { // 2 ASCII bytes are followed by 1 checksum byte
      output += char(SN[i]);
      output += char(SN[i+1]);
    }
  }
  if (output == "") Serial.println(F("SPS30 sensor not available."));
  else {
    Serial.print("SPS30 serial #: "); Serial.println(output);
  }
  return output;
}

boolean SGP30_init(uint8_t addr) {
  // Reads the serial number.
  uint8_t command[2];
  command[0] = 0x36;
  command[1] = 0x82;
  if (!readWordFromCommand(command, 2, 10, sgp30_sn, 3))
    return false;

  uint16_t featureset;
  command[0] = 0x20;
  command[1] = 0x2F;
  if (!readWordFromCommand(command, 2, 10, &featureset, 1))
    return false;
  //Serial.print("Featureset 0x"); Serial.println(featureset, HEX);
  if ((featureset & 0xF0) != SGP30_FEATURESET)
    return false;
  // Commands the sensor to begin the IAQ algorithm. Must be called after startup.
  command[0] = 0x20;
  command[1] = 0x03;
  if (!readWordFromCommand(command, 2, 10, 0, 0))
    return false;

  return true;
}

boolean SGP30_measure(void) {
  uint8_t command[2];
  command[0] = 0x20;
  command[1] = 0x08;
  uint16_t reply[2];
  if (!readWordFromCommand(command, 2, 12, reply, 2))
    return false;
  TVOC = reply[1];
  eCO2 = reply[0];
  return true;
}

boolean readWordFromCommand(uint8_t command[], uint8_t commandLength, uint16_t delayms, uint16_t *readdata, uint8_t readlen) {
  // required for i2c comm with SGP30, could be generalised in the future though
  Wire.beginTransmission(SGP30_addr);

  for (uint8_t i = 0; i < commandLength; i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission();

  delay(delayms);

  if (readlen == 0)
    return true;

  uint8_t replylen = readlen * (SGP30_WORD_LEN + 1);
  if (Wire.requestFrom(SGP30_addr, replylen) != replylen)
    return false;
  uint8_t replybuffer[replylen];
  for (uint8_t i = 0; i < replylen; i++) {
    replybuffer[i] = Wire.read();
  }

  for (uint8_t i = 0; i < readlen; i++) {
    uint8_t crc = SGP30_generateCRC(replybuffer + i * 3, 2);
    if (crc != replybuffer[i * 3 + 2])
      return false;
    // success! store it
    readdata[i] = replybuffer[i * 3];
    readdata[i] <<= 8;
    readdata[i] |= replybuffer[i * 3 + 1];
  }
  return true;
}

uint8_t SGP30_generateCRC(uint8_t *data, uint8_t datalen) {
  // calculates 8-Bit checksum with given polynomial
  uint8_t crc = SGP30_CRC8_INIT;

  for (uint8_t i = 0; i < datalen; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ SGP30_CRC8_POLYNOMIAL;
      else
        crc <<= 1;
    }
  }
  return crc;
}

/*!
 *   @brief  Request baseline calibration values for both CO2 and TVOC IAQ
 *           calculations. Places results in parameter memory locaitons.
 *   @param  eco2_base 
 *           A pointer to a uint16_t which we will save the calibration
 *           value to
 *   @param  tvoc_base 
 *           A pointer to a uint16_t which we will save the calibration value to
 *   @return True if command completed successfully, false if something went
 *           wrong!
 */
boolean SGP30_getIAQBaseline(uint16_t *eco2_base, uint16_t *tvoc_base) {
  uint8_t command[2];
  command[0] = 0x20;
  command[1] = 0x15;
  uint16_t reply[2];
  if (!readWordFromCommand(command, 2, 10, reply, 2))
    return false;
  *eco2_base = reply[0];
  *tvoc_base = reply[1];
  return true;
}

/*!
 *  @brief  Assign baseline calibration values for both CO2 and TVOC IAQ
 *          calculations.
 *  @param  eco2_base 
 *          A uint16_t which we will save the calibration value from
 *  @param  tvoc_base 
 *          A uint16_t which we will save the calibration value from
 *  @return True if command completed successfully, false if something went
 *          wrong!
 */
boolean SGP30_setIAQBaseline(uint16_t eco2_base, uint16_t tvoc_base) {
  uint8_t command[8];
  command[0] = 0x20;
  command[1] = 0x1e;
  command[2] = tvoc_base >> 8;
  command[3] = tvoc_base & 0xFF;
  command[4] = SGP30_generateCRC(command + 2, 2);
  command[5] = eco2_base >> 8;
  command[6] = eco2_base & 0xFF;
  command[7] = SGP30_generateCRC(command + 5, 2);

  return readWordFromCommand(command, 8, 10, 0, 0);
}

/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t getAbsoluteHumidity(float temperature, float humidity) { // doesn't work here yet due to float precision / memory error (?)
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    return absoluteHumidityScaled;
}

/*!
 *  @brief  Set the absolute humidity value [mg/m^3] for compensation to increase
 *          precision of TVOC and eCO2.
 *  @param  absolute_humidity 
 *          A uint32_t [mg/m^3] which we will be used for compensation.
 *          If the absolute humidity is set to zero, humidity compensation
 *          will be disabled.
 *  @return True if command completed successfully, false if something went
 *          wrong!
 */
boolean SGP30_setHumidity(uint32_t absolute_humidity) {
  if (absolute_humidity > 256000) {
    return false;
  }

  uint16_t ah_scaled = (uint16_t)(((uint64_t)absolute_humidity * 256 * 16777) >> 24);
  uint8_t command[5];
  command[0] = 0x20;
  command[1] = 0x61;
  command[2] = ah_scaled >> 8;
  command[3] = ah_scaled & 0xFF;
  command[4] = SGP30_generateCRC(command + 2, 2);

  return readWordFromCommand(command, 5, 10, 0, 0);
}

// encode a 28 digit number String into a 12 byte ASCII string
// this function was used to encode more data using fewer bytes
// this function is no longer needed
//String encodeMsg28(String msg) {
//    if (msg.length() != 28) {
//        String ret = "" + msg.length();
//        return ret;
//    }
//
//    unsigned char bytes[3];
//    String out = "";
//
//    for (int j = 0; j < 4; j++)
//    {
//        String temp = "";
//        for (int i = 0; i < 7; i++)
//            temp += msg[j * 7 + i];
//        unsigned long int t = temp.toInt();
//
//        bytes[0] = (t >> 16);
//        bytes[1] = (t >> 8);
//        bytes[2] = t;
//        //out += (unsigned char) bytes[0] + (unsigned char) bytes[1] + (unsigned char) bytes[2];
//        out += (char) bytes[0];
//        out += (char) bytes[1];
//        out += (char) bytes[2];
//    }
//    return out;
//}

//String trimGPS(double lati) { // returns longitude/latitude trimmed to 5 relevant digits
//  int temp = ((int)(lati*100000))%100000; // also works but might cause loss of precision: =(int)((lati-(int)lati)*100000);
//  String out = (String)temp;
//  temp = 5-out.length(); // fill up with leading zeroes if necessary
//  for (int i=0; i<temp; i++) out="0"+out;
//  return out;
//}
