#include "logger.h"



SerialLogEndpoint::SerialLogEndpoint() {};

SerialLogEndpoint::~SerialLogEndpoint() {};

void SerialLogEndpoint::log(std::string message) {
    Serial.println(message.c_str());

};


MQTTLogEndpoint::MQTTLogEndpoint(PubSubClient* mqtt_client, std::string topic) :  mqtt_client{mqtt_client}, topic{topic} {};

MQTTLogEndpoint::~MQTTLogEndpoint() {};

void MQTTLogEndpoint::log(std::string message) {
    mqtt_client->publish(topic.c_str(), message.c_str());
};



void LogStrategy::set_logendpoint(LogEndpoint* _logendpoint) {
    logendpoint = _logendpoint;
};


EagerLogStrategy::EagerLogStrategy(LogEndpoint* _logendpoint) {
    set_logendpoint(_logendpoint);
}


void EagerLogStrategy::log(std::string message) {
    logendpoint->log(message);
};

void EagerLogStrategy::commit() {};        


LazyLogStrategy::LazyLogStrategy(LogEndpoint* _logendpoint) {
    set_logendpoint(_logendpoint);
}

void LazyLogStrategy::log(std::string message) {
    buffer += message + "\n";

};

void LazyLogStrategy::commit() {
    logendpoint->log(buffer);
};        



Logger::Logger(LogStrategy* logstrategies) {

};

void Logger::log(std::string message) {

};

void Logger::commit() {

};


