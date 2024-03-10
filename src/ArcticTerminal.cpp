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
#include <ArcticTerminal.h>

// Initialize static variables
int ArcticTerminal::serviceCount = 0;

// Constructor for consoles
ArcticTerminal::ArcticTerminal(const std::string& monitorName) {
	_monitorName = monitorName;
	pServer = nullptr;
	serviceID = -1;
}

// Start: Create server and service
void ArcticTerminal::start(NimBLEServer* existingServer, NimBLEAdvertising* existingAdvertising) {
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		if (existingServer != nullptr) {
			pServer = existingServer;
		}
		serviceID = createService(existingAdvertising);
	}
}

// Start: Create server and service
void ArcticTerminal::start() {
	if (ArcticClient::arctic_interface == ARCTIC_WIFI || ArcticClient::arctic_interface == ARCTIC_UART) {
		serviceID = createServiceUUID();
	}
}

// Create service: Create console with TX, TXS, RX and Name Characteristics
int ArcticTerminal::createService(NimBLEAdvertising* existingAdvertising) {

	// Assign UUIDs for Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {

		// Create service
		char serviceUUID[37];
		snprintf(serviceUUID, sizeof(serviceUUID), "4fafc201-1fb5-459e-3%03x-c5c9c3319f%02x", serviceCount, serviceCount);
		NimBLEService* pService = pServer->createService(serviceUUID);

		// TX
		char txCharUUID[37];
		snprintf(txCharUUID, sizeof(txCharUUID), "4fafc201-1fb5-459e-3%03x-c5c9c3319a%02x", serviceCount, serviceCount);
		NimBLECharacteristic* txCharacteristic = pService->createCharacteristic(txCharUUID, NIMBLE_PROPERTY::NOTIFY);

		// TX (single)
		char txsCharUUID[37];
		snprintf(txsCharUUID, sizeof(txsCharUUID), "4fafc201-1fb5-459e-3%03x-c5c9c3319b%02x", serviceCount, serviceCount);
		NimBLECharacteristic* txsCharacteristic = pService->createCharacteristic(txsCharUUID, NIMBLE_PROPERTY::NOTIFY);

		// RX
		char rxCharUUID[37];
		snprintf(rxCharUUID, sizeof(rxCharUUID), "4fafc201-1fb5-459e-3%03x-c5c9c3319c%02x", serviceCount, serviceCount);
		NimBLECharacteristic* rxCharacteristic = pService->createCharacteristic(rxCharUUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
		rxCharacteristic->setCallbacks(new RxCharacteristicCallbacks(this));

		// Start the service
		pService->start();
		existingAdvertising->addServiceUUID(pService->getUUID());

		// Add service to map
		services[serviceCount] = ServiceCharacteristics{txCharacteristic, txsCharacteristic, rxCharacteristic};
		return serviceCount++;
	}
	return -1;
}

int ArcticTerminal::createServiceUUID() {
	// Assign UUIDs for WiFi and UART
	if (ArcticClient::arctic_interface == ARCTIC_WIFI || ArcticClient::arctic_interface == ARCTIC_UART) {

		// Service
		char uuid_service[6];
		snprintf(uuid_service, sizeof(uuid_service), "ATS%02x", serviceCount);

		// TX (multiline)
		char uuid_txm[6];
		snprintf(uuid_txm, sizeof(uuid_txm), "TXM%02x", serviceCount);

		// TX (single)
		char uuid_txs[6];
		snprintf(uuid_txs, sizeof(uuid_txs), "TXS%02x", serviceCount);

		// RX
		char uuid_rxm[6];
		snprintf(uuid_rxm, sizeof(uuid_rxm), "RXM%02x", serviceCount);

		serialServices[serviceCount] = ServiceStringUUIDs{uuid_service, uuid_txm, uuid_txs, uuid_rxm};
		return serviceCount++;
	}
	return -1;
}

// Printf TX: Multiline TX with format
void ArcticTerminal::printf(const char* format, ...) {
	if (!ArcticClient::arctic_connection_status) return;
	if (serviceID == -1) {
		return;
	}

	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		auto servicePair = services.find(serviceID);
		if (servicePair != services.end()) {
			if (pServer->getConnectedCount() > 0) {
				NimBLECharacteristic* txCharacteristic = servicePair->second.txCharacteristic;
				if (txCharacteristic) {
					txCharacteristic->setValue((uint8_t*)buffer, strlen(buffer));
					txCharacteristic->notify(true);
				}
			}
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		auto servicePair = serialServices.find(serviceID);
		if (servicePair != serialServices.end()) {
			// concatenate the serial string with the buffer
			std::string txString = servicePair->second.uuid_txm;
			txString += ":";
			txString += buffer;
			txString += "\n";

			ArcticClient::_uplink_client.print(txString.c_str());
		}
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		auto servicePair = serialServices.find(serviceID);
		if (servicePair != serialServices.end()) {
			// concatenate the serial string with the buffer
			std::string txString = servicePair->second.uuid_txm;
			txString += ":";
			txString += buffer;
			txString += "\n";

			ArcticClient::_uart_port->print(txString.c_str());
		}
	}

	va_end(args);
}

// Singlef TX: Single line TX with format
void ArcticTerminal::singlef(const char* format, ...) {
	if (!ArcticClient::arctic_connection_status) return;
	if (serviceID == -1) {
		return;
	}
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		auto servicePair = services.find(serviceID);
		if (servicePair != services.end()) {
			if (pServer->getConnectedCount() > 0) {
				NimBLECharacteristic* txsCharacteristic = servicePair->second.txsCharacteristic;
				if (txsCharacteristic) {
					txsCharacteristic->setValue((uint8_t*)buffer, strlen(buffer));
					txsCharacteristic->notify();
				}
			}
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		if (ArcticClient::_uplink_client && ArcticClient::_uplink_client.connected()) {
			auto servicePair = serialServices.find(serviceID);
			if (servicePair != serialServices.end()) {
				// concatenate the serial string with the buffer
				std::string txString = servicePair->second.uuid_txs;
				txString += ":";
				txString += buffer;
				txString += "\n";

				ArcticClient::_uplink_client.print(txString.c_str());
			}
		}
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		auto servicePair = serialServices.find(serviceID);
		if (servicePair != serialServices.end()) {
			// concatenate the serial string with the buffer
			std::string txString = servicePair->second.uuid_txs;
			txString += ":";
			txString += buffer;
			txString += "\n";

			ArcticClient::_uart_port->print(txString.c_str());
		}
	}

	va_end(args);
}

// Updates new data flag
void ArcticTerminal::setNewDataAvailable(bool available, std::string command) {
	// Process backend commands for console
	ArcticCommand com = ArcticCommand(command);
	if (com.base() == "ARCTIC_COMMAND_GET_NAME") {
		singlef("ARCTIC_COMMAND_REQ_NAME:%s", _monitorName.c_str());
		newDataAvailable = false;
		return;
	}

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		// We do nothing becase the data is already handled by the callback
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		_wifi_command = command;
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		_uart_command = command;
	}
	newDataAvailable = available;
}

// Available RX: Check if new data is available
bool ArcticTerminal::available() {
	if (!ArcticClient::arctic_connection_status) return false;
	bool available = newDataAvailable.load();
	newDataAvailable = false;
	return available;
}

// Read RX: Read RX data until delimiter
std::string ArcticTerminal::read(char delimiter) {
	if (!ArcticClient::arctic_connection_status) return std::string();

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		auto servicePair = services.find(serviceID);
		if (servicePair != services.end()) {
			NimBLECharacteristic* rxCharacteristic = servicePair->second.rxCharacteristic;
			if (rxCharacteristic) {
				std::string value = rxCharacteristic->getValue();
				size_t pos = value.find(delimiter);
				if (pos != std::string::npos) {
					std::string data = value.substr(0, pos);
					rxCharacteristic->setValue(value.substr(pos + 1));
					return data;
				}
			}
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		std::string command = _wifi_command;
		return command;
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		std::string command = _uart_command;
		size_t pos = command.find(delimiter);
		if (pos != std::string::npos) {
			std::string data = command.substr(0, pos);
			_uart_command = command.substr(pos + 1);
			return data;
		}
	}
	return std::string();
}

// Read RX: Read raw RX data as vector
std::vector<uint8_t> ArcticTerminal::raw() {

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		auto servicePair = services.find(serviceID);
		if (servicePair != services.end()) {
			NimBLECharacteristic* rxCharacteristic = servicePair->second.rxCharacteristic;
			if (rxCharacteristic) {
				return rxCharacteristic->getValue();
			}
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		std::string command = _wifi_command;
		return std::vector<uint8_t>(command.begin(), command.end());
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		std::string command = _uart_command;
		return std::vector<uint8_t>(command.begin(), command.end());
	}
	return std::vector<uint8_t>();
}

void ArcticTerminal::hide() {
	singlef("ARCTIC_COMMAND_HIDE");
}

void ArcticTerminal::show() {
	singlef("ARCTIC_COMMAND_SHOW");
}

std::string ArcticTerminal::get_name() {
	return _monitorName;
}

std::string ArcticTerminal::get_uuid_ats() {
	auto servicePair = serialServices.find(serviceID);
	return serialServices[serviceID].uuid_ats;
}

std::string ArcticTerminal::get_uuid_txm() {
	auto servicePair = serialServices.find(serviceID);
	return serialServices[serviceID].uuid_txm;
}

std::string ArcticTerminal::get_uuid_txs() {
	auto servicePair = serialServices.find(serviceID);
	return serialServices[serviceID].uuid_txs;
}

std::string ArcticTerminal::get_uuid_rxm() {
	auto servicePair = serialServices.find(serviceID);
	return serialServices[serviceID].uuid_rxm;
}