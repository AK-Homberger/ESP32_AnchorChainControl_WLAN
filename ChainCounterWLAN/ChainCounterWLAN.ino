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

// Chain Counter WLAN. Counts events on GPIO 32 and shows it in Web Gauge via WLAN
// Version 0.2, 10.02.2020, AK-Homberger

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "index_html.h"

WebServer server(80);

// Chain Counter

#define Chain_Calibration_Value 0.33 // Translates counter impuls to meter 0,33 m per pulse
#define Chain_Counter_Pin 32  // counter impulse is measured as interrupt on pin 32
unsigned long Last_int_time = 0;
int ChainCounter = 0;
int UpDown=1; // 1 =  Chain down / count up, -1 = Chain up / count backwards
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;  


// Relais

#define Chain_Up_Pin 14
#define Chain_Down_Pin 12

void debug_log(char* str) {
#if ENABLE_DEBUG_LOG == 1
  Serial.println(str);
#endif
}

// Chain Event Interrupt
// Enters on falling edge
//=======================================
void IRAM_ATTR handleInterrupt() {

  if (millis() > Last_int_time + 10) {  // Debouncing
    portENTER_CRITICAL_ISR(&mux);
    ChainCounter+=UpDown;
    portEXIT_CRITICAL_ISR(&mux);
  }
  Last_int_time = millis();
}



void setup() {
  
  // Ralais output
  pinMode(Chain_Up_Pin, OUTPUT);                                            // sets pin 
  pinMode(Chain_Down_Pin, OUTPUT);                                          // sets pin high
  digitalWrite(Chain_Up_Pin, LOW );
  digitalWrite(Chain_Down_Pin, LOW );

  // Init Chain Count measure with interrupt
  pinMode(Chain_Counter_Pin, INPUT_PULLUP);                                 // sets pin input with pullup 
  attachInterrupt(digitalPinToInterrupt(Chain_Counter_Pin), handleInterrupt, FALLING); // attaches pin to interrupt on Falling Edge

  // Init serial
  Serial.begin(115200);
  Serial.print("");
  Serial.println("Start");

  // Init WLAN AP
  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.softAP("chaincount", "kette0102"); // Name und Passwort des Wi-Fi Netzes
  Serial.println("Starte AP  Chaincounter");
  Serial.print("IP Adresse ");       //Ausgabe aktueller IP des Servers
  Serial.println(WiFi.softAPIP());

  server.on("/", Ereignis_Index);
  server.on("/gauge.min.js", Ereignis_js);
  server.on("/ADC.txt", Ereignis_ChainCount);
  server.on("/up", Ereignis_Up);
  server.on("/down", Ereignis_Down);
  server.on("/stop", Ereignis_Stop);
  server.on("/reset", Ereignis_Reset);
  
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP Server gestarted");
}


void Ereignis_Up(){
  Ereignis_ChainCount();
  Serial.println("Up");
  digitalWrite(Chain_Up_Pin, HIGH );
  digitalWrite(Chain_Down_Pin, LOW );
  UpDown=-1;
}


void Ereignis_Down(){
  Ereignis_ChainCount();
  Serial.println("Down");
  digitalWrite(Chain_Up_Pin, LOW );
  digitalWrite(Chain_Down_Pin, HIGH );
  UpDown=1;
}

void Ereignis_Stop(){
  Ereignis_ChainCount();
  Serial.println("Stop");
  digitalWrite(Chain_Up_Pin, LOW );
  digitalWrite(Chain_Down_Pin, LOW );
}

void Ereignis_Reset(){
  ChainCounter=0;
  Ereignis_ChainCount();
  Serial.println("Reset");
}



void Ereignis_Index()    // Wenn "http://<ip address>/" aufgerufen wurde
{
  server.send(200, "text/html", indexHTML);  //dann Index Webseite senden
}

void Ereignis_js()      // Wenn "http://<ip address>/gauge.min.js" aufgerufen wurde
{
  server.send(200, "text/html", gauge);     // dann gauge.min.js senden
}

void Ereignis_ChainCount()     // Wenn "http://<ip address>/ADC.txt" aufgerufen wurde
{
  float temp = (ChainCounter * Chain_Calibration_Value); // Chain in meters
  server.sendHeader("Cache-Control", "no-cache");  // Sehr wichtig !!!!!!!!!!!!!!!!!!!
  server.send(200, "text/plain", String (temp));   // dann text mit ADC Wert senden
}

void handleNotFound()
{
  server.send(404, "text/plain", "File Not Found\n\n");
}


void loop() {

  server.handleClient();

  delay(50);

  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }

}
