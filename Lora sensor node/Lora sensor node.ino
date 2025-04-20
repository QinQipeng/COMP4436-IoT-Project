#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <ArduinoJson.h>
#include <Wire.h>
#include "Adafruit_SHT4x.h"

// === Constants & Definitions ===
#define SIZE 200
#define TX_TIMEOUT 10000
#define V_EXT GPIO6
#define SOIL ADC
// #define LIGHT ADC
#define TURN_ON GPIO0

// === LoRaWAN Credentials (OTAA - Light Sensor) ===
// uint8_t devEui[] = {0x54, 0xAF, 0x45, 0x67, 0xD0, 0x63, 0x7F, 0xD9}; //Class A
// uint8_t appEui[] = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
// uint8_t appKey[] = {0xC9, 0x7A, 0x34, 0x6A, 0x4E, 0xC4, 0xA3, 0xD4, 0xAB, 0x3A, 0x93, 0xF3, 0x72, 0x0E, 0x84, 0x7D}; //Class A

// === LoRaWAN Credentials (OTAA - Soil Sensor) ===
uint8_t devEui[]  = {0x2C, 0x4D, 0xC9, 0x53, 0xD4, 0x8F, 0x54, 0x18};
uint8_t appEui[]  = {0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77};
uint8_t appKey[]  = {0x56, 0xC3, 0xC1, 0x80, 0xFA, 0x89, 0x7F, 0x95, 0x3C, 0x28, 0x94, 0x9F, 0x04, 0x97, 0x2D, 0xB8};

// === LoRaWAN Config ===
uint8_t appPort = 2;
bool isTxConfirmed = LORAWAN_UPLINKMODE;
bool overTheAirActivation = LORAWAN_NETMODE;
bool loraWanAdr = LORAWAN_ADR;
bool keepNet = LORAWAN_NET_RESERVE;
DeviceClass_t loraWanClass = LORAWAN_CLASS;
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
uint32_t appTxDutyCycle = 30000;
uint8_t confirmedNbTrials = 4;
uint16_t userChannelsMask[6] = { 0x00FF, 0, 0, 0, 0, 0 };

// === Sensor Setup ===
Adafruit_SHT4x sht4;
StaticJsonDocument<100> jsData;

// === Globals ===
char msg[SIZE];
uint64_t current_time = 0, previous_time = 0;
unsigned long msg_count = 0;
bool tx_done = false;
byte retrial = 0;

// === Soil Moisture Reference Values ===
const int AirValue = 4095;
const int WaterValue = 2090;

// === Utility Functions ===
const char* soil_level(int val) {
  static char level[16];
  int interval = (AirValue - WaterValue) / 3;

  if (val < WaterValue + interval)          strcpy(level, "Very Wet");
  else if (val < AirValue - interval)       strcpy(level, "Wet");
  else if (val <= AirValue)                 strcpy(level, "Dry");
  else                                      strcpy(level, "Unknown");

  return level;
}

void create_data() {
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);

  float temp_val = round(temp.temperature * 100) / 100.0;
  float hum_val  = round(humidity.relative_humidity * 100) / 100.0;
  int soil_val   = analogRead(SOIL);
    // int light_val   = analogRead(LIGHT);

  jsData["node-ID"]   = "07";
  jsData["Temp"]      = temp_val;
  jsData["Hum"]       = hum_val;
  jsData["Soil"]      = soil_val;
  // jsData["Light"]     = soil_val;
  jsData["Soil Msg"]  = soil_level(soil_val);

  serializeJson(jsData, msg);
  Serial.println(msg);
}

static void prepareTxFrame(uint8_t port) {
  create_data();
  appDataSize = strlen(msg);
  memcpy(appData, msg, appDataSize);
}

// === Arduino Setup & Loop ===
void setup() {
  pinMode(TURN_ON, OUTPUT);
  pinMode(V_EXT, OUTPUT);
  digitalWrite(V_EXT, LOW);
  digitalWrite(TURN_ON, HIGH);
  Serial.begin(115200);

  Serial.println("Wakeup, Hello world!");
  Wire.begin();

  if (!sht4.begin()) {
    Serial.println("Can't find SHT4x");
    while (true) delay(1);
  }

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  deviceState = DEVICE_STATE_INIT;
}

void loop() {
  switch (deviceState) {
    case DEVICE_STATE_INIT:
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
#if(AT_SUPPORT)
      getDevParam();
#endif
      printDevParam();
      LoRaWAN.init(loraWanClass, loraWanRegion);
      deviceState = DEVICE_STATE_JOIN;
      break;

    case DEVICE_STATE_JOIN:
      LoRaWAN.join();
      break;

    case DEVICE_STATE_SEND:
      retrial = 0;
      msg_count++;
      previous_time = millis();
      Serial.println("Sending");

      prepareTxFrame(appPort);
      LoRaWAN.send();

      while (!tx_done) {
        current_time = millis();
        if (current_time - previous_time > TX_TIMEOUT) {
          if (++retrial > 4) {
            Serial.println("Transmission error! Please check the gateway!!");
            break;
          }
          Serial.println("Resending! Attempt: " + String(retrial));
          prepareTxFrame(appPort);
          LoRaWAN.send();
          previous_time = millis();
        }
      }

      deviceState = DEVICE_STATE_CYCLE;
      tx_done = false;
      break;

    case DEVICE_STATE_CYCLE:
      txDutyCycleTime = appTxDutyCycle;
      LoRaWAN.cycle(txDutyCycleTime);
      Serial.println("Going to Sleep!");
      Serial.flush();
      deviceState = DEVICE_STATE_SLEEP;
      break;

    case DEVICE_STATE_SLEEP:
      LoRaWAN.sleep();
      break;

    default:
      deviceState = DEVICE_STATE_INIT;
      break;
  }
}
