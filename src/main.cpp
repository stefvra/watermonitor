#include <Arduino.h>

/* This example shows how to use continuous mode to take
range measurements with the VL53L0X. It is based on
vl53l0x_ContinuousRanging_Example.c from the VL53L0X API.
The range readings are in units of mm. */


#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <string>

#include "secrets.h"
#include "sensor.h"


#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  864000   /* Time ESP32 will go to sleep (in seconds) */



WiFiClient espClient;

PubSubClient client(espClient);
std::string message;

DistanceSensor distancesensor;
int d;

// VoltageSensor USBVoltageSensor = VoltageSensor(33);
// SFE_BMP180 pressure;
// int v_in, v_supply, v_pv, v_batt;


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, WIFI_PWD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", MQTT_UN, MQTT_PWD)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}




void setup() {

  Serial.begin(9600);
  Serial.println("starting");
  Wire.begin();

  setup_wifi();
  client.setServer(MQTT_IP, MQTT_PORT);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());




  distancesensor.init();
  //USBVoltageSensor.init();


  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);


  /*


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



  server.on("/", handle_root);
  server.begin();
  Serial.println("HTTP server started");

  delay(100);
  */
}

void loop() {

  d = distancesensor.measure();
  // v_in = InputVoltageSensor.measure();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  message = std::to_string(d);
  Serial.println("starting publish");
  client.publish("watermonitor", message.c_str());
  Serial.println("stoping publish");

  delay(1000);

  // esp_deep_sleep_start();


}

/*
// Handle root url (/)
void handle_root() {
  Serial.println("request received...");


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

}
*/



