// Sourdough Monitor
// See: https://github.com/bernhard-hofmann/SourdoughMonitor

// Display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "/home/bernhard/github/bernhard-hofmann/SourdoughMonitorSecrets.h"
/*
The secrets file is stored outside the repository folder and contains the following values:
const char *ssid = "<YOUR WIFI SSID>";
const char *password = "<YOUR WIFI PASSWORD>";
const char *iotPlotterUrl = "http://iotplotter.com/api/v2/feed/<YOUR FEED ID>";
const char *iotPlotterApiKey = "<YOUR API KEY>";

*/

// Time of flight distance sensor
#include <Adafruit_VL6180X.h>

// DHT sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DHT
#define DHTPIN 10     // Digital pin connected to the DHT sensor 

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT_Unified dht(DHTPIN, DHTTYPE);

// Time of flight distance sensor
Adafruit_VL6180X vl = Adafruit_VL6180X();

unsigned int insertionIndex = 0;
// 86400 = one value per second for 24 hours
float humidity[86400];
float temperature[86400];
float range[86400];

void setup() {
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();

  pinMode(LED_BUILTIN, OUTPUT);

  // Initialize device.
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("WiFi connecting");
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.printDiag(Serial);
  Serial.println(F("------------------------------------"));

  if (! vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }
  Serial.println("Sensor found!");
}

void loop() {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  char buf[1024];
  float temp = 0.0;
  float humd = 0.0;
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    temp = event.temperature;
    sprintf(buf, "Temp: %f", event.temperature);
    display.println(buf);

    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    humd = event.relative_humidity;
    sprintf(buf, "Humd: %f", event.relative_humidity);
    display.println(buf);
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
  }

  uint8_t range = vl.readRange();
  uint8_t status = vl.readRangeStatus();

  if (status == VL6180X_ERROR_NONE) {
    sprintf(buf, "Range: %d", range);
    display.println(buf);
    Serial.print("Range: "); Serial.println(range);
  }

  // Some error occurred, print it out!
  if  ((status >= VL6180X_ERROR_SYSERR_1) && (status <= VL6180X_ERROR_SYSERR_5)) {
    display.println("System error");
    Serial.println("System error");
  }
  else if (status == VL6180X_ERROR_ECEFAIL) {
    display.println("ECE failure");
    Serial.println("ECE failure");
  }
  else if (status == VL6180X_ERROR_NOCONVERGE) {
    display.println("No convergence");
    Serial.println("No convergence");
  }
  else if (status == VL6180X_ERROR_RANGEIGNORE) {
    display.println("Ignoring range");
    Serial.println("Ignoring range");
  }
  else if (status == VL6180X_ERROR_SNR) {
    display.println("Signal/Noise error");
    Serial.println("Signal/Noise error");
  }
  else if (status == VL6180X_ERROR_RAWUFLOW) {
    display.println("Raw reading underflow");
    Serial.println("Raw reading underflow");
  }
  else if (status == VL6180X_ERROR_RAWOFLOW) {
    display.println("Raw reading overflow");
    Serial.println("Raw reading overflow");
  }
  else if (status == VL6180X_ERROR_RANGEUFLOW) {
    display.println("Range reading underflow");
    Serial.println("Range reading underflow");
  }
  else if (status == VL6180X_ERROR_RANGEOFLOW) {
    display.println("Range reading overflow");
    Serial.println("Range reading overflow");
  }

  // region WiFi POST
  if (WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      Serial.print("Sending data to ");
      Serial.println(iotPlotterUrl);
      http.begin(client, iotPlotterUrl);

      // Specify content-type header
      http.addHeader("api-key", iotPlotterApiKey);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Data to send with HTTP POST
      sprintf(buf, "{\"data\":{\"Temperature\":[{\"value\":%f}],\"Humidity\":[{\"value\":%f}], \"Range\":[{\"value\":%d}]}}", temp, humd, range);
      Serial.print("Sending: "); Serial.println(buf);
      int httpResponseCode = http.POST(buf);
      Serial.print("HTTP: "); Serial.println(httpResponseCode);

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  // endregion WiFi POST

  display.display();
  delay(5000);
}
