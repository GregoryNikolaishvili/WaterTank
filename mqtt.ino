#include "utility/w5100.h"

byte mac[] = { 0x54, 0x34, 0x41, 0x30, 0x30, 0x07 };
//IPAddress ip(192, 168, 2, 7);
IPAddress ip(192, 168, 1, 7);

EthernetClient ethClient;
PubSubClient mqttClient("192.168.1.23", 1883, callback, ethClient);     // Initialize a MQTT mqttClient instance
//PubSubClient mqttClient("212.72.150.154", 1883, callback, ethClient);     // Initialize a MQTT mqttClient instance

#define MQTT_BUFFER_SIZE 256

char buffer[MQTT_BUFFER_SIZE];

bool doLog = true;

extern SettingStructure settings[TANK_COUNT];

void InitEthernet()
{
	Serial.println(F("Starting ethernet.."));

	Ethernet.begin(mac, ip);
	ethClient.setConnectionTimeout(2000);

	W5100.setRetransmissionTime(0x07D0);
	W5100.setRetransmissionCount(3);

	Serial.print(F("IP Address: "));
	Serial.println(Ethernet.localIP());
}

void InitMqtt()
{
	mqttClient.setBufferSize(320);
	mqttClient.setSocketTimeout(5);
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

void PublishAlive()
{
	if (!mqttClient.connected()) return;

	const char* topic = "cha/wl/alive";
	int len = setHexInt32(buffer, now(), 0);
	PublishMqtt(topic, buffer, len, false);
}

void ReconnectMqtt() {

	if (!mqttClient.connected()) {

		Serial.print(F("Connecting to MQTT broker..."));

		// Attempt to connect
		if (mqttClient.connect("water_tank", "hub/controller/water_tank", 1, true, "{\"state\":\"disconnected\"}")) {

			Serial.println(F("connected"));

			// Once connected, publish an announcement...
			mqttClient.publish("hub/controller/water_tank", "{\"state\":\"disconnected\"}", true);  // Publish ethernet connected status to MQTT topic

			// ... and resubscribe
			mqttClient.subscribe("chac/wl/#", 1);     // Subscribe to a MQTT topic, qos = 1

			mqttClient.publish("hubcommand/gettime2", "chac/wl/settime2");     // request time

			PublishSettings();
			PublishAllStates();
		}
		else {
			Serial.print(F("failed, rc="));
			Serial.println(mqttClient.state());
		}
	}
}

void PublishAllStates()
{
	doLog = false;

	for (byte id = 0; id < TANK_COUNT; id++)
		PublishTankState(id);

	for (byte id = 0; id < RELAY_COUNT; id++)
		PublishRelayState(id, isRelayOn(id));

	doLog = true;
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

void PublishTime()
{
	if (!mqttClient.connected()) return;

	const char* topic = "cha/wl/time";
	int len = setHexInt32(buffer, now(), 0);
	PublishMqtt(topic, buffer, len, false);
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


	if (strcmp(topic, "chac/wl/alive") == 0)
	{
		PublishAlive();
		return;
	}

	if (strcmp(topic, "chac/wl/refresh") == 0)
	{
		PublishAllStates();
		return;
	}

	if (strcmp(topic, "chac/wl/gettime2") == 0)
	{
		PublishTime();
		return;
	}
	
	if (len == 0)
		return;
	
	if (strncmp(topic, "chac/wl/state/", 14) == 0)
	{
		byte id = hexCharToByte(topic[14]);
		bool value = payload[0] != '0';
		//Serial.print("id=");
		//Serial.print(id);
		//Serial.print(", value=");
		//Serial.println(value);

		relaySet(id, value);
		return;
	}

	if (strncmp(topic, "chac/wl/ballvalve/", 18) == 0)
	{
		byte id = hexCharToByte(topic[18]);
		bool value = payload[0] != '0';
		//Serial.print("id=");
		//Serial.print(id);
		//Serial.print(", value=");
		//Serial.println(value);

		if (value)
			openBallValve(id);
		else
			closeBallValve(id);
		return;
	}

	if (strncmp(topic, "chac/wl/settings2/", 18) == 0)
	{
		byte id = hexCharToByte(topic[18]);
		char* p = (char*)payload;

		settings[id].MaxDistance = readHexInt16(p);
		p += 4;
		settings[id].MinDistance = readHexInt16(p);
		//p += 4;

		saveSettings(true);
    PublishTankState(id);
    return;
	}

	if (strcmp(topic, "chac/wl/settime2") == 0)
	{
		char* data = (char*)payload;
		long tm = readHexInt32(data);

		setTime(tm);
		//RTC.set(now());
		printDateTime(&Serial, now());
		Serial.println();
		return;
	}

	if (strcmp(topic, "chac/wl/settime") == 0)
	{
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
		//RTC.set(now());
		printDateTime(&Serial, now());
	}
	return;
}
