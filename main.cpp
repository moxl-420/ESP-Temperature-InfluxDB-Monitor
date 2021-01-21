//-----------------------------------------------------------------------------
//
//   SCript made by MoxL  - V0.4
//   Made For Wemos D1 mini V3.0.0 ESP8266EX
//   Connects to WIFI, gets Time from NTP, Gets info from InfluxDB and displays it on screen. 
//    
//   Needs to be compiled in ESP-IDF with Platformio.ini file
//
//   - Needed items -
//      -----------> Wemos D1 mini
//      -----------> BME280 I2C sensor for temperature 
//      -----------> SSD1306 I2C screen (Yellow/blue version prefered) 128x64 
//
//   - Wiring for wemos D1 MINI V3.0.0 - 
//      D2  = SDA
//      D1  = SCL
//      VCC = 3.3V pin.
//      GND = GND
//
//-----------------------------------------------------------------------------
// ----> Platform.Ini File for needed Libraries.
//[env:my_build_env]
//platform = espressif8266
//board = d1_mini
//framework = arduino
//monitor_speed = 115200
//lib_deps = 
//	adafruit/Adafruit SSD1306@^2.4.0
//	adafruit/Adafruit BusIO@^1.5.0
//	fabyte/Tiny BME280@^1.0.2
//	adafruit/Adafruit BME280 Library@^2.1.2
//	m5ez/ezTime@^0.8.3
//	arduino-libraries/WiFi@^1.2.7
//	tobiasschuerg/ESP8266 Influxdb@^3.7.0
//-----------------------------------------------------------------------------
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ezTime.h>
#include <ESP8266WiFi.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

//-----------------------------------------------------------------------------
//
//   Start of Parameters to fill in.
//
//-----------------------------------------------------------------------------
//SCREEN
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)


//Timer for polling when to read temps, set time and get data from influx
unsigned long timed;
unsigned long timepreviouscalc;
unsigned long timecalctimer = 60000; //60000 -> Every minute

//Timer for when to refresh the screen.
unsigned long timed2;
unsigned long timepreviouscalc2;
unsigned long timecalctimer2 = 11000; // 11000---> Every 11 seconds.

//WiFi.begin(SSID, WIFIPASS);
//Set wifi credentials.
String SSID = "ssid";
String WIFIPASS = "pass";

// Provide official timezone names
// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
  String Timezone_zone = "Europe/Amsterdam";


//-----------------------------------------------------------------------------
//Query result error: Flux query service disabled. Verify flux-enabled=true in the [http] section of the InfluxDB config.
//error: /etc/influxdb/influxdb.conf by setting flux-enabled=true
//-----------------------------------------------------------------------------
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
// InfluxDB 1.8+  (v2 compatibility API) server url, e.g. http://192.168.1.48:8086
#define INFLUXDB_URL "http://192.168.1.48:8086"
// InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
// InfluxDB 1.8+ (v2 compatibility API) use form user:password, eg. admin:adminpass
#define INFLUXDB_TOKEN "admin:adminpass"
// InfluxDB v2 organization name or id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
// InfluxDB 1.8+ (v2 compatibility API) use any non empty string
#define INFLUXDB_ORG "Main Org."
// InfluxDB v2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
// InfluxDB 1.8+ (v2 compatibility API) use database name
#define INFLUXDB_BUCKET "InfluxDB"
// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"


//-----------------------------------------------------------------------------
//
//   End of Parameters to fill in.
//
//-----------------------------------------------------------------------------



// 'invMox', 119x53px -MoxBMP
static const unsigned char PROGMEM MoxBMP[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x08, 0x20, 0x00, 0x00, 
0x00, 0x00, 0x1f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x1c, 0x60, 0x00, 0x00, 0x00, 
0x30, 0x1f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xc0, 0x1c, 0x60, 0x00, 0x00, 0x07, 0xf0, 
0x3f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x1c, 0x60, 0x00, 0x00, 0x07, 0xf8, 0x3f, 
0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x1c, 0x60, 0x00, 0x00, 0x0f, 0xf8, 0x3f, 0xc0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x1c, 0x60, 0x1f, 0xf8, 0x0f, 0xf8, 0x7f, 0xe0, 0x00, 
0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x1c, 0x60, 0x0f, 0xf0, 0x0f, 0xfc, 0x7f, 0xe0, 0x1f, 0x80, 
0x00, 0x0c, 0x01, 0xff, 0x00, 0x1c, 0x60, 0x00, 0x00, 0x0f, 0xfc, 0x7f, 0xe0, 0x7f, 0xc0, 0x30, 
0x1e, 0x01, 0xfe, 0x00, 0x1c, 0x60, 0x00, 0x00, 0x1f, 0xfe, 0xff, 0xe0, 0xff, 0xe0, 0x78, 0x1f, 
0x03, 0xfe, 0x00, 0x1c, 0x60, 0x00, 0x00, 0x1f, 0xfe, 0xff, 0xe0, 0xff, 0xf0, 0xfc, 0x3f, 0x83, 
0xfe, 0x00, 0x1c, 0x60, 0x01, 0x00, 0x1f, 0xff, 0xff, 0xe1, 0xff, 0xf1, 0xfe, 0x7f, 0x83, 0xfc, 
0x00, 0x1c, 0x60, 0x07, 0xc0, 0x1f, 0xff, 0xff, 0xe1, 0xfb, 0xfb, 0xff, 0xff, 0x83, 0xfc, 0x00, 
0x1c, 0x60, 0x0f, 0xe0, 0x3f, 0xff, 0xff, 0xe3, 0xf1, 0xf9, 0xff, 0xff, 0x07, 0xfc, 0x00, 0x1c, 
0x60, 0x1c, 0x70, 0x3f, 0xbf, 0xff, 0xe3, 0xf1, 0xf8, 0xff, 0xff, 0x07, 0xf8, 0x00, 0x1c, 0x60, 
0x18, 0x30, 0x3f, 0x3f, 0xef, 0xe3, 0xf1, 0xf8, 0x7f, 0xfe, 0x07, 0xf8, 0x00, 0x1c, 0x60, 0x18, 
0x30, 0x3f, 0x3f, 0xef, 0xe7, 0xf1, 0xf8, 0x1f, 0xfc, 0x0f, 0xf8, 0x00, 0x1c, 0x60, 0x1c, 0x70, 
0x7f, 0x1f, 0xef, 0xf7, 0xe1, 0xf8, 0x0f, 0xf8, 0x0f, 0xf0, 0x18, 0x1c, 0x60, 0x0f, 0xe0, 0x7f, 
0x1f, 0xcf, 0xf7, 0xe1, 0xf8, 0x0f, 0xfc, 0x0f, 0xf0, 0xf8, 0x1c, 0x60, 0x07, 0xc0, 0x7f, 0x1f, 
0xc7, 0xf7, 0xe1, 0xf8, 0x1f, 0xfe, 0x0f, 0xf7, 0xf8, 0x1c, 0x60, 0x00, 0x00, 0x7f, 0x0f, 0xc7, 
0xf7, 0xe1, 0xf8, 0x3f, 0xff, 0x1f, 0xff, 0xf8, 0x1c, 0x60, 0x00, 0x00, 0xff, 0x0c, 0x07, 0xf7, 
0xf3, 0xf0, 0x3f, 0xff, 0x9f, 0xff, 0xfc, 0x1c, 0x60, 0x1f, 0xf8, 0xfe, 0x00, 0x07, 0xf7, 0xff, 
0xf0, 0x7f, 0xff, 0xcf, 0xff, 0xfc, 0x1c, 0x60, 0x1f, 0xf8, 0x7e, 0x00, 0x07, 0xe3, 0xff, 0xe0, 
0x7f, 0x9f, 0xe7, 0xff, 0xf0, 0x1c, 0x60, 0x00, 0x00, 0x3e, 0x00, 0x07, 0xc3, 0xff, 0xe0, 0x3f, 
0x0f, 0xc3, 0xff, 0xc0, 0x1c, 0x60, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x01, 0xff, 0xc0, 0x1e, 0x07, 
0x81, 0xfe, 0x00, 0x1c, 0x60, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0xff, 0x80, 0x06, 0x03, 0x00, 
0x78, 0x00, 0x1c, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x1c, 0x60, 0x0f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x1c, 0x60, 0x1f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 
0x60, 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x60, 
0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x60, 0x3c, 
0x01, 0xc1, 0x00, 0xf9, 0x99, 0xf8, 0x0f, 0x87, 0xe0, 0x30, 0xfc, 0x88, 0x1c, 0x60, 0x3e, 0x07, 
0xe1, 0xdf, 0xf9, 0xf9, 0xfc, 0x0f, 0xe7, 0xf8, 0x3d, 0xfc, 0xfc, 0x1c, 0x60, 0x3e, 0x43, 0xe3, 
0xdf, 0xfd, 0xf9, 0xfe, 0x0f, 0xf7, 0xfc, 0x3d, 0xfc, 0xfc, 0x1c, 0x60, 0x1f, 0xe3, 0xf3, 0x9f, 
0xf9, 0xfd, 0xff, 0x0f, 0xf7, 0xbc, 0x3d, 0xe0, 0xfc, 0x1c, 0x60, 0x1f, 0xe3, 0xfb, 0x9f, 0xc1, 
0xc1, 0xef, 0x0e, 0xf7, 0x9c, 0x3d, 0xe0, 0xe0, 0x1c, 0x60, 0x1f, 0xe3, 0xff, 0x9f, 0xc1, 0xc1, 
0xe7, 0x0e, 0x77, 0x9c, 0x3d, 0xf0, 0xe0, 0x1c, 0x60, 0x1f, 0x03, 0xff, 0x81, 0xc1, 0xf9, 0xef, 
0x0e, 0x77, 0x9c, 0x3c, 0xf8, 0xfc, 0x1c, 0x60, 0x1f, 0x1b, 0xdf, 0x01, 0xc1, 0xf9, 0xfe, 0x1e, 
0xf3, 0xfc, 0x38, 0x7c, 0xfc, 0x1c, 0x60, 0x0f, 0xf9, 0xdf, 0x01, 0xe0, 0xe1, 0xfe, 0x1f, 0xf3, 
0xf8, 0x38, 0x3c, 0xf0, 0x1c, 0x60, 0x0f, 0xfd, 0xef, 0x01, 0xe0, 0xec, 0xff, 0x1f, 0xe3, 0xfc, 
0x38, 0x1e, 0xf6, 0x1c, 0x60, 0x0f, 0xfd, 0xee, 0x01, 0xe0, 0xfe, 0xff, 0x1f, 0x83, 0xde, 0x38, 
0x3e, 0x7e, 0x1c, 0x60, 0x0f, 0xf9, 0xe0, 0x01, 0xe0, 0xfe, 0xf7, 0x9e, 0x03, 0xde, 0x78, 0xfc, 
0x7e, 0x1c, 0x60, 0x0f, 0xc1, 0xe0, 0x01, 0xe0, 0xf8, 0xf3, 0x1e, 0x03, 0xcc, 0x78, 0xfc, 0x7c, 
0x1c, 0x60, 0x04, 0x01, 0xc0, 0x01, 0x80, 0xc0, 0xf0, 0x1e, 0x03, 0xc0, 0x70, 0x70, 0x40, 0x1c, 
0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x60, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'humidity', 33x30px
static const unsigned char PROGMEM humidityBMP[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x00, 0x00, 0x01, 0xe7, 
0xc0, 0x00, 0x00, 0x03, 0xc1, 0xe0, 0x00, 0x00, 0x07, 0x00, 0xf0, 0x00, 0x00, 0x0f, 0x00, 0x7f, 
0x00, 0x00, 0x0e, 0x00, 0x3f, 0x80, 0x00, 0x0c, 0x00, 0x33, 0xc0, 0x00, 0x0c, 0x00, 0x00, 0xe0, 
0x00, 0x0c, 0x00, 0x00, 0x60, 0x00, 0x0c, 0x00, 0x00, 0x70, 0x00, 0x0e, 0x00, 0x00, 0x30, 0x00, 
0x0e, 0x00, 0x00, 0x70, 0x00, 0x07, 0x00, 0x00, 0x60, 0x00, 0x03, 0x81, 0x00, 0xe0, 0x00, 0x03, 
0xf3, 0x83, 0xc0, 0x00, 0x01, 0xf7, 0xcf, 0x80, 0x00, 0x00, 0x67, 0xce, 0x00, 0x00, 0x00, 0x67, 
0xc0, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'temp', 18x30px
const unsigned char tempBMP [] PROGMEM = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x03, 0xe0, 0x00, 0x07, 
0xf0, 0x00, 0x07, 0xf0, 0x00, 0x07, 0x70, 0x00, 0x06, 0x30, 0x00, 0x06, 0x30, 0x00, 0x06, 0x30, 
0x00, 0x06, 0x30, 0x00, 0x06, 0x30, 0x00, 0x06, 0x30, 0x00, 0x06, 0x30, 0x00, 0x06, 0x30, 0x00, 
0x06, 0x30, 0x00, 0x06, 0x30, 0x00, 0x06, 0x30, 0x00, 0x0e, 0x38, 0x00, 0x1c, 0x0c, 0x00, 0x1c, 
0x0c, 0x00, 0x1c, 0x0c, 0x00, 0x1e, 0x1c, 0x00, 0x0f, 0x38, 0x00, 0x07, 0xf0, 0x00, 0x03, 0xe0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'wifi', 16x16px
static const unsigned char PROGMEM wifiBMP[] = {
0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x3c, 0x3c, 0x60, 0x06, 0xc7, 0xe3, 0x1c, 0x38, 0x30, 0x0c, 
0x27, 0xe4, 0x0e, 0x70, 0x0c, 0x30, 0x01, 0x80, 0x03, 0xc0, 0x03, 0xc0, 0x00, 0x00, 0x00, 0x00
};

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
// Screen setup
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//BME SENSOR DECLARATION
Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();
sensors_event_t temp_event, pressure_event, humidity_event;

// Screen data 
int screhum;
int scretemp;
int screencounter = 0;
int firstrun = 0;


//Time
Timezone myTZ;
String uur;
String weekdag;
String temp_gang;
String datum_nu;


void setup() {

 Serial.begin(115200);
 Serial.println();
 Serial.println("-----------------------------------------------------------------------------------------------------");
 Serial.println("Setup Starting, please wait");
 Serial.println("-----------------------------------------------------------------------------------------------------");



  //setup display
 Serial.println("Setting up Display");
// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }else{
  Serial.println(F("SSD1306 allocation ok"));
  }

// Show initial display buffer contents on the screen --
// the library initializes this with an Adafruit splash screen.
  display.display();
        //drawbitmap (x,y onscreen, image, width,height,1=on0=off)
        // 'invMox', 119x53px -MoxBMP
   
      display.clearDisplay();
      display.setCursor(15, 0);     // (x,y/horizontaal, verticaal)
      display.setTextSize(2);      // Normal 1:1 pixel scale
      display.print("Loading");
      display.drawBitmap(0, 15, MoxBMP, 119, 53, 1);
      display.display();

      


   //BME280
     Serial.println(F("BME280 Sensor event test"));
       if (!bme.begin()) {
          Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
        while (1) delay(10);
        }
  bme_temp->printSensorDetails();
  bme_pressure->printSensorDetails();
  bme_humidity->printSensorDetails();


  Serial.println(F("Starting wifi"));
  WiFi.begin(SSID, WIFIPASS);
  Serial.println("------------------------------------");
	// Uncomment the line below to see what it does behind the scenes
	// setDebug(INFO);
	Serial.println(F("Fetcing time"));
	waitForSync();

	myTZ.setLocation(Timezone_zone);
	Serial.print(F("Local Time:     "));
	Serial.println(myTZ.dateTime());
  Serial.println("------------------------------------");

if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());

  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
 Serial.println("------------------------------------");

Serial.println("-----------------------------------------------------------------------------------------------------");
Serial.println("Setup Completed, starting loop");
Serial.println("-----------------------------------------------------------------------------------------------------");
}
void loop() {
  
//-----------------------------------------------------------------------------
//
//   Start of First timer - Fetching data.
//
//-----------------------------------------------------------------------------
   
//Fetch Data
timed = millis();
if((timed >= (timepreviouscalc + timecalctimer))==true ){
  timepreviouscalc = millis();
  Serial.println("----------------------------------------------");
  Serial.println("Fetching BME data");
  Serial.println("----------------------------------------------");
  
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);
  
  Serial.print(F("Temperature = "));
  Serial.print(temp_event.temperature);
  Serial.println(" *C");
  Serial.print(F("Humidity = "));
  Serial.print(humidity_event.relative_humidity);
  Serial.println(" %");
  Serial.println();
      //format Temp/hum
      float scrhu = humidity_event.relative_humidity;
      screhum = scrhu;
      float scrte = temp_event.temperature;
      scretemp = scrte;
  
  Serial.println("----------------------------------------------");
  Serial.println("Fetching Time and date");
  Serial.println("----------------------------------------------");

        String minu;
        String uuur;

        String o = String(myTZ.day());
        String o2 = String(myTZ.month());
        datum_nu = o + "/" + o2;

        if (myTZ.hour() <= 9){
          uuur = "0" + String(myTZ.hour());
        }else{
          uuur = String(myTZ.hour());
        }

        if (myTZ.minute() <= 9){
          minu = "0" + String(myTZ.minute());
        }else{
          minu = String(myTZ.minute());
        }
        uur = uuur + ":" + minu;
                      //Change the value off the String according to your language
                      if(myTZ.weekday()==1){
                            weekdag="Sunday";  //Sunday
                      }else if(myTZ.weekday()==2){
                            weekdag="Monday";  //Monday
                      }else if(myTZ.weekday()==3){
                            weekdag="Tuesday";  //Tuesday
                      }else if(myTZ.weekday()==4){
                            weekdag="Wednesday"; //Wednesday
                      }else if(myTZ.weekday()==5){
                            weekdag="Thursday"; //Thursday
                      }else if(myTZ.weekday()==6){
                            weekdag="Friday";   //Friday
                      }else{
                        weekdag="Saturday";  // Saturday
                      }

                      Serial.println(weekdag);
                      Serial.println(uur);

  String FluxstringfromDB;
  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
    String query = "from(bucket: \"MoxlDB\")|> range(start:-1m)|> filter(fn: (r) => r[\"_measurement\"] == \"Motion_2_Temperature\")";
                          Serial.println("==== List results ====");
                          
                          // Print composed query
                          Serial.print("Querying with: ");
                          Serial.println(query);
                          // Send query to the server and get result
                          FluxQueryResult result = client.query(query);
                          // Iterate over rows. Even there is just one row, next() must be called at least once.
                          while (result.next()) {
                            // Check for new grouping key
                            if(result.hasTableChanged()) {
                              Serial.println("Table:");
                              Serial.print("  ");
                              // Print all columns name
                              for(String &name: result.getColumnsName()) {
                                Serial.print(name);
                                Serial.print(",");
                              }
                              Serial.println();
                              Serial.print("  ");
                              // Print all columns datatype
                              for(String &tp: result.getColumnsDatatype()) {
                                Serial.print(tp);
                                Serial.print(",");
                              }
                            }
                            Serial.println("  ");
                            // Print values of the row
                            for(FluxValue &val: result.getValues()) {
                              // Check wheter the value is null
                              if(!val.isNull()) {
                                // Use raw string, unconverted value
                                Serial.print(val.getRawValue());
                                    FluxstringfromDB = FluxstringfromDB + "," + String(val.getRawValue());
                              } else {
                                // Print null value substite
                                Serial.print("<null>");  
                              }
                              Serial.print(",");
                            }
                            Serial.println();
                          }
                          // Check if there was an error
                          if(result.getError().length() > 0) {
                            Serial.print("Query result error: ");
                            Serial.println(result.getError());
                          }
                            Serial.println("---------------------------------------------------------");
                            Serial.print(">>> FluxstringfromDB: ");
                            Serial.println(FluxstringfromDB);
                            //Check your personal string to filter the data.
                            int g = FluxstringfromDB.lastIndexOf("Z,");
                            int h = FluxstringfromDB.lastIndexOf(",value");
                            String s = FluxstringfromDB.substring(g+2,h);  
                            temp_gang = s.substring(0,2);                                                  
                            Serial.print(">>> Temperatuur gang: ");
                            Serial.println(s);
                            Serial.println(temp_gang);
                            
                          // Close the result
                          result.close();
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  
  
  
  
  
  Serial.println("----------------------------------------------");
  if ((screencounter==0)&&(firstrun==0)) {
        Serial.println("This was the first datafetch");
        Serial.println("Setting first run to ok so screens can be set.");
        screencounter=1;
        firstrun = 1;
        Serial.println("----------------------------------------------");
  }
  //end of timed events
  Serial.println("----------------------------------------------");
   }

//-----------------------------------------------------------------------------
//
//   Start of Second timer - Fetching data.
//
//-----------------------------------------------------------------------------



//screenrotation
timed2 = millis();
if((timed2 >= (timepreviouscalc2 + timecalctimer2))==true ){
  timepreviouscalc2 = millis();
  Serial.print("Refresh Screen, showing screen");
  Serial.println(screencounter);

    //Needed
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    //if wifi is connected then display Wifi icon
            if(WiFi.isConnected()){
                  display.drawBitmap(3, 0, wifiBMP, 16, 16, 1);
            }
    //Display time.
      display.setCursor(32, 0);     // (x,y/horizontaal, verticaal)
      display.setTextSize(2);      // Normal 1:1 pixel scale
      display.print(uur);
    

    //draw line
    //(x1, y1, x2, y2, color)
    display.drawLine(0, 15, 127, 15, WHITE);
    
    if ((screencounter==1)&&(firstrun==1)) {

                     
                      //Temp
                      display.drawRoundRect(2, 17, 60, 40, 2, WHITE); //display.drawRoundRect(X1, Y1, Width in px, Height in px, 2, WHITE);
                      display.drawBitmap(10, 20, tempBMP, 18, 30, 1);
                      display.setCursor(32, 20);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(1);      // Normal 1:1 pixel scale
                      display.print("BME");
                      display.setCursor(32, 30);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(2);      // Normal 1:1 pixel scale
                      display.print(scretemp);
                      //humidity
                      display.drawRoundRect(64, 17, 63, 40, 2, WHITE); //display.drawRoundRect(X1, Y1, Width in px, Height in px, 2, WHITE);
                      display.drawBitmap(67, 20, humidityBMP, 33, 30, 1);
                      display.setCursor(100, 20);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(1);      // Normal 1:1 pixel scale
                      display.print("BME");
                      display.setCursor(100, 30);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(2);      // Normal 1:1 pixel scale
                      display.print(screhum);

                      screencounter=2;
    }else if ((screencounter==2)&&(firstrun==1)) {

                      display.drawRoundRect(2, 17, 125, 40, 2, WHITE);
                      display.setCursor(40, 20);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(2);      // Normal 1:1 pixel scale
                      display.print(datum_nu);
                      display.setCursor(10, 40);     // (x,y/horizontaal, verticaal)
                      display.print(weekdag);

                      screencounter=3;
    }else if ((screencounter==3)&&(firstrun==1)) {
                      display.drawRoundRect(2, 17, 60, 40, 2, WHITE); //display.drawRoundRect(X1, Y1, Width in px, Height in px, 2, WHITE);
                      display.drawBitmap(10, 20, tempBMP, 18, 30, 1);
                      display.setCursor(32, 20);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(1);      // Normal 1:1 pixel scale
                      display.print("Hall");
                      display.setCursor(32, 30);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(2);      // Normal 1:1 pixel scale
                      display.print(temp_gang);

                      display.drawRoundRect(64, 17, 63, 40, 2, WHITE); //display.drawRoundRect(X1, Y1, Width in px, Height in px, 2, WHITE);
                      display.drawBitmap(67, 20, humidityBMP, 33, 30, 1);
                      display.setCursor(100, 20);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(1);      // Normal 1:1 pixel scale
                      display.print("None");
                      display.setCursor(100, 30);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(2);      // Normal 1:1 pixel scale
                      //display.print(screhum);

                      screencounter=1;// Last screen points back to firstscreen.
    }else {
                      display.clearDisplay();
                      display.setCursor(15, 0);     // (x,y/horizontaal, verticaal)
                      display.setTextSize(2);      // Normal 1:1 pixel scale
                      display.print("Loading");
                      display.drawBitmap(0, 15, MoxBMP, 119, 53, 1);
                      display.display();
}

  //show info onscreen.
   display.display();
  //end of timed events
   }

//end of loop
}
