#pragma once

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <time.h>
#include <simpleDSTadjust.h>

#include <Wire.h>
#include "SSD1306Wire.h"

#define WIFI_CHANNEL 10

extern "C" 
{
  #include <espnow.h>
  #include <user_interface.h>
}

const int timezone = 1; // UTC standard time timezone
struct dstRule StartRule = {"CEST", Last, Sun, Mar, 2, 3600};    // Daylight time = UTC/GMT +2 hours
struct dstRule EndRule = {"CET", Last, Sun, Oct, 3, 0};          // Standard time = UTC/GMT +1 hour

simpleDSTadjust dstAdjusted(StartRule, EndRule);

SSD1306Wire  display(0x3C, SDA, SCL);


struct __attribute__((packed)) DISPLAY_DATA 
{
  float temp[2];
  float humi[2];
  uint16_t batV;
  time_t time;
};

DISPLAY_DATA data;

unsigned long last_update = 0;
unsigned long last_draw = 0;
struct tm *timeinfo;

uint8_t mac_arr[6];

bool esp_now_info_sent = false;
bool package_recieved = false;

// prototypes

void recieve_callback(uint8_t *mac, uint8_t *data, uint8_t len);
void init_esp_now();
void print_package();
void display_data();
String get_time_str(time_t rawTime);
String pad_number(int val, int n);