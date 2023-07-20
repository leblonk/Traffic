#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <Fonts/FreeSansBold9pt7b.h>

// Defines:
// const char* wifiNetwork1 = "???";
// const char* wifiNetwork2 = "???";
// const char* wifiPassword = "???";
// Change path to your local system
#include "/Users/leblonk/.ssh/wifi.h"
// Defines:
// const char* mapquest_key = "???";
// Change path to your local system
#include "/Users/leblonk/.ssh/mapquest_key.h"

const char* fromAddress = "26%20Franklin%20Dr,East Orange,NJ%2007044";
const char* toAddress = "954%20S%203rd%20St,Harrison,NJ%2007029";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

ESP8266WiFiMulti WiFiMulti;

int blink = 0;

void showBlink() {
  display.clearDisplay();
  display.setCursor(0, 0);
  if ( blink >= 0 ) display.drawPixel(0, 0, WHITE);
  if ( blink >= 1 ) display.drawPixel(1, 0, WHITE);
  if ( blink >= 2 ) display.drawPixel(1, 1, WHITE);
  if ( blink >= 3 ) display.drawPixel(0, 1, WHITE);
  display.display();
  blink = (blink + 1) % 4;
}

void showText(const String& text) {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(text);
  display.display(); 
}

void showText(const String& top, int number, const String& bottom) {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(22, 0);
  display.println(top);

  display.setFont(&FreeSansBold9pt7b);
  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.setCursor(55, 22);
  display.println(number);

  display.setFont();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(42, 25);
  display.println(bottom);

  display.display(); 
}

void setup() {
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display.setRotation(2);
  int connecting = 0;
  const char* connectStrings[] = {"Connecting..", "Connecting..."};
  showText(connectStrings[connecting++ % 2]);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(wifiNetwork1, wifiPassword);
  WiFiMulti.addAP(wifiNetwork2, wifiPassword);

  while ( WiFiMulti.run() != WL_CONNECTED ) {
    delay(500);
    showText(connectStrings[connecting++ % 2]);
    Serial.print(".");
  }

  Serial.println("WiFi connected");  

  showText("Connected");
  delay(1000);
}

void loop() {
  uint8_t status = WiFiMulti.run();
  if ( status != WL_CONNECTED ) {
    Serial.println("Wifi status " + status);
  } else {
    WiFiClient client;
    HTTPClient http;
    http.setReuse(false);

    String timeUrl = "http://worldtimeapi.org/api/timezone/America/New_York";
    String routeUrl = String("http://www.mapquestapi.com/directions/v2/route")
    + "?key="
    + mapquest_key
    + "&from="
    + fromAddress
    + "&to="
    + toAddress
    + "&narrativeType=none"
    + "&maxLinkId=0";
  
    DynamicJsonDocument doc(20000);

    boolean showDriveTime = false;
    if ( http.begin(client, timeUrl) ) {
      int httpCode = http.GET();
      if ( httpCode == HTTP_CODE_OK ) {
        const String& payload = http.getString();
        DeserializationError error = deserializeJson(doc, payload);
        if ( !error ) {
          int day = doc["day_of_week"];
          // format: "2019-12-11T00:00:29.993387-05:00"
          String datetime = doc["datetime"];
          String hourStr = datetime.substring(11, 13);
          int hour = hourStr.toInt();
          // only show drive time between 3pm and 8pm on a weekday
          // needed to not exceed the max API calls allowed
          if ( day >=1 && day <= 5 && hour >= 15 && hour <= 20 ) {
            showDriveTime = true;
            Serial.printf("showing drivetime on %d:%d\n", day, hour);
          } else {
            Serial.printf("not showing drivetime on %d:%d\n", day, hour);
          }
        } else {
          Serial.printf("json error: %s\n", error.c_str());
          Serial.println(payload);
        }
      } else {
        Serial.printf("http code %d\n", httpCode);
      }
      http.end();
    } else {
      Serial.println("Unable to connect to timeServer");
    }

    if ( showDriveTime ) {
      if ( http.begin(client, routeUrl) ) {
        int httpCode = http.GET();
        if ( httpCode == HTTP_CODE_OK ) {
          const String& payload = http.getString();
          DeserializationError error = deserializeJson(doc, payload);
          if ( !error ) {
            long time = doc["route"]["time"];
            long minutes = time / 60;
            String message = String("Drive time home\n") + minutes + " minutes";
            Serial.println(message);
            showText("Drive time home", minutes, "minutes");
          } else {
            Serial.printf("json error: %s\n", error.c_str());
            Serial.println(payload);
          }
        } else {
          Serial.printf("http code %d", httpCode);
        }
        http.end();
      } else {
        Serial.println("Unable to connect to route");
      }
    } else {
      showBlink();
    }
  }
  delay(60000);
}
