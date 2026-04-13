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

// HMTL page Index -> Runs when called by callback function
/* Convert float array to JSON */
String arrayToJSON()
{
  String json = "[";

  for(int i=0;i<NUM_LINKS;i++)
  {
    json += String(linkForceData[i]);

    if(i < NUM_LINKS-1)
      json += ",";
  }

  json += "]";
  return json;
}

/* HTML page */
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Interactive Truss</title>
<style>
body {
    font-family:Arial;
    margin:20px;
}
table {
    border-collapse:collapse;
}
th,td {
    border:1px solid black;
    padding:8px;
    text-align:center;
}
</style>
</head>
<body>
<h1>Interactive Truss</h1>
<table id="trussTable">
    <tr>
        <th>Link</th>
        <th>Value</th>
    </tr>
</table>

<script>
    const table = document.getElementById("trussTable");

    /* Create table rows */
    for(let i=0;i<NUM_LINKS;i++) {
        let row = table.insertRow();
        let linkCell = row.insertCell(0);
        let valueCell = row.insertCell(1);

        linkCell.innerHTML = "Link " + (i+1);

        valueCell.id = "value"+i;
        valueCell.innerHTML = "0";
    }

    /* Connect to ESP32 SSE stream */
    const source = new EventSource("/events");

    /* Event listener for event.send() */
    source.addEventListener("update", function(event) {
        let data = JSON.parse(event.data);

        for(let i=0;i<NUM_LINKS;i++) {
            document.getElementById("value"+i).innerHTML = data[i];
        }
    });

</script>
</body>
</html>
)rawliteral";

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

    // Callback function for requesting the main page of the webserver
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
        //request->send(200, "text/html", "<h1>Hello from ESP32 Async Server!</h1>");
    });
    
    // Start events handler
    server.addHandler(&events);
    server.begin();

    // Set pin mode for reset pin
    pinMode(ZERO_PIN, INPUT); 

    // TEST
    zeroMsg.zero_signal = false;
}

// Runs continuously after setup() to perform main program functions
void loop() {
    // Data Transmission Debug Test
    zeroMsg.zero_signal = !zeroMsg.zero_signal;

    // Send reset to all links
    esp_err_t send_err = esp_now_send(0, (uint8_t *) &zeroMsg, sizeof(zeroMsg));

    for (uint8_t i = 0; i < NUM_LINKS; i++) {
        Serial.println(linkForceData[i]);
    }

    // Update table data every 400 ms
    if(millis() - lastEventTime >= 400) {
        lastEventTime = millis();

        String jsonArray = arrayToJSON();

        // Update table data
        events.send(jsonArray.c_str(), "update", millis());
  }

    delay(500); // Reduce sample rate and data transmission to conserve battery life
}