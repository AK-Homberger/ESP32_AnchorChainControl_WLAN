/*
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// Anchor Chain Remote Control / Chain Counter with WLAN.
// Version 1.0, 14.02.2020, AK-Homberger

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Preferences.h>

#include "index_html.h"              // Web site information for Gauge / Buttons

#define ENABLE_DEMO 1                // Set to 1 to enable Demo Mode with up/down counter
#define SAFETY_STOP 2                // Defines safety stop for chain up. Stops defined number of events before reaching zero
#define MAX_CHAIN_LENGTH 40          // Define maximum chan length. Relay off after the value is reached

// Wifi: Select AP or Client

#define WiFiMode_AP_STA 0            // Defines WiFi Mode 0 -> AP (with IP:192.168.4.1 and  1 -> Station (client with IP: via DHCP)
const char *ssid = "chaincount";     // Set WLAN name
const char *password = "kette0102";  // Set password


WebServer server(80);                // Web Server at port 80
Preferences preferences;             // Nonvolatile storage on ESP32 - To store ChainCounter

// Chain Counter

#define Chain_Calibration_Value 0.33 // Translates counter impuls to meter 0,33 m per pulse
#define Chain_Counter_Pin 32         // Counter impulse is measured as interrupt on GPIO pin 32
unsigned long Last_int_time = 0;     // Time of last interrupt
unsigned long Last_event_time = 0;   // Time of last event for engine watchdog
int ChainCounter = 0;                // Counter for chain events
int LastSavedCounter = 0;            // Stores last ChainCounter value to allow storage to nonvolatile storage in case of value changes
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;  // To lock/unlock interrupt

// Relay

#define Chain_Up_Pin 14              // GPIO pin 14 for Chain Up Relay
#define Chain_Down_Pin 12            // GPIO pin 14 for Chain Down Relay
int UpDown = 1;                      // 1 =  Chain down / count up, -1 = Chain up / count backwards
int OnOff = 0;                       // Relay On/Off - Off = 0, On = 1
unsigned long Watchdog_Timer = 0;    // Watchdog timer to stop relay after 1 second of inactivity e.g. connection loss to client



// Chain Event Interrupt
// Enters on falling edge
//=======================================
void IRAM_ATTR handleInterrupt() {

  if (millis() > Last_int_time + 10) {  // Debouncing. No new events for 10 milliseconds

    portENTER_CRITICAL_ISR(&mux);

    ChainCounter += UpDown;             // Chain event: Count up/down

    if ( ( (ChainCounter <= SAFETY_STOP) && (UpDown == -1) && (OnOff == 1) ) ||     // Safety stop counter reached while chain is going up
         ( (UpDown == 1) && (abs(ChainCounter) * Chain_Calibration_Value >= MAX_CHAIN_LENGTH) ) ) {  // Maximum chain lenght reached

      digitalWrite(Chain_Up_Pin, LOW );
      digitalWrite(Chain_Down_Pin, LOW );
      OnOff = 0;
    }
    Last_event_time = millis();         // Store last event time to detect blocking chain

    portEXIT_CRITICAL_ISR(&mux);
  }
  Last_int_time = millis();             // Store last interrupt time for debouncing
}



void setup() {
  int wifi_retry = 0;

  // Relay output
  pinMode(Chain_Up_Pin, OUTPUT);            // Sets pin as output
  pinMode(Chain_Down_Pin, OUTPUT);          // Sets pin as output
  digitalWrite(Chain_Up_Pin, LOW );         // Relay off
  digitalWrite(Chain_Down_Pin, LOW );       // Relay off

  // Init Chain Count measure with interrupt
  pinMode(Chain_Counter_Pin, INPUT_PULLUP); // Sets pin input with pullup resistor
  attachInterrupt(digitalPinToInterrupt(Chain_Counter_Pin), handleInterrupt, FALLING); // Attaches pin to interrupt on falling edge

  // Init serial
  Serial.begin(115200);
  Serial.print("");
  Serial.println("Start");

  preferences.begin("nvs", false);                  // Open nonvolatile storage (nvs)
  ChainCounter = preferences.getInt("counter", 0);  // Read last saved counter
  LastSavedCounter = ChainCounter;                  // Initialise last counter value
  preferences.end();                                // Close nvs

  // Init WLAN AP
  if (WiFiMode_AP_STA == 0) {

    WiFi.mode(WIFI_AP);                              // WiFi Mode Access Point
    delay (100);
    WiFi.softAP(ssid, password); // AP name and password
    Serial.println("Start WLAN AP");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());

  } else {

    Serial.println("Start WLAN Client DHCP");         // WiFi Mode Client with DHCP
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {           // Check connection
      wifi_retry++;
      delay(500);
      Serial.print(".");
      if (wifi_retry > 10) {
        Serial.println("\nReboot");                   // Reboot after 10 connection tries
        ESP.restart();
      }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

  }


  // Handle HTTP request events

  server.on("/", Event_Index);
  server.on("/gauge.min.js", Event_js);
  server.on("/ADC.txt", Event_ChainCount);
  server.on("/up", Event_Up);
  server.on("/down", Event_Down);
  server.on("/stop", Event_Stop);
  server.on("/reset", Event_Reset);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP Server started");
}


void Event_Up() {                          // Handle UP request
  server.send(200, "text/plain", "-1000"); // Send response "-1000" means no  chainlenght
  Serial.println("Up");
  digitalWrite(Chain_Up_Pin, HIGH );
  digitalWrite(Chain_Down_Pin, LOW );
  Last_event_time = millis();
  UpDown = -1;
  OnOff = 1;
}


void Event_Down() {                         // Handle Down request
  server.send(200, "text/plain", "-1000");  // Send response "-1000" means no  chainlenght
  Serial.println("Down");
  digitalWrite(Chain_Up_Pin, LOW );
  digitalWrite(Chain_Down_Pin, HIGH );
  Last_event_time = millis();
  UpDown = 1;
  OnOff = 1;
}

void Event_Stop() {                         // Handle Stop request
  server.send(200, "text/plain", "-1000");  // Send response "-1000" means no  chainlenght
  Serial.println("Stop");
  digitalWrite(Chain_Up_Pin, LOW );
  digitalWrite(Chain_Down_Pin, LOW );
  OnOff = 0;
}

void Event_Reset() {                        // Handle reset request to reset counter to 0
  ChainCounter = 0;                         
  server.send(200, "text/plain", "-1000");  // Send response "-1000" means no  chainlenght
  Serial.println("Reset");
}



void Event_Index() {                         // If "http://<ip address>/" requested
  server.send(200, "text/html", indexHTML);  // Send Index Website
}


void Event_js() {                            // If "http://<ip address>/gauge.min.js" requested
  server.send(200, "text/html", gauge);      // Then send gauge.min.js
}



void Event_ChainCount() {                    // If  "http://<ip address>/ADC.txt" requested

  float temp = (ChainCounter * Chain_Calibration_Value); // Chain in meters
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/plain", String (temp));

  Watchdog_Timer = millis();                 // Watchdog timer is set to current uptime


#if ENABLE_DEMO == 1                         // Demo Mode - Counts automatically UP/Down every 500 ms

  if (OnOff == 1) ChainCounter += UpDown;

  if ( ( (ChainCounter <= SAFETY_STOP) && (UpDown == -1) && (OnOff == 1) ) ||     // Safety stop counter reached while chain is going up
       ( (UpDown == 1) && (abs(ChainCounter) * Chain_Calibration_Value >= MAX_CHAIN_LENGTH) ) ) {  // Maximum chain lenght reached

    digitalWrite(Chain_Up_Pin, LOW );
    digitalWrite(Chain_Down_Pin, LOW );
    OnOff = 0;
  }
  Last_event_time = millis();
#endif

}

void handleNotFound() {                                           // Unknown request. Send error 404
  server.send(404, "text/plain", "File Not Found\n\n");
}



void loop() {
  int wifi_retry = 0;

  server.handleClient();                                           // Handle HTTP requests

  if ( ( millis() > Watchdog_Timer + 1000 ) ||                     // Check HTTP connnection
       ( (OnOff == 1) && (millis() > Last_event_time + 1000)) )  { // Check events if engine is on

    digitalWrite(Chain_Up_Pin, LOW );                              // Relay off after 1 second inactivity
    digitalWrite(Chain_Down_Pin, LOW );
    OnOff = 0;
  }

  if (ChainCounter != LastSavedCounter) {                          // Store Chain Counter to nonvolatile storage (if changed)
    preferences.begin("nvs", false);
    preferences.putInt("counter", ChainCounter);
    LastSavedCounter = ChainCounter;
    preferences.end();
  }

  if (WiFiMode_AP_STA == 1) {                                      // Check connection if working as client

    while (WiFi.status() != WL_CONNECTED && wifi_retry < 5 ) {     // Connection lost, 5 tries to reconnect
      wifi_retry++;
      Serial.println("WiFi not connected. Try to reconnect");
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      delay(100);
    }
    if (wifi_retry >= 5) {
      Serial.println("\nReboot");                                  // Did not work -> restart ESP32
      ESP.restart();
    }
  }

  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }
}
