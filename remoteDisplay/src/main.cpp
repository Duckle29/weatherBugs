#include <main.h>

void setup()
{
	wifi_set_channel(WIFI_CHANNEL);
	WiFi.hostname("Remote-display");
	Serial.begin(115200);
	Serial.println("\nStarted");
	Wire.begin();
	display.init();
    display.flipScreenVertically();
	display_data();

	Serial.println(WiFi.macAddress());

	init_esp_now();
}

void loop()
{
	if(package_recieved)
	{
		package_recieved = false;
		last_update = millis();
		if(data.humi[1] >= 0)
		{
			print_package();
			display_data();
		}
	}
	if(millis() - last_draw > 1000*60)
	{
		display_data();
	}
	delay(5000);
}

String get_time_str(time_t rawTime)
{
	char *dstAbbrev;
	time_t t = dstAdjusted.time(&dstAbbrev, rawTime);
	
	timeinfo = localtime(&t);
	return pad_number(timeinfo->tm_hour, 2) + ":" + pad_number(timeinfo->tm_min, 2);

}

String pad_number(int val, int n)
{
	int32_t multiplier = 1;
	int i=n;
	while( i >= 0)
	{
		if (val <= multiplier-1)
		{
			break;
		}
		i--;
		multiplier *= 10;
	}

	String out = "";
	
	if (val == 0)
	{
	    i--;
	}
	
	while(i-- > 0)
	{
		out += "0";
	}
	return out + String(val);
}

void display_data() 
{
	display.clear();
	const int offset = 2;
	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.setFont(ArialMT_Plain_16);
	display.drawString(0, 0, "  IN");
	display.drawString(0, 16 - offset, String(data.temp[0]) + String("°C") );
	display.drawString(0, 32 - offset*2, String(data.humi[0]) + String("%"));

	display.setTextAlignment(TEXT_ALIGN_RIGHT);
	display.drawString(128, 0, "OUT  ");
	display.drawString(128, 16 - offset, String(data.temp[1]) + String("°C"));
	display.drawString(128, 32 - offset*2, String(data.humi[1]) + String("%"));

	display.setTextAlignment(TEXT_ALIGN_CENTER);
	display.setFont(ArialMT_Plain_10);
	display.drawString(64, 0, String(data.batV) + "mv");

	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.setFont(ArialMT_Plain_10);
	display.drawString(0, 48, get_time_str(data.time));
	display.setTextAlignment(TEXT_ALIGN_RIGHT);
	display.drawString(128, 48, String((millis() - last_update)/1000/60) + "min");

	if(data.batV < 3200)
	{
		display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
		display.setFont(ArialMT_Plain_24);
		display.drawString(64, 32, "!");
		display.drawString(64, 32+24, "BAT");
	}
	display.display();
}

void print_package()
{
	// Print MAC of bug
	Serial.print("Sens: ");
	for(uint8_t i=0; i<5; i++)
	{
		Serial.print(mac_arr[i], HEX);Serial.print(':');
	}
	Serial.println(mac_arr[5], HEX);

	// Print package data (and calculated dew point)
	Serial.print("  Temp: "); Serial.print(data.temp[0]); Serial.print(" °C-i"); Serial.print(data.temp[1]); Serial.println(" °C");
	Serial.print("  Humi: "); Serial.print(data.humi[0]); Serial.print(" %-i"); Serial.print(data.humi[1]); Serial.print(" %");
	Serial.print("  BatV: "); Serial.print(data.batV); Serial.println(" mV\n");
	Serial.print("  Time: "); Serial.print(data.time); Serial.println(" s\n");
	
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
		esp_now_register_recv_cb(recieve_callback);
}

void recieve_callback(uint8_t *mac, uint8_t *rxData, uint8_t len)
{
	memcpy(&data, rxData, len);
	memcpy(mac_arr, mac, 6);

	package_recieved = true;
}
