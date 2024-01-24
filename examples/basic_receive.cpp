// Description: A basic example of how to receive data from the ArcticClient.
// Write "ping" to the Simple Console to get "pong" as a response.

// In this example we also rename the visible bluetooth device to "ESP32Device"
// If no name is given to the client instance, default name will be "ArcticTerminal"

#include <Arduino.h>
#include <ArcticClient.h>

ArcticClient arctic_client("ESP32Device");
ArcticTerminal simple_console("Simple Console");

void setup() {
	arctic_client.begin();
	arctic_client.add(simple_console);
	arctic_client.start();
}

void loop() {
    if (simple_console.available()) {
        std::string com = simple_console.read();
        
        if (com == "ping") {
            simple_console.printf("pong\n");
        }
    }
}