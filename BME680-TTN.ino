
#include <Arduino.h>
#include "bsec.h"

#define USE_DISPLAY
#ifdef USE_DISPLAY
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <Adafruit_NeoPixel.h>
#define LED_PIN 23
#define LED_COUNT 1
#define LEDALWAYSON
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);
#endif

#include <SimpleLMIC.h>
#include "TTN-configuration.h"
SimpleLMIC ttn;
int TTNDebugLoopInterval = 1000;
uint32_t TTNDebugLoopMillis = millis() - TTNDebugLoopInterval;

int TTNSendLoopInterval = 60*1000;
uint32_t TTNSendLoopMillis = millis() - TTNSendLoopInterval;

int DisplayUpdateLoopInterval = 1000;
uint32_t DisplayUpdateLoopMillis = millis() - DisplayUpdateLoopInterval;
int NeoPixelUpdateLoopInterval = 1000;
uint32_t NeoPixelUpdateLoopMillis = millis() - NeoPixelUpdateLoopInterval;

// Create an object of the class Bsec
Bsec iaqSensor;

String output;

// Entry point for the example
void setup(void)
{
  Serial.begin(115200);
  Wire.begin();

  iaqSensor.begin(BME680_I2C_ADDR_PRIMARY, Wire);
  output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  
#ifdef USE_DISPLAY
  setupDisplay();
#endif

  // Print the header
  output = "Timestamp [ms], raw temperature [°C], pressure [hPa], raw relative humidity [%], gas [Ohm], IAQ, IAQ accuracy, temperature [°C], relative humidity [%], Static IAQ, CO2 equivalent, breath VOC equivalent";
  Serial.println(output);

  strip.begin();
  strip.show();
  strip.setBrightness(50);

  ttn.begin();
  ttn.setSubBand(2);
  ttn.onMessage(message);
  ttn.join(devEui, appEui, appKey);
}

// Function that is looped forever
void loop(void)
{
  ttn.loop();
  unsigned long time_trigger = millis();
  if (iaqSensor.run()) { // If new data is available
    output = String(time_trigger);
    output += ", " + String(iaqSensor.rawTemperature);
    output += ", " + String(iaqSensor.pressure);
    output += ", " + String(iaqSensor.rawHumidity);
    output += ", " + String(iaqSensor.gasResistance);
    output += ", " + String(iaqSensor.iaq);
    output += ", " + String(iaqSensor.iaqAccuracy);
    output += ", " + String(iaqSensor.temperature);
    output += ", " + String(iaqSensor.humidity);
    output += ", " + String(iaqSensor.staticIaq);
    output += ", " + String(iaqSensor.co2Equivalent);
    output += ", " + String(iaqSensor.breathVocEquivalent);
    Serial.println(output);
  } else {
    checkIaqSensorStatus();
  }
#ifdef USE_DISPLAY
  updateDisplay();
#endif
  updateNeoPixel();

  if (!ttn.isBusy() && ttn.isLink() && millis() - TTNSendLoopMillis > TTNSendLoopInterval)
  {
    TTNSendLoopMillis = millis();
    Serial.println("Not Busy!");
    Serial.println(iaqSensor.co2Equivalent);
    Serial.println(sizeof(iaqSensor.co2Equivalent));
    uint8_t buffer[sizeof(iaqSensor.co2Equivalent)];
    ::memcpy(buffer, &iaqSensor.co2Equivalent, sizeof(iaqSensor.co2Equivalent));
    ttn.write(buffer, sizeof(buffer));
    ttn.send();
  }
}

// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
}

void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

#ifdef USE_DISPLAY
void setupDisplay() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_courB08_tr);	// choose a suitable font
  updateDisplay();
}

void updateDisplay() {
  if(millis() - DisplayUpdateLoopMillis > DisplayUpdateLoopInterval) {
    DisplayUpdateLoopMillis = millis();
    char displayText[256];
    u8g2.clearBuffer();

    char tempString[10];
    dtostrf(iaqSensor.temperature, 4, 2, tempString);
    sprintf(displayText, "Temp: %s", tempString);
    u8g2.drawStr(0,10,displayText);

    char humidString[10];
    dtostrf(iaqSensor.humidity, 4, 2, humidString);
    sprintf(displayText, "Humidity: %s", humidString);
    u8g2.drawStr(0,20,displayText);

    char co2String[10];
    dtostrf(iaqSensor.co2Equivalent, 4, 2, co2String);
    sprintf(displayText, "CO2-Equivalent: %s", "");
    u8g2.drawStr(0,30,displayText);
    sprintf(displayText, "%20s", co2String);
    u8g2.drawStr(0,40,displayText);

    sprintf(displayText, "ttn.isBusy(): %d", ttn.isBusy());
    u8g2.drawStr(0,50,displayText);

    sprintf(displayText, "ttn.isLink(): %d", ttn.isLink());
    u8g2.drawStr(0,60,displayText);

    u8g2.sendBuffer();
  }
}
#endif

void updateNeoPixel(){
  if(millis() - NeoPixelUpdateLoopMillis > NeoPixelUpdateLoopInterval) {
    NeoPixelUpdateLoopMillis = millis();
    double warningValue = 800;
    double criticalValue = 1500;
    double steps = 2000/strip.numPixels();
    float co2 = iaqSensor.co2Equivalent;
    for(int i=0; i<strip.numPixels(); i++) {
      #ifndef LEDALWAYSON
      if(co2 >= steps*(i+1)) {
      #endif
        if(co2 >= criticalValue) strip.setPixelColor(i, 255, 0, 0);
        else if(co2 >= warningValue) strip.setPixelColor(i, 255,   165,   0);
        else if(co2 < warningValue) strip.setPixelColor(i, 0,   255,   0);
      #ifndef LEDALWAYSON
      }
      else {
        strip.setPixelColor(i, 0,   0,   0);
      }
      #endif
      strip.show();
    }
  }
}

void printTTNDebug() {
  Serial.print("ttn.isBusy(): ");
  Serial.print(ttn.isBusy());
  Serial.print(" ttn.isLink(): ");
  Serial.print(ttn.isLink());

  Serial.println();
}

void message(uint8_t *payload, size_t size, uint8_t port)
{
  Serial.println("Received " + String(size) + " bytes on port " + String(port));
  switch (port) {
    case 1:
      break;
  }
}
