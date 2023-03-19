#include "logger.h"



SerialLogEndpoint::SerialLogEndpoint() {
    delay(10);
};

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
    buffer += message + " ";

};

void LazyLogStrategy::commit() {
    logendpoint->log(buffer);
};        



Logger::Logger() {
    logstrategies = new LogStrategy*[n_loggers_declared];
    n_loggers_defined = 0;
};

Logger::~Logger() {
    delete[] logstrategies;
};


void Logger::add_logstrategy(LogStrategy* logstrategy) {
    if (n_loggers_defined < n_loggers_declared) {
        logstrategies[n_loggers_defined] = logstrategy;
        n_loggers_defined++;    
    } else {
        log(std::string("Too many loggers requested"));
        commit();
    }
};

void Logger::log(std::string message) {
    for (int i = 0; i < n_loggers_defined; i++) {
        logstrategies[i]->log(message);
    }
};

void Logger::commit() {
    for (int i = 0; i < n_loggers_defined; i++) {
        logstrategies[i]->commit();
    }
};


