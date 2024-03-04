/*
 * This file is part of ArcticTerminal Library.
 * Copyright (C) 2023 Alejandro Nicolini
 *
 * ArcticTerminal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ArcticTerminal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ArcticTerminal. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <atomic>
#include <cstdarg>
#include <sstream>
#include <string>
#include <vector>
#include <Update.h>

#include <NimBLEDevice.h>

#include <ArcticOTA.h>

class ArcticTerminal {
public:
	ArcticTerminal(const std::string& monitorName);

	void start(NimBLEServer* existingServer, NimBLEAdvertising* existingAdvertising);
	void start();
	void printf(const char* format, ...);
	void singlef(const char* format, ...);
	bool available();
	void hide();
	void show();
	std::string read(char delimiter = '\n');
	std::vector<uint8_t> raw();

	int createService(NimBLEAdvertising* existingAdvertising);
	int createServiceUUID();

	void setNewDataAvailable(bool available, std::string command);

private:
	bool _debug_enabled = false;

	// Used for Bluetooth
	struct ServiceCharacteristics {
		NimBLECharacteristic* txCharacteristic;
		NimBLECharacteristic* txsCharacteristic;
		NimBLECharacteristic* rxCharacteristic;
	};

	// Used for WiFi and UART
	struct ServiceStringUUIDs {
		std::string txStringUUID;
		std::string txsStringUUID;
		std::string rxStringUUID;
	};

	std::string _monitorName;
	static int serviceCount;
	int serviceID;

	NimBLEServer* pServer;
	NimBLEService* pService;

	std::atomic<bool> newDataAvailable{false};
	std::map<int, ServiceCharacteristics> services;

	std::map<int, ServiceStringUUIDs> serialServices;
};