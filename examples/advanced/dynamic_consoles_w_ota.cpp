// This example shows how to create consoles dynamically for testing purposes.

#include <Arduino.h>
#include <ArcticClient.h>
#include <SmartSyncEvent.h>

#include <map>
#include <vector>
#include <string>
#include <random>
#include <sstream>

#define MAX_CONSOLES 16

ArcticClient arctic_client;
std::vector<ArcticTerminal*> consoles;
std::map<ArcticTerminal*, std::string> consoleNames;

void addConsoles(int count) {
	count = (count > MAX_CONSOLES) ? MAX_CONSOLES : count;
	for (int i = 0; i < count; i++) {
		char consoleName[21];
		sprintf(consoleName, "DC %d", i + 1);
		ArcticTerminal* newConsole = new ArcticTerminal(consoleName);
		consoles.push_back(newConsole);
		consoleNames[newConsole] = consoleName;
	}
}

std::string generate_random_hash() {
	std::string characters = "0123456789abcdef";
	std::string hash = "";
	for (int i = 0; i < 128; ++i) {
		int randomIndex = random(0, characters.length());
		hash += characters[randomIndex];
	}
	return hash;
}

void setup() {
	arctic_client.begin();
	addConsoles(16);
	for (auto& console : consoles) {
		arctic_client.add(*console);
	}
	arctic_client.start();
}

void loop() {
	for (auto& console : consoles) {
		if (console->available()) {
			std::string com = console->read();
			std::string consoleName = consoleNames[console];
			if (com == "info") {
				std::string hash = generate_random_hash();
				console->singlef("%lu > This is %s [%s]\n", millis(), consoleName.c_str(), hash.c_str());
			}
		}
	}
	if (SYNC_EVENT(1000)) {
		for (auto& console : consoles) {
			std::string consoleName = consoleNames[console];
			std::string hash = generate_random_hash();
			console->printf("%lu > This is %s %s\n", millis(), consoleName.c_str(), hash.c_str());
		}
	}
}

void cleanup() {
	for (auto& console : consoles) {
		delete console;
	}
	consoles.clear();
}