byte mac[] = { 0x54, 0x34, 0x41, 0x30, 0x30, 0x07 };
//IPAddress ip(192, 168, 2, 7);
IPAddress ip(192, 168, 1, 7);

EthernetClient ethClient;
PubSubClient mqttClient("192.168.1.23", 1883, callback, ethClient);     // Initialize a MQTT mqttClient instance
//PubSubClient mqttClient("212.72.150.154", 1883, callback, ethClient);     // Initialize a MQTT mqttClient instance

#define MQTT_BUFFER_SIZE 256

char buffer[MQTT_BUFFER_SIZE];

boolean publishInitialState = true;

extern SettingStructure settings[TANK_COUNT];

void InitEthernet()
{
	Serial.println(F("Starting ethernet.."));

	Ethernet.begin(mac, ip);

	Serial.print(F("IP Address: "));
	Serial.println(Ethernet.localIP());
}

void InitMqtt()
{
	ReconnectMqtt();
}

void ProcessMqtt()
{
	mqttClient.loop();
}

void PublishMqtt(const char* topic, const char* message, int len, boolean retained)
{
	Serial.print(F("Publish. topic="));
	Serial.print(topic);
	Serial.print(F(", length="));
	Serial.print(len);

	Serial.print(F(", payload="));
	for (int i = 0; i < len; i++)
		Serial.print(message[i]);
	Serial.println();

	mqttClient.publish(topic, (byte*)message, len, retained);
}

void PublishMqttAlive(const char* topic)
{
	setHexInt32(buffer, now(), 0);
	PublishMqtt(topic, buffer, 8, false);
}


void ReconnectMqtt() {

	if (!mqttClient.connected()) {
		publishInitialState = true;

		Serial.print(F("Connecting to MQTT broker..."));

		// Attempt to connect
		if (mqttClient.connect("WL controller", "cha/sys/WL controller", 1, true, "disconnected")) {

			Serial.println(F("connected"));

			// Once connected, publish an announcement...
			mqttClient.publish("cha/sys/WL controller", "connected", true);  // Publish ethernet connected status to MQTT topic

			// ... and resubscribe
			mqttClient.subscribe("chac/wl/#", 1);     // Subscribe to a MQTT topic, qos = 1

			mqttClient.publish("cha/hub/gettime", "chac/wl/settime");     // request time

			PublishControllerState();
			PublishSettings();
			PublishAllStates(true);
		}
		else {
			Serial.print(F("failed, rc="));
			Serial.println(mqttClient.state());
		}
	}
}

void PublishControllerState()
{
	if (!mqttClient.connected()) return;

	setHexInt16(buffer, waterLevelControllerState, 0);
	PublishMqtt("cha/wl/state", buffer, 4, true);
}

void PublishAllStates(bool isInitialState)
{
	for (byte id = 0; id < TANK_COUNT; id++)
		PublishTankState(id);
}

void PublishTankState(byte id)
{
	if (!mqttClient.connected()) return;

	char topic[15];
	strcpy(topic, "cha/wl/state/?");
	topic[13] = byteToHexChar(id);

	setHexInt16(buffer, ultrasound_sensor_distances[id], 0);
	setHexInt16(buffer, ultrasound_sensor_percents[id], 4);
	buffer[8] = float_switch_states[id] ? '1' : '0';
	setHexInt16(buffer, ball_valve_state[id], 9);
	buffer[13] = ball_valve_switch_state[id];

	PublishMqtt(topic, buffer, 14, true);
}

void PublishRelayState(byte id, bool value)
{
	if (!mqttClient.connected()) return;

	char topic[12];
	strcpy(topic, "cha/wl/rs/?");
	topic[10] = byteToHexChar(id);
	PublishMqtt(topic, value ? "1" : "0", 1, true);
}

void PublishSettings()
{
	if (!mqttClient.connected()) return;

	const char* topic = "cha/wl/settings";
	int idx = 0;

	for (byte i = 0; i < TANK_COUNT; i++)
	{
		idx = setHexInt16(buffer, settings[i].MaxDistance, idx);
		idx = setHexInt16(buffer, settings[i].MinDistance, idx);
	}

	PublishMqtt(topic, buffer, idx, true);
}


void callback(char* topic, byte * payload, unsigned int len) {

	Serial.print(F("message arrived: topic='"));
	Serial.print(topic);
	Serial.print(F("', length="));
	Serial.print(len);
	Serial.print(F(", payload="));
	Serial.write(payload, len);
	Serial.println();

	if (len == 0)
		return;

	if (strcmp(topic, "chac/wl/refresh") == 0)
	{
		PublishAllStates(false);
		PublishMqttAlive("cha/wl/alive");

		return;
	}


	if (strcmp(topic, "chac/wl/settings") == 0)
	{
		char* p = (char*)payload;
		for (byte i = 0; i < TANK_COUNT; i++)
		{
			settings[i].MaxDistance = readHex(p, 4);
			p += 4;
			settings[i].MinDistance = readHex(p, 4);
			p += 4;
		}

		saveSettings(true);
		return;
	}

	if (strcmp(topic, "chac/wl/settime") == 0)
	{
		if (publishInitialState)
		{
			publishInitialState = false;

			char* data = (char*)payload;
			int yr, month, day;
			int hr, min, sec;

			yr = 2000 + (*data++ - '0') * 10;
			yr += (*data++ - '0');

			month = (*data++ - '0') * 10;
			month += (*data++ - '0');

			day = (*data++ - '0') * 10;
			day += (*data++ - '0');

			data++;

			hr = (*data++ - '0') * 10;
			hr += (*data++ - '0');
			min = (*data++ - '0') * 10;
			min += (*data++ - '0');
			sec = (*data++ - '0') * 10;
			sec += (*data++ - '0');

			setTime(hr, min, sec, day, month, yr);
			printDateTime(&Serial, now());

			//PublishAllStates(false, true);
		}
		return;
	}
}
