#include <main.h>

// Initialize the library with your credentials, and start a new query.
Wunderground wg(WG_ID, WG_PASS);

void setup()
{
	WiFi.hostname("Spare-display");
	Serial.begin(115200);
	Serial.println("\nStarted");
	Wire.begin();
	sht31.begin(0x44);
	display.init();
    display.flipScreenVertically();
	
	Serial.println(WiFi.macAddress());

	if(connect_wifi() == 0)
	{
		update_ntp();
		get_time();
	}

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

		data.temp[0] = localSensorData.temp;
		data.temp[1] = sensorData.temp;
		data.humi[0] = localSensorData.humi;
		data.humi[1] = sensorData.humi;
		data.batV = sensorData.batV;

		if(sensorData.humi >= 0)
		{
			wg.add_temp_c(sensorData.temp);
			wg.add_relative_humidity(sensorData.humi);

			print_package();
			esp_send();

			if( connect_wifi() == 0)
			{
				update_ntp();
				get_time();
				last_ntp_packet_ms = millis();
				if(wg_enable)
				{
					last_status = wg.send_update();
					Serial.println(last_status); Serial.println();
				}
			}
			
			
			display_data();
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
	sample_local();
	delay(10);
}

void update_ntp()
{
	configTime(timezone * 3600, 0, NTP_SERVERS);
	delay(500);
	uint8_t cnt = 0;
	while(!time(nullptr))
	{
		cnt++;
		if(cnt%5 == 0)
		{
			Serial.print("#");
		}
		delay(100);
	}
}

void get_time()
{
	char *dstAbbrev;
	time_t t = dstAdjusted.time(&dstAbbrev);
	data.time = t;
	
	timeinfo = localtime(&t);
}

String get_hour_pad()
{
	if(WiFi.status() != WL_CONNECTED)
	{
		return String("00");
	}
	return (timeinfo->tm_hour < 10) ? "0" + String(timeinfo->tm_hour) : String(timeinfo->tm_hour);
}

String get_minute_pad()
{
	if(WiFi.status() != WL_CONNECTED)
	{
		return String("00");
	}
	return (timeinfo->tm_min < 10) ? "0" + String(timeinfo->tm_min) : String(timeinfo->tm_min);
}

/**
 * Read the local sensor and update the display
 **/
void sample_local()
{
	if(millis()-last_sample > sample_time)
	{
		last_sample = millis();
		SHT31D res = sht31.readTempAndHumidity(SHT3XD_REPEATABILITY_HIGH, SHT3XD_MODE_POLLING, 200);
		if(res.error != SHT3XD_NO_ERROR)
		{
			localSensorData.temp = res.error;
			localSensorData.humi = -2;
		}
		else
		{
			localSensorData.temp = res.t;
			localSensorData.humi = res.rh;
			display_data();
		}
		Serial.println(res.t);
	}
	delay(0);
}

void display_data() 
{
	display.clear();
	const int offset = 2;
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.setFont(ArialMT_Plain_16);
	display.drawString(0, 0, "  IN");
	display.drawString(0, 16 - offset, String(localSensorData.temp) + String("°C") );
	display.drawString(0, 32 - offset*2, String(localSensorData.humi) + String("%"));

	display.setTextAlignment(TEXT_ALIGN_RIGHT);
	display.drawString(128, 0, "OUT  ");
	display.drawString(128, 16 - offset, String(sensorData.temp) + String("°C"));
	display.drawString(128, 32 - offset*2, String(sensorData.humi) + String("%"));

	display.setTextAlignment(TEXT_ALIGN_CENTER);
	display.setFont(ArialMT_Plain_10);
	display.drawString(64, 0, String(sensorData.batV) + "mv");

	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.setFont(ArialMT_Plain_10);
	display.drawString(0, 48, get_hour_pad() + ':' + get_minute_pad());
	display.setTextAlignment(TEXT_ALIGN_RIGHT);
	display.drawString(128, 48, String((millis() - last_ntp_packet_ms)/1000/60) + "min");

	if(sensorData.batV < 3200)
	{
		display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
		display.setFont(ArialMT_Plain_24);
		display.drawString(64, 32, "!");
		display.drawString(64, 32+24, "BAT");
	}
	display.display();
}

// Setting the MAC address seems to have broken, so using actual unique MAC of the station
// void initVariant() 
// {
// 	WiFi.mode(WIFI_AP);
// 	wifi_set_macaddr(SOFTAP_IF, &mac[0]);
// }

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
	WiFi.mode(WIFI_STA);
	//Serial.println(wifi_get_channel());
	if(esp_now_init() != 0) 
	{
		Serial.println("*** ESP_Now init failed");
		ESP.restart();
	}
	Serial.println("Listening for ESP-NOW packets");
	esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
	esp_now_add_peer(remoteMac, ESP_NOW_ROLE_COMBO, WIFI_CHANNEL, NULL, 0);
	esp_now_register_recv_cb(recieve_callback);
	esp_now_register_send_cb(send_callback);
}

void recieve_callback(uint8_t *mac, uint8_t *data, uint8_t len)
{
	memcpy(&sensorData, data, len);
	memcpy(mac_arr, mac, 6);

	package_recieved = true;
}

void send_callback(uint8_t mac[], uint8_t sendStatus)
{
  Serial.printf("sendcb: send done, status: %i\n", sendStatus);  
}

void esp_send()
{
   Serial.println("sending data");
   uint8_t bs[sizeof(data)];
   memcpy(bs, &data, sizeof(data));
   esp_now_send(NULL, bs, sizeof(data)); // NULL means send to all peers
}

int8_t connect_wifi()
{
	esp_now_deinit();

	Serial.print(String("Connecting to ") + String(ssid));
	WiFi.mode(WIFI_OFF);    //Prevents reconnection issue (taking too long to connect)
	delay(500);
	WiFi.mode(WIFI_STA);    
	WiFi.begin(ssid, password);

	// Wait for connection
	uint32_t start = millis();
	uint8_t cnt = 0;
	while (WiFi.status() != WL_CONNECTED) 
	{
		if(millis()-start > wifi_timeout)
		{
			Serial.println("Connecting timed out");
			WiFi.disconnect();
			WiFi.mode(WIFI_OFF);
			return -10; // Timeout
		}
		delay(100);
		
		cnt++;
		if(cnt%5 == 0)
		{
			Serial.print(".");
		}
	}
	Serial.print("Connected to: "); Serial.println(WiFi.SSID());

	return 0;
}
