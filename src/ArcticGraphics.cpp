/*
 * This file is part of ArcticGraphics Library.
 * Copyright (C) 2023 Alejandro Nicolini
 *
 * ArcticGraphics is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ArcticGraphics is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ArcticGraphics. If not, see <https://www.gnu.org/licenses/>.
 */

#include <ArcticClient.h>
#include <ArcticGraphics.h>

// Initialize static variables
int ArcticGraphics::serviceCount = 0;

// Constructor for plotters
ArcticGraphics::ArcticGraphics(const std::string& monitorName) {
	_monitorName = monitorName;
	pServer = nullptr;
	serviceID = -1;
}

// Start: Create server and service
void ArcticGraphics::start(NimBLEServer* existingServer, NimBLEAdvertising* existingAdvertising) {
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		if (existingServer != nullptr) {
			pServer = existingServer;
		}
		serviceID = createService(existingAdvertising);
	}
}

// Start: Create server and service
void ArcticGraphics::start() {
	if (ArcticClient::arctic_interface == ARCTIC_WIFI || ArcticClient::arctic_interface == ARCTIC_UART) {
		serviceID = createServiceUUID();
	}
}

// Create service: Create console with TX, TXS, RX and Name Characteristics
int ArcticGraphics::createService(NimBLEAdvertising* existingAdvertising) {

	// Assign UUIDs for Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {

		// Create service
		char serviceUUID[37];
		snprintf(serviceUUID, sizeof(serviceUUID), ARCTIC_UUID_BLE_GRAPH_ATS, serviceCount, serviceCount);
		NimBLEService* pService = pServer->createService(serviceUUID);

		// TX
		char txCharUUID[37];
		snprintf(txCharUUID, sizeof(txCharUUID), ARCTIC_UUID_BLE_GRAPH_TX, serviceCount, serviceCount);
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

int ArcticGraphics::createServiceUUID() {
	// Assign UUIDs for WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {

		// Service
		char uuid_service[9];
		snprintf(uuid_service, sizeof(uuid_service), ARCTIC_UUID_WIFI_GRAPH_ATS, serviceCount);

		// TX (multiline)
		char uuid_txm[9];
		snprintf(uuid_txm, sizeof(uuid_txm), ARCTIC_UUID_WIFI_GRAPH_TX, serviceCount);

		serialServices[serviceCount] = ServiceStringUUIDs{uuid_service, uuid_txm};
		return serviceCount++;
	}

	// Assign UUIDs for UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {

		// Service
		char uuid_service[9];
		snprintf(uuid_service, sizeof(uuid_service), ARCTIC_UUID_UART_GRAPH_ATS, serviceCount);

		// TX (multiline)
		char uuid_txm[9];
		snprintf(uuid_txm, sizeof(uuid_txm), ARCTIC_UUID_UART_GRAPH_TX, serviceCount);

		serialServices[serviceCount] = ServiceStringUUIDs{uuid_service, uuid_txm};
		return serviceCount++;
	}
	return -1;
}


void ArcticGraphics::plot(std::initializer_list<float> values) {
	this->plot(_monitorName, values, {});
}

void ArcticGraphics::plot(std::string plot_name, std::initializer_list<float> values, std::initializer_list<std::string> labels) {
	if (!ArcticClient::arctic_connection_status) return;
	if (!ArcticClient::arctic_uplink_enabled) return;
	if (serviceID == -1) {
		return;
	}

	// Check if the number of values and labels are equal
	if (values.size() != labels.size()) {
		return;
	}

    std::string buffer = plot_name; // Start with the plot name

    if (values.size() > 0) {
        auto label_it = labels.begin();
        for (auto value : values) {
            if (label_it == labels.end()) {
                break; // Break if there are no more labels
            }

            if (&value != &*values.begin()) {
                buffer += ARCTIC_DEFAULT_PRIMARY_DELIMITER;
            }
            buffer += std::to_string(value) + ARCTIC_DEFAULT_SECONDARY_DELIMITER + *label_it;

            ++label_it; // Move to the next label
        }

        // Handle any remaining labels that don't have corresponding values
        while (label_it != labels.end()) {
            buffer += ARCTIC_DEFAULT_PRIMARY_DELIMITER + std::string("NaN") + ARCTIC_DEFAULT_SECONDARY_DELIMITER + *label_it;
            ++label_it;
        }
    } else {
        // In case there are no values, append a default message or handle it appropriately
        buffer += ARCTIC_DEFAULT_PRIMARY_DELIMITER + std::string("No data available");
    }

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		auto servicePair = services.find(serviceID);
		if (servicePair != services.end()) {
			if (pServer->getConnectedCount() > 0) {
				NimBLECharacteristic* txCharacteristic = servicePair->second.txCharacteristic;
				if (txCharacteristic) {
					txCharacteristic->setValue((uint8_t*)buffer.c_str(), buffer.length());
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
			txString += buffer;

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
			txString += buffer;

			ArcticClient::_uart_port->print(txString.c_str());
			ArcticClient::_uart_port->print(ARCTIC_DEFAULT_SCAPE_SEQUENCE);
		}
	}
}

std::string ArcticGraphics::get_name() {
	return _monitorName;
}

std::string ArcticGraphics::get_uuid_ats() {
	auto servicePair = serialServices.find(serviceID);
	return serialServices[serviceID].uuid_ats;
}

std::string ArcticGraphics::get_uuid_txm() {
	auto servicePair = serialServices.find(serviceID);
	return serialServices[serviceID].uuid_txm;
}