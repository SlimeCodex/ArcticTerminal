/*
 * This file is part of ArcticTerminal Library.
 * Copyright (C) 2024 Alejandro Nicolini
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

#include <ArcticClient.h>
#include <ArcticTerminal.h>
#include <ArcticOTA.h>

// Callback Connection per server
class ATCallbacks : public NimBLEServerCallbacks {
public:
	void onConnect(NimBLEServer* pServer) {
		ArcticClient::arctic_connection_status = true;
	};

	void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
		// clang-format off
		pServer->updateConnParams(
			desc->conn_handle,
			ArcticClient::arctic_cparams.min,
			ArcticClient::arctic_cparams.max,
			0,
			ArcticClient::arctic_cparams.sup
		);
		// clang-format on
		NimBLEDevice::setMTU(ArcticClient::arctic_cparams.mtu);
		ArcticClient::arctic_connection_status = true;
	};

	void onDisconnect(NimBLEServer* pServer) {
		NimBLEDevice::startAdvertising();
		ArcticClient::arctic_connection_status = false;
	};

	// TODO: Implement MTU negotiation
	void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc){
	};
};

// Callback RX per console
class RxCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
	ArcticClient* handler_instance;
	ArcticTerminal* console_instance;
	ArcticOTA* ota_instance;

public:
	RxCharacteristicCallbacks(ArcticTerminal* console) {
		console_instance = console;
	}
	RxCharacteristicCallbacks(ArcticClient* handler) {
		handler_instance = handler;
	}
	RxCharacteristicCallbacks(ArcticOTA* ota) {
		ota_instance = ota;
	}
	void onWrite(NimBLECharacteristic* pCharacteristic) {
		if (console_instance) {
			console_instance->setNewDataAvailable(true, pCharacteristic->getValue());
		}
		if (ota_instance) {
			ota_instance->setNewDataAvailable(true, pCharacteristic->getValue());
		}
		if (handler_instance) {
			handler_instance->setNewDataAvailable(true, pCharacteristic->getValue());
		}
	}
};