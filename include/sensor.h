#pragma once
#include <Arduino.h>
#include <VL53L0X.h>
#include <SFE_BMP180.h>



class Sensor {
    private:
    public:
    virtual int measure() = 0;
    virtual void init() = 0;
};


class DistanceSensor : public Sensor {
    private:
    VL53L0X *sensor;
    public:
    DistanceSensor();
    ~DistanceSensor();
    int measure();
    void init();
};


class VoltageSensor : public Sensor {
    private:
    int pin = 33;
    public:
    VoltageSensor();
    VoltageSensor(int _pin);
    int measure();
    void init();
};
