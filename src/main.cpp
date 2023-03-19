#include <Arduino.h>

/* This example shows how to use continuous mode to take
range measurements with the VL53L0X. It is based on
vl53l0x_ContinuousRanging_Example.c from the VL53L0X API.
The range readings are in units of mm. */



#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <string>
#include <EEPROM.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "secrets.h"
#include "root_ca.h"
#include "sensor.h"
#include "logger.h"


#define GSM // GSM or WIFI




#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1800ULL //3600 * 6  /* Time ESP32 will go to sleep (in seconds) */
#define EEPROM_SIZE 12
#define MAX_MQTT_RETRIES 5
#define NR_MEASUREMENTS 3 /* amount of measurements to perform */

// mqtt definitions
#define TOPIC "watermonitor/production"
std::string V1_NAME = "distance";
std::string V2_NAME = "vbat";
std::string V3_NAME = "temperature";

// gpio definitions
#define VBAT_PIN 14 
#define LED_PIN 19   

#ifdef GSM
  #define TINY_GSM_MODEM_SIM800
  #include <TinyGsmClient.h>
  #define SerialAT Serial2
  #define SIM800_PIN 12   
  #define CONNECTION_TIMEOUT 25000L // time in ms to try getting network connection

  // debugging option
  // #define DUMP_AT_COMMANDS
  #ifdef DUMP_AT_COMMANDS
    #include <StreamDebugger.h>
    StreamDebugger debugger(SerialAT, Serial);
    TinyGsm        modem(debugger);
  #else
    TinyGsm        modem(SerialAT);
  #endif
  
  TinyGsmClient client(modem);
#else
  WiFiClient client; // secure client to be implemented
  #define MAX_WIFI_RETRIES 20
#endif

PubSubClient mqtt(client);


bool online;
std::string message;

// define measurements
measurement distance;
DistanceSensor __distancesensor;
SensorInitializer _distancesensor(&__distancesensor);
SensorValidator distancesensor(&_distancesensor, 5, 10, 2000);

measurement temp;
BMPSensor __temperaturesensor;
SensorInitializer _temperaturesensor(&__temperaturesensor);
SensorValidator temperaturesensor(&_temperaturesensor, 5, -10, 50);

measurement vbat;
VoltageSensor ___vbatsensor(VBAT_PIN);
SensorInitializer __vbatsensor(&___vbatsensor);
SensorScaler _vbatsensor(&__vbatsensor, 2 * 3.3 / 4095 * 4.81 / 4.72, 0);
SensorValidator vbatsensor(&_vbatsensor, 5, 3, 5);



MQTTLogEndpoint mqtt_logendpoint(&mqtt, std::string(TOPIC));
LazyLogStrategy mqtt_logger(&mqtt_logendpoint);

SerialLogEndpoint seriallogendpoint;
EagerLogStrategy seriallogger(&seriallogendpoint);

Logger logger;


#ifdef WIFI
  bool setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(SSID);

    WiFi.begin(SSID, WIFI_PWD);

    int wifi_retries = 0;
    while (wifi_retries < MAX_WIFI_RETRIES) {
      wifi_retries++;
      if (WiFi.status() == WL_CONNECTED)
        {
          Serial.println("");
          Serial.println("WiFi connected");
          Serial.println("IP address: ");
          Serial.println(WiFi.localIP());
          // client.setCACert(root_ca); // secure client to be implemented
          return true;
        }
      delay(500);
      Serial.print(".");
    }
    return false;
  }
#endif

#ifdef GSM
  bool setup_gsm() {

    Serial.println("Starting modem connection...");
    // Set GSM module baud rate
    SerialAT.begin(115200);
    modem.setBaud(115200);
    delay(500);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    Serial.println("Initializing modem...");
    if (!modem.init()) {
      Serial.println(" fail");
      return false;      
    };


    String modemInfo = modem.getModemInfo();
    Serial.print("Modem Info: ");
    Serial.println(modemInfo);


    if (SIM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(SIM_PIN); }

    Serial.print("Waiting for network...");
    if (!modem.waitForNetwork(CONNECTION_TIMEOUT)) {
      Serial.println(" fail");
      return false;
    }
    Serial.println(" success");

    Serial.print(F("Connecting to "));
    Serial.print(APN);
    if (!modem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
      Serial.println(" fail");
      return false;
    }
    Serial.println(" success");

    if (modem.isGprsConnected()) { Serial.println("GPRS connected"); }

    Serial.print("signal quality (0-30): ");
    Serial.print(modem.getSignalQuality());
    Serial.println();
    
    return true;


  }
#endif


bool setup_mqtt() {


  mqtt.setServer(MQTT_IP, MQTT_PORT);

  // Loop until we're reconnected
  int mqtt_retries = 0;
  while (mqtt_retries < MAX_MQTT_RETRIES) {
    mqtt_retries++;
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect("ESP8266Client", MQTT_UN, MQTT_PWD)) {
      Serial.println("connected");
      mqtt.loop();  
      return true;
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 3 seconds");
      // Wait 5 seconds before retrying
      delay(3000);
    }
  }
  return false;
}

int get_consecutive_false_startups() {
  // get amount of times that esp was started up not due to wakeup from deep sleep
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  EEPROM.begin(EEPROM_SIZE);
  int address = 0;
  int faulty_startups = EEPROM.read(address);

  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    // correct wakeup.
    faulty_startups = 0;
  } else {
    // false startup, trying again
    faulty_startups++;
  };

  EEPROM.write(address, faulty_startups);
  EEPROM.commit();
  return faulty_startups;

}


void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


void start_deep_sleep() {
  #ifdef GSM
    Serial.println("Turning off SIM module...");
    digitalWrite(SIM800_PIN, HIGH);
  #endif
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.print("deep sleep mode set, sleeping for ");
  Serial.print(TIME_TO_SLEEP);
  Serial.println(" seconds...");
  delay(300);
  esp_deep_sleep_start();
}



void publish_measurement(measurement result, std::string name) {
  if (result.valid) {
    Serial.println("starting publish");
    message = "{ \"" + name + "\": " + std::to_string(result.value) + "}";
    Serial.println(message.c_str());
    mqtt.publish(TOPIC, message.c_str());
  } else {
    Serial.println("result not valid, not publishing");
  }

}



void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  // at startup system is not online
  online = false;
  
  // setup loggers
  logger.add_logstrategy(&seriallogger);
  logger.add_logstrategy(&mqtt_logger);

  logger.log("hello");

  // set control led
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // start serial output
  Serial.begin(9600);
  delay(10);
  Serial.println("starting");

  print_wakeup_reason();
  Serial.print("consecutive false startups: ");
  Serial.println(get_consecutive_false_startups());


  // startup I2C communication
  Wire.begin();

  // stetup mqtt connection
  #ifdef GSM
    // switch on sim module
    pinMode(SIM800_PIN, OUTPUT);
    digitalWrite(SIM800_PIN, HIGH);
    if (setup_gsm()) {
      if (setup_mqtt()) {
        online = true;
      };
    };
  #else
    if (setup_wifi()) {
      if (setup_mqtt()) {
        online = true;
      };
    };
  #endif


  if (!online) {
    Serial.println("was not able to go in online mode.");
    start_deep_sleep();
  }


  // perform measurements
  Serial.println("Starting distance measurement...");
  distance = distancesensor.measure();
  publish_measurement(distance, V1_NAME);
  Serial.println("Starting temperature measurement...");
  temp = temperaturesensor.measure();
  publish_measurement(temp, V3_NAME);
  Serial.println("Starting vbat measurement...");
  vbat = vbatsensor.measure();
  publish_measurement(vbat, V2_NAME);  
    
  
  // delay to finish mqtt publishing. Not ideal, in some cases last messages are not published.
  delay(1000 * NR_MEASUREMENTS);

  start_deep_sleep();

  


}

void loop() {


}

