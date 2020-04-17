/*
 ESP-NOW based sensor 

 Sends readings to an ESP-Now server with a fixed mac address

 Anthony Elder
 License: Apache License v2
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ClosedCube_SHT31D.h>

// bool SERIAL_ENABLED = false;
bool SERIAL_ENABLED = true;

extern "C" 
{
  #include <espnow.h>
}

struct __attribute__((packed)) SENSOR_DATA 
{
  float temp;
  float humi;
  uint16_t batV;
} sensorData;

uint8_t remoteMac[] = {0x3C, 0x71, 0xBF, 0x32, 0xC5, 0xDD};

#define WIFI_CHANNEL 1
//#define SLEEP_SECS 15 * 60 // 15 minutes
#define SLEEP_SECS 15*60   // 15 minutes
#define SEND_TIMEOUT 300  // 100 milliseconds timeout 

// 1v max on ADC. 10 bit ADC, voltage divider with 390k and 100k R1,R2
const float cal_factor = 1.0;
const float mv_per_adc = ((1000.0 / 1024.0) / (10.0/(39.0+10.0))) * cal_factor;

// Prototypes:
void esp_send();
void send_callback(uint8_t mac[], uint8_t sendStatus);
void please_sleep();