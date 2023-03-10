#include "sensor.h"


measurement faulty_measurement = FAULTY_MEASUREMENT_INITIALIZER;



bool Decorator::init() {
  return sensor->init();
};

void Decorator::set_sensor(Sensor* _sensor) {
  sensor = _sensor;
};


SensorValidator::SensorValidator(Sensor* _sensor, int _max_retries, float _lower_limit, float _upper_limit) :
  max_retries{_max_retries},
  lower_limit{_lower_limit},
  upper_limit{_upper_limit}
  {set_sensor(_sensor);};

SensorValidator::~SensorValidator() {};


measurement SensorValidator::measure() {

  // implement measurement of a sensor. tries maximum max_retries
  // first value within limits is returned
  bool valid_measurement = false;
  int retries = 0;
  measurement result;

  while (valid_measurement == false and retries < max_retries) {
    Serial.print("starting measurement ");
    Serial.println(retries);
    result = sensor->measure();
    Serial.print("result:  ");
    Serial.println(result.value);
    retries++;
    if (result.value < upper_limit and result.value > lower_limit) {
      valid_measurement = result.valid;
    }
  };

  if (valid_measurement) {
    Serial.println("Valid measurement");
    return result;
  } else {
    result.valid = false;
    return faulty_measurement;
  };
}


SensorScaler::SensorScaler(Sensor* _sensor, float _scale, float _offset) :
  scale{_scale},
  offset{_offset}
  {set_sensor(_sensor);};

SensorScaler::~SensorScaler() {};

measurement SensorScaler::measure() {

  // implement measurement of a sensor. tries maximum max_retries
  // first value within limits is returned
  measurement result;
  result = sensor->measure();
  result.value = offset + scale * result.value;
  return result;
}


SensorInitializer::SensorInitializer(Sensor* _sensor)
  {set_sensor(_sensor);};

SensorInitializer::~SensorInitializer() {};

measurement SensorInitializer::measure() {

  if (sensor->init()) {
    return sensor->measure();
  } else {
    return faulty_measurement;
  };

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
    Serial.println("Failed to initialize distance sensor!");
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



measurement DistanceSensor::measure() {

  measurement result;
  int D;
  D = sensor->readRangeSingleMillimeters();
  if (sensor->timeoutOccurred()){
  Serial.println("Distance sensor TIMEOUT");
  return faulty_measurement;
  } else {
  Serial.print("distance: ");
  Serial.print(D);
  Serial.println(" mm");
  }
  result.value = (float) D;
  result.valid = true;
  return result;
}





VoltageSensor::VoltageSensor(int _pin) {
  pin = _pin;
}


bool VoltageSensor::init() {
  pinMode(pin, INPUT);
  return true;
}



measurement VoltageSensor::measure() {

  measurement result;
  int D;
  D = analogRead(pin);
  result.value = (float) D;
  result.valid = true;
  return result;
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
    Serial.println("BMP180 init fail");
    return false;
  }


}


measurement BMPSensor::measure() {

  char status;
  double T,P,p0,a;
  measurement result;

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
  result.value = (float) T;
  result.valid = true;
  return result;
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
  };
  return faulty_measurement;
}