/*
  SigFox Bee Project

  Reads sensor outputs and sends the data using Sigfox technology.

  Hardware:
  Microcontroller:
  - MKR FOX 1200 Arduino module with Sigfox functionality
  
  Sensors
  - Bosche Load Cell H40A 200 kg connected over Sparkfun OpenScale, which has an integrated temperature sensor 
  TMP 102
  https://www.bosche.eu/en/scale-components/load-cells/single-point-load-cell/single-point-load-cell-h40a
  http://www.ti.com/lit/ds/symlink/tmp102.pdf
  https://www.sparkfun.com/products/13261
  - Temperature and humidity sensor DHT22
  - MKR FOX 1200 onboard temperature sensor
  - Bosch BMP280 barometric pressure sensor and temperature sensor, connected over I2C

  Since the Sigfox network can send a maximum of 120 messages per day (depending on your plan)
  we'll optimize the readings and send data in compact binary format
*/

#include <ArduinoLowPower.h>
#include <SigFox.h>
#include <Adafruit_BMP280.h>
#include "conversions.h"
#include "DHT.h"

// Set oneshot to false to trigger continuous mode when you finisched setting up the whole flow
int oneshot = true;

#define STATUS_OK     0
#define STATUS_DHT_HUM_KO 1
#define STATUS_DHT_TMP_KO 2
#define STATUS_SCALE_WT_KO 4
#define STATUS_SCALE_TMP_KO 8
#define STATUS_BMP_PRS_KO 16
#define STATUS_BMP_TMP_KO 32

#define DHTPIN 2
#define DHTTYPE DHT22 

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280  bmp;

/*
    ATTENTION - the structure we are going to send MUST
    be declared "packed" otherwise we'll get padding mismatch
    on the sent data - see http://www.catb.org/esr/structure-packing/#_structure_alignment_and_padding
    for more details
*/
typedef struct __attribute__ ((packed)) sigfox_message {
  uint8_t status; //unsigned fixed length 8 bit
  int8_t moduleTemperature; //signed fixed length 8 bit
  int8_t dhtTemperature;
  uint16_t dhtHumidity;
  uint16_t scaleWeight;
  uint8_t scaleTemperature;
  uint16_t bmpPressure;
  uint8_t bmpTemperature;
  uint8_t lastMessageStatus;
} SigfoxMessage;

// stub for message which will be sent
SigfoxMessage msg;

void setup() {

  if (oneshot == true) {
    // Wait for the serial
    Serial.begin(9600);
    while (!Serial) {}
  }

  if (!SigFox.begin()) {
    // Something is really wrong, try rebooting
    // Reboot is useful if we are powering the board using an unreliable power source
    // (eg. solar panels or other energy harvesting methods)
    reboot();
  }

  //Send module to standby until we need to send a message
  SigFox.end();

  if (oneshot == true) {
    // Enable debug prints and LED indication if we are testing
    SigFox.debug();
  }

  // Configure the sensors and populate the status field
  dht.begin();
  //if (!dht.begin()){
  //  msg.status |= STATUS_DHT_HUM_KO;
  //  msg.status |= STATUS_DHT_TMP_KO;
  //} else {
    if (oneshot == true) {
      Serial.println("DHT OK");
  //  }
  }

  if (!bmp.begin()){
    msg.status |= STATUS_BMP_PRS_KO;
    msg.status |= STATUS_BMP_TMP_KO;
  } else {
    if (oneshot == true) {
       Serial.println("BMP OK");
    }
  }

  Serial1.begin(9600);
  
  //if (!Serial1.begin(9600)){
  //  msg.status |= STATUS_SCALE_WT_KO;
  //  msg.status |= STATUS_SCALE_TMP_KO;
  //} else {
  
  bool stringComplete=false;
  char inChar;
  String scaleReading;
  
  //skip headers sent by serial by OpenScale
    for (int i=1; i<=8; i++){
      delay(500);
      while (Serial1.available() && stringComplete==false){
        inChar=(char)Serial1.read();
        scaleReading+=inChar;
        if (inChar=='\n'){
          stringComplete=true;
          Serial.println(scaleReading);
        }
      }
    }
    if (oneshot==true){
      Serial.println("Scale OK");
    }
  }
//}
 
int messageCount=0;

void loop() {
  // Every 15 minutes, read all the sensors and send data
  // Let's try to optimize the data format
  // Only use floats as intermediate representaion, don't send them directly

  messageCount=messageCount+1;
  
  //DHT22
  float dhtHumidity = dht.readHumidity();
  msg.dhtHumidity = convertoFloatToUInt16(dhtHumidity, 100);
  float dhtTemperature = dht.readTemperature();
  msg.dhtTemperature = convertoFloatToInt8(dhtTemperature, 60, -60);

  //BMP280
  float bmpPressure = bmp.readPressure();
  msg.bmpPressure = convertoFloatToUInt16(bmpPressure, 200000);
  float bmpTemperature = bmp.readTemperature();
  msg.bmpTemperature = convertoFloatToInt8(bmpTemperature, 60, -60);

  //OpenScale
  String scaleReading;
  float scaleWeight;
  float scaleTemperature;
  char inChar;
  bool stringComplete = false;  // whether the string is complete

  delay(2000);
  while (Serial1.available() && stringComplete==false){
    inChar=(char)Serial1.read();
    scaleReading+=inChar;
    if (inChar=='\n'){
      stringComplete=true;
    }
  }
  Serial.println(scaleReading);//Po odladění smazat!
  int ind1=scaleReading.indexOf(",");
  int ind2=scaleReading.indexOf(",",ind1+1);
  int ind3=scaleReading.indexOf(",",ind2+1);
  int ind4=scaleReading.indexOf(",",ind3+1);
  int ind5=scaleReading.indexOf(",",ind4+1);
  Serial.println(String(ind1));
  Serial.println(String(ind2));
  Serial.println(String(ind3));
  Serial.println(String(ind4));
  scaleWeight=scaleReading.substring(ind1+1,ind2).toFloat();
  msg.scaleWeight=convertoFloatToInt16(scaleWeight,-100,100);
    
  scaleTemperature=scaleReading.substring(ind3+1,ind4).toFloat();
  msg.scaleTemperature=convertoFloatToInt8(scaleTemperature,-60,60);

  // Start the module
  SigFox.begin();
  // Wait at least 30ms after first configuration (100ms before)
  delay(100);

  // We can only read the module temperature before SigFox.end()
  float moduleTemperature = SigFox.internalTemperature();
  msg.moduleTemperature = convertoFloatToInt8(moduleTemperature, 60, -60);

  if (oneshot==true){
    Serial.println("Sigfox temperature: " + String(moduleTemperature));
    Serial.println("DHT temperature: " + String(dhtTemperature));
    Serial.println("DHT humidity: " + String(dhtHumidity));
    Serial.println("Weight: " + String(scaleWeight));
    Serial.println("OpenScale temperature: " + String(scaleTemperature));
  }
  // Clears all pending interrupts
  SigFox.status();
  delay(1);

  SigFox.beginPacket();
  SigFox.write((uint8_t*)&msg, 12);

  msg.lastMessageStatus = SigFox.endPacket();

  Serial.println("Status: " + String(msg.lastMessageStatus));
  SigFox.end();

  if (oneshot == true) {
    // spin forever, so we can test that the backend is behaving correctly
    while (1) {}
  }

  //Sleep for 15 minutes
  LowPower.sleep(15 * 60 * 1000);
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/

void reboot() {
  NVIC_SystemReset();
  while (1);
}
