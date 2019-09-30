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
int oneshot = false;

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
  int8_t scaleTemperature;
  uint16_t bmpPressure;
  int8_t bmpTemperature;
  uint8_t lastMessageStatus;
} SigfoxMessage;

// stub for message which will be sent
SigfoxMessage msg;

bool stringComplete;
char inChar;
String scaleReading;
int messageCount = 0;

void setup() {

  //if (oneshot == true) {
  // Wait for the serial
  Serial.begin(115200);
  while (!Serial) {}
  //}

  if (!SigFox.begin()) {
    // Something is really wrong, try rebooting
    // Reboot is useful if we are powering the board using an unreliable power source
    // (eg. solar panels or other energy harvesting methods)
    reboot();
  }

  //Send module to standby until we need to send a message
  SigFox.end();

  // Enable debug prints and LED indication if we are testing
  SigFox.debug();

  // Configure the sensors and populate the status field
  // DHT22
  dht.begin();
  Serial.println("DHT OK");

  if (!bmp.begin()) {
    msg.status |= STATUS_BMP_PRS_KO;
    msg.status |= STATUS_BMP_TMP_KO;
  }
  else {
    Serial.println("BMP OK");
  }

  //BMP280
  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  //OpenScale
  //Readings from OpenScale are being sent over serial1
  Serial1.begin(9600);
  // Wait for Serial1
  while (!Serial1) {
    delay(50);
    Serial.print("o");
  }

  //skip headers sent by serial by OpenScale
  for (int i = 1; i <= 8; i++) {
    while (!Serial1.available()) {
      delay(50);
    }
    stringComplete = false;
    while (Serial1.available() && stringComplete == false) {
      inChar = (char)Serial1.read();
      scaleReading += inChar;
      if (inChar == '\n') {
        stringComplete = true;
        Serial.println(scaleReading);
        scaleReading="";
      }
    }
  }
  Serial.println("Scale OK");
}

void loop() {
  // Every 15 minutes, read all the sensors and send data
  // Floats converted to bytes

  messageCount++;
  Serial.println("Message Nr.: " + String(messageCount));

  //Readings
  //DHT22
  float dhtHumidity = dht.readHumidity();
  float dhtTemperature = dht.readTemperature();

  //BMP280
  float bmpPressure = bmp.readPressure();
  float bmpTemperature = bmp.readTemperature();

  //OpenScale
  String scaleReading = "";
  float scaleWeight;
  float scaleTemperature;

  //Wait for message
  //In case of timeout error, send a dummy message.
  int timeOutCounter = 0;
  stringComplete = false;

  while (stringComplete == false) {
    if (Serial1.available()) {
      inChar = (char)Serial1.read();
      scaleReading += inChar;
      if (inChar == '\n') {
        stringComplete = true;
      }
    }
    else {
      delay(50);
      timeOutCounter++;
      if (timeOutCounter > 1000) {
        Serial.println("Timeout Error Reading from OpenScale.");
        scaleReading = scaleReading + "0,0,0,0,0";
        stringComplete = true;
        break;
      }
    }
  }

  Serial.println(scaleReading);//Po odladění smazat!
  int ind1 = scaleReading.indexOf(",");
  int ind2 = scaleReading.indexOf(",", ind1 + 1);
  int ind3 = scaleReading.indexOf(",", ind2 + 1);
  int ind4 = scaleReading.indexOf(",", ind3 + 1);
  Serial.println(String(ind1));
  Serial.println(String(ind2));
  Serial.println(String(ind3));
  Serial.println(String(ind4));

  scaleWeight = scaleReading.substring(ind1 + 1, ind2).toFloat();
  scaleTemperature = scaleReading.substring(ind3 + 1, ind4).toFloat();


  //Conversions
  msg.dhtHumidity = convertoFloatToUInt16(dhtHumidity, 100);
  msg.dhtTemperature = convertoFloatToInt8(dhtTemperature, 100, -100);
  msg.bmpPressure = convertoFloatToUInt16(bmpPressure, 200000);
  msg.bmpTemperature = convertoFloatToInt8(bmpTemperature, 100, -100);
  msg.scaleWeight = convertoFloatToInt16(scaleWeight, 100, -100);
  msg.scaleTemperature = convertoFloatToInt8(scaleTemperature, 100, -100);

  // Start the module
  SigFox.begin();
  // Wait at least 30ms after first configuration (100ms before)
  delay(100);

  // We can only read the module temperature before SigFox.end()
  float moduleTemperature = SigFox.internalTemperature();
  msg.moduleTemperature = convertoFloatToInt8(moduleTemperature, 100, -100);

  Serial.println("Sigfox temperature: " + String(moduleTemperature));
  Serial.println("DHT temperature: " + String(dhtTemperature));
  Serial.println("DHT humidity: " + String(dhtHumidity));
  Serial.println("Weight: " + String(scaleWeight));
  Serial.println("OpenScale temperature: " + String(scaleTemperature));

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

void reboot() {
  NVIC_SystemReset();
  while (1);
}
