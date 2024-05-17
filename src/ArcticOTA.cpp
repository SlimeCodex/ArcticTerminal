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
#include <ArcticOTA.h>

// Constructor for consoles
ArcticOTA::ArcticOTA() {
	pServer = nullptr;
}

// Start: Create server and service
void ArcticOTA::start(NimBLEServer* existingServer, NimBLEAdvertising* existingAdvertising) {
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		if (existingServer != nullptr) {
			pServer = existingServer;
		}
		createService(existingAdvertising);
	}
}

// Start: Create server and service
void ArcticOTA::start() {
	// Assign UUIDs for WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		uuid_ats = ARCTIC_UUID_WIFI_OTA_ATS;
		uuid_txm = ARCTIC_UUID_WIFI_OTA_TX;
		uuid_rxm = ARCTIC_UUID_WIFI_OTA_RX;
	}

	// Assign UUIDs for UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		uuid_ats = ARCTIC_UUID_UART_OTA_ATS;
		uuid_txm = ARCTIC_UUID_UART_OTA_TX;
		uuid_rxm = ARCTIC_UUID_UART_OTA_RX;
	}
}

// Create service: Create console with TX, TXS, RX and Name Characteristics
void ArcticOTA::createService(NimBLEAdvertising* existingAdvertising) {

	// Assign UUIDs for Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		NimBLEService* pService = pServer->createService(ARCTIC_UUID_BLE_OTA_ATS);
		_txCharacteristic = pService->createCharacteristic(ARCTIC_UUID_BLE_OTA_TX, NIMBLE_PROPERTY::NOTIFY); // TX
		_rxCharacteristic = pService->createCharacteristic(ARCTIC_UUID_BLE_OTA_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR); // RX
		_rxCharacteristic->setCallbacks(new RxCharacteristicCallbacks(this));
		pService->start(); // Start the service
		existingAdvertising->addServiceUUID(pService->getUUID());
	}
}

// Updates new data flag
void ArcticOTA::setNewDataAvailable(bool available, std::string command) {
	// Process backend commands for OTA
	ArcticCommand com = ArcticCommand(command);
	if (com.base() == "ARCTIC_COMMAND_OTA_SETUP") {
		std::string size_str = com.arg("-s");
		uint32_t size = std::stoul(size_str);
		std::string hash_str = com.arg("-md5");

		_ota_file_size = size;
		_ota_file_hash = hash_str;
	}
	newDataAvailable = available;

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		if (this->available()) {
			if (this->download()) {
				delay(500);
				ESP.restart();
			}
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		_wifi_command = command;
		if (this->available()) {
			if (this->download()) {
				delay(500);
				ESP.restart();
			}
		}
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		_uart_command = command;
		if (this->available()) {
			if (this->download()) {
				delay(500);
				ESP.restart();
			}
		}
	}
}

// Available RX: Check if new data is available
bool ArcticOTA::available() {
	if (!ArcticClient::arctic_connection_status) {
		if (_ota_started) {
			if (_debug_enabled)
				Serial.println("OTA connection lost, aborting update");
			ota_clear();
		}
		return false;
	}

	// Check OTA state synchronously
	if (_ota_started) {

		// Check if OTA update is done
		if (Update.isFinished()) {
			if (Update.end()) {
				if (_md5_started) {
					_ota_md5.calculate();
					const char* _ota_md5_str = _ota_md5.toString().c_str();
				}
				ota_send_ack("DONE");
				_ota_done = true;
				return true;
			}
			else {
				ota_send_ack("ERROR");
			}
			ota_clear();
		}

		// Check if OTA update timed out
		if (millis() - _ota_timeout > 5000) {
			ota_send_ack("TIMEOUT");
			Update.abort();
			ota_handle_error();
			ota_clear();
			return false;
		}
	}

	newDataAvailable.load();
	if (newDataAvailable) {
		newDataAvailable = false;
		return true;
	}
	return false;
}

// Send TX: Single line TX
void ArcticOTA::send(const char* format, ...) {
	if (!ArcticClient::arctic_connection_status) return;

	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);

	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		if (pServer->getConnectedCount() > 0) {
			if (_txCharacteristic) {
				_txCharacteristic->setValue((uint8_t*)buffer, strlen(buffer));
				_txCharacteristic->notify();
			}
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		// Concatenate UUID and command
		std::string wifi_command = uuid_txm + ":" + std::string(buffer);
		ArcticClient::_uplink_client.println(wifi_command.c_str());
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		// Concatenate UUID and command
		std::string uart_command = uuid_txm + ":" + std::string(buffer);
		ArcticClient::_uart_port->println(uart_command.c_str());
	}

	va_end(args);
}

// Read RX: Read raw RX data as vector
std::vector<uint8_t> ArcticOTA::raw() {
	// Bluetooth
	if (ArcticClient::arctic_interface == ARCTIC_BLUETOOTH) {
		if (_rxCharacteristic) {
			std::string value = _rxCharacteristic->getValue();
			std::vector<uint8_t> bytes(value.begin(), value.end());
			return bytes;
		}
	}

	// WiFi
	if (ArcticClient::arctic_interface == ARCTIC_WIFI) {
		// Convert WiFi command to bytes
		std::vector<uint8_t> bytes(_wifi_command.begin(), _wifi_command.end());
		return bytes;
	}

	// UART
	if (ArcticClient::arctic_interface == ARCTIC_UART) {
		// Convert UART command to bytes
		std::vector<uint8_t> bytes(_uart_command.begin(), _uart_command.end());
		return bytes;
	}

	return std::vector<uint8_t>();
}

// Download OTA file
bool ArcticOTA::download() {
	if (!ArcticClient::arctic_connection_status) return false;
	if (_ota_done) return true;

	if (!_ota_started) {
		_ota_timeout = millis();

		Update.setMD5(_ota_file_hash.c_str());
		if (!Update.begin(_ota_file_size)) {
			ota_send_ack("ERROR");
			ota_handle_error();
			ota_clear();
			return false;
		}
		ota_send_ack("READY");
		_ota_started = true;
	}
	else {
		ota_digest_chunk();
	}
	return false;
}

void ArcticOTA::hide() {
	send("ARCTIC_COMMAND_HIDE");
}

void ArcticOTA::show() {
	send("ARCTIC_COMMAND_SHOW");
}

// Digest OTA chunk
void ArcticOTA::ota_digest_chunk() {
	std::vector<uint8_t> ota_bytes = raw();
	_ota_timeout = millis(); // Reset timeout
	if (!_md5_started) {
		_ota_md5.begin();
		_md5_started = true;
	}
	_ota_md5.add(ota_bytes.data(), ota_bytes.size());
	size_t written = Update.write(ota_bytes.data(), ota_bytes.size());
	if (written != ota_bytes.size()) {
		// Error writing OTA chunk
	}
	ota_send_ack("ACK");
}

// Handle OTA errors
void ArcticOTA::ota_handle_error() {
	int err = Update.getError();
	switch (err) {
		case UPDATE_ERROR_SPACE:
			if (_debug_enabled)
				Serial.println("Not enough space to begin OTA");
			break;
		case UPDATE_ERROR_SIZE:
			if (_debug_enabled)
				Serial.println("Bad size given for the OTA");
			break;
		case UPDATE_ERROR_MD5:
				Serial.println("MD5 hash does not match");
			break;
		case UPDATE_ERROR_MAGIC_BYTE:
			if (_debug_enabled)
				Serial.println("OTA magic byte is not present");
			break;
		default:
			if (_debug_enabled)
				Serial.println("An unknown error occurred");
			break;
	}
}

// Send ACK response, can be READY, ACK, ERROR or DONE
void ArcticOTA::ota_send_ack(const char* ack) {
	if (_debug_enabled)
		Serial.printf("%s[%d]\n", ack, _ack_counter);
	send("%s[%d]", ack, _ack_counter);
	_ack_counter++;
}

// Clear OTA variables
void ArcticOTA::ota_clear() {
	_ota_started = false;
	_ota_done = false;
	_md5_started = false;
	_ota_timeout = 0;
	_ack_counter = 0;
}