/*
 * This file is part of ArcticMap Library.
 * Copyright (C) 2024 Alejandro Nicolini
 *
 * ArcticMap is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ArcticMap is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ArcticMap. If not, see <https://www.gnu.org/licenses/>.
 */

#include <ArcticClient.h>
#include <ArcticMap.h>

// Initialize static variables
int ArcticMap::serviceCount = 0;

// Constructor for plotters
ArcticMap::ArcticMap(const std::string& monitorName) {
	_map_name = monitorName;
	pServer = nullptr;
	serviceID = -1;
	ArcticClient::add(*this);
}

// Start: Create server and service
void ArcticMap::start(NimBLEServer* existingServer, NimBLEAdvertising* existingAdvertising) {
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		if (existingServer != nullptr) {
			pServer = existingServer;
		}
		serviceID = createService(existingAdvertising);
	}
}

// Start: Create server and service
void ArcticMap::start() {
	if (ArcticClient::arctic_interface == ARCTIC_WIFI || ArcticClient::arctic_interface == ARCTIC_UART) {
		serviceID = createServiceUUID();
	}
}

// Create service: Create console with TX, TXS, RX and Name Characteristics
int ArcticMap::createService(NimBLEAdvertising* existingAdvertising) {

	// Assign UUIDs for Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {

		// Create service
		char serviceUUID[37];
		snprintf(serviceUUID, sizeof(serviceUUID), ARCTIC_UUID_BLE_MAP_ATS, serviceCount, serviceCount);
		NimBLEService* pService = pServer->createService(serviceUUID);

		// TX
		char txCharUUID[37];
		snprintf(txCharUUID, sizeof(txCharUUID), ARCTIC_UUID_BLE_MAP_TX, serviceCount, serviceCount);
		NimBLECharacteristic* txCharacteristic = pService->createCharacteristic(txCharUUID, NIMBLE_PROPERTY::NOTIFY);

		// Start the service
		pService->start();
		existingAdvertising->addServiceUUID(pService->getUUID());

		// Add service to map
		services[serviceCount] = ServiceCharacteristics{txCharacteristic};
		return serviceCount++;
	}
	return -1;
}

int ArcticMap::createServiceUUID() {
	// Assign UUIDs for WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {

		// Service
		char uuid_service[9];
		snprintf(uuid_service, sizeof(uuid_service), ARCTIC_UUID_WIFI_MAP_ATS, serviceCount);

		// TX (multiline)
		char uuid_txm[9];
		snprintf(uuid_txm, sizeof(uuid_txm), ARCTIC_UUID_WIFI_MAP_TX, serviceCount);

		serialServices[serviceCount] = ServiceStringUUIDs{uuid_service, uuid_txm};
		return serviceCount++;
	}

	// Assign UUIDs for UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {

		// Service
		char uuid_service[9];
		snprintf(uuid_service, sizeof(uuid_service), ARCTIC_UUID_UART_MAP_ATS, serviceCount);

		// TX (multiline)
		char uuid_txm[9];
		snprintf(uuid_txm, sizeof(uuid_txm), ARCTIC_UUID_UART_MAP_TX, serviceCount);

		serialServices[serviceCount] = ServiceStringUUIDs{uuid_service, uuid_txm};
		return serviceCount++;
	}
	return -1;
}

void ArcticMap::location(double latitude, double longitude) {
	if (!ArcticClient::arctic_connection_status) return;
	if (!ArcticClient::arctic_uplink_enabled) return;
	if (serviceID == -1) {
		return;
	}

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		auto servicePair = services.find(serviceID);
		if (servicePair != services.end()) {
			if (pServer->getConnectedCount() > 0) {
				NimBLECharacteristic* txCharacteristic = servicePair->second.txCharacteristic;
				if (txCharacteristic) {
                    std::string buffer = std::to_string(latitude) + "," + std::to_string(longitude);
					
					txCharacteristic->setValue(buffer);
					txCharacteristic->notify(true);
				}
			}
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		auto servicePair = serialServices.find(serviceID);
		if (servicePair != serialServices.end()) {
			// Concatenate the serial string with the buffer
			std::string txString = servicePair->second.uuid_txm;
			txString += ":";
            txString += std::to_string(latitude) + "," + std::to_string(longitude);

			ArcticClient::_uplink_client.print(txString.c_str());
			ArcticClient::_uplink_client.print(ARCTIC_DEFAULT_SCAPE_SEQUENCE);
		}
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		auto servicePair = serialServices.find(serviceID);
		if (servicePair != serialServices.end()) {
			// Concatenate the serial string with the buffer
			std::string txString = servicePair->second.uuid_txm;
			txString += ":";
            txString += std::to_string(latitude) + "," + std::to_string(longitude);

			ArcticClient::_uart_port->print(txString.c_str());
			ArcticClient::_uart_port->print(ARCTIC_DEFAULT_SCAPE_SEQUENCE);
		}
	}
}

std::string ArcticMap::get_name() {
	return _map_name;
}

std::string ArcticMap::get_uuid_ats() {
	auto servicePair = serialServices.find(serviceID);
	return serialServices[serviceID].uuid_ats;
}

std::string ArcticMap::get_uuid_txm() {
	auto servicePair = serialServices.find(serviceID);
	return serialServices[serviceID].uuid_txm;
}