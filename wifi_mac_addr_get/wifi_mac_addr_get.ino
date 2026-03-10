#include <esp_wifi.h>
#include <Wifi.h>

// getMacAddress(): Obtains, formats, and outputs the MAC address of ESP32
void getMacAddress() {
  // Variable setup to hold MAC address
  uint8_t macAddr[6];
  esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, macAddr);

  // Check for error obtaining MAC address
  if (err != ESP_OK) {
    // Output error message to serial monitor
    Serial.println("Error obtaining MAC address");
    return;
  }

  // Format and output the MAC address to the serial monitor
  Serial.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", macAddr[i]);

    if (i < 5) {
      Serial.print(":");
    }
  }
}

// Runs once at startup to initialize program values and settings
void setup() {
  // Initialize serial monitor
  Serial.begin(115200);

  // Setup WiFi station and start WiFi
  WiFi.mode(WIFI_STA);
  WiFI.STA.begin();

  // Obtain the MAC address
  getMacAddress();
}

// Runs continuously after setup() to perform main program functions
void loop() {
  // Empty since MAC address needs to be obtained only once during setup
}
