# SCD4x_CO2_Sensor_ESP32
Arduino Repository for an e-paper CO2 Sensor with the ESP32

![alt text](https://github.com/davidkreidler/SCD4x_CO2_Sensor_ESP32/raw/main/pictures/Header.png)

# Selbstgebauter CO2-Sensor

Vor allem im Winter bei geschlossenen Fenstern ist eine Erinnerung, regelmäßig zu lüften, sinnvoll für die Gesundheit, 
den Komfort und das Wohlbefinden. Schlechte Raumluftqualität kann zu verminderter Produktivität und Lernstörungen führen.
Daher habe ich ein ESP32-S2 Hobby Projekt entwickelt, welches mittels eines E-Paper Displays den CO2 Gehalt der Luft anzeigt. 
Vergleichbare Kommerzielle Messgeräte kosten deutlich mehr und besitzen weniger Features.

# CO2-Sensor
Mit dem SCD40 bietet Sensirion einen völlig neuen miniaturisierten CO2-Sensor, welcher auf dem photoakustischen Sensorprinzip basiert.
Der integrierte, branchenführende Feuchte- und Temperatursensor bietet eine hohe Genauigkeit bei einem geringen Energieverbrauch.

# Klares E-Paper-Display
1,54” groß, mit einer Auflösung von 200x200 Pixeln und niedrigem Stromverbrauch bei breitem Betrachtungswinkel. Per partiellem Refresh werden die Messwerte alle fünf Sekunden aktualisiert.

# RGB-LED
zur Darstellung der Luftqualität als Ampel (grün / gelb / rot). Helligkeit und Farbe sind per Software einstellbar.

# 3D-Gedrucktes Gehäuse
(45 x 41 x 23 mm) designed in Fusion 360

# WLAN
mittels des ESP32-S2 Prozessors auf einem selbst designten PCB ist eine APP Anbindung per Software Update geplant.
Somit sind Push Benachrichtigungen Fenster (auf / zu) und Langzeitmessungen möglich.

![alt text](https://github.com/davidkreidler/SCD4x_CO2_Sensor_ESP32/raw/main/pictures/animation.gif)
