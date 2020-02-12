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
// Version 0.3, 12.02.2020, AK-Homberger

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Preferences.h>

#include "index_html.h"              // Web site information Gauge / Buttons

#define ENABLE_DEMO 1                // Set to 1 to enable Demo Mode with up/down counter
#define SAFETY_STOP 2                // Defines safety stop for chain up. Stops defined number of events before reaching zero

WebServer server(80);                // Web Server at port 80
Preferences preferences;             // Nonvolatile storage on ESP32 - To store ChainCounter

// Chain Counter

#define Chain_Calibration_Value 0.33 // Translates counter impuls to meter 0,33 m per pulse
#define Chain_Counter_Pin 32         // counter impulse is measured as interrupt on pin 32
unsigned long Last_int_time = 0;     // Time of last interrupt
unsigned long Last_event_time = 0;        // Time of last event for engine watchdog
int ChainCounter = 0;                // Conter for chain events
int LastSavedCounter = 0;            // Stores last ChainCounter saves to nonvolatile storag
int UpDown = 1;                      // 1 =  Chain down / count up, -1 = Chain up / count backwards
int OnOff = 0;                       // Relay On/Off - Off = 0;
unsigned long Watchdog_Timer = 0;    // Watchdog timer to stop relay aftre 1 second inactivity e.g. connection loss to client
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;  // To lock/unlock interrupt

// Relay

#define Chain_Up_Pin 14              // GPIO Pin 14 for Chain Up Relay
#define Chain_Down_Pin 12            // GPIO Pin 14 for Chain Down Relay


// Chain Event Interrupt
// Enters on falling edge
//=======================================
void IRAM_ATTR handleInterrupt() {

  if (millis() > Last_int_time + 10) {  // Debouncing. No new events for 10 Milliseconds
    
    portENTER_CRITICAL_ISR(&mux);

    ChainCounter += UpDown;             // Chain Even: Count up/down
    if ( (ChainCounter == SAFETY_STOP) && (UpDown == -1) && (OnOff == 1) ) {  // Safety stop
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

  // Ralais output
  pinMode(Chain_Up_Pin, OUTPUT);            // sets pin as output
  pinMode(Chain_Down_Pin, OUTPUT);          // sets pin as output
  digitalWrite(Chain_Up_Pin, LOW );         // Relay off
  digitalWrite(Chain_Down_Pin, LOW );       // Relay off

  // Init Chain Count measure with interrupt
  pinMode(Chain_Counter_Pin, INPUT_PULLUP); // sets pin input with pullup
  attachInterrupt(digitalPinToInterrupt(Chain_Counter_Pin), handleInterrupt, FALLING); // attaches pin to interrupt on Falling Edge

  // Init serial
  Serial.begin(115200);
  Serial.print("");
  Serial.println("Start");

  preferences.begin("nvs", false);                  // Open nonvolatile storage (nvs)
  ChainCounter = preferences.getInt("counter", 0);  // Read last saved counter
  LastSavedCounter = ChainCounter;                  
  preferences.end();                                // Close nvs

  // Init WLAN AP
  WiFi.mode(WIFI_AP);
  delay(100);
  
  WiFi.softAP("chaincount", "kette0102"); // AP name and password. Chang to your needs
  
  Serial.println("Starte AP  Chaincounter");
  Serial.print("IP Adresse ");
  Serial.println(WiFi.softAPIP());

  
  // Handle HTTP request events
  
  server.on("/", Ereignis_Index);
  server.on("/gauge.min.js", Ereignis_js);
  server.on("/ADC.txt", Ereignis_ChainCount);
  server.on("/up", Ereignis_Up);
  server.on("/down", Ereignis_Down);
  server.on("/stop", Ereignis_Stop);
  server.on("/reset", Ereignis_Reset);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP Server started");
}


void Ereignis_Up() {                  // Handle UP request
  Ereignis_ChainCount();              // Send response
  Serial.println("Up");
  digitalWrite(Chain_Up_Pin, HIGH );  
  digitalWrite(Chain_Down_Pin, LOW );
  Last_event_time = millis();         
  UpDown = -1;
  OnOff = 1;
}


void Ereignis_Down() {                  // Handle UP request
  Ereignis_ChainCount();                // Send response
  Serial.println("Down");
  digitalWrite(Chain_Up_Pin, LOW );
  digitalWrite(Chain_Down_Pin, HIGH );
  Last_event_time = millis();
  UpDown = 1;
  OnOff = 1;
}

void Ereignis_Stop() {                   // Handle Stop reques
  Ereignis_ChainCount();                 // Send response
  Serial.println("Stop");
  digitalWrite(Chain_Up_Pin, LOW );
  digitalWrite(Chain_Down_Pin, LOW );
  OnOff = 0;
}

void Ereignis_Reset() {                  // Handle reset request to reset counter to zero
  ChainCounter = 0;
  Ereignis_ChainCount();                 // Send response
  Serial.println("Reset");
}



void Ereignis_Index() {   // Wenn "http://<ip address>/" aufgerufen wurde
  server.send(200, "text/html", indexHTML);  //dann Index Webseite senden
}


void Ereignis_js() {     // Wenn "http://<ip address>/gauge.min.js" aufgerufen wurde
  server.send(200, "text/html", gauge);     // dann gauge.min.js senden
}



void Ereignis_ChainCount() {    // Wenn "http://<ip address>/ADC.txt" aufgerufen wurde

  float temp = (ChainCounter * Chain_Calibration_Value); // Chain in meters
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/plain", String (temp));

  Watchdog_Timer = millis();   // Watchdog timer is set to current uptime


#if ENABLE_DEMO == 1                   // Demo Mode - Counts automatically UP/Down every 500 ms
  
  if (OnOff == 1) ChainCounter += UpDown; 

  if ( (ChainCounter == SAFETY_STOP) && (UpDown == -1) && (OnOff == 1) ) {  // Safety stop
  digitalWrite(Chain_Up_Pin, LOW );
    digitalWrite(Chain_Down_Pin, LOW );
    OnOff = 0;
  }
  Last_event_time = millis();
#endif

}

void handleNotFound() {
  server.send(404, "text/plain", "File Not Found\n\n");
}



void loop() {

  server.handleClient();                                        // Handle HTTP requests

  if ( (millis() > Watchdog_Timer + 1000) ||                    // Check connnection
       (OnOff == 1 && (millis() > Last_event_time + 1000)) )  { // Check events if engine is on

    digitalWrite(Chain_Up_Pin, LOW );                           // Relay off after 1 second inactivity
    digitalWrite(Chain_Down_Pin, LOW );
    OnOff = 0;
  }

  if (ChainCounter != LastSavedCounter) {                       // Store Chain Counter to nonvolatile storage (if changed)
    preferences.begin("nvs", false);
    preferences.putInt("counter", ChainCounter);
    LastSavedCounter = ChainCounter;
    preferences.end();
  }

  delay(50);

  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }

}
