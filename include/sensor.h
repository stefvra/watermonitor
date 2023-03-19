#pragma once
#include <Arduino.h>
#include <VL53L0X.h>
#include <SFE_BMP180.h>
#include <float.h>
#include "logger.h"

#define FAULTY_MEASUREMENT FLT_MAX
#define FAULTY_MEASUREMENT_INITIALIZER { .value = 0, .valid = false}



struct measurement {
    float value;
    bool valid;
};


// measurement faulty_measurement = { .value = 0, .valid = false};



class Sensor {
    private:
    public:
        virtual measurement measure() = 0;
        virtual bool init() = 0;
};


class Decorator : public Sensor {
    protected:
        Sensor* sensor;
    public:
        bool init();
        void set_sensor(Sensor*);
};

class SensorValidator : public Decorator {
    private:
        int max_retries;
        float lower_limit;
        float upper_limit;
    public:
        ~SensorValidator();
        SensorValidator(Sensor* _sensor, int _max_retries, float _lower_limit, float _upper_limit);
        measurement measure();
};

class SensorScaler : public Decorator {
    private:
        float scale;
        float offset;
    public:
        ~SensorScaler();
        SensorScaler(Sensor* _sensor, float _scale, float _offset);
        measurement measure();
};

class SensorInitializer : public Decorator {
    private:
    public:
        ~SensorInitializer();
        SensorInitializer(Sensor* _sensor);
        measurement measure();
};



class DistanceSensor : public Sensor {
    private:
        VL53L0X *sensor;
    public:
        DistanceSensor();
        ~DistanceSensor();
        measurement measure();
        bool init();
};


class VoltageSensor : public Sensor {
    private:
        int pin;
    public:
        VoltageSensor(int _pin);
        measurement measure();
        bool init();
};

class BMPSensor : public Sensor {
    private:
        SFE_BMP180 *sensor;
    public:
        BMPSensor();
        ~BMPSensor();
        measurement measure();
        bool init();
};