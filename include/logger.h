#pragma once
#include <Arduino.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <string>


class LogEndpoint {
    public:
        virtual void log(std::string message) = 0;
};

class SerialLogEndpoint : public LogEndpoint {
    public:
        SerialLogEndpoint();
        ~SerialLogEndpoint();
        void log(std::string message);
};

class MQTTLogEndpoint : public LogEndpoint {
    private:
        PubSubClient* mqtt_client;
        std::string topic;
        int max_message_size;
    public:
        MQTTLogEndpoint(PubSubClient* mqtt_client, std::string topic, int max_message_size=100);
        ~MQTTLogEndpoint();
        void log(std::string message);
};



class LogStrategy {
    protected:
        LogEndpoint* logendpoint;
    public:
        void set_logendpoint(LogEndpoint* logendpoint);
        virtual void log(std::string message) = 0;
        virtual void commit() = 0;
};

class EagerLogStrategy : public LogStrategy {
    public:
        EagerLogStrategy(LogEndpoint* logendpoint);
        void log(std::string message);
        void commit();        
};

class LazyLogStrategy : public LogStrategy  {
    private:
        std::string buffer;
    public:
        LazyLogStrategy(LogEndpoint* logendpoint);
        void log(std::string message);
        void commit();        
};



class Logger {
    private:
        LogStrategy** logstrategies;
        int n_loggers_defined;
        int n_loggers_declared = 5;
    public:
        Logger();
        ~Logger();
        void add_logstrategy(LogStrategy* logstrategy);
        void log(std::string message);
        void commit();
};

