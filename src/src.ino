// Sourdough Monitor
// See: https://github.com/bernhard-hofmann/SourdoughMonitor

// Display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "Button.h"

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

#define PUSHBUTTON_PIN_2  14

// DHT
#define DHTPIN 10     // Digital pin connected to the DHT sensor 

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
DHT_Unified dht(DHTPIN, DHTTYPE);

// Time of flight distance sensor
Adafruit_VL6180X vl = Adafruit_VL6180X();

word insertionIndex = 0;
// 86400 = one value per 1 seconds for 24 hours
// 17280 = one value per 5 seconds for 24 hours
const int maxSamples = 128;
/*
   Notes for later
   - store 128 values (one per pixel)
   - store 17280/128 samples so that the next sample stored in the 128 wide array is an average of the last "however long each pixel represents"
*/

// Temperature is stored as 10x the difference from a baseline of 20 degrees C
// E.g. 23.6C is +3.6C from 20C, stored as 36 (10x) (Max 127 = 20+12.7 = 32.7 Min -128 = 20-12.8 = 7.2)
const byte tempBaseLine = 20;
char temperatureSamples[maxSamples];
// Humidity is relative % (i.e. values 0-100) and stored without decimal places
char humiditySamples[maxSamples];
// Range is 0-255
byte rangeSamples[maxSamples];

Button pushButton(PUSHBUTTON_PIN_2);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pushButton.init();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  ShowProductScreen();

  Serial.begin(9600);

  InitializeTempAndHumiditySensor();
  InitializeDistanceSensor();
  InitializeWiFi();
}

void ShowProductScreen() {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // Draw white text on black
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  const float version = 0.3;
  display.println("Sourdough Monitor");
  display.print("Version ");
  display.println(version);
  display.println("github:bernhard-hofmann/SourdoughMonitor");
  display.display();
}

void InitializeTempAndHumiditySensor() {
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
}

void InitializeDistanceSensor() {
  if (!vl.begin()) {
    Serial.println("Failed to find sensor");
    while (1);
  }
}

void InitializeWiFi() {
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
}

void ShowStats(float temp, float humd, byte range) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  char buf[128];

  // Line 1: Temp
  if (temp == -999999999) {
    sprintf(buf, "Temp FAIL");
  } else {
    sprintf(buf, "Temp : %.1fC", temp);
  }
  Serial.println(buf);
  display.println(buf);

  // Line 2: Humidity
  if (humd == -999999999) {
    sprintf(buf, "Humidity FAIL");
  } else {
    sprintf(buf, "Humd : %.1f%%", temp);
  }
  Serial.println(buf);
  display.println(buf);

  // Line 3: Range
  if (range >= 255) {
    sprintf(buf, "Range FAIL");
  } else {
    sprintf(buf, "Range: %dmm", range);
  }
  display.println(buf);
  Serial.println(buf);

}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis % 5000 == 0) {
    float temp = 0.0;
    float humd = 0.0;
    byte range;

    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      temp = -999999999;
    }
    else {
      temp = event.temperature;
    }

    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      humd = -999999999;
    }
    else {
      humd = event.relative_humidity;
    }

    range = vl.readRange();
    uint8_t status = vl.readRangeStatus();

    if (status != VL6180X_ERROR_NONE) {
      range = 255;
    }

    ShowStats(temp, humd, range);

    // region Store data samples
    Serial.print("Inserting data at position ");
    Serial.print(insertionIndex);
    Serial.print(". Temp=");
    Serial.printf("%d", (char)((temp - tempBaseLine) * 10));
    temperatureSamples[insertionIndex] = (char)((temp - tempBaseLine)*10);
    Serial.print(". RelHum=");
    Serial.printf("%d", (char)humd);
    humiditySamples[insertionIndex] = (char)humd;
    Serial.print(". Range=");
    Serial.printf("%d", (byte)range);
    Serial.println();
    rangeSamples[insertionIndex] = (byte)range;

    insertionIndex++;
    if (insertionIndex > maxSamples - 1) {
      insertionIndex = 0;
    }
    // end Store data samples

    display.println("Logging data...");
    display.display();

    // region WiFi POST
    if (WiFi.status() == WL_CONNECTED) {
      SendToCloud(temp, humd, range);
    }
    else {
      InitializeWiFi();
      SendToCloud(temp, humd, range);
    }
    // endregion WiFi POST
  } else {
    if (currentMillis % 5 == 0) {
      bool isPushButtonDown = pushButton.onPress();
      if (isPushButtonDown) {
        HandleButtonPress();
      }
    }
  }
}

void HandleButtonPress() {
  Serial.println("Button!");
}

void SendToCloud(float temp, float humd, byte range) {
  WiFiClient client;
  HTTPClient http;
  char buf[1024];

  // Your Domain name with URL path or IP address with path
  Serial.print("Sending data to ");
  Serial.println(iotPlotterUrl);
  http.begin(client, iotPlotterUrl);

  // Specify content-type header
  http.addHeader("api-key", iotPlotterApiKey);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  // Data to send with HTTP POST
  sprintf(buf, "{\"data\":{\"Temperature\":[{\"value\":%.1f}],\"Humidity\":[{\"value\":%.1f}], \"Range\":[{\"value\":%d}]}}", temp, humd, range);
  Serial.print("Sending: "); Serial.println(buf);
  int httpResponseCode = http.POST(buf);
  Serial.print("HTTP: "); Serial.println(httpResponseCode);

  // Free resources
  http.end();

  display.fillRect(0, 24, 128, 8, SSD1306_BLACK);
  display.display();
  display.setCursor(0, 24);
  if (httpResponseCode >= 200 && httpResponseCode < 300) {
    display.println("Data logged OK");
  } else {
    sprintf(buf, "Logging FAILED: %d", httpResponseCode);
    display.println(buf);
  }
  display.display();
}
