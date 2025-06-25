# OpenCO2 Sensor using ESP32 and SCD4x

[![Build OpenCO2_Sensor](https://github.com/davidkreidler/OpenCO2_Sensor/actions/workflows/arduino_build.yml/badge.svg)](https://github.com/davidkreidler/OpenCO2_Sensor/actions/workflows/arduino_build.yml)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/Header.png)

OpenCO2 Sensor is an Arduino IDE compatible Repository for an E-Ink Indoor air quality Sensor using the ESP32, SCD4X and an RGB LED.

## Buy it [here on Tindie](https://www.tindie.com/products/davidkreidler/open-co2-sensor/)

Especially in winter, when windows are closed, a reminder to ventilate regularly is useful for health, comfort and well-being. Poor indoor air quality can lead to decreased productivity and learning disabilities. Therefore, I developed an Open-source ESP32 project that uses an E-Ink display and a LED to show the indoor CO2 content. Take the small Sensor anywhere you go to monitor the Air Quality, with Battery life of 11+ days.

# CO2 Sensor

With the SCD40, Sensirion offers a completely new miniaturized CO2 sensor based on the photoacoustic sensor principle.
The integrated, industry-leading humidity and temperature sensor offers high accuracy with low power consumption.
* Automatic self-calibration ensures the highest long-term stability
* CO2 output range: 400 ppm – 40'000 ppm
* Accuracy:
	* CO2 ±(50ppm + 5% of reading)
	* Temperature ±0.8°C
	* Humidity ±6%

# Clear E-Ink display

1.54" in size with a very high resolution (200x200 Pixel). Enables low power consumption and wide viewing angle. Per partial refresh, readings are updated every five seconds (every 30 seconds in battery mode).

# RGB LED

For displaying the air quality as a traffic light (green, yellow, red, magenta). Brightness and color are adjustable via software.

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

Size: 4.7 x 4.1 x 2.4 cm
* [3D Model viewer](https://a360.co/3syuvEk)
* [3D File FRONT](https://a360.co/3CSICGq)
* [3D File BACK](https://a360.co/437Ak88)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/drawing.png)

# Menu

Press the Menu button on the backside of the OpenCO2 Sensor. Select an option via long press (1+ sec) or move to next menu point via a short press. Choose between:
* `LED` control color, brightness and when the LED should be enabled
* `Display` can be inverted. Also change the Temperature unit, language and font
* `Calibrate` put the sensor outside for 3+ minutes (only run this, if calibration is needed)
* `History` display up to 24h of CO2, temperature and humidity as a graph. Values can also be exported via QR codes containing 1h of CO2 measurements each.
* `Wi-Fi` enable or disable wireless connections
* `Info` shows MAC Address, serial numbers, uptime, version and battery details
* `Rainbow` fun little easter egg

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
2. Plug a data USB-C cable into your PC and the Sensor
3. copy `FIRMWARE.BIN` to the USB device

# OTA Update

Download `FIRMWARE.BIN` from the latest [release](https://github.com/davidkreidler/OpenCO2_Sensor/releases).
Enable Wi-Fi via the Menu button, in an area where no previously known network is active. Connect power. Then connect to `OpenCO2 Sensor` and navigate to http://192.168.4.1 . Under `Update` select the `FIRMWARE.BIN` file and click `Update`. The Sensor will restart.
![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/OTA.jpg)

# Update via USB (from an old release)

1. Download all files from the latest [release](https://github.com/davidkreidler/OpenCO2_Sensor/releases)
2. Make sure, that the power switch is in the `ON` position (down)
3. Plug a data USB-C cable into your PC and the Sensor
4. Hold the Button on the backside of the CO2 Sensor near the USB-C port and shortly push simultaneously the reset ↪️ Button
5. In Device Manager under `Ports (COM & LPT)` find the COM number and modify it in `update.bat`
6. Run `update.bat`
7. Afterwards push the reset ↪️ Button

# Installation inside the Arduino IDE

1. Copy the esp32-waveshare-epd Folder to your `Arduino/libraries/`
2. [Install the ESP32 support for Arduino IDE](https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/installing.html)
3. Choose `ESP32S2 Dev Module` under `Tools -> Board -> esp32`
4. Connect the Sensor as described in step "Update to the latest release" and choose the new Port under `Tools -> Boards -> Port`

# Dependencies

* [Adafruit DotStar](https://github.com/adafruit/Adafruit_DotStar)
* [Sensirion Core](https://github.com/Sensirion/arduino-core)
* [Sensirion I2C SCD4x Arduino Library](https://github.com/Sensirion/arduino-i2c-scd4x)
* [WiFiManager](https://github.com/tzapu/WiFiManager)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/animation.gif)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/schematic.png)

![alt text](https://github.com/davidkreidler/OpenCO2_Sensor/raw/main/pictures/pcb.png)
