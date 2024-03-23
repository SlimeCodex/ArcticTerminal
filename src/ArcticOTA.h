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

#include <atomic>
#include <string>
#include <vector>

#include <MD5Builder.h>
#include <NimBLEDevice.h>
#include <Update.h>

class ArcticOTA {
public:
	ArcticOTA();
	void start(NimBLEServer* existingServer, NimBLEAdvertising* existingAdvertising);
	bool available();
	bool download();
	void hide();
	void show();
	void send(const char* format, ...);
	std::string read(char delimiter = '\n');
	std::vector<uint8_t> raw();

	void createService(NimBLEAdvertising* existingAdvertising);
	void setNewDataAvailable(bool available, std::string command);
	NimBLECharacteristic* _txCharacteristic;
	NimBLECharacteristic* _rxCharacteristic;

private:
	NimBLEServer* pServer;
	NimBLEService* pService;

	bool _debug_enabled = false;
	std::atomic<bool> newDataAvailable{false};

	// OTA variables
	bool _ota_started = false;
	bool _ota_done = false;
	bool _md5_started = false;
	unsigned long _ota_timeout = 0;
	unsigned int _ack_counter = 0;
	MD5Builder _ota_md5;
	uint32_t _ota_file_size = 0;
	std::string _ota_file_hash = "";

	// OTA functions
	void ota_digest_chunk();
	void ota_handle_error();
	void ota_send_ack(const char* ack);
	void ota_clear();
};