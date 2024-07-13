#include "utility/w5100.h"

IPAddress ip(192, 168, 68, 7);
IPAddress gateway(192, 168, 68, 1);
IPAddress subnet(255, 255, 252, 0);

byte mac[] = { 0x54, 0x34, 0x41, 0x30, 0x30, 0x07 };

static void InitEthernet()
{
	Serial.println(F("Starting ethernet.."));

	Ethernet.begin(mac, ip, gateway, gateway, subnet);
	client.setConnectionTimeout(2000);

	W5100.setRetransmissionTime(0x07D0);
	W5100.setRetransmissionCount(3);

	Serial.print(F("IP Address: "));
	Serial.println(Ethernet.localIP());

	device.setUniqueId(mac, sizeof(mac));
}

static void InitMqtt() {
	// Configure MQTT
	mqtt.begin(MQTT_BROKER, MQTT_USERNAME, MQTT_PASSWORD);
}
