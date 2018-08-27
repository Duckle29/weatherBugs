#include <main.h>

// Initialize the library with your credentials, and start a new query
Wunderground wg(WG_ID, WG_PASS);

void setup()
{
	Serial.begin(115200);
	Serial.println("\nStarted");
	
	//wg.add_temp_c(20);
	//wg.add_relative_humidity(50);
	//Serial.println(wg.send_update());

	init_esp_now();
}

uint32_t count = 0;

void loop()
{
	if(package_recieved)
	{
		Serial.println(count);
		count++;
		package_recieved = false;
		Serial.print("Sens: "); // $$ just an indicator that this line is a received ESP-Now message
		// mac address of remote ESP-Now device
		for(uint8_t i=0; i<5; i++)
		{
			Serial.print(mac_arr[i], HEX);Serial.print(':');
		}
		Serial.println(mac_arr[5], HEX);

  	Serial.print("  Temp: "); Serial.print(sensorData.temp); Serial.println(" °C");
		Serial.print("  Humi: "); Serial.print(sensorData.humi); Serial.println(" %");
		Serial.print("  BatV: "); Serial.print(sensorData.batV); Serial.println(" mV\n");
	}
	delay(0);
}

void initVariant() 
{
  WiFi.mode(WIFI_AP);
  wifi_set_macaddr(SOFTAP_IF, &mac[0]);
}

void init_esp_now()
{
	WiFi.mode(WIFI_OFF);
	delay(100);
	WiFi.mode(WIFI_AP);

	if(esp_now_init() != 0) 
	{
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(recieve_callback);
}

void recieve_callback(uint8_t *mac, uint8_t *data, uint8_t len)
{
	memcpy(&sensorData, data, len);
	memcpy(mac_arr, mac, 6);

	package_recieved = true;
}

int8_t connect_wifi()
{
	esp_now_deinit();

	Serial.print("Connecting");
	WiFi.mode(WIFI_OFF);    //Prevents reconnection issue (taking too long to connect)
	delay(200);
	WiFi.mode(WIFI_STA);    //This line hides the viewing of ESP as wifi hotspot
	WiFi.begin(ssid, password);

	// Wait for connection
	uint32_t start = millis();
	while (WiFi.status() != WL_CONNECTED) 
	{
		if(millis()-start < wifi_timeout)
		{
			Serial.print("Connecting timed out");
			WiFi.mode(WIFI_OFF);
			return -10; // Timeout
		}
		delay(500);
		Serial.print(".");
	}
	return 0;
}