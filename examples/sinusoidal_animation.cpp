// Description: Funny animation demonstrating a sinusoidal function display.
// This example primarily serves to test the responsiveness of the library.

// Load this example in your ESP32, connect to it, open the console and watch the animation!
// If you want, try to send a character from the ArcticStream to change the output wave symbol.

#include <Arduino.h>
#include <ArcticClient.h>

ArcticClient arctic_client;
ArcticTerminal sinusoidal_console{"Sinusoidal Console"};

char wave_char = '|'; // Default character for the wave

void setup() {
	arctic_client.begin();
	arctic_client.add(sinusoidal_console);
	arctic_client.start();
}

void loop() {
	constexpr int amplitude = 40;
	constexpr int length = 80;

	// Renders signal
	for (int x = 0; x < length; ++x) {
		int sin_value = static_cast<int>(amplitude * (1 + sin(2 * PI * x / length)) / 2);
		std::string line(2 * amplitude, ' ');

		int start = max(0, amplitude - sin_value);
		int end = min((int)line.size() - 1, amplitude + sin_value);
		for (int i = start; i <= end; ++i) {
			line[i] = wave_char;
		}

		sinusoidal_console.printf("%s\n", line.c_str());
		delay(20);
	}

	// Updates new character if received
	if (sinusoidal_console.available()) {
		std::string com = sinusoidal_console.read();

		if (!com.empty()) {
			wave_char = com[0];
		}
	}
}