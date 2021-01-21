# ESP-Temperature-InfluxDB-Monitor
D1 Mini Temp and influxdb Display

Connects to WIFI, gets Time from NTP, Gets info from InfluxDB and displays it on screen. 
Script made by MoxL  - V0.4

# Needed:
  - Wemos D1 Mini ESP8266ex v3.0.0
  - i2c: BME280 Temperature sensor
  - i2c: SSD1306 0.96 inch OLED display 128x64(Yellow/blue version prefered) 
  
 # Wiring for wemos D1 MINI V3.0.0 - 
      D2  = SDA
      D1  = SCL
      VCC = 3.3V pin.
      GND = GND

# Platform.Ini File for needed Libraries.
Needs to be compiled in ESP-IDF with Platformio.ini file.
[env:my_build_env]
platform = espressif8266
board = d1_mini
framework = arduino
monitor_speed = 115200
lib_deps = 
	adafruit/Adafruit SSD1306@^2.4.0
	adafruit/Adafruit BusIO@^1.5.0
	fabyte/Tiny BME280@^1.0.2
	adafruit/Adafruit BME280 Library@^2.1.2
	m5ez/ezTime@^0.8.3
	arduino-libraries/WiFi@^1.2.7
	tobiasschuerg/ESP8266 Influxdb@^3.7.0
