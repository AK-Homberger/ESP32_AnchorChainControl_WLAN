# ESP32 Anchor Chain Remote Contol / Counter via WLAN

This repository shows how to remotely control the achor chain relay via WLAN from phone/tablet.
Anchor chain events from the chain sensor are measured and presented on the phone/tablet.

Just set the AP name and password according to your needs. 
Set Chain_Calibration_Value according to your sensor (e.g. 0.33. meter per event).

On the phone/tablet connect to the defined AP and start "192.168.4.1" in the browser.

![Picture2](https://github.com/AK-Homberger/ESP32_ChainCounter_WLAN/blob/master/IMG_1252.PNG)

To control the anchor chain relay just press:
- "Down" for anchor down
- "Up" for anchor up
- "Stop" for Stop
- "Reset" to reset the chain counter to zero

Features:
- Saftey stop to stop "anchor up" two events before reaching zero (can be changed in code with SAFETY_STOP).
- Watchdog timer to stop power after 1 second inactivity of client (e.g. due to connection problems).
- Watchdog timer to detect blocking chain. Engine stops if no events are detected within 1 second for up/down command.
- Current Chain Counter is stored in nonvolatile memory. ESP32 can be switched off after anchoring (counter is restored after new start).
- Demo mode to check functionality without having a windlass / chain counter connected to ESP32 (set ENABLE_DEMO to 1).

![Picture1](https://github.com/AK-Homberger/ESP32_ChainCounter_WLAN/blob/master/ESP32ChainCounterWLAN_OC_Relais.png)

The current design should work for a Quick or Lofrans anchor chain relay and chain sensor (which which looks like simple reed relay triggerd from a magnet).

History:

12.02.2020 - Version 0.3: Added watchdog timer, safety stop and demo mode

11.02.2020 - Version 0.1: Initial version

