# SCD4x_CO2_Sensor_ESP32
Arduino Repository for an e-paper CO2 Sensor with the ESP32-S2

![alt text](https://github.com/davidkreidler/SCD4x_CO2_Sensor_ESP32/raw/main/pictures/Header.png)

# Flash the binary to the CO2-Sensor

1. Download [esptool.py](https://raw.githubusercontent.com/espressif/esptool/master/esptool.py)
2. Download the `.bin` files from the latest [release](https://github.com/davidkreidler/SCD4x_CO2_Sensor_ESP32/releases)
3. Make sure, that the power switch is in the `ON` position (down)
4. Plug a data USB cable into your PC and the Sensor
5. Hold the Button on the backside of the CO2 Sensor near the USB-C port and push simultaneously the reset ↪️ Button
6. Release the reset ↪️ Button first and then the other one
7. Run the following commands in the folder containing the `.bin` and `esptool.py`
   port for Windows: `COM7` or Linux: `/dev/ttyUSB0`|`/dev/ttyACM0`
```
$ cd ~/Downloads
$ python3 -m pip install pyserial
$ python3 esptool.py --chip esp32s2 --port [COM7|/dev/ttyUSB0|/dev/ttyACM0] --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0xe000 boot_app0.bin 0x1000 co2_scd4x.ino.bootloader.bin 0x10000 co2_scd4x.ino.bin 0x8000 co2_scd4x.ino.partitions.bin
```
7. Afterwards push the reset ↪️ Button

# Installation

Install the ESP32-S2 support for Arduino IDE:
1. Open `File -> Preferences` in the Arduino IDE
2. Add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json` to `Additional Boards Manager URLs` and click OK
3. Install the board via `Tools -> Boards -> Boards Manager` search for `esp32` by Espressif Systems
4. Choose the board under `Tools -> Boards -> ESP32 Arduino -> ESP32S2 Dev Module`
5. Connect the Sensor as described in step "Programming" and choose the new Port under `Tools -> Boards -> Port`

Copy the esp32-waveshare-epd Folder to your `Arduino/libraries/`

# Dependencies

* [Adafruit DotStar](https://github.com/adafruit/Adafruit_DotStar)
  Install via `Sketch => Include Library => Manage Libaries... Ctrl+Shift+I` search for `Adafruit DotStar` and click Install
* [Sensirion Core](https://github.com/Sensirion/arduino-core)
  Install via `Sketch => Include Library => Manage Libaries... Ctrl+Shift+I` search for `Sensirion Core` and click Install
* [Sensirion I2C SCD4x Arduino Library](https://github.com/Sensirion/arduino-i2c-scd4x)
  Download repro as .zip file and add it to your Arduino IDE via `Sketch => Include Library => Add .ZIP Library...`

# Programming

* Make sure, that the power switch is in the `ON` position (down)
* Plug a data USB cable into your PC and the Sensor
* Hold the Button on the backside of the CO2 Sensor near the USB-C port and push simultaneously the reset ↪️ Button
* Release the reset ↪️ Button first and then the other one

* Click `Upload` in the Arduino IDE and wait
* Afterwards push the reset ↪️ Button

# Selbstgebauter CO2-Sensor

Vor allem im Winter bei geschlossenen Fenstern ist eine Erinnerung, regelmäßig zu lüften, sinnvoll für die Gesundheit, 
den Komfort und das Wohlbefinden. Schlechte Raumluftqualität kann zu verminderter Produktivität und Lernstörungen führen.
Daher habe ich ein ESP32-S2 Hobby Projekt entwickelt, welches mittels eines E-Paper Displays den CO2 Gehalt der Luft anzeigt. 
Vergleichbare kommerzielle Messgeräte kosten deutlich mehr und besitzen weniger Features.

# CO2-Sensor
Mit dem SCD40 bietet Sensirion einen völlig neuen miniaturisierten CO2-Sensor, welcher auf dem photoakustischen Sensorprinzip basiert.
Der integrierte, branchenführende Feuchte- und Temperatursensor bietet eine hohe Genauigkeit bei einem geringen Energieverbrauch.

# Klares E-Paper-Display
1,54” groß, mit einer Auflösung von 200x200 Pixeln und niedrigem Stromverbrauch bei breitem Betrachtungswinkel. Per partiellem Refresh werden die Messwerte alle fünf Sekunden aktualisiert.

# RGB-LED
Zur Darstellung der Luftqualität als Ampel (grün / gelb / rot). Helligkeit und Farbe sind per Software einstellbar.

# 3D-Gedrucktes Gehäuse
(45 x 41 x 23 mm) designed in Fusion 360

# WLAN
Mittels des ESP32-S2 Prozessors auf einem selbst designten PCB ist eine APP Anbindung per Software Update geplant.
Somit sind Push Benachrichtigungen Fenster (auf / zu) und Langzeitmessungen möglich.

![alt text](https://github.com/davidkreidler/SCD4x_CO2_Sensor_ESP32/raw/main/pictures/animation.gif)

![alt text](https://github.com/davidkreidler/SCD4x_CO2_Sensor_ESP32/raw/main/pictures/pcb.png)
