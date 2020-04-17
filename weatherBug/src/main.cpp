#include <main.h>

// Important note. with deep-sleep, 
// the wemos is fully reset each time it wakes up again,
// and therefore runs all code from the start

ClosedCube_SHT31D sht31;

#define SPRINT(x) if(SERIAL_ENABLED){Serial.print(x);}
#define SPRINTLN(x) if(SERIAL_ENABLED){Serial.println(x);}
#define SPRINTF(x, args...) if(SERIAL_ENABLED){Serial.printf(x, args);}

void setup() 
{
  Serial.begin(115200); SPRINTLN("\nBug woken");

  //WiFi.mode(WIFI_STA);
  wifi_set_channel(WIFI_CHANNEL);
  SPRINT(wifi_get_channel());

  sensorData.temp = -128;
  sensorData.humi = -1;
  
  Wire.begin();
  sht31.begin(0x44); // Set to 0x45 for alternate i2c addr

  SPRINTLN("Getting sens data");
  SHT31D res = sht31.readTempAndHumidity(SHT3XD_REPEATABILITY_HIGH, SHT3XD_MODE_POLLING, 200);
  if(res.error != SHT3XD_NO_ERROR)
  {
    sensorData.temp = res.error;
    sensorData.humi = -2;
  }
  else
  {
    sensorData.temp = res.t;
    sensorData.humi = res.rh;
  }
  delay(0);
  SPRINT("Got: "); 
  SPRINT(sensorData.humi);
  SPRINT("% "); 
  SPRINT(sensorData.temp);
  SPRINTLN("C");

  if (esp_now_init() != 0) 
  {
    SPRINTLN("espnow issue");
    please_sleep();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(remoteMac, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0);
  esp_now_register_send_cb(send_callback);

  sensorData.batV = float(analogRead(A0)) * mv_per_adc;
  SPRINT("batV: "); SPRINTLN(sensorData.batV);
  
  esp_send();
}

void loop() 
{
  if(millis() > SEND_TIMEOUT)
  {
    please_sleep();
  }
  delay(0);
}

void esp_send()
{
  SPRINTLN("sending data");
  uint8_t bs[sizeof(sensorData)];
  memcpy(bs, &sensorData, sizeof(sensorData));
  esp_now_send(NULL, bs, sizeof(sensorData)); // NULL means send to all peers
}

void send_callback(uint8_t mac[], uint8_t sendStatus)
{
  SPRINTF("sendcb: send done, status: %i\n", sendStatus);
  please_sleep();
  
}

void please_sleep() 
{
  // add some randomness to avoid collisions with multiple devices
  int sleepSecs = SLEEP_SECS + ((uint8_t)RANDOM_REG32/8); 
  SPRINTF("Awake for %i ms, going to sleep for %i secs...\n", millis(), sleepSecs); 
  ESP.deepSleepInstant(sleepSecs * 1000000, RF_NO_CAL);
}