#pragma once

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <time.h>
#include <simpleDSTadjust.h>

#include <ESP_Wunderground_PWS.h>

#include <Wire.h>
#include <ClosedCube_SHT31D.h>
#include "SSD1306Wire.h"

#include "credentials.h" 

extern "C" 
{
  #include <espnow.h>
  #include <user_interface.h>
}

#define WIFI_CHANNEL 10

const bool wg_enable = false;

const int timezone = 1; // UTC standard time timezone
#define NTP_SERVERS "us.pool.ntp.org", "pool.ntp.org", "time.nist.gov"
struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600};    // Daylight time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 3, 0};          // Standard time = UTC/GMT +1 hour

simpleDSTadjust dstAdjusted(StartRule, EndRule);

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

struct __attribute__((packed)) REMOTE_DATA 
{
  float temp[2];
  float humi[2];
  uint16_t batV;
  time_t time;
};

SENSOR_DATA sensorData;
SENSOR_DATA localSensorData;
REMOTE_DATA data;

unsigned long last_ntp_packet_ms = 0;
struct tm *timeinfo;

uint8_t mac_arr[6];
uint16_t last_status = 0;
uint32_t count = 0;
const uint32_t sample_time = 5*60*1000; // Sample every 5 minutes
uint32_t last_sample = 0 - sample_time; // Sample 

bool esp_now_info_sent = false;
bool package_recieved = false;
uint8_t remoteMac[] = {0xB4, 0xE6, 0x2D, 0x53, 0xD5, 0xBF};


// prototypes

void recieve_callback(uint8_t *mac, uint8_t *data, uint8_t len);
void send_callback(uint8_t mac[], uint8_t sendStatus);
int8_t connect_wifi();
void init_esp_now();
void esp_send();
void print_package();
void sample_local();
void display_data();
void update_ntp();
void get_time();
String get_hour_pad();
String get_minute_pad();