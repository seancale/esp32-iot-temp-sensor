# esp32-iot-temp-sensor
An ESP32 based IoT temperature sensor, powered by the open source ESPHome firmware (https://esphome.io)

---

# Instructions
TODO: Add instructions on how to set up Home Assistant and ESPHome servers, and flash ESPHome onto the ESP board

## Wiring

Use this table to hook up the DHT22 to the ESP32:

DHT22 Pin | ESP32 Pin
--- | ---
Positive | +3.3v or 3V3
Signal | GPIO4 or D4
Negative | GND

## Adding device in ESPHome

Go to the ESPHome dashboard located at `http://your-esphome:6052` and click Add Device. Name the device whatever you want, and enter your wifi credentials. Choose ESP32 for the board. It will then ask you to install ESPHome. 
Click skip for now. Go to your newly created device and click edit. Paste in the configuration from `esphome-config.yaml` and adjust the parameters that I marked with comments.
