#include <Arduino.h>

/* This example shows how to use continuous mode to take
range measurements with the VL53L0X. It is based on
vl53l0x_ContinuousRanging_Example.c from the VL53L0X API.
The range readings are in units of mm. */


#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <VL53L0X.h>
#include <SFE_BMP180.h>
#include "secrets.h"

VL53L0X sensor;
SFE_BMP180 pressure;

void handle_root();

WebServer server(80); // Object of WebServer(HTTP port, 80 is defult)


void setup() {
  Serial.begin(9600);
  Wire.begin();

  sensor.setTimeout(500);
  if (!sensor.init())
  {
  Serial.println("Failed to detect and initialize sensor!");
  while (1) {}
  }


  // setup for long range mode
  // lower the return signal rate limit (default is 0.25 MCPS)
  sensor.setSignalRateLimit(0.1);
  // increase laser pulse periods (defaults are 14 and 10 PCLKs)
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
  sensor.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);


  // BMP180 sensor
  if (pressure.begin())
  Serial.println("BMP180 init success");
  else
  {
  // Oops, something went wrong, this is usually a connection problem,
  // see the comments at the top of this sketch for the proper connections.

  Serial.println("BMP180 init fail\n\n");
  while(1); // Pause forever.
  }

  Serial.println("Try Connecting to ");
  Serial.println(SSID);



  // Connect to your wi-fi modem

  WiFi.begin(SSID, PWD);

  // Check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected successfully");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP()); //Show ESP32 IP on serial

  server.on("/", handle_root);
  server.begin();
  Serial.println("HTTP server started");

  delay(100);
}

void loop() {
  server.handleClient();
}


// Handle root url (/)
void handle_root() {
  Serial.println("request received...");

  int D;
  D = sensor.readRangeSingleMillimeters();
  if (sensor.timeoutOccurred()){
  Serial.println("Distance sensor TIMEOUT");
  } else {
  Serial.print("distance: ");
  Serial.print(D);
  Serial.println(" mm");
  }
  delay(300);

  // pressure and temp readings
  char status;
  double T,P,p0,a;

  status = pressure.startTemperature();
  if (status != 0)
  {
  // Wait for the measurement to complete:
  delay(status);
  // Retrieve the completed temperature measurement:
  // Note that the measurement is stored in the variable T.
  // Function returns 1 if successful, 0 if failure.
  status = pressure.getTemperature(T);
  if (status != 0)
  {
  // Print out the measurement:
  Serial.print("temperature: ");
  Serial.print(T,2);
  Serial.print(" deg C, ");

  // Start a pressure measurement:
  // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = pressure.startPressure(3);
  if (status != 0)
  {
  // Wait for the measurement to complete:
  delay(status);

  // Retrieve the completed pressure measurement:
  // Note that the measurement is stored in the variable P.
  // Note also that the function requires the previous temperature measurement (T).
  // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
  // Function returns 1 if successful, 0 if failure.

  status = pressure.getPressure(P,T);
  if (status != 0)
  {
  // Print out the measurement:
  Serial.print("pressure: ");
  Serial.print(P,2);
  Serial.println(" mb, ");
  }
  else Serial.println("error retrieving pressure measurement\n");
  }
  else Serial.println("error starting pressure measurement\n");
  }
  else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");

  String response;
  response = "Dist: " + String(D) + ", temp" + String(T) + ", press" + String(P);
  server.send(200, "text/plain", response);
}