# ESP32 Anchor Chain Remote Contol / Counter via WLAN

This repository shows how to remotely control the achor chain relay via WLAN from phone/tablet.
Anchor chain events from the chain sensor are measured and presented on the phone/tablet.

Just set the WLAN ssid and password according to your needs. 
Select WLAN type with setting WiFiMode_AP_STA to "0" means Acess Point, or "1" means Client with DHCP.

Also set Chain_Calibration_Value according to your sensor (e.g. 0.33. meter per event).

If working as Access Point, connect the phone/tablet to the defined AP and start "192.168.4.1" in the browser.
If working as WLAN client, check the DHCP IP address with Serial Monitor of IDE and start browser with the shown IP address.

![Picture1](https://github.com/AK-Homberger/ESP32_ChainCounter_WLAN/blob/master/IMG_1254.PNG)

To control the anchor chain relay just press:
- "Down" for anchor down
- "Up" for anchor up
- "Stop" for Stop
- "Reset" to reset the chain counter to zero

Features:
- Saftey stop to stop "anchor up" two events before reaching zero (can be changed in code with SAFETY_STOP).
- Safety stop if maximum chain lenght is reached (standard 40 meters, can be changed with MAX_CHAIN_LENGTH)
- Watchdog timer to stop power after 1 second inactivity of client (e.g. due to connection problems).
- Watchdog timer to detect blocking chain. Engine stops if no events are detected within 1 second for up/down command.
- Current Chain Counter is stored in nonvolatile memory. ESP32 can be switched off after anchoring (counter is restored after new start).
- Demo mode to check functionality without having a windlass / chain counter connected to ESP32 (set ENABLE_DEMO to 1).

![Picture2](https://github.com/AK-Homberger/ESP32_ChainCounter_WLAN/blob/master/ESP32ChainCounterWLAN_OC_Relais.png)

The current design should work for a Quick or Lofrans anchor chain relay and chain sensor (which looks like a simple reed relay triggerd from a magnet). Connection details  for a Quick windlass/counter can be found here: https://www.quickitaly.com/resources/downloads_qne-prod/1/CHC1203_IT-EN-FR_REV001A.pdf

The resistors R4, R5 and the transistors Q3, Q4 are currently not necessary. They shall support a manual override detection in the futere (currently not yet imlemented in the code).

# Updates:

14.02.2020 - Version 1.0: Changed web page layout.

13.02.2020 - Version 0.6: Added maximum chain lenght function.

13.02.2020 - Version 0.5: Added WLAN selection (AP / Client) and WLAN reconnect.

12.02.2020 - Version 0.4: Deleted delay() in loop() to improve responsiveness of web server.

12.02.2020 - Version 0.3: Added watchdog timer, safety stop and demo mode.

10.02.2020 - Version 0.1: Initial version.

