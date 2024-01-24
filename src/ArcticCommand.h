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

#include <Arduino.h>

#include <sstream>
#include <string>
#include <map>

class ArcticCommand {
public:
	ArcticCommand(const std::string& input) {
		std::istringstream stream(input);
		std::string token;
		std::string lastKey;

		stream >> baseCommand;
		while (stream >> token) {
			if (token[0] == '-') {
				lastKey = token;
				arguments[token] = "";
			}
			else if (!lastKey.empty()) {
				arguments[lastKey] = token;
				// Serial.printf("Parsed argument: %s, Value: %s\n", lastKey.c_str(), token.c_str()); // Corrected debug output
				lastKey = "";
			}
		}

		// Debugging output
		// Serial.printf("Base command: %s\n", baseCommand.c_str());
	}

	std::string base() const {
		return baseCommand;
	}

	bool check(const std::string& arg) const {
		return arguments.find(arg) != arguments.end();
	}

	std::string arg(const std::string& arg) const {
		auto it = arguments.find(arg);
		if (it != arguments.end()) {
			return it->second;
		}
		return "";
	}

private:
	std::string baseCommand;
	std::map<std::string, std::string> arguments;
};