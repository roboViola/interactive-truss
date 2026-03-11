#include <esp_now.h>
#include <WiFi.h>
#include "HX711.h"
#include "../libraries/message_types.h"

// Define structures used for data transmission
sense_msg forceMsg;
zero_msg zeroMsg;

// Define variables for data transmission
uint8_t hubAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Replace with Hub Module Address
esp_now_peer_info_t peerInfo;

// Define HX711 Module
HX711 forceSensor;

// Define offset and scaling 
float sensor_scale = 0; // FIXME: add actual value

bool OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    return status == ESP_NOW_SEND_SUCCESS;
}

bool OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int length) {
    memcpy(&zeroMsg, incomingData, sizeof(zeroMsg));

    if (zeroMsg.zero_signal == true) {
        forceSensor.tare();
    }
}

// Runs once at startup to initialize program values and settings
void setup() {
    // Set device to be a WiFi Station
    WiFi.mode(WIFI_STA);
    esp_err_t init_err = esp_now_init();
    
    // Set callback function of transmitted packet status
    esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

    // Set peer information
    memcpy(peerInfo.peer_addr, hubAddr, sizeof(hubAddr));
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Add peer and check for errors 
    esp_err_t peer_err = esp_now_add_peer(&peerInfo);

    if (peer_err != ESP_OK) {
        // FIXME: Update to flash one of the LEDs as an error code
        Serial.println("Error adding peer");
        return;
    }

    // Setup HX711 Module
    forceSensor.begin(0, 0); // FIXME: Add dataPin and clockPin ports, respectively
    forceSensor.set_raw_mode(); // Sets the HX711 module to raw read (no averaging)
    forceSensor.set_scale(sensor_scale);
}

// Runs continuously after setup() to perform main program functions
void loop() {
    // Obtain adjusted force data from the Wheatstone Bridge via the HX711
    if (forceSensor.is_ready()) {
        forceMsg.force_data = forceSensor.get_units();
    }

    esp_err_t send_err = esp_now_send(hubAddr, (uint8_t *) &forceMsg, sizeof(forceMsg));

    if (send_err != ESP_OK) {
        // FIXME: Update to flash one of the LEDs as an error code
        Serial.println("Error sending data");
        return;
    }

    delay(150); // Reduce sample rate and data transmission to conserve battery life
}