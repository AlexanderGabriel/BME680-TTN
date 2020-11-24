const char *devEui = "";
const char *appEui = "";
const char *appKey = "";

//config for Arduino Due, GPS connected to Serial2... Don't ask...
const lmic_pinmap lmic_pins = {
   .nss = 10,
   .rxtx = LMIC_UNUSED_PIN,
   .rst = LMIC_UNUSED_PIN,
   .dio = {2, 3, LMIC_UNUSED_PIN},
};
#define GPSSerial Serial2

//Config for TTGO T-Beam T22 v1.1
const lmic_pinmap lmic_pins = {
   .nss = 18,
   .rxtx = LMIC_UNUSED_PIN,
   .rst = 23,
   .dio = {26, 33, 32},
};
#define GPSSerial Serial1
//#define GPS_RX_PIN 34
//#define GPS_TX_PIN 12

//Heltec Lora 32 v2
const lmic_pinmap lmic_pins = {
   .nss = 18,
   .rxtx = LMIC_UNUSED_PIN,
   .rst = 14,
   .dio = {26, 34, 35},
};