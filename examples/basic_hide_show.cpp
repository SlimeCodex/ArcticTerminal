// Description: This example shows how to use the hide and show functions of the ArcticTerminal class.
// Write "hide" to the Control Console to hide the Hidden Console.
// Write "show" to the Control Console to show the Hidden Console.

#include <Arduino.h>
#include <ArcticClient.h>

ArcticClient arctic_client;
ArcticTerminal simple_console("Control");
ArcticTerminal hidden_console("Hidden");

void setup() {
	arctic_client.begin();
	arctic_client.add(simple_console);
	arctic_client.add(hidden_console);
	arctic_client.start();
}

void loop() {
	if (simple_console.available()) {
		std::string com = simple_console.read();
		
		if (com == "hide") {
			hidden_console.hide();
		}
		if (com == "show") {
			hidden_console.show();
		}
	}
}