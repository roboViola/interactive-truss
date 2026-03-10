#include <esp_wifi.h>
#include <Wifi.h>

void getMacAddress() {
  // Variable setup to hold MAC address
  uint8_t macAddr[6];
  esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, macAddr);

  // Check for error obtaining MAC address
  if (err != ESP_OK) {
    // Output error message to serial monitor
    Serial.println("Error getting MAC address");
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

void setup() {
  // Initialize serial monitor
  Serial.begin(115200);

  // Setup WiFi station and start WiFi
  WiFi.mode(WIFI_STA);
  WiFI.STA.begin();

  // Obtain the MAC address
  getMacAddress();
}

void loop() {
  // Empty since MAC address needs to be obtained only once during setup
}
