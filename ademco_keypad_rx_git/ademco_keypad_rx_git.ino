//Some codes were authored by me and I didn't remember all licensing info for codes I pulled. Probably MIT and 
//others
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
//#include <ArduinoJson.h>
#include <TelnetStream.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <Arduino.h>

#include "OTA.h"

WiFiClient client;

WiFiUDP ntpUDP;

// Telnet server port
#define TELNET_PORT 23

//Channel ID and API key to thingspeak test 2 channel
#define espchip_id "NodeMCU-32"

//
// Pin definitions
#define SERIAL_IN_PIN 16 // Change to the GPIO pin used for input
// Baud rate is 2300 -> 435us. After sweeping, error free reception seems to be 430-436. Use medium.
#define BIT_TIME_US 433  
volatile uint8_t dataBuffer[8]; // Buffer to store captured bytes
volatile bool startConditionDetected = false;
volatile int bitIndex = 0;

const char* ssid = "xxxxx";
const char* password = "xxxxx";

uint16_t counter_daily;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  ArduinoOTA.setHostname(espchip_id);
  setup_OTA();
  Serial.println("Ready for OTA:");
  Serial.println(WiFi.localIP());  
  delay(1000);
  if (MDNS.begin(espchip_id)) {
    Serial.println("mDNS ok");
  } else {
    Serial.println("Error MDNS responder!");
  }

  // Start Telnet server
  TelnetStream.begin();
  // Register Telnet server
  MDNS.addService("telnet", "tcp", TELNET_PORT);

  pinMode(SERIAL_IN_PIN, INPUT_PULLUP);  // Set data pin as input
  Serial.println("ESP Serial Capture Started");
//
}

void loop() {
  ArduinoOTA.handle();

  waitForStartCondition();  
  if (startConditionDetected) {
    delayMicroseconds(BIT_TIME_US/2); //Sample data near the center of each bit time
    captureData();
    startConditionDetected = false; // Reset detection flag
  }
}

void waitForStartCondition() {
    // Wait for 0-to-1 transition
    while (digitalRead(SERIAL_IN_PIN) == 1);
    while (digitalRead(SERIAL_IN_PIN) == 0);
//    Serial.println("1 to 0 detected");
    delayMicroseconds(BIT_TIME_US/2); //trying to sample near the center of each bit time
    if (detectStartPattern()) {
//        Serial.println("Start condition detected!");
        startConditionDetected = true;
    }
}

bool detectStartPattern() {
  int onesCount = 0, zerosCount = 0;
// Sample 14 consecutive ones
  for (int i = 0; i < 14; i++) {
    if (digitalRead(SERIAL_IN_PIN) == 1) {
      onesCount++;
    } else { 
        return false; 
      }
    delayMicroseconds(BIT_TIME_US);    
  }    
// Sample 14 consecutive zeros
  for (int i = 0; i < 14; i++) {
    if (digitalRead(SERIAL_IN_PIN) == 0) {
      zerosCount++;
    } else { 
        return false; 
      }
    delayMicroseconds(BIT_TIME_US);  
  }

  while (digitalRead(SERIAL_IN_PIN) == 1); //skipping the variable bit time interval
  return true;
}

int flag = 0;
void captureData() {
  for (int byteIdx = 0; byteIdx < 8; byteIdx++) {
  uint8_t byteData = 0;
//    digitalWrite(2,flag);  
    for (int bit = 7; bit > -1; bit--) {
      byteData |= (digitalRead(SERIAL_IN_PIN) << bit);
      delayMicroseconds(BIT_TIME_US);          
    }        
//    digitalWrite(2,0);  
    dataBuffer[byteIdx] = ~byteData; //inverting a byte, bitwise
//    flag = !flag;  
    }    
    for (int i = 0; i < 8; i++) {
      Serial.printf("Byte %d: 0x%02X\n", i+1, dataBuffer[i]);
    }
    Serial.println("Done");
}

