#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "RadioLib.h"

namespace esphome {
  namespace weatherstation
  {

    /// This class implements support for the BME280 Temperature+Pressure+Humidity i2c sensor.
    class WeatherStationComponent : public PollingComponent
    {
    public:
      // ========== INTERNAL METHODS ==========
      // (In most use cases you won't need these)
      void setup() override;
      void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
      void set_humidity_sensor(sensor::Sensor *humidity_sensor) { humidity_sensor_ = humidity_sensor; }

      float get_setup_priority() const override;
      void dump_config() override;
      void update() override;

    protected:
      sensor::Sensor *temperature_sensor_{nullptr};
      sensor::Sensor *humidity_sensor_{nullptr};
    };

  } // namespace weatherstation
}  // namespace esphome
