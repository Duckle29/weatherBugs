#include <main.h>

// Initialize the library with your credentials, and start a new query.
Wunderground wg(WG_ID, WG_PASS);

void setup()
{
	Serial.begin(115200);
	Serial.println("\nStarted");
	Wire.begin();

	//Serial.println(wg.send_update());

	init_esp_now();
}

void loop()
{
	if(Serial.available())
	{
		switch(Serial.read())
		{
			case 'r':
				ESP.restart();
				break;
			case 'p':
				print_package();
				Serial.print("Count"); Serial.println(count);
				Serial.print("Status: "); Serial.println(last_status);
		}
	}

	if(package_recieved)
	{
		count++;
		package_recieved = false;
		if(sensorData.humi >= 0)
		{
			wg.add_temp_c(sensorData.temp);
			wg.add_relative_humidity(sensorData.humi);

			print_package();

			connect_wifi();				
			last_status = wg.send_update();
			Serial.println(last_status); Serial.println();
			init_esp_now();
		}
		else
		{
			Serial.print("Issues with sensor: ");

			for(uint8_t i=0; i<5; i++)
			{
				Serial.print(mac_arr[i], HEX);Serial.print(':');
			}
			Serial.println(mac_arr[5], HEX);

			Serial.print("  Typ: "); Serial.println(sensorData.humi);
			Serial.print("  Err: "); Serial.println(sensorData.temp);
		}
	}
	delay(0);
}

void initVariant() 
{
  WiFi.mode(WIFI_AP);
  wifi_set_macaddr(SOFTAP_IF, &mac[0]);
}

void print_package()
{
	if(count == 0)
	{
		return;
	}
		Serial.println(count);
		
		// Print MAC of bug
		Serial.print("Sens: ");
		for(uint8_t i=0; i<5; i++)
		{
			Serial.print(mac_arr[i], HEX);Serial.print(':');
		}
		Serial.println(mac_arr[5], HEX);

		// Print package data (and calculated dew point)
		Serial.print("  Temp: "); Serial.print(sensorData.temp); Serial.println(" °C");
		Serial.print("  Humi: "); Serial.print(sensorData.humi); Serial.println(" %");
		Serial.print("  Dewp: "); Serial.print(wg._dew_point_c(sensorData.temp, sensorData.humi)); Serial.println(" °C");
		Serial.print("  BatV: "); Serial.print(sensorData.batV); Serial.println(" mV\n");
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
	Serial.println("Listening for ESP-NOW packets");
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
	delay(500);
	WiFi.mode(WIFI_STA);    //This line hides the viewing of ESP as wifi hotspot
	WiFi.begin(ssid, password);

	// Wait for connection
	uint32_t start = millis();
	while (WiFi.status() != WL_CONNECTED) 
	{
		if(millis()-start > wifi_timeout)
		{
			Serial.print("Connecting timed out");
			WiFi.mode(WIFI_OFF);
			return -10; // Timeout
		}
		delay(500);
		Serial.print(".");
	}
	Serial.print("Connected to: "); Serial.println(WiFi.SSID());
	return 0;
}
