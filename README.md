# ESP32 Anchor Chain Remote Contol / Counter via WLAN

This repository shows how to remotely control the achor chain relais via WLAN from phone/tablet.
Anchor chain events are measured and presented on the phone/tablet.

Just set the AP name and password according to your needs. 

Set Chain_Calibration_Value according to your sensor (e.g. 0.33. meter per event).

To control the anchor chain relay just press:
- "Down" for anchor down
- "Up" for anchor up
- "Stop" for Stop
- "Reset" to reset the chain counter to zero

![Picture2](https://github.com/AK-Homberger/ESP32_ChainCounter_WLAN/blob/master/IMG_1252.PNG)


This picture shows the interface for an Open Collector output whicht take the signal to GNG:

![Picture1](https://github.com/AK-Homberger/ESP32_ChainCounter_WLAN/blob/master/ESP32ChainCounterWLAN_OC_Relais.png)

The current design works for a Quick anchor chain relais and chain monitor (which is open collectot and taks the output to GND).




