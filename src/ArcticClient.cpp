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

#include <ArcticCallbacks.h>
#include <ArcticClient.h>

// Initialize static variables
bool ArcticClient::arctic_connection_status = false;
uint8_t ArcticClient::arctic_interface = ARCTIC_BLUETOOTH;
BLEConnParams ArcticClient::arctic_cparams = {0, 0, 0, 0};

WiFiServer* ArcticClient::_uplink_server = nullptr;
WiFiServer* ArcticClient::_downlink_server = nullptr;
WiFiClient ArcticClient::_uplink_client;
WiFiClient ArcticClient::_downlink_client;

HardwareSerial* ArcticClient::_uart_port = nullptr;

// Constructor for handler
ArcticClient::ArcticClient(const std::string& bleDeviceName) {
	_bleDeviceName = bleDeviceName;
	ArcticClient::arctic_cparams.mtu = BLE_ATT_MTU_MAX; // Default MTU
	profile(ARCTIC_PROFILE_HIGH_SPEED); // Default profile
}

// Begin: Initialize BLE
void ArcticClient::begin(uint8_t interface) {
	// Initialize interface
	switch (interface) {
		case ARCTIC_BLUETOOTH: // default
			ArcticClient::arctic_interface = ARCTIC_BLUETOOTH;
			NimBLEDevice::init(_bleDeviceName);
			pServer = NimBLEDevice::createServer();
			pServer->setCallbacks(new ATCallbacks());
			break;
		case ARCTIC_UART: // UART interface
			ArcticClient::arctic_interface = ARCTIC_UART;
			break;
		case ARCTIC_WIFI: // WiFi interface
			ArcticClient::arctic_interface = ARCTIC_WIFI;
			break;
	}
}

// Interface: Set interface
void ArcticClient::interface(HardwareSerial& uart_interface) {
	_uart_port = &uart_interface;
}

// Baudrate: Set baudrate
void ArcticClient::baudrate(uint32_t bauds) {
	_bauds = bauds;
}

// Connect: Connect to WiFi
void ArcticClient::connect(const std::string& ssid, const std::string& password, uint16_t socket_uplink, uint16_t socket_downlink) {
	_ssid = ssid;
	_password = password;
	_socket_port_uplink = socket_uplink;
	_socket_port_downlink = socket_downlink;

	// Connect to WiFi
	WiFi.begin(ssid.c_str(), password.c_str());
	while (WiFi.status() != WL_CONNECTED) {
		delay(100);
	}
	IPAddress ip = WiFi.localIP();
}

// Disconnect: Disconnect from WiFi
void ArcticClient::disconnect() { // TODO: CAUSES CRASH
	WiFi.disconnect();
	delete _uplink_server;
	delete _downlink_server;
	_uplink_server = nullptr;
	_downlink_server = nullptr;
}

// Static wrapper function
void ArcticClient::arctic_server_task(void* pvParameters) {
	ArcticClient* instance = static_cast<ArcticClient*>(pvParameters); // Cast to ArcticClient pointer
	instance->server_task(); // Call the actual task method
}

// Add console to global map
void ArcticClient::add(ArcticTerminal& console) {
	consoles.push_back(console);
}

// Start: Start BLE server and advertising
void ArcticClient::start() {

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		// Initialize services and characteristics
		pAdvertising = NimBLEDevice::getAdvertising();

#ifdef ARCTIC_ENABLE_DEFAULT_SERVICES
		// Some OS may require this services to be enabled

		// HID Service
		NimBLEService* pService = pServer->createService(BLEUUID((uint16_t)BLE_UUID_HUMAN_INTERFACE_DEVICE_SERVICE));
		NimBLECharacteristic* pCharacteristic = pService->createCharacteristic(BLEUUID((uint16_t)BLE_UUID_HID_INFORMATION_CHAR), NIMBLE_PROPERTY::READ);
		pCharacteristic = pService->createCharacteristic(BLEUUID((uint16_t)BLE_UUID_REPORT_MAP_CHAR), NIMBLE_PROPERTY::READ);
		pCharacteristic = pService->createCharacteristic(BLEUUID((uint16_t)BLE_UUID_HID_CONTROL_POINT_CHAR), NIMBLE_PROPERTY::READ);
		pCharacteristic = pService->createCharacteristic(BLEUUID((uint16_t)BLE_UUID_REPORT_CHAR), NIMBLE_PROPERTY::READ);
		pService->start();
		pAdvertising->addServiceUUID(pService->getUUID());

		// Battery Service
		pService = pServer->createService(BLEUUID((uint16_t)BLE_UUID_BATTERY_SERVICE));
		pCharacteristic = pService->createCharacteristic(BLEUUID((uint16_t)BLE_UUID_BATTERY_LEVEL_CHAR), NIMBLE_PROPERTY::READ);
		pService->start();
		pAdvertising->addServiceUUID(pService->getUUID());

		// Device Information Service
		pService = pServer->createService(BLEUUID((uint16_t)BLE_UUID_DEVICE_INFORMATION_SERVICE));
		pService->start();
		pAdvertising->addServiceUUID(pService->getUUID());
#endif

		// Create system (background) service
		createService(pAdvertising);

		// Start OTA service
		ota.start(pServer, pAdvertising);

		// Start consoles
		for (auto& console : consoles) {
			console.get().start(pServer, pAdvertising);
		}

		// Start advertising
		if (!pAdvertising->isAdvertising()) {
			pAdvertising->start();
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {

		// Start WiFi server
		_uplink_server = new WiFiServer(_socket_port_uplink);
		_downlink_server = new WiFiServer(_socket_port_downlink);
		xTaskCreatePinnedToCore(arctic_server_task, "ArcticServerTask", 4096, this, 1, NULL, 0);

		// Start consoles
		for (auto& console : consoles) {
			console.get().start();
		}
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {

		// Start UART server
		xTaskCreatePinnedToCore(arctic_server_task, "ArcticServerTask", 4096, this, 1, NULL, 0);

		// Start consoles
		for (auto& console : consoles) {
			console.get().start();
		}
	}
}

// Profile: Set BLE connection parameters
void ArcticClient::profile(uint8_t profile) {
	switch (profile) {
		case ARCTIC_PROFILE_HIGH_SPEED:
			ArcticClient::arctic_cparams = {10, 16, 100, arctic_cparams.mtu};
			break;
		case ARCTIC_PROFILE_BALANCED:
			ArcticClient::arctic_cparams = {24, 40, 200, arctic_cparams.mtu};
			break;
		case ARCTIC_PROFILE_POWER_SAVING: // not fully tested
			ArcticClient::arctic_cparams = {80, 100, 300, arctic_cparams.mtu};
			break;
		case ARCTIC_PROFILE_LONG_RANGE: // not fully tested
			ArcticClient::arctic_cparams = {160, 200, 400, arctic_cparams.mtu};
			break;
		case ARCTIC_PROFILE_MAX_SPEED:
			ArcticClient::arctic_cparams = {6, 8, 50, arctic_cparams.mtu};
			break;
	}
}

// Debug: Enable debug messages
void ArcticClient::debug(bool status) {
	_debug_enabled = status;
}

// Create system service
void ArcticClient::createService(NimBLEAdvertising* existingAdvertising) {
	NimBLEService* pService = pServer->createService("4fafc201-1fb5-459e-1000-c5c9c3319f00");
	_txCharacteristic = pService->createCharacteristic("4fafc201-1fb5-459e-1000-c5c9c3319a00", NIMBLE_PROPERTY::NOTIFY); // TX
	_rxCharacteristic = pService->createCharacteristic("4fafc201-1fb5-459e-1000-c5c9c3319b00", NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR); // RX
	_rxCharacteristic->setCallbacks(new RxCharacteristicCallbacks(this));
	pService->start(); // Start the service
	existingAdvertising->addServiceUUID(pService->getUUID());
}

bool ArcticClient::connected() {
	return ArcticClient::arctic_connection_status;
}

void interface(uint8_t interface) {
}

void ArcticClient::server_task() {

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		// Process is handled by NimBLE
		while (1) {
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		_uplink_server->begin();
		_downlink_server->begin();
		while (1) {
			WiFiClient uplink_client = _uplink_server->available();
			if (uplink_client) {
				_uplink_client = uplink_client;
			}

			// Update WiFi client
			WiFiClient downlink_client = _downlink_server->available();
			if (downlink_client) {
				_downlink_client = downlink_client;

				// Read data from client
				if (_downlink_client.connected()) {
					while (_downlink_client.available()) {
						std::string command;
						while (_downlink_client.available()) {
							char c = _downlink_client.read();
							if (c == '\n') break;
							command += c;
						}

						// Respond to name and MAC address
						if (command == "ARCTIC_COMMAND_GET_DEVICE") {
							std::string response_com = "ARCTIC_COMMAND_GET_DEVICE:";
							std::string mac = WiFi.macAddress().c_str();
							std::string response = response_com + _bleDeviceName + "," + mac;
							_downlink_client.println(response.c_str());
						}

						// Respond to services available
						if (command == "ARCTIC_COMMAND_GET_SERVICES") {
							std::string response_com = "ARCTIC_COMMAND_GET_SERVICES:";
							std::string response = response_com;
							for (auto& console : consoles) {
								response += console.get().get_name() + "," + console.get().get_uuid_ats() + "," + console.get().get_uuid_txm() + "," + console.get().get_uuid_txs() + "," + console.get().get_uuid_rxm();
								if (&console != &consoles.back()) response += ":";
							}
							_downlink_client.println(response.c_str());
						}

						// Split UUID from input data (UUID:DATA)
						if (command.find(":") != std::string::npos) {
							std::string uuid = command.substr(0, command.find(":"));
							std::string data = command.substr(command.find(":") + 1);
							for (auto& console : consoles) {
								if (console.get().get_uuid_rxm().compare(uuid) == 0) {
									console.get().setNewDataAvailable(true, data);
								}
							}
						}
					}
				}
			}

			// Update connection status
			if (_uplink_client.connected()) {
				ArcticClient::arctic_connection_status = true;
			}
			else {
				ArcticClient::arctic_connection_status = false;
			}

			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		_uart_port->begin(_bauds);
		while (1) {
			if (_uart_port->available() > 0) {

				// Update connection status
				_uart_timer = millis();

				// Read data from client
				std::string command;
				while (_uart_port->available()) {
					char c = _uart_port->read();
					if (c == '\n') break;
					command += c;
				}

				// Respond to name and MAC address
				if (command == "ARCTIC_COMMAND_GET_DEVICE") {
					std::string response_com = "ARCTIC_COMMAND_GET_DEVICE:";
					std::string mac = WiFi.macAddress().c_str();
					std::string response = response_com + _bleDeviceName + "," + mac;
					_uart_port->println(response.c_str());
				}

				// Respond to services available
				if (command == "ARCTIC_COMMAND_GET_SERVICES") {
					std::string response_com = "ARCTIC_COMMAND_GET_SERVICES:";
					std::string response = response_com;
					for (auto& console : consoles) {
						response += console.get().get_name() + "," + console.get().get_uuid_ats() + "," + console.get().get_uuid_txm() + "," + console.get().get_uuid_txs() + "," + console.get().get_uuid_rxm();
						if (&console != &consoles.back()) response += ":";
					}
					_uart_port->println(response.c_str());
				}

				// Split UUID from input data (UUID:DATA)
				if (command.find(":") != std::string::npos) {
					std::string uuid = command.substr(0, command.find(":"));
					std::string data = command.substr(command.find(":") + 1);
					for (auto& console : consoles) {
						if (console.get().get_uuid_rxm().compare(uuid) == 0) {
							console.get().setNewDataAvailable(true, data);
						}
					}
				}
			}

			// Update connection status by timeout
			if (millis() - _uart_timer > ARCTIC_DEFAULT_UART_TIMEOUT) {
				ArcticClient::arctic_connection_status = false;
			}
			else {
				ArcticClient::arctic_connection_status = true;
			}
			
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}
}