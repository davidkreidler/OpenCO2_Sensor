# OpenCO2 Sensor with ESP32, Sensirion SCD40, E-Ink display, Wi-Fi, LED and a Battery

[![Build OpenCO2_Sensor](https://github.com/davidkreidler/OpenCO2_Sensor/actions/workflows/arduino_build.yml/badge.svg)](https://github.com/davidkreidler/OpenCO2_Sensor/actions/workflows/arduino_build.yml)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/Header.png)

OpenCO2 Sensor is an open-source indoor air quality sensor made in Germany. Measurements are displayed via an 1.54" E-Ink display, RGB LED in traffic light colors, or Wi-Fi, e.g., to Home Assistant. The Arduino code for the ESP32-S2 and Sensirion SCD40 is available here on GitHub.

## Buy it [here on Open-Sensor.com](https://open-sensor.com/products/openco2-monitor-co2-sensor-luftqualitatssensor-co2-messgerat-made-in-germany-mit-e-ink-display-innenraum-luftqualitat-uberwachung?variant=62823149797725)

Especially in winter, when windows stay closed, reminders to ventilate regularly help health and comfort. Poor indoor air quality can lead to lower productivity and learning difficulties. This open-source project helps you keep an eye on this at all times using the E-Ink display and LED. Take the small sensor with you wherever you go to monitor air quality – the battery life is up to 11 weeks.

# CO2 Sensor

Based on the SCD40 from Sensirion, a completely new, miniaturized CO2 sensor with photoacoustic measurement principle.
The integrated, industry-leading humidity and temperature sensor delivers high accuracy with low energy consumption.
* Automatic self-calibration ensures maximum long-term stability
* CO2 measuring range: 400 ppm – 40,000 ppm
* Accuracy:
    * CO2: ±(50 ppm + 5% of measured value)
    * Temperature: ±0.8 °C
    * Humidity: ±6%

# Clear E-Ink display

1.54" in size with high resolution (200 x 200 pixels). Enables low power consumption and a wide viewing angle. Display refreshes every 5 seconds during normal operation; in battery mode it refreshes ~every 30 seconds.

# RGB LED

For displaying air quality as a traffic light (green, yellow, red, magenta). Brightness and color are adjustable via the Menu.

# Home Assistant

Add this config to the `configuration.yaml` of your Home Assistant. Below Version 5.4 please change `OpenCO2` to the IP address.
```
rest:
    scan_interval: 60
    resource: http://OpenCO2:9925/metrics
    method: GET
    sensor:
      - name: "CO2"
        device_class: carbon_dioxide
        unique_id: "d611314f-9010-4d0d-aa3b-37c7f350c82f"
        value_template: >
            {{ value | regex_findall_index("(?:rco2.*})(\d+)") }}
        unit_of_measurement: "ppm"
      - name: "Temperature"
        unique_id: "d611314f-9010-4d0d-aa3b-37c7f350c821"
        device_class: temperature
        value_template: >
            {{ value | regex_findall_index("(?:atmp.*})((?:\d|\.)+)") }}
        unit_of_measurement: "°C"
      - name: "Humidity"
        unique_id: "d611314f-9010-4d0d-aa3b-37c7f350c822"
        device_class: humidity
        value_template: >
            {{ value | regex_findall_index("(?:rhum.*})((?:\d|\.)+)") }}
        unit_of_measurement: "%"
```

# 3D-printed housing

Size: 4.9 x 3.8 x 2 cm
* [3D Model viewer](https://a360.co/44f2th9)
* [3D File FRONT](https://github.com/davidkreidler/OpenCO2_Sensor/blob/main/case/FRONT.obj)
* [3D File BACK](https://github.com/davidkreidler/OpenCO2_Sensor/blob/main/case/BACK.obj)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/animation.gif)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/drawing.png)

# Menu

Press the Menu button on the back. Long press (≥1 s) selects, short press cycles to the next item.
Choose between:
* `LED` control color, brightness and when the LED should be enabled.
* `Display` can be inverted. Max battery charge percentage threshold. Also change the Temperature unit, language and font.
* `Calibrate` put the sensor outside for 3+ minutes (only run this, if calibration is needed).
* `History` display up to 24h of CO2, temperature and humidity as a graph. Values can also be exported via QR codes containing 1h of CO2 measurements each.
* `Wi-Fi` enable or disable wireless connections. Setup via connecting to the Network "OpenCO2 Sensor".
* `Info` shows MAC Address, serial numbers, uptime, version and battery details.
* `Fun` little easter eggs

# Wi-Fi

Enable Wi-Fi via the Menu button. When power is connected, an access point `OpenCO2 Sensor` is enabled. Connect to it and navigate to http://192.168.4.1 (it will open automatically on modern Smartphones). Insert your home Wi-Fi credentials under `Configure WiFi`. Choose your network name from the list in the top and insert the password. Click `Save`. The sensor will now be automatically connected. Navigate to [openco2:9925](http://OpenCO2:9925) to see a graph of CO2, Temperature and Humidity measurements.
![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/setup.jpg)
![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/website.png)

# Machine-readable Measurements over WiFi
Connect the sensor to your Wi-Fi network and find out the IP of the sensor in your home network via the ui of your router or on the sensor itself via the button under Menu -> Info .
`curl [IP]:9925/metrics`

Result:
```
# HELP rco2 CO2 value, in ppm
# TYPE rco2 gauge
rco2{id="Open CO2 Sensor",mac="7C:DF:A1:96:B9:72"}571
# HELP atmp Temperature, in degrees Celsius
# TYPE atmp gauge
atmp{id="Open CO2 Sensor",mac="7C:DF:A1:96:B9:72"}25.37
# HELP rhum Relative humidity, in percent
# TYPE rhum gauge
rhum{id="Open CO2 Sensor",mac="7C:DF:A1:96:B9:72"}55.15
```

# AirGradient / Grafana

Use [internet-pi](https://github.com/geerlingguy/internet-pi) to store the CO2 / Temperature / Humidity data on your Pi. First connect the OpenCO2 Sensor to your Wi-Fi network and follow the instructions https://www.youtube.com/watch?v=Cmr5VNALRAg Then download the https://raw.githubusercontent.com/davidkreidler/OpenCO2_Sensor/main/grafana_OpenCO2_Sensor.json and import it into Grafana.
![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/grafana.png)

# Update via USB

1. Download `FIRMWARE.BIN` from the latest [release](https://github.com/davidkreidler/OpenCO2_Sensor/releases)
2. Plug a data USB-C Cable
3. Restart the OpenCO2 Sensor using the switch on the side
4. Copy `FIRMWARE.BIN` to the USB device

# OTA Update

1. Download `FIRMWARE.BIN` from the latest [release](https://github.com/davidkreidler/OpenCO2_Sensor/releases).
2. Enable Wi-Fi via the Menu button, in an area where no previously known network is active.
3. Make sure that power is connected.
4. Connect you PC/mobile to the WiFi network `OpenCO2 Sensor` and navigate to http://192.168.4.1
5. Under `Update` select the `FIRMWARE.BIN` file and click `Update`.
6. The Sensor will restart.
![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/OTA.jpg)


# Installation inside the Arduino IDE

1. Copy the esp32-waveshare-epd folder to your `Arduino/libraries/` directory
2. [Install the ESP32 support for Arduino IDE](https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/installing.html)
3. Choose `ESP32S2 Dev Module` under `Tools -> Board -> esp32`
4. Connect the Sensor via USB and flip the power switch to on while holding the button on the back
5. Choose the new COM Port in the Arduino IDE under `Tools -> Port -> COMXX`

# Dependencies

* [FastLED](https://github.com/FastLED/FastLED)
* [Sensirion Core](https://github.com/Sensirion/arduino-core)
* [Sensirion I2C SCD4x Arduino Library](https://github.com/Sensirion/arduino-i2c-scd4x)
* [WiFiManager](https://github.com/tzapu/WiFiManager)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/schematic.png)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/pcb.png)
