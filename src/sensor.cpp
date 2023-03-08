#include "sensor.h"



bool Decorator::init() {
  return sensor->init();
};

void Decorator::set_sensor(Sensor* _sensor) {
  sensor = _sensor;
};


SensorValidator::SensorValidator(Sensor* _sensor, int _max_retries, int _lower_limit, int _upper_limit) :
  max_retries{_max_retries},
  lower_limit{_lower_limit},
  upper_limit{_upper_limit}
  {set_sensor(_sensor);};

SensorValidator::~SensorValidator() {};


float SensorValidator::measure() {

  // implement measurement of a sensor. tries maximum max_retries
  // first value within limits is returned
  bool valid_measurement = false;
  int retries = 0;
  float result;

  while (valid_measurement == false and retries < max_retries) {
    result = measure();
    retries++;
    if (result < upper_limit and result > lower_limit) {
      valid_measurement = true;
    }
  };

  if (valid_measurement) {
    return result;
  } else {
    return 0;
  };
}


SensorScaler::SensorScaler(Sensor* _sensor, int _scale, int _offset) :
  scale{_scale},
  offset{_offset}
  {set_sensor(_sensor);};

SensorScaler::~SensorScaler() {};

float SensorScaler::measure() {

  // implement measurement of a sensor. tries maximum max_retries
  // first value within limits is returned
  return offset + scale * measure();
}




DistanceSensor::DistanceSensor() {
    sensor = new VL53L0X();
};


DistanceSensor::~DistanceSensor() {
    delete sensor;
};


bool DistanceSensor::init() {

  sensor->setTimeout(500);
  if (!sensor->init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    return false;
  } else {
    // setup for long range mode
    // lower the return signal rate limit (default is 0.25 MCPS)
    sensor->setSignalRateLimit(0.1);
    // increase laser pulse periods (defaults are 14 and 10 PCLKs)
    sensor->setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
    sensor->setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
    return true;
  }

}



float DistanceSensor::measure() {

  int D;
  D = sensor->readRangeSingleMillimeters();
  if (sensor->timeoutOccurred()){
  Serial.println("Distance sensor TIMEOUT");
  } else {
  Serial.print("distance: ");
  Serial.print(D);
  Serial.println(" mm");
  }
  return (float) D;
}





VoltageSensor::VoltageSensor(int _pin) {
  pin = _pin;
}

VoltageSensor::VoltageSensor() {}



bool VoltageSensor::init() {
  pinMode(pin, INPUT);
  return true;
}



float VoltageSensor::measure() {

  int D;
  D = analogRead(pin);
  return (float) D;
}





BMPSensor::BMPSensor() {
    sensor = new SFE_BMP180();
}


BMPSensor::~BMPSensor() {
    delete sensor;
}


bool BMPSensor::init() {

  // BMP180 sensor
  if (sensor->begin()) {
    Serial.println("BMP180 init success");
    return true;
  } else {
  // Oops, something went wrong, this is usually a connection problem,
  // see the comments at the top of this sketch for the proper connections.
    Serial.println("BMP180 init fail\n\n");
    return false;
  }


}


float BMPSensor::measure() {

  char status;
  double T,P,p0,a;

  status = sensor->startTemperature();
  if (status != 0)
  {
  // Wait for the measurement to complete:
  delay(status);
  // Retrieve the completed temperature measurement:
  // Note that the measurement is stored in the variable T.
  // Function returns 1 if successful, 0 if failure.
  status = sensor->getTemperature(T);
  if (status != 0)
  {
  // Print out the measurement:
  return (float) T;
  Serial.print("temperature: ");
  Serial.print(T,2);
  Serial.print(" deg C, ");

  // Start a pressure measurement:
  // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = sensor->startPressure(3);
  if (status != 0)
  {
  // Wait for the measurement to complete:
  delay(status);

  // Retrieve the completed pressure measurement:
  // Note that the measurement is stored in the variable P.
  // Note also that the function requires the previous temperature measurement (T).
  // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
  // Function returns 1 if successful, 0 if failure.

  status = sensor->getPressure(P,T);
  if (status != 0)
  {
  // Print out the measurement:
  Serial.print("pressure: ");
  Serial.print(P,2);
  Serial.println(" mb, ");
  }
  }
  }
  }
}