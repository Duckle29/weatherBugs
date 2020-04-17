#pragma once

#include <Arduino.h>
#include <ESP_Wunderground_PWS.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ClosedCube_SHT31D.h>
#include "SSD1306Wire.h"


#include "credentials.h" 

extern "C" 
{
  #include <espnow.h>
  #include <user_interface.h>
}

const bool wg_enable = false;

ClosedCube_SHT31D sht31;
SSD1306Wire  display(0x3C, D2, D1);

const uint16_t wifi_timeout = 30000;

// Setting the MAC address seems to have broken, so using actual unique MAC of the station
// uint8_t mac[] = {0x0A, 0x00, 0x00, 0x00, 0x00, 0x01};

struct __attribute__((packed)) SENSOR_DATA 
{
  float temp;
  float humi;
  uint16_t batV;
};

SENSOR_DATA sensorData;
SENSOR_DATA localSensorData;

uint8_t mac_arr[6];
uint16_t last_status = 0;
uint32_t count = 0;
const uint32_t sample_time = 5*60*1000; // Sample every 5 minutes
uint32_t last_sample = 0 - sample_time; // Sample 

bool esp_now_info_sent = false;
bool package_recieved = false;

// prototypes

void recieve_callback(uint8_t *mac, uint8_t *data, uint8_t len);
int8_t connect_wifi();
void init_esp_now();
void print_package();
void sample_local();
void display_data();