#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "../libraries/message_types.h"

// Define structures used for data transmission
zero_msg zeroMsg;
pair_msg pairMsg;

// Define variables for data transmission
const uint8_t NUM_LINKS = 16;
bool buttonStates[16] = {false};
float linkForceData[NUM_LINKS]; // Array for all the force data
uint8_t next_pair_id = 1; // Next ID number to be given to a peer
esp_now_peer_info_t peerInfo;

// Define SSID and password for access-point station
const char* ssid = "StationDemo";
const char* password = "StationDemo";

// Set up webserver
AsyncWebServer server(80); // Use HTTP port 80
AsyncEventSource events("/events"); // Live updates to webpage viewed on phone or laptop
unsigned long lastEventTime = 0;

// Define reset pin
const bool ZERO_PIN = 0; // FIXME: Replace with actual pin number

// resetForceVals():
void resetForceVals(float data[16]) {
    for (uint8_t i = 0; i < NUM_LINKS; i++) {
        data[i] = 1000;
    }
}

// OnDataSent(): Executes when data is sent
bool OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    return status == ESP_NOW_SEND_SUCCESS;
}

// OnDataRecv(): Executes when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int length) {
    sense_msg forceMsg;
    uint8_t mac_addr_pair[6];

    // Determine which message type is being received
    switch (incomingData[0]) {
    // Force data
    case static_cast<int>(MessageType::MSG_DATA):
        // Copy received data to data structure and array for HTML page
        memcpy(&forceMsg, incomingData, sizeof(forceMsg));
        linkForceData[forceMsg.id - 1] = forceMsg.force_data;
        
        break;

    // Pairing data
    case static_cast<int>(MessageType::MSG_PAIR_SN):
        // Copy received data to data structure
        memcpy(&pairMsg, incomingData, sizeof(pairMsg));

        if (pairMsg.id > 0) {     // do not replay to server itself
            if (pairMsg.msg_type == MessageType::MSG_PAIR_SN) { 
                pairMsg.id = next_pair_id; // Set ID number to assign link module with
                next_pair_id++;

                // Add peer
                addPeer(pairMsg.mac_addr);

                // Prepare and send return message with assigned ID
                pairMsg.msg_type = MessageType::MSG_PAIR_SV;
                esp_wifi_get_mac(WIFI_IF_STA, pairMsg.mac_addr);
                esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t *) &pairMsg, sizeof(pairMsg));
            }
        }
        
        break; 
    }    
}

// addPeer(): Adds discovered peer to list of peers
bool addPeer(const uint8_t *peer_addr) { 
  // Copy and set up local data
  memset(&peerInfo, 0, sizeof(peerInfo));
  const esp_now_peer_info_t *peer = &peerInfo;
  memcpy(peerInfo.peer_addr, peer_addr, 6);
  
  // Set communication properties
  peerInfo.channel = 1; // pick a channel
  peerInfo.encrypt = 0; // no encryption

  // Check if the peer exists
  bool exists = esp_now_is_peer_exist(peerInfo.peer_addr);
  if (exists) {
    // Peer already paired.
    Serial.println("Already Paired");
    return true;
  }
  else {
    esp_err_t addStatus = esp_now_add_peer(peer);
    if (addStatus == ESP_OK) {
      // Pair success
      Serial.println("Pair success");
      return true;
    }
    else 
    {
      Serial.println("Pair failed");
      return false;
    }
  }
} 

void setupServer(float data[16], bool buttons[16]) {

    server.on("/", HTTP_GET, [data](AsyncWebServerRequest *request) {
        request->send(200, "text/html", generateWebPage(data));
    });

    server.on("/data", HTTP_GET, [data](AsyncWebServerRequest *request) {
        request->send(200, "application/json", arrayToJSON(data));
    });

    server.on("/update", HTTP_GET, [buttons](AsyncWebServerRequest *request) {

        if (request->hasParam("b")) {
            String val = request->getParam("b")->value();
            parseButtonJSON(val, buttons);
        }

        request->send(200, "text/plain", "OK");
    });

    server.begin();
}

// HMTL page Index -> Runs when called by callback function
/* Convert float array to JSON */
String arrayToJSON(float arr[16]) {
    String json = "[";
    
    for (uint8_t i = 0; i < 16; i++) {
        json += String(arr[i], 1);
        
        if (i < 15) {
            json += ",";
        }
    }

    json += "]";

    return json;
}

void parseButtonJSON(String str, bool out[16]) {
    for (uint8_t i = 0; i < 16; i++) out[i] = false;

    uint8_t idx = 0;

    for (uint8_t i = 0; i < str.length(); i++) {
        if ((str[i] == '0' || str[i] == '1') && idx < 16) {
            out[idx++] = (str[i] == '1');
        }
    }
}

/* HTML page */
String generateWebPage(float data[16]) {

    String json = arrayToJSON(data);

    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">

<style>
body {
    margin: 0;
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh;
    background: #111;
    font-family: Arial;
}

.grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 10px;
    width: 90vw;
    height: 90vh;
}

button {
    border: none;
    border-radius: 15px;
    width: 100%;
    height: 100%;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
}
</style>

</head>

<body>

<div class="grid" id="grid"></div>

<script>

let values = )rawliteral" + json + R"rawliteral(;
let buttons = new Array(16).fill(false);

// getColor(): Determines the color of the UI buttons
function getColor(val) {

    if (val === 1000) {
        return ["gray", "white"];
    }

    if (val >= -0.5 && val <= 0.5) {
        return ["yellow", "black"];
    }

    if (val > 0) {
        return ["blue", "white"];
    }

    return ["red", "black"];
}

// buildGrid(): creates the 4 x 4 grid of buttons with outputs
function buildGrid() {

    const grid = document.getElementById("grid");

    for (let i = 0; i < 16; i++) {

        const btn = document.createElement("button");
        btn.id = "btn-" + i;

        const title = document.createElement("div");
        title.style.fontSize = "18pt";
        title.textContent = "Link " + (i + 1);

        const value = document.createElement("div");
        value.style.fontSize = "14pt";
        value.id = "val-" + i;

        btn.appendChild(title);
        btn.appendChild(value);

        // MOMENTARY INPUT (still active even if Offline)
        btn.onmousedown = () => sendState(i, true);
        btn.onmouseup = () => sendState(i, false);
        btn.onmouseleave = () => sendState(i, false);

        btn.ontouchstart = () => sendState(i, true);
        btn.ontouchend = () => sendState(i, false);
        btn.ontouchcancel = () => sendState(i, false);

        grid.appendChild(btn);
    }
}

// updateButtons(): Updates the data and text on the buttons without rebuilding them
function updateButtons() {

    for (let i = 0; i < 16; i++) {

        const val = values[i];

        const btn = document.getElementById("btn-" + i);
        const valEl = document.getElementById("val-" + i);

        if (!btn || !valEl) continue;

        const color = getColor(val);

        btn.style.backgroundColor = color[0];
        btn.style.color = color[1];

        if (val === 1000) {
            valEl.textContent = "Offline";
        } else {
            valEl.textContent = val.toFixed(1);
        }
    }
}

// sendState(): sends whether the button was pressed or not back to ESP32 board
function sendState(index, state) {

    buttons[index] = state;

    let payload = "";
    for (let i = 0; i < 16; i++) {
        payload += buttons[i] ? "1" : "0";
    }

    fetch("/update?b=" + payload);
}

// updateValues(): Fetches and updates the webpage
function updateValues() {

    fetch("/data")
    .then(r => r.json())
    .then(data => {
        values = data;
        updateButtons();
    });
}

setInterval(updateValues, 250);

// INIT
buildGrid();

</script>

</body>
</html>
)rawliteral";

    return html;
}

// Runs once at startup to initialize program values and settings
void setup() {
    // Start Serial Terminal with delay
    Serial.begin(115200);
    delay(2000); // wait 2 seconds

    // Set device to be a WiFi Access-Point Station
    WiFi.mode(WIFI_AP_STA);
    esp_err_t init_err = esp_now_init();
    WiFi.softAP(ssid, password);

    // Check for initialization errors
    if (init_err != ESP_OK) {
        // FIXME: Update to flash one of the LEDs as an error code
        Serial.println("Error initializing WiFi Access Point Station");
        //return;
    }
    
    // Set callback function of transmitted packet status
    esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

    // Set callback function of received packed status
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

    /*// Callback function for requesting the main page of the webserver
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
        //request->send(200, "text/html", "<h1>Hello from ESP32 Async Server!</h1>");
    });*/
    setupServer(linkForceData, buttonStates);
    
    // Start events handler
    server.addHandler(&events);
    server.begin();

    // Set pin mode for reset pin
    pinMode(ZERO_PIN, INPUT); 
    resetForceVals(linkForceData);

    // TEST
    zeroMsg.zero_signal = false;
}

// Runs continuously after setup() to perform main program functions
void loop() {
    // Data Transmission Debug Test
    zeroMsg.zero_signal = !zeroMsg.zero_signal;
    
    // Check if the zero button was pressed
    if (digitalRead(ZERO_PIN) == HIGH) {
        zeroMsg.zero_signal = HIGH;
    }

    // Send reset to all links
    esp_err_t send_err = esp_now_send(0, (uint8_t *) &zeroMsg, sizeof(zeroMsg));

    for (uint8_t i = 0; i < NUM_LINKS; i++) {
        Serial.println(linkForceData[i]);
    }

    // Update table data every 400 ms
    if(millis() - lastEventTime >= 400) {
        lastEventTime = millis();

        String jsonArray = arrayToJSON(linkForceData);

        // Update table data
        events.send(jsonArray.c_str(), "update", millis());
  }

    delay(500); // Reduce sample rate and data transmission to conserve battery life
}