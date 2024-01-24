// Description: Basic example of using ArcticTerminal with two consoles.
// Write "hello" to the Core Console or "hello" to the WiFi Console to get a response.
// Write "get_mac" to the Core Console to get a response.
// Write "reset" to the Core Console to restart the MCU.
// Write "connect" to the WiFi Console to get a response.
// Write "disconnect" to the WiFi Console to get a response.

#include <Arduino.h>
#include <ArcticClient.h>

ArcticClient arctic_client;
ArcticTerminal console_core("Core Console");
ArcticTerminal console_wifi("WiFi Console");

void setup() {
	arctic_client.begin();
	arctic_client.add(console_core);
	arctic_client.add(console_wifi);
	arctic_client.start();
}

void loop() {

	if (console_core.available()) {
		std::string com = console_core.read();

		if (com == "hello") {
			console_core.printf("Logging to Core Console!\n");
			console_core.singlef("Hello from the Core Console!\n");
		}
		if (com == "get_mac") {
			console_core.printf("AA:BB:CC:DD:EE:FF\n");
		}
		if (com == "reset") {
			console_core.printf("Restarting MCU ...\n");
			delay(500);
			ESP.restart();
		}
	}

	if (console_wifi.available()) {
		std::string com = console_wifi.read();
		
		if (com == "hello") {
			console_wifi.printf("Logging to WiFi Console!\n");
			console_wifi.singlef("Hello from the WiFi Console!\n");
		}
		if (com == "connect") {
			console_wifi.printf("Connecting!\n");
		}
		if (com == "disconnect") {
			console_wifi.printf("Disconnected\n");
		}
	}
}