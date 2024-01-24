// Description: This example demonstrates the integration of multiple consoles, RTOS, and WiFi functionalities on an ESP32 system.

// SlimyTask for RTOS Integration:
// SlimyTask simplifies the creation and management of tasks in RTOS environments.
// While SlimyTask is used here for ease, it's not mandatory. You can use your own RTOS implementation.
// SlimyTask is open-source and can be found at https://github.com/SlimeCodex/SlimyTask

// WiFi Connection Commands:
// To connect to WiFi, use "connect -u <your wifi ssid> -p <your wifi password>".
// Use "disconnect" to force a WiFi disconnection.

#include <Arduino.h>
#include <ArcticClient.h>
#include <SmartSyncEvent.h>
#include <SlimyTask.h>
#include <WiFi.h>

ArcticClient arctic_client;
ArcticTerminal console_core("Core Console");
ArcticTerminal console_wifi("WiFi Console");

void task_core(void* pvParameter);
void task_wifi(void* pvParameter);
void task_ota(void* pvParameter);

SlimyTask esp_task_core(8 * 1024, task_core, "core", 1, 1);
SlimyTask esp_task_wifi(8 * 1024, task_wifi, "wifi", 1, 1);
SlimyTask esp_task_ota(8 * 1024, task_ota, "ota", 1, 1);

void setup() {
	Serial.begin(115200);

	arctic_client.begin();
	arctic_client.add(console_core);
	arctic_client.add(console_wifi);
	arctic_client.start();

	esp_task_core.start();
	esp_task_wifi.start();
	esp_task_ota.start();
}

void task_core(void* pvParameter) {
	bool core_enabled = false;
	uint32_t core_counter = 0;
	
	while (1) {
		if (SYNC_EVENT(10) && core_enabled) {
			console_core.printf("%lu > Core task is running %d\n", millis(), core_counter);
			core_counter++;
		}

		if (console_core.available()) {
			ArcticCommand com(console_core.read());

			if (com.base() == "hide") {
				console_core.printf("%lu > Hiding wifi console\n", millis());
				console_wifi.hide();
			}
			if (com.base() == "show") {
				console_core.printf("%lu > Showing wifi console\n", millis());
				console_wifi.show();
			}
			if (com.base() == "enable") {
				console_core.printf("%lu > Starting core dump\n", millis());
				core_enabled = true;
			}
			if (com.base() == "disable") {
				console_core.printf("%lu > Stopping core dump\n", millis());
				core_enabled = false;
			}
			if (com.base() == "get_mac") {
				console_core.printf("%lu > AA:BB:CC:DD:EE:FF\n", millis());
			}
			if (com.base() == "version") {
				console_core.printf("%lu > Version 1.0.0\n", millis());
			}
			if (com.base() == "reset") {
				console_core.printf("%lu > Restarting MCU\n", millis());
				delay(500);
				ESP.restart();
			}
			if (com.base() == "load") {
				std::string progress_bar;
				for (int i = 0; i <= 100; i++) {
					progress_bar.clear();
					progress_bar.append(i / 2, '|');
					progress_bar.append(50 - i / 2, ' ');
					console_core.singlef("%lu > Loading data |%s| %d%%\n", millis(), progress_bar.c_str(), i);
					delay(20);
				}
			}
		}
	}
}

void task_wifi(void* pvParameter) {
	while (1) {
		if (console_wifi.available()) {
			ArcticCommand com(console_wifi.read());

			if (com.base() == "connect") {
				if (com.check("-u") && com.check("-p")) {
					std::string user = com.arg("-u");
					std::string pass = com.arg("-p");

					console_wifi.printf("%lu > Connecting to [%s] with password [%s] ", millis(), user.c_str(), pass.c_str());
					WiFi.begin(user.c_str(), pass.c_str());

					while (WiFi.status() != WL_CONNECTED) {
						console_wifi.printf(".");
						delay(500);
					}
					console_wifi.printf("\n");
					console_wifi.printf("%lu > Connected with IP %s\n", millis(), WiFi.localIP().toString().c_str());
				}
			}
			if (com.base() == "disconnect") {
				console_wifi.printf("%lu > Disconnected\n", millis());
				WiFi.disconnect();
			}
		}
	}
}

void task_ota(void* pvParameter) {
	while (1) {
		if (arctic_client.ota.available()) {
			if (arctic_client.ota.download()) {
				delay(500);
				ESP.restart();
			}
		}
	}
}

void loop() {
	vTaskDelete(NULL);
}