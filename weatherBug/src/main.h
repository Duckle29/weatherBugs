/*
 ESP-NOW based sensor 

 Sends readings to an ESP-Now server with a fixed mac address

 Anthony Elder
 License: Apache License v2
*/
#include <Arduino.h>
#include <Wire.h>
#include <ClosedCube_SHT31D.h>

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


uint8_t remoteMac[] = {0x0A, 0x00, 0x00, 0x00, 0x00, 0x01};

#define WIFI_CHANNEL 1
//#define SLEEP_SECS 15 * 60 // 15 minutes
#define SLEEP_SECS 15*60   // 15 minutes
#define SEND_TIMEOUT 300  // 100 milliseconds timeout 

const float mv_per_adc = 8.84;

// Prototypes:
void esp_send();
void send_callback(uint8_t mac[], uint8_t sendStatus);
void please_sleep();