#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "../libraries/message_types.h"

// Define structures used for data transmission
sense_msg forceMsg;
zero_msg zeroMsg;

// Define variables for data transmission
const uint8_t NUM_LINKS = 16;
const uint8_t linkAddrs[NUM_LINKS][6] = { // Add all MAC addresses for the links
    {0x94, 0x54, 0xC5, 0xB0, 0x92, 0x68}, // Link 1
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 2
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 3
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 4
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 5
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 6
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 7
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 8
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 9
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 10
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 11
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 12
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 13
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 14
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Link 15
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} // Link 16
};
float linkForceData[NUM_LINKS]; // Array for all the force data, index maps with linkAddrs
esp_now_peer_info_t peerInfo;

// Define SSID and password for access-point station
const char* ssid = "StationDemo";
const char* password = "StationDemo";

// Set up webserver
AsyncWebServer server(80); // Use HTTP port 80
AsyncEventSource events("/events"); // Live updates to webpage viewed on phone or laptop
unsigned long lastEventTime = 0;

// Initialize data queues
QueueHandle_t linkAddrsQueue; // Queue for the received link MAC addresses
QueueHandle_t forceDataQueue; // Queue for the received force data

// Define queue variables for loop processing
uint8_t linkAddr;
float forceData;

// Define reset pin
const bool ZERO_PIN = 0; // FIXME: Replace with actual pin number

// OnDataSent(): Executes when data is sent
bool OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    return status == ESP_NOW_SEND_SUCCESS;
}

// OnDataRecv(): Executes when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int length) {
    /*
    // Copy received data to data structure
    memcpy(&forceMsg, incomingData, sizeof(forceMsg));

    // Add received data to data queue for processing in main loop without packet loss
    xQueueSend(linkAddrsQueue, &mac_addr, 0);
    xQueueSend(forceDataQueue, &forceMsg.force_data, 0);
    */
   string test;
   memcpy(&test, incomingData, sizeof(test))
   Serial.println(test);
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
    // Start Serial Terminal
    Serial.begin(115200);
    delay(2000); // wait 2 seconds
    // Set device to be a WiFi Access-Point Station
    WiFi.mode(WIFI_AP);
    esp_err_t init_err = esp_now_init();
    //delay(10000);
    WiFi.softAP(ssid, password);

    //WiFi.begin(ssid, password);
    //delay(5000);
    Serial.print("Setting AP (Access Point)…");
    Serial.println(WiFi.softAPIP());

    if (init_err != ESP_OK) {
        // FIXME: Update to flash one of the LEDs as an error code
        Serial.println("Error initializing WiFi Access Point Station");
        //return;
    }
    
    // Set callback function of transmitted packet status
    esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

    // Set callback function of received packed status
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

    // Set peer information
    for (uint8_t i = 0; i < sizeof(linkAddrs) / sizeof(linkAddrs[0]); i++) {
        memcpy(peerInfo.peer_addr, linkAddrs[i], sizeof(linkAddrs[i]));
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        // Add peer and check for errors 
        esp_err_t peer_err = esp_now_add_peer(&peerInfo);

        if (peer_err != ESP_OK) {
            // FIXME: Update to flash one of the LEDs as an error code
            Serial.println("Error adding peer");
            //return;
        }
    }

    // Callback function for requesting the main page of the webserver
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
        //request->send(200, "text/html", "<h1>Hello from ESP32 Async Server!</h1>");
    });
    
    // Start events handler
    server.addHandler(&events);
    Serial.println("Getting ready to start server");
    server.begin();
    Serial.println("Server started");
    //delay(5000);

    // Set pin mode for reset pin
    pinMode(ZERO_PIN, INPUT); 

    // Create force data queue
    linkAddrsQueue = xQueueCreate(NUM_LINKS, sizeof(uint8_t));
    forceDataQueue = xQueueCreate(NUM_LINKS, sizeof(float));
}

// Runs continuously after setup() to perform main program functions
void loop() {
    
    // Check if the zero button was pressed
    if (digitalRead(ZERO_PIN) == HIGH) {
        zeroMsg.zero_signal = HIGH;
    }

    esp_err_t send_err = esp_now_send(0, (uint8_t *) &zeroMsg, sizeof(zeroMsg));

    if (send_err != ESP_OK) {
        // FIXME: Update to flash one of the LEDs as an error code
        Serial.println("Error sending data");
        //return;
    }

    // FIFO unload data from the queues for processing and addition to data arrays
    if (xQueueReceive(linkAddrsQueue, &linkAddr, 0) && xQueueReceive(forceDataQueue, &forceData, 0)) {
        // Look for the link address that sent the data
        for (uint8_t i = 0; i < sizeof(linkAddrs) / sizeof(linkAddrs[0]); i++) {
            if (memcmp(&linkAddr, linkAddrs[i], 6)) {
                // Update the force data array element that corresponds to the link address
                linkForceData[i] = forceData;
            }
        }
    }

    // Update table data every 400 ms
    if(millis() - lastEventTime >= 400) {
        lastEventTime = millis();

        String jsonArray = arrayToJSON();

        // Update table data
        events.send(jsonArray.c_str(), "update", millis());
  }

    delay(50); // Reduce sample rate and data transmission to conserve battery life
}