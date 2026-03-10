#include <esp_now.h>
#include <Wifi.h>
#include "HX711.h"
#include "message_types.h"

// Define structures used for data transmission
sense_msg forceSensor;
zero_msg zeroMsg;

// Setup peer for recieving
esp_now_peer_info_t peerInfo;

bool OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    return status == ESP_NOW_SEND_SUCCESS;
}

// Runs once at startup to initialize program values and settings
void setup() {
    // Set device to be a WiFi Station
    Wifi.mode(WIFI_STA);
    esp_err_t init_err = esp_now_init();
    
    // Obtain MAC address as 6 digit array
    WiFi.macAddress(linkAddr);

    // Check for error obtaining MAC address
    if (init_err != ESP_OK) {
        // Output error message to serial monitor
        // FIXME: Update to flash one of the LEDs as an error code
        Serial.println("Error obtaining MAC address");
        return;
    }
    
    // Set callback function of transmitted packet status
    esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

    // Set peer information
    memcpy(peerInfo.peer_addr, linkAddr, sizeof(linkAddr));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer and check for errors 
    esp_err_t peer_err = esp_now_add_peer(&peerInfo);

    if (peer_err != ESP_OK) {
        // FIXME: Update to flash one of the LEDs as an error code
        Serial.println("Error adding peer");
        return;
    }
}

// Runs continuously after setup() to perform main program functions
void loop() {
    forceSensor.force_data = 0; // FIXME: change to be the sensor data acquired from HX711

    esp_err_t send_err = esp_now_send(linkAddr, (uint8_t *) &forceSensor, sizeof(forceSensor));
}