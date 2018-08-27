#include <main.h>

// this is the MAC Address of the remote ESP server which receives these sensor readings

// keep in sync with slave struct

void setup() 
{
  Serial.begin(115200); Serial.println("");

  if (esp_now_init() != 0) 
  {
    Serial.println("*** ESP_Now init failed");
    please_sleep();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(remoteMac, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);
  esp_now_register_send_cb(send_callback);

  sensorData.temp = 10;
  sensorData.humi = 50;
}

void loop() 
{
  sensorData.batV = analogRead(A0) * mv_per_adc;
  esp_send();
  delay(1000);
}

void esp_send()
{
  uint8_t bs[sizeof(sensorData)];
  memcpy(bs, &sensorData, sizeof(sensorData));
  esp_now_send(NULL, bs, sizeof(sensorData)); // NULL means send to all peers
}

void send_callback(uint8_t mac[], uint8_t sendStatus)
{
  Serial.printf("sendcb: send done, status: %i\n", sendStatus);
  please_sleep();
}

void please_sleep() 
{
  // add some randomness to avoid collisions with multiple devices
  int sleepSecs = SLEEP_SECS; //+ ((uint8_t)RANDOM_REG32/8); 
  Serial.printf("Awake for %i ms, going to sleep for %i secs...\n", millis(), sleepSecs); 
  ESP.deepSleepInstant(sleepSecs * 1000000, RF_NO_CAL);
}