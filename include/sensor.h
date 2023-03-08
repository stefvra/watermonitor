#pragma once
#include <Arduino.h>
#include <VL53L0X.h>
#include <SFE_BMP180.h>



class Sensor {
    private:
    public:
        virtual float measure() = 0;
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
        int lower_limit;
        int upper_limit;
    public:
        ~SensorValidator();
        SensorValidator(Sensor* _sensor, int _max_retries, int _lower_limit, int _upper_limit);
        float measure();
};

class SensorScaler : public Decorator {
    private:
        int scale;
        int offset;
    public:
        ~SensorScaler();
        SensorScaler(Sensor* _sensor, int _scale, int _offset);
        float measure();
};


class DistanceSensor : public Sensor {
    private:
        VL53L0X *sensor;
    public:
        DistanceSensor();
        ~DistanceSensor();
        float measure();
        bool init();
};


class VoltageSensor : public Sensor {
    private:
        int pin = 33;
    public:
        VoltageSensor();
        VoltageSensor(int _pin);
        float measure();
        bool init();
};

class BMPSensor : public Sensor {
    private:
        SFE_BMP180 *sensor;
    public:
        BMPSensor();
        ~BMPSensor();
        float measure();
        bool init();
};