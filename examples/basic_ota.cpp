// Description: Basic example of using ArcticTerminal with OTA updates.
// Open the ArcticStream application and connect to the device, then click the "OTA" tab and upload a file.
// The device will automatically restart and the new firmware will be running.

// Please note that the OTA might fail sometimes, just try again if it does.
// Its also recommended to wait a few seconds after the the OTA fails before trying again.

// By default OTA is loaded with a default profile for connection parameters.

#include <Arduino.h>
#include <ArcticClient.h>

ArcticClient arctic_client;

void setup() {
	// You can delete these lines if you want to use the default profile
	//arctic_client.profile(ARCTIC_PROFILE_BALANCED); // Mid speed, mid power consumption
	//arctic_client.profile(ARCTIC_PROFILE_HIGH_SPEED); // Default profile, mid-high speed, mid-high power consumption
	//arctic_client.profile(ARCTIC_PROFILE_LONG_RANGE); // Low speed, low power consumption, longer time between connection intervals
	//arctic_client.profile(ARCTIC_PROFILE_MAX_SPEED); // Max speed, max power consumption
	//arctic_client.profile(ARCTIC_PROFILE_POWER_SAVING); // Low speed, low power consumption, short time between connection intervals

	arctic_client.begin();
	// any aditional consoles can be implemented here as shown in other examples
	arctic_client.start();
}

void loop() {
	if (arctic_client.ota.available()) {
		if (arctic_client.ota.download()) {
			delay(500);
			ESP.restart();
		}
	}
}