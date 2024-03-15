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
#include <ArcticPatterns.h>

// Enumeration for connection profiles
#define ARCTIC_PROFILE_HIGH_SPEED 0x00 // default
#define ARCTIC_PROFILE_BALANCED 0x01
#define ARCTIC_PROFILE_POWER_SAVING 0x02
#define ARCTIC_PROFILE_LONG_RANGE 0x03
#define ARCTIC_PROFILE_MAX_SPEED 0x04

// Enumeration for interfaces
#define ARCTIC_BLUETOOTH 0x00 // default
#define ARCTIC_WIFI 0x02
#define ARCTIC_UART 0x01

// Default Device Name for BLE
#define ARCTIC_DEFAULT_DEVICE_NAME "ArcticTerminal"

// Default Port for WiFi
#define ARCTIC_DEFAULT_PORT_UPLINK 56320
#define ARCTIC_DEFAULT_PORT_DOWNLINK 56321

// Default Baudrate for UART
#define ARCTIC_DEFAULT_BAUDS 230400
#define ARCTIC_DEFAULT_UART_ACTIVITY_TIMEOUT 500
#define ARCTIC_DEFAULT_UART_KEEPALIVE_TIMEOUT 400

#define ARCTIC_DEFAULT_PRIMARY_DELIMITER ":"
#define ARCTIC_DEFAULT_SECONDARY_DELIMITER ","

#define ARCTIC_DEFAULT_KEEPALIVE_SYMBOL "0"

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

// Structure to hold the UART connection parameters
struct UARTConnParams {
	uint32_t bauds;
	uint8_t data;
	uint8_t stop;
	uint8_t parity;
};

// Structure to hold the WiFi connection parameters
struct WiFiConnParams {
	std::string ssid;
	std::string password;
	uint16_t port;
};

class ArcticClient {
public:
	ArcticClient(const std::string& bleDeviceName = ARCTIC_DEFAULT_DEVICE_NAME);
	void begin(uint8_t interface = ARCTIC_BLUETOOTH);
	void interface(HardwareSerial& uart_interface);
	void baudrate(uint32_t bauds = ARCTIC_DEFAULT_BAUDS);
	void connect(const std::string& ssid, const std::string& password,
		uint16_t socket_uplink = ARCTIC_DEFAULT_PORT_UPLINK,
		uint16_t socket_downlink = ARCTIC_DEFAULT_PORT_DOWNLINK
	);
	void disconnect();
	static void arctic_server_task(void* pvParameters);
	void server_task();
	void add(ArcticTerminal& console); // Register data console
	void start();
	void profile(uint8_t profile);
	void debug(bool enable);
	void createService(NimBLEAdvertising* existingAdvertising);
	bool connected();
	static bool arctic_connection_status;
	static bool arctic_uplink_enabled;
	static uint8_t arctic_interface;
	static WiFiServer* _uplink_server;
	static WiFiServer* _downlink_server;
	static WiFiClient _uplink_client;
	static WiFiClient _downlink_client;
	static BLEConnParams arctic_cparams;
	static HardwareSerial* _uart_port;
	static uint32_t _uart_keepalive_timer;
	static uint32_t _wifi_keepalive_timer;

	ArcticOTA ota;
	NimBLECharacteristic* _txCharacteristic;
	NimBLECharacteristic* _rxCharacteristic;

private:
	uint8_t _active_interface;
	std::string _bleDeviceName;
	bool _debug_enabled = false;
	bool _ota_console = false;
	NimBLEServer* pServer;
	NimBLEAdvertising* pAdvertising;
	std::vector<std::reference_wrapper<ArcticTerminal>> consoles;
	uint32_t _bauds = ARCTIC_DEFAULT_BAUDS;
	uint16_t _socket_port_uplink;
	uint16_t _socket_port_downlink;
	std::string _ssid;
	std::string _password;
	bool _uart_ready_notify = false;
	bool _wifi_ready_notify = false;

	uint32_t _uart_activity_timer = 0;
};