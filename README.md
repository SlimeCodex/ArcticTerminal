# ArcticTerminal Library

ArcticTerminal is a library for the ESP32 microcontroller. It's specifically designed to allow the communication with the [ArcticStream](https://github.com/SlimeCodex/ArcticStream) Python-based visualization software. This library plays a crucial role in enabling wireless Bluetooth connections between an ESP32 device and the ArcticStream application.

## Key Features
- **ArcticStream Integration**: Specially tailored for easy and efficient connection with the ArcticStream visualizer.
- **Robust BLE Communication**: Enables strong and stable Bluetooth connections, ensuring reliable data transfer.
- **Thread-Safety**: Guarantees safe and consistent operation in multi-threaded environments.
- **Easy-to-Use API**: Simplifies the process of setting up BLE servers and managing client connections.
- **Flexible Data Handling**: Offers a variety of formats and profiles for data transmission and reception, tailored to diverse application needs.

## Installation using PlatformIO

PlatformIO offers a streamlined way to manage your project dependencies.
1. Open your platformio.ini file in the root of your project.
2. Add the following line under your environment's lib_deps:

```
lib_deps =
	https://github.com/SlimeCodex/ArcticTerminal@^1.0.0
```

3. Save the `platformio.ini` file.
4. Build your project. PlatformIO will automatically fetch and install the ArcticTerminal library.

## Installation for Arduino IDE
1. Download the repository as a .zip file.
2. Open the Arduino IDE and use the 'Import Library' option to install the downloaded file.
3. This library relies on NimBLE for Bluetooth Low Energy (BLE) functionalities. Therefore, you also need to install the NimBLE library.

## Disabling NimBLE Logs
To prevent NimBLE debug logs from appearing in your serial output, you need to add a build flag in your `platformio.ini` file. This will set the NimBLE C++ log level to 0, effectively disabling these logs. Make sure to add this line under the appropriate environment section in your `platformio.ini`:

```
build_flags = 
	-DCONFIG_NIMBLE_CPP_LOG_LEVEL=0
```

## Usage

Here's a basic example of how to use the ArcticTerminal Library:

```cpp
#include <Arduino.h>
#include <ArcticClient.h>

ArcticClient arctic_client;
ArcticTerminal simple_console("Simple Console");

void setup() {
	arctic_client.begin();
	arctic_client.add(simple_console);
	arctic_client.start();
}

void loop() {
	simple_console.printf("Hello World!\n");
	delay(1000);
}
```

# License

ArcticTerminal is released under the GNU General Public License v3.0. See the LICENSE file for full license text.