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

#include <NimBLEDevice.h>

#include <ArcticPatterns.h>

class ArcticGraphics {
public:
    ArcticGraphics(const std::string& monitorName);

    void start(NimBLEServer* existingServer, NimBLEAdvertising* existingAdvertising);
    void start();

	void setup(const std::string& plot_name);
	void setup(const std::string& plot_name, std::initializer_list<std::string> labels);
	void setup(const std::string& plot_name, std::initializer_list<std::string> axles, std::initializer_list<std::string> labels);
	void plot(std::string plot_name, std::initializer_list<float> values);

	int createService(NimBLEAdvertising* existingAdvertising);
	int createServiceUUID();

	std::string get_name();
	std::string get_uuid_ats();
    std::string get_uuid_txm();

private:
	bool _debug_enabled = false;

	// Used for Bluetooth
	struct ServiceCharacteristics {
		NimBLECharacteristic* txCharacteristic;
	};

	// Used for WiFi and UART
	struct ServiceStringUUIDs {
		std::string uuid_ats;
		std::string uuid_txm;
	};

	std::string _monitorName;
	static int serviceCount;
	int serviceID;

	std::string _plot_name;
    struct PlotConfig {
        std::vector<std::string> axles;
        std::vector<std::string> labels;
    };
    std::map<std::string, PlotConfig> plotConfigs;

	NimBLEServer* pServer;
	NimBLEService* pService;

	std::map<int, ServiceCharacteristics> services;
	std::map<int, ServiceStringUUIDs> serialServices;

};