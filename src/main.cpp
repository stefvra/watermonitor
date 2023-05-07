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

// ADDED
#define WIFI // GSM or WIFI





#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  3600 * 6  /* Time ESP32 will go to sleep (in seconds) */
#define EEPROM_SIZE 12
#define MQTT_MAX_RETRIES 5
#define MQTT_DELAY_AFTER_PUBLISH 5000 // should be large enough to send logging info
#define MQTT_BUFFERSIZE 4 * 256 // 32 times normal size for logging purposes. too large messages anyway not possible to send
#define NR_MEASUREMENTS 3 /* amount of measurements to perform */

// mqtt definitions
#define TOPIC "watermonitor/production"
#define LOG_TOPIC "watermonitor/production/log"
std::string V1_NAME = "distance";
std::string V2_NAME = "vbat";
std::string V3_NAME = "temperature";

// gpio definitions
#define VBAT_PIN 34
#define LED_PIN 19   

#ifdef GSM
  #define TINY_GSM_MODEM_SIM800
  #include <TinyGsmClient.h>
  #define SerialAT Serial2
  #define SIM800_PIN 33   
  #define CONNECTION_TIMEOUT 25000L // time in ms to try getting network connection

  // debugging option
  #define DUMP_AT_COMMANDS
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



MQTTLogEndpoint mqtt_logendpoint(&mqtt, std::string(LOG_TOPIC));
LazyLogStrategy mqtt_logger(&mqtt_logendpoint);

SerialLogEndpoint seriallogendpoint;
EagerLogStrategy seriallogger(&seriallogendpoint);


// ADDED
WiFiClient wifi_client; // secure client to be implemented
#define MAX_WIFI_RETRIES 20
PubSubClient wifi_mqtt(wifi_client);
MQTTLogEndpoint wifi_mqtt_logendpoint(&wifi_mqtt, std::string(LOG_TOPIC), 1000);
LazyLogStrategy wifi_mqtt_logger(&wifi_mqtt_logendpoint);





Logger logger;


#ifdef WIFI
  bool setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
    logger.log("Connecting to ");
    logger.log(SSID);

    WiFi.begin(SSID, WIFI_PWD);

    int wifi_retries = 0;
    while (wifi_retries < MAX_WIFI_RETRIES) {
      wifi_retries++;
      if (WiFi.status() == WL_CONNECTED)
        {
          logger.log("");
          logger.log("WiFi connected");
          logger.log("IP address: ");
          logger.log(std::to_string(WiFi.localIP()));
          // client.setCACert(root_ca); // secure client to be implemented
          return true;
        }
      delay(500);
      logger.log(".");
    }
    return false;
  }
#endif

#ifdef GSM
  bool setup_gsm() {

    logger.log("m conn");
    // Set GSM module baud rate
    SerialAT.begin(115200);
    modem.setBaud(115200);
    delay(500);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    logger.log("m init...");
    if (!modem.init()) {
      logger.log(" fail");
      return false;      
    };


    char modemInfo_char[100];
    modem.getModemInfo().toCharArray(modemInfo_char, 100);
    logger.log("M Info ");
    logger.log(modemInfo_char);


    if (SIM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(SIM_PIN); }

    logger.log("nw init...");
    if (!modem.waitForNetwork(CONNECTION_TIMEOUT)) {
      logger.log(" fail");
      return false;
    }
    logger.log(" success");

    logger.log("Conn to ");
    logger.log(APN);
    if (!modem.gprsConnect(APN)) {
      logger.log(" fail");
      return false;
    }
    logger.log(" success");

    if (modem.isGprsConnected()) { logger.log("GPRS ok"); }

    logger.log("sq ");
    logger.log(std::to_string(modem.getSignalQuality()));
    
    return true;


  }
#endif


bool setup_mqtt(PubSubClient& mqtt_client) {

  mqtt_client.setBufferSize(MQTT_BUFFERSIZE);
  mqtt_client.setServer(MQTT_IP, MQTT_PORT);

  // Loop until we're reconnected
  int mqtt_retries = 0;
  while (mqtt_retries < MQTT_MAX_RETRIES) {
    mqtt_retries++;
    logger.log("MQTT conn...");
    // Attempt to connect
    if (mqtt_client.connect("ESP8266Client", MQTT_UN, MQTT_PWD)) {
      logger.log("ok");
      mqtt_client.loop();  
      return true;
    } else {
      logger.log("failed, rc=");
      logger.log(std::to_string(mqtt_client.state()));
      logger.log(" retry in 3s");
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
    case ESP_SLEEP_WAKEUP_EXT0 : logger.log("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : logger.log("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : logger.log("timer Wakeup"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : logger.log("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : logger.log("Wakeup caused by ULP program"); break;
    default : {
      logger.log("faulty wakeup");
      logger.log(std::to_string(wakeup_reason));
      break;
      }
  }
}


void start_deep_sleep() {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  #ifdef GSM
    digitalWrite(SIM800_PIN, HIGH);
    logger.log("SIM down");
  #endif
  logger.log("sleeping for ");
  logger.log(std::to_string(TIME_TO_SLEEP));
  logger.log("s, goodnight...");
  logger.commit();
  Serial.println("really going to sleep now...");
  delay(MQTT_DELAY_AFTER_PUBLISH); // time needed to finish mqtt publishing. Logging takes some time
  esp_deep_sleep_start();
}



void publish_measurement(measurement result, std::string name) {
  if (result.valid) {
    logger.log("start pub " + name);
    message = "{ \"" + name + "\": " + std::to_string(result.value) + "}";
    mqtt.publish(TOPIC, message.c_str());
    delay(100);
  } else {
    logger.log("r not valid");
  }

}



void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  // at startup system is not online
  online = false;


  // start serial output
  Serial.begin(9600);
  delay(10);  // DO NOT REMOVE
  Serial.println("starting");

  // setup loggers
  logger.add_logstrategy(&seriallogger);
  logger.add_logstrategy(&mqtt_logger);
  // ADDED
  logger.add_logstrategy(&wifi_mqtt_logger);

  // set control led
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  print_wakeup_reason();
  logger.log("false sups: ");
  logger.log(std::to_string(get_consecutive_false_startups()));


  // startup I2C communication
  Wire.begin();

  // stetup mqtt connection
  #ifdef GSM
    // switch on sim module
    pinMode(SIM800_PIN, OUTPUT);
    digitalWrite(SIM800_PIN, HIGH);
    if (setup_gsm()) {
      if (setup_mqtt(mqtt)) {
        online = true;
      };
    };
  #endif

  #ifdef WIFI
    if (setup_wifi()) {
      if (setup_mqtt(wifi_mqtt)) {
        online = true;
      };
    };
  #endif


  if (!online) {
    logger.log("was not able to go in online mode.");
    start_deep_sleep();
  }


  // perform measurements
  logger.log("Start dist meas...");
  distance = distancesensor.measure();
  publish_measurement(distance, V1_NAME);
  logger.log("Start temp meas...");
  temp = temperaturesensor.measure();
  publish_measurement(temp, V3_NAME);
  logger.log("Start vbat meas...");
  vbat = vbatsensor.measure();
  publish_measurement(vbat, V2_NAME);  
    
  
  // delay to finish mqtt publishing. Not ideal, in some cases last messages are not published.
  delay(1000 * NR_MEASUREMENTS);

  start_deep_sleep();

  


}

void loop() {


}

