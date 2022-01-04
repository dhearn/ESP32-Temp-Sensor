#ifdef ARDUINO_ARCH_ESP32
  //include ESP32 specific libs
  #include <WiFi.h>
  #include <HTTPClient.h>
#elif defined(ARDUINO_ARCH_ESP8266) 
  //include esp8266 specific libs
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
#else  
//...
#endif
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include "credentials.h"

// Data wire is connected to GPIO 4
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with a OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Real 
DeviceAddress sensor1 = { 0x28, 0x2E, 0x41, 0xEB, 0x04, 0x00, 0x00, 0xDC };
DeviceAddress sensor2 = { 0x28, 0x03, 0x96, 0x75, 0xD0, 0x01, 0x3C, 0x85 };
DeviceAddress sensor3 = { 0x28, 0xFF, 0xAA, 0xDE, 0x91, 0x16, 0x05, 0x28 };

// Test
// DeviceAddress sensor1 = { 0x28, 0x67, 0xCB, 0x43, 0x0C, 0x00, 0x00, 0xD9 };
// DeviceAddress sensor2 = { 0x28, 0xD7, 0xDE, 0x5B, 0x0D, 0x00, 0x00, 0xFF };
// DeviceAddress sensor3 = { 0x28, 0xD7, 0xDE, 0x5B, 0x0D, 0x00, 0x00, 0xFF };

// Set in credentials.h
const char ssid[] = WIFI_SSID;
const char wifiPassword[] = WIFI_PASSWORD;

//Your Domain name with URL path or IP address with path
#ifdef ARDUINO_ARCH_ESP32
  // ESP32 supports HTTPS
  String serverName = "https://emoncms.org:443/input/post";
#elif defined(ARDUINO_ARCH_ESP8266) 
  // ESP8266 does not support HTTPS, so fallback to HTTP only
  String serverName = "http://emoncms.org/input/post";
#else  
//...
#endif
const String emoncmsWriteKey = EMONCMS_WRITE_KEY;
const String emoncmsNode = EMONCMS_NODE;

// Sensor returned -127 (when being moved?), so only log values which appear sensible
const float minValidTemperature = -20;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;

// Set timer to 1 minute (60000)
unsigned long timerDelay = 60000;
// Set timer to 20 secs (20000)
//unsigned long timerDelay = 20000;

// Set the value to ensure we always do a reading on startup
unsigned long timeOfLastLoopCheck = timerDelay + 1; 

void pollForDevice();
void setupWifi();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Setup started");
  // Serial.setDebugOutput(true);
  // esp_log_level_set("*", ESP_LOG_VERBOSE);
  // esp_log_level_set("*", ESP_LOG_INFO);
  // esp_log_level_set("wifi", ESP_LOG_WARN);      // enable WARN logs from WiFi stack

  setupWifi();

  pollForDevice();
}

void loop() {
  // put your main code here, to run repeatedly:

  // Added this to loop(), as just having it in setup() seemed to cause sensor values to never change
  sensors.begin();

  //Send an HTTP POST request every X minutes
  if (timeOfLastLoopCheck > timerDelay) {

    Serial.println("Timer expired, checking WiFi connected");

    String jsonData;
    StaticJsonDocument<192> doc;

    Serial.print("Requesting temperatures...");
    sensors.setWaitForConversion(true);
    sensors.requestTemperatures(); // Send the command to get temperatures
    // delay(5000);
    Serial.println("DONE");
    
    float sensor1Value = sensors.getTempC(sensor1);
    Serial.print("Sensor 1(*C): ");
    Serial.println(sensor1Value); 
  
    float sensor2Value = sensors.getTempC(sensor2);
    Serial.print("Sensor 2(*C): ");
    Serial.println(sensor2Value); 
    
    float sensor3Value = sensors.getTempC(sensor3);
    Serial.print("Sensor 3(*C): ");
    Serial.println(sensor3Value);

    if ((sensor1Value > minValidTemperature) &&
        (sensor2Value > minValidTemperature) &&
        (sensor3Value > minValidTemperature)) {

      if (sensor1Value > minValidTemperature) {
        doc["outdoor-temp"] = sensor1Value;
      }
      else {
        Serial.println("Sensor 1 value is less than minimum valid value, omitting this value");
      }
      
      if (sensor2Value > minValidTemperature) {
        doc["boiler-flow-temp"] = sensor2Value;
      }
      else {
        Serial.println("Sensor 2 value is less than minimum valid value, omitting this value");
      }

      if (sensor3Value > minValidTemperature) {
        doc["boiler-return-temp"] = sensor3Value;
      }
      else {
        Serial.println("Sensor 3 value is less than minimum valid value, omitting this value");
      }

      serializeJson(doc, jsonData);
      Serial.println(jsonData);

      Serial.println("Checking WiFi status");

      //Check WiFi connection status
      if(WiFi.status() == WL_CONNECTED){

        Serial.println("WiFi connected");

        WiFiClient client;
        HTTPClient http;
        http.setReuse(true);
        http.useHTTP10(false);
          
        Serial.println("Building HTTP request with readings");

        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        // http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        // http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);

        String httpRequestData = "?node=" + emoncmsNode + "&fulljson=" + jsonData + "&apikey=" + emoncmsWriteKey;

        Serial.print("httpRequestData: ");
        Serial.println(httpRequestData);

        String serverPathRequest = serverName + httpRequestData;

        // Your Domain name with URL path or IP address with path
        http.begin(client, serverPathRequest.c_str());
        
        Serial.println("Sending HTTP request");

        // Send HTTP GET request
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0) {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          String payload = http.getString();
          Serial.println(payload);
        }
        else {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
        }

        // Free resources
        http.end();
      }
      else {
        Serial.println("WiFi Disconnected");
      }
    }
    else {
      Serial.println("All temperature values were less than minimum value, skipping until next cycle");
    }

    lastTime = millis();
  }

  timeOfLastLoopCheck = (millis() - lastTime);
}

void pollForDevice() {
  #define ONE_WIRE_ID_LENGTH 8
  uint8_t deviceAddress[ONE_WIRE_ID_LENGTH];

  Serial.println("Polling for devices");

  sensors.begin();

  uint8_t deviceCount = sensors.getDeviceCount();

  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" devices");

  sensors.setWaitForConversion(true);
  sensors.requestTemperatures(); // Send the command to get temperatures

  // Display temperature from each sensor
  for (uint8_t deviceIndex = 0;  deviceIndex < deviceCount;  deviceIndex++)
  {
    Serial.print("Sensor ");
    Serial.print(deviceIndex);
    Serial.print(" - ");
    sensors.getAddress(deviceAddress, deviceIndex);

    for (uint8_t i = 0; i < ONE_WIRE_ID_LENGTH; i++) {
      Serial.write(' ');
      Serial.print(deviceAddress[i], HEX);
    } 

    Serial.print(" : ");
    float tempC = sensors.getTempCByIndex(deviceIndex);
    Serial.println(tempC);
  }



  // byte i;
  // #define ONE_WIRE_ID_LENGTH 8
  // byte addr[ONE_WIRE_ID_LENGTH];
  
  // Serial.println("Polling for 1-wire devices.");

  // if (!oneWire.search(addr)) {
  //   Serial.println(" No more addresses.");
  //   Serial.println();
  //   oneWire.reset_search();
  //   delay(250);
  //   return;
  // }

  // Serial.print(" ROM =");
  // for (i = 0; i < ONE_WIRE_ID_LENGTH; i++) {
  //   Serial.write(' ');
  //   Serial.print(addr[i], HEX);
  // }  
}

void setupWifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, wifiPassword);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(WiFi.status());
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}