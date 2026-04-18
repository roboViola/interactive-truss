#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include "HX711.h"
#include "../libraries/message_types.h"

// Addressible LED properties
const uint8_t NUM_LEDS = 5;
const uint8_t LED_DATA = D4;

Adafruit_NeoPixel strip(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);

// Define colors for the addressible LEDs
// Pixels are GRB, not RGB
uint32_t RED = strip.Color(0, 255, 0); // Compression (-)
uint32_t YELLOW = strip.Color(255, 255, 0); // Zero-Force
uint32_t BLUE = strip.Color(0, 0, 255); // Tension (+)

// Define structures used for data transmission
sense_msg forceMsg;
zero_msg zeroMsg;
pair_msg pairMsg;

// Define variables for data transmission
uint8_t hubAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t defaultId = 99; // Default ID value that represents unassigned ID
const uint8_t chan = 1; // All devices will be set to the same channel, so no need to parse
esp_now_peer_info_t peerInfo;

// Define HX711 Module
const uint8_t DAT_PIN = 2;
const uint8_t CLK_PIN = 3;

HX711 forceSensor;

// Define offset and scaling 
const float sensor_scale = 0; // FIXME: add actual value

// SetLightColors(): Determines the color and number of addressible LEDs to turn on
void SetLightColors(float force) {
    /* 
    Force -> LED Conversion
    - 0.5 <= Force < 10: 1 LED
    - 10 <= Force < 20: 2 LED
    - 20 <= Force < 30: 3 LED
    - 30 <= Force < 40: 4 LED
    - 40 <= Force < 50: 5 LED
    */

    // Clear the last colors from the LED strip
    strip.clear();

    // Check if Zero-Force Member
    if (abs(force) < 0.5) {
        strip.fill(YELLOW, 0, 1);
        Serial.println("Yellow");
    }
    // Check if Compression Member
    else if (force < 0) {
        strip.fill(RED, 0, force / 10);
        Serial.println("Red");
    }
    // Check if Tension Member
    else {
        strip.fill(BLUE, 0, force / 10);
        Serial.println("Blue");
    }
}

// OnDataSent(): Executes when data is sent
bool OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    return status == ESP_NOW_SEND_SUCCESS;
}

// OnDataRecv(): Executes when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int length) {
    uint8_t mac_addr_pair[6];

    // Determine which message type is being received
    switch (incomingData[0]) {
    // Reset signal
    case static_cast<int>(MessageType::MSG_RESET) :      // we received data from server
        memcpy(&zeroMsg, incomingData, sizeof(zeroMsg));
        
        break;

    // Pairing data
    case static_cast<int>(MessageType::MSG_PAIR_SV):    // we received pairing data from server
        memcpy(&pairMsg, incomingData, sizeof(pairMsg));
        
        if (pairMsg.id > 0) {              // the message comes from server
            addPeer(pairMsg.mac_addr, chan); // add the server  to the peer list 
            defaultId = pairMsg.id; // set the ID number of the link
        }
        
        break;
  }  
}

void addPeer(const uint8_t * mac_addr, uint8_t chan){
  // Set channel and check for errors
  ESP_ERROR_CHECK(esp_wifi_set_channel(chan, WIFI_SECOND_CHAN_NONE));
  
  // Set up properties
  esp_now_del_peer(mac_addr);
  memset(&peerInfo, 0, sizeof(esp_now_peer_info_t));
  peerInfo.channel = chan;
  peerInfo.encrypt = false;
  memcpy(peerInfo.peer_addr, mac_addr, sizeof(uint8_t[6]));

  // Check for error adding peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    //return;
  }

  // Copy peer data
  memcpy(hubAddr, mac_addr, sizeof(uint8_t[6]));
}

// Runs once at startup to initialize program values and settings
void setup() {
    // Start serial monitor with 2 second delay
    Serial.begin(115200);
    delay(2000);
    
    // Set device to be a WiFi Station
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_err_t init_err = esp_now_init();

    // Check for initialization errors
    if (init_err != ESP_OK) {
        // FIXME: Update to flash one of the LEDs as an error code
        Serial.println("Error initializing WiFi Station");
        return;
    }
    
    // Set callback function of transmitted packet status
    esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

    // Set callback function of received packed status
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

    // Set peer information
    memcpy(peerInfo.peer_addr, hubAddr, sizeof(hubAddr));
    peerInfo.channel = chan;
    peerInfo.encrypt = false;

    esp_wifi_get_mac(WIFI_IF_STA, pairMsg.mac_addr);
    pairMsg.id = defaultId; // This is used to know the link is not paired yet

    // Add peer and check for errors 
    esp_err_t peer_err = esp_now_add_peer(&peerInfo);

    if (peer_err != ESP_OK) {
        // FIXME: Update to flash one of the LEDs as an error code
        Serial.println("Error adding peer");
        //return;
    }

    // Setup HX711 Module
    forceSensor.begin(DAT_PIN, CLK_PIN);
    forceSensor.set_raw_mode(); // Sets the HX711 module to raw read (no averaging)
    forceSensor.set_scale(sensor_scale);

    // Setup the addressible LED strip
    strip.begin();
    strip.show();

    // TEST
    forceMsg.force_data = 0;
}

// Runs continuously after setup() to perform main program functions
void loop() {
    // For testing and debugging communications
    forceMsg.force_data = forceMsg.force_data + 1;
    SetLightColors(forceMsg.force_data); // Set LED colors
    strip.show(); // Push the color data out to the addressible LEDs 
    
    // Send pairing request message if not already paired
    if (defaultId == 99) {
        pairMsg.msg_type = MessageType::MSG_PAIR_SN;
        esp_err_t send_err = esp_now_send(hubAddr, (uint8_t *) &pairMsg, sizeof(pairMsg));
        //Serial.println(defaultId);
    }
    // Send force data if pairing complete
    else {
        forceMsg.id = defaultId;

        /*if (forceSensor.is_ready()) {
            forceMsg.force_data = forceSensor.get_units();
            forceMsg.force_data = forceSensor.get_units(); // Move into structure for transmission
            
            SetLightColors(forceMsg.force_data); // Set LED colors
            strip.show(); // Push the color data out to the addressible LEDs 
        }*/

        esp_err_t send_err = esp_now_send(hubAddr, (uint8_t *) &forceMsg, sizeof(forceMsg));
    }

    delay(250); // Reduce sample rate and data transmission to conserve battery life
}