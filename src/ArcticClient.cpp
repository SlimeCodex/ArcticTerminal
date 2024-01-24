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
BLEConnParams ArcticClient::arctic_cparams = {0, 0, 0, 0};

// Constructor for handler
ArcticClient::ArcticClient(const std::string& bleDeviceName) {
	_bleDeviceName = bleDeviceName;
	ArcticClient::arctic_cparams.mtu = BLE_ATT_MTU_MAX; // Default MTU
	profile(ARCTIC_PROFILE_HIGH_SPEED); // Default profile
}

// Begin: Initialize BLE
void ArcticClient::begin() {
	NimBLEDevice::init(_bleDeviceName);
	pServer = NimBLEDevice::createServer();
	pServer->setCallbacks(new ATCallbacks());
}

// Add console to global map
void ArcticClient::add(ArcticTerminal& console) {
	consoles.push_back(console);
}

// Start: Start BLE server and advertising
void ArcticClient::start() {
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