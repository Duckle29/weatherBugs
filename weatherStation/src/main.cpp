#include <main.h>

// Initialize the library with your credentials, and start a new query.
Wunderground wg(WG_ID, WG_PASS);

void setup()
{
	Serial.begin(115200);
	Serial.println("\nStarted");
	Wire.begin();
	sht31.begin(0x44);
	display.init();
	display.flipScreenVertically();

	//Serial.println(wg.send_update());

	connect_wifi();
	update_ntp();
	get_time();

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
			update_ntp();
			get_time();
			
			last_ntp_packet_ms = millis();
			//last_status = wg.send_update();
			Serial.println(last_status); Serial.println();
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
	
	timeinfo = localtime(&t);
}

String get_hour_pad()
{
	return (timeinfo->tm_hour < 10) ? "0" + String(timeinfo->tm_hour) : String(timeinfo->tm_hour);
}

String get_minute_pad()
{
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
	uint8_t cnt = 0;
	while (WiFi.status() != WL_CONNECTED) 
	{
		if(millis()-start > wifi_timeout)
		{
			Serial.print("Connecting timed out");
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
