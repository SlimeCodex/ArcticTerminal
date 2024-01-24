// Description: A basic example of how to send data to the ArcticClient.

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
	simple_console.printf("Hello World!\n");
	delay(1000);
}