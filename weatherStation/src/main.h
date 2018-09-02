#pragma once

#include <Arduino.h>
#include <ESP_Wunderground_PWS.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ClosedCube_SHT31D.h>

#include "credentials.h" 

extern "C" 
{
  #include <espnow.h>
  #include <user_interface.h>
}

const uint16_t wifi_timeout = 30000;
uint8_t mac[] = {0x0A, 0x00, 0x00, 0x00, 0x00, 0x01};

struct __attribute__((packed)) SENSOR_DATA 
{
  float temp;
  float humi;
  uint16_t batV;
} sensorData;

uint8_t mac_arr[6];
uint16_t last_status = 0;
uint32_t count = 0;

bool esp_now_info_sent = false;
bool package_recieved = false;

// prototypes

void recieve_callback(uint8_t *mac, uint8_t *data, uint8_t len);
int8_t connect_wifi();
void init_esp_now();
void print_package();