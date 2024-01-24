// Description: This example shows how to use basic arguments.
// Write "ping" to the console to get "pong" as a response.
// Write "ping -c 5" to the console to get "pong" 5 times as a response.

#include <Arduino.h>
#include <ArcticClient.h>

ArcticClient arctic_client;
ArcticTerminal simple_console("Simple Console");

void setup() {
	arctic_client.begin();
	arctic_client.add(simple_console);
	arctic_client.start();
}

void loop() {
	if (simple_console.available()) {
		ArcticCommand com(simple_console.read());
		
		if (com.base() == "ping") {
			if (com.check("-c")) {
				int count = std::stoi(com.arg("-c"));
				for (int i = 1; i <= count; i++) {
					simple_console.printf("pong #%d\n", i);
				}
			} else {
				simple_console.printf("pong\n");
			}
		}
	}
}