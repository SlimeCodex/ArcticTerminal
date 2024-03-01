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

#include <Arduino.h>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <NimBLEDevice.h>
#include <HardwareSerial.h>

#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <ArcticCommand.h>
#include <ArcticOTA.h>
#include <ArcticTerminal.h>

// Enumeration for connection profiles
#define ARCTIC_PROFILE_HIGH_SPEED 0x00 // default
#define ARCTIC_PROFILE_BALANCED 0x01
#define ARCTIC_PROFILE_POWER_SAVING 0x02
#define ARCTIC_PROFILE_LONG_RANGE 0x03
#define ARCTIC_PROFILE_MAX_SPEED 0x04

// Enumeration for interfaces
#define ARCTIC_BLUETOOTH 0x00 // default
#define ARCTIC_USB 0x01
#define ARCTIC_WIFI 0x02

// Some OS may require this services to be enabled
#ifdef ARCTIC_ENABLE_DEFAULT_SERVICES
#define BLE_UUID_HUMAN_INTERFACE_DEVICE_SERVICE 0x1812
#define BLE_UUID_HID_INFORMATION_CHAR 0x2A4A
#define BLE_UUID_REPORT_MAP_CHAR 0x2A4B
#define BLE_UUID_HID_CONTROL_POINT_CHAR 0x2A4C
#define BLE_UUID_REPORT_CHAR 0x2A4D
#define BLE_UUID_BATTERY_SERVICE 0x180F
#define BLE_UUID_BATTERY_LEVEL_CHAR 0x2A19
#define BLE_UUID_DEVICE_INFORMATION_SERVICE 0x180A
#endif

// Structure to hold BLE connection parameters
struct BLEConnParams {
	uint16_t min;
	uint16_t max;
	uint16_t sup;
	uint16_t mtu;
};

class ArcticClient {
public:
	ArcticClient(const std::string& bleDeviceName = "ArcticTerminal");
	void begin(uint8_t interface = ARCTIC_BLUETOOTH);
	void interface(HardwareSerial& uart_interface);
	void baudrate(uint32_t bauds);
	void connect(const std::string& ssid, const std::string& password, uint16_t socket_port);
	void disconnect();
	void add(ArcticTerminal& console); // Register data console
	void start();
	void profile(uint8_t profile);
	void debug(bool enable);
	void createService(NimBLEAdvertising* existingAdvertising);
	bool connected();
	static bool arctic_connection_status;
	static BLEConnParams arctic_cparams;
	ArcticOTA ota;
	NimBLECharacteristic* _txCharacteristic;
	NimBLECharacteristic* _rxCharacteristic;

private:
	uint8_t _active_interface;
	HardwareSerial* _uart_interface;
	WiFiClient* _wifi_client;
	WiFiServer* _wifi_server;
	std::string _bleDeviceName;
	bool _debug_enabled = false;
	bool _ota_console = false;
	NimBLEServer* pServer;
	NimBLEAdvertising* pAdvertising;
	std::vector<std::reference_wrapper<ArcticTerminal>> consoles;
	uint32_t _bauds = 115200;
	uint16_t _socket_port = 13200;
	std::string _ssid;
	std::string _password;
};