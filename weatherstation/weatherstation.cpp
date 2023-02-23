#include "weatherstation.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include "WeatherSensorCfg.h"
#include "WeatherSensor.h"

namespace esphome {
namespace weatherstation {

static const char *const TAG = "weatherstation.sensor";

static CC1101 radio = new Module(15, 5, 2, 4);

void WeatherStationComponent::setup()
{
  ESP_LOGCONFIG(TAG, "Setting up Weatherstation...");
  int state = radio.begin(868.3, 8.21, 57.136417, 270, 10, 32);
  if (state == RADIOLIB_ERR_NONE)
  {
    ESP_LOGD(TAG, "Radio successfully configured");
  } else {
    ESP_LOGD(TAG, "Radio initialization failed");
  }
}

  void WeatherStationComponent::dump_config()
  {
    ESP_LOGCONFIG(TAG, "WeatherStation Config:");
  }

  float WeatherStationComponent::get_setup_priority() const { return setup_priority::DATA; }

  void WeatherStationComponent::update()
  {
    ESP_LOGD(TAG, "WeatherStation update");
    int state = radio.begin(868.3, 8.21, 57.136417, 270, 10, 32);
    if (state == RADIOLIB_ERR_NONE)
    {
      ESP_LOGD(TAG, "Radio successfully configured");
    }
    else
    {
      ESP_LOGD(TAG, "Radio initialization failed");
    }
    this->temperature_sensor_->publish_state(12.3);
    this->humidity_sensor_->publish_state(34.5);
  }

}  // namespace weatherstation
}  // namespace esphome
