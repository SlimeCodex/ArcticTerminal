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
	if (existingServer != nullptr) {
		pServer = existingServer;
	}
	serviceID = createService(existingAdvertising);
}

// Create service: Create console with TX, TXS, RX and Name Characteristics
int ArcticTerminal::createService(NimBLEAdvertising* existingAdvertising) {

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

	va_end(args);
}

// Updates new data flag
void ArcticTerminal::setNewDataAvailable(bool available, std::string command) {
	// Process background commands for console
	ArcticCommand com = ArcticCommand(command);
	if (com.base() == "ARCTIC_COMMAND_GET_NAME") {
		singlef("ARCTIC_COMMAND_REQ_NAME:%s", _monitorName.c_str());
		newDataAvailable = false;
		return;
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
	auto servicePair = services.find(serviceID);
	if (servicePair != services.end()) {
		NimBLECharacteristic* rxCharacteristic = servicePair->second.rxCharacteristic;
		if (rxCharacteristic) {
			std::string value = rxCharacteristic->getValue();
			std::stringstream valueStream(value);
			std::string line;
			std::getline(valueStream, line, delimiter);
			return line;
		}
	}
	return std::string();
}

// Read RX: Read raw RX data as vector
std::vector<uint8_t> ArcticTerminal::raw() {
	if (!ArcticClient::arctic_connection_status) return std::vector<uint8_t>();
	auto servicePair = services.find(serviceID);
	if (servicePair != services.end()) {
		NimBLECharacteristic* rxCharacteristic = servicePair->second.rxCharacteristic;
		if (rxCharacteristic) {
			std::string value = rxCharacteristic->getValue();
			std::vector<uint8_t> bytes(value.begin(), value.end());
			return bytes;
		}
	}
	return std::vector<uint8_t>();
}

void ArcticTerminal::hide() {
	singlef("ARCTIC_COMMAND_HIDE");
}

void ArcticTerminal::show() {
	singlef("ARCTIC_COMMAND_SHOW");
}