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
	if (existingServer != nullptr) {
		pServer = existingServer;
	}
	createService(existingAdvertising);
}

// Create service: Create console with TX, TXS, RX and Name Characteristics
void ArcticOTA::createService(NimBLEAdvertising* existingAdvertising) {
	NimBLEService* pService = pServer->createService("4fafc201-1fb5-459e-2000-c5c9c3319f00");
	_txCharacteristic = pService->createCharacteristic("4fafc201-1fb5-459e-2000-c5c9c3319a00", NIMBLE_PROPERTY::NOTIFY); // TX
	_rxCharacteristic = pService->createCharacteristic("4fafc201-1fb5-459e-2000-c5c9c3319b00", NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR); // RX
	_rxCharacteristic->setCallbacks(new RxCharacteristicCallbacks(this));
	pService->start(); // Start the service
	existingAdvertising->addServiceUUID(pService->getUUID());
}

// Updates new data flag
void ArcticOTA::setNewDataAvailable(bool available, std::string command) {
	// Process background commands for console
	ArcticCommand com = ArcticCommand(command);
	if (com.base() == "ARCTIC_COMMAND_OTA_SETUP") {
		std::string size_str = com.arg("-s");
		uint32_t size = std::stoul(size_str);
		std::string hash_str = com.arg("-md5");

		_ota_file_size = size;
		_ota_file_hash = hash_str;
	}

	newDataAvailable = available;
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

	bool available = newDataAvailable.load();
	newDataAvailable = false;
	return available;
}

// Send TX: Single line TX
void ArcticOTA::send(const char* format, ...) {
	if (!ArcticClient::arctic_connection_status) return;

	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);

	if (pServer->getConnectedCount() > 0) {
		if (_txCharacteristic) {
			_txCharacteristic->setValue((uint8_t*)buffer, strlen(buffer));
			_txCharacteristic->notify();
		}
	}

	va_end(args);
}

// Read RX: Read RX data until delimiter
std::string ArcticOTA::read(char delimiter) {
	if (_rxCharacteristic) {
		std::string value = _rxCharacteristic->getValue();
		std::stringstream valueStream(value);
		std::string line;
		std::getline(valueStream, line, delimiter);
		return line;
	}
	return std::string();
}

// Read RX: Read raw RX data as vector
std::vector<uint8_t> ArcticOTA::raw() {
	if (_rxCharacteristic) {
		std::string value = _rxCharacteristic->getValue();
		std::vector<uint8_t> bytes(value.begin(), value.end());
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