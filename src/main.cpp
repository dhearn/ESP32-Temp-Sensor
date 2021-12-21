#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is connected to GPIO 4
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with a OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

DeviceAddress sensor1 = { 0x28, 0x2E, 0x41, 0xEB, 0x04, 0x00, 0x00, 0xDC };
DeviceAddress sensor2 = { 0x28, 0x03, 0x96, 0x75, 0xD0, 0x01, 0x3C, 0x85 };
DeviceAddress sensor3= { 0x28, 0xFF, 0xAA, 0xDE, 0x91, 0x16, 0x05, 0x28 };

void pollForDevice();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Setup successfully");

  pollForDevice();

  sensors.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  
  Serial.print("Sensor 1(*C): ");
  Serial.println(sensors.getTempC(sensor1)); 
 
  Serial.print("Sensor 2(*C): ");
  Serial.println(sensors.getTempC(sensor2)); 
  
  Serial.print("Sensor 3(*C): ");
  Serial.println(sensors.getTempC(sensor3)); 
  
  delay(2000);
}

void pollForDevice() {
  byte i;
  #define ONE_WIRE_ID_LENGTH 8
  byte addr[ONE_WIRE_ID_LENGTH];
  
  Serial.println("Polling for 1-wire devices.");

  if (!oneWire.search(addr)) {
    Serial.println(" No more addresses.");
    Serial.println();
    oneWire.reset_search();
    delay(250);
    return;
  }

  Serial.print(" ROM =");
  for (i = 0; i < ONE_WIRE_ID_LENGTH; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }  
}