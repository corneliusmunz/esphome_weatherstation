#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
//#include "Arduino.h"
//#include "string"
#include "RadioLib.h"

// Sensor Types
// 0 - Weather Station          5-in-1; PN 7002510..12/7902510..12
// 1 - Weather Station          6-in-1; PN 7002585
// 2 - Thermo-/Hygro-Sensor     6-in-1; PN 7009999
// 4 - Soil Moisture Sensor     6-in-1; PN 7009972
// 9 - Professional Rain Gauge  (5-in-1 decoder)
// ? - Air Quality Sensor
// ? - Water Leakage Sensor
// ? - Pool Thermometer
// ? - Lightning Sensor

#define SENSOR_TYPE_WEATHER0 0     // Weather Station
#define SENSOR_TYPE_WEATHER1 1     // Weather Station
#define SENSOR_TYPE_THERMO_HYGRO 2 // Thermo-/Hygro-Sensor
#define SENSOR_TYPE_SOIL 4         // Soil Temperature and Moisture (from 6-in-1 decoder)
#define SENSOR_TYPE_RAIN 9         // Professional Rain Gauge (from 5-in-1 decoder)

// Sensor specific rain gauge overflow threshold (mm)
#define WEATHER0_RAIN_OV 1000
#define WEATHER1_RAIN_OV 100000

// Flags for controlling completion of reception in getData()
#define DATA_COMPLETE 0x1  // only completed slots (as opposed to partially filled)
#define DATA_TYPE 0x2      // at least one slot with specific sensor type
#define DATA_ALL_SLOTS 0x8 // all slots completed

#define NUM_SENSORS 1

#define USE_CC1101

// List of sensor IDs to be excluded - can be empty
#define SENSOR_IDS_EXC \
  {                    \
  }
//#define SENSOR_IDS_EXC { 0x39582376 }

// List of sensor IDs to be included - if empty, handle all available sensors
#define SENSOR_IDS_INC \
  {                    \
  }
//#define SENSOR_IDS_INC { 0x83750871 }

#if defined(ESP8266)
#define ARDUHAL_LOG_LEVEL_NONE 0
#define ARDUHAL_LOG_LEVEL_ERROR 1
#define ARDUHAL_LOG_LEVEL_WARN 2
#define ARDUHAL_LOG_LEVEL_INFO 3
#define ARDUHAL_LOG_LEVEL_DEBUG 4
#define ARDUHAL_LOG_LEVEL_VERBOSE 5

// Set desired level here!
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE

#if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_NONE
#define log_e(...)                      \
  {                                     \
    DEBUG_ESP_PORT.printf(__VA_ARGS__); \
    DEBUG_ESP_PORT.println();           \
  }
#else
#define log_e(...) \
  {                \
  }
#endif
#if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_ERROR
#define log_w(...)                      \
  {                                     \
    DEBUG_ESP_PORT.printf(__VA_ARGS__); \
    DEBUG_ESP_PORT.println();           \
  }
#else
#define log_w(...) \
  {                \
  }
#endif
#if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_WARN
#define log_i(...)                      \
  {                                     \
    DEBUG_ESP_PORT.printf(__VA_ARGS__); \
    DEBUG_ESP_PORT.println();           \
  }
#else
#define log_i(...) \
  {                \
  }
#endif
#if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_INFO
#define log_d(...)                      \
  {                                     \
    DEBUG_ESP_PORT.printf(__VA_ARGS__); \
    DEBUG_ESP_PORT.println();           \
  }
#else
#define log_d(...) \
  {                \
  }
#endif
#if defined(DEBUG_ESP_PORT) && CORE_DEBUG_LEVEL > ARDUHAL_LOG_LEVEL_DEBUG
#define log_v(...)                      \
  {                                     \
    DEBUG_ESP_PORT.printf(__VA_ARGS__); \
    DEBUG_ESP_PORT.println();           \
  }
#else
#define log_v(...) \
  {                \
  }
#endif

#endif

#define RECEIVER_CHIP "[CC1101]"

namespace esphome
{
  namespace weatherstation
  {

    // Radio message decoding status
    typedef enum DecodeStatus
    {
      DECODE_INVALID,
      DECODE_OK,
      DECODE_PAR_ERR,
      DECODE_CHK_ERR,
      DECODE_DIG_ERR,
      DECODE_SKIP,
      DECODE_FULL
    } DecodeStatus;

    typedef struct SensorMap
    {
      uint32_t id;      //!< ID if sensor (as transmitted in radio message)
      std::string name; //!< Name of sensor (e.g. for MQTT topic)
    } SensorMap;

    /// This class implements support for the BME280 Temperature+Pressure+Humidity i2c sensor.
    class WeatherStationComponent : public PollingComponent
    {
    public:
      // ========== INTERNAL METHODS ==========
      // (In most use cases you won't need these)
      void setup() override;
      void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
      void set_humidity_sensor(sensor::Sensor *humidity_sensor) { humidity_sensor_ = humidity_sensor; }
      void set_wind_speed_sensor(sensor::Sensor *windspeed_sensor) { windspeed_sensor_ = windspeed_sensor; }
      void set_wind_direction_sensor(sensor::Sensor *winddirection_sensor) { winddirection_sensor_ = winddirection_sensor; }
      void set_rain_sensor(sensor::Sensor *rain_sensor) { rain_sensor_ = rain_sensor; }

      float get_setup_priority() const override;
      void dump_config() override;
      void update() override;

      struct Sensor
      {
        uint32_t sensor_id;        //!< sensor ID (5-in-1: 1 byte / 6-in-1: 4 bytes)
        uint8_t s_type;            //!< sensor type (only 6-in1)
        uint8_t chan;              //!< channel (only 6-in-1)
        bool valid;                //!< data valid (but not necessarily complete)
        bool complete;             //!< data is split into two separate messages is complete (only 6-in-1 WS)
        bool temp_ok = false;      //!< temperature o.k. (only 6-in-1)
        bool humidity_ok = false;  //!< humidity o.k.
        bool uv_ok = false;        //!< uv radiation o.k. (only 6-in-1)
        bool wind_ok = false;      //!< wind speed/direction o.k. (only 6-in-1)
        bool rain_ok = false;      //!< rain gauge level o.k.
        bool battery_ok = false;   //!< battery o.k.
        bool moisture_ok = false;  //!< moisture o.k. (only 6-in-1)
        float temp_c;              //!< temperature in degC
        float uv;                  //!< uv radiation (only 6-in-1)
        float rain_mm;             //!< rain gauge level in mm
        float wind_direction_deg;  //!< wind direction in deg
        float wind_gust_meter_sec; //!< wind speed (gusts) in m/s
        float wind_avg_meter_sec;  //!< wind speed (avg)   in m/s
        uint8_t humidity;          //!< humidity in %
        uint8_t moisture;          //!< moisture in % (only 6-in-1)
        float rssi;                //!< received signal strength indicator in dBm
      };

      typedef struct Sensor sensor_t; //!< Shortcut for struct Sensor
      sensor_t sensor[NUM_SENSORS];   //!< sensor data array
      float rssi;                     //!< received signal strength indicator in dBm

    protected:
      sensor::Sensor *temperature_sensor_{nullptr};
      sensor::Sensor *humidity_sensor_{nullptr};
      sensor::Sensor *windspeed_sensor_{nullptr};
      sensor::Sensor *winddirection_sensor_{nullptr};
      sensor::Sensor *rain_sensor_{nullptr};

    private:
      int16_t begin(void);
      bool getData(uint32_t timeout, uint8_t flags = 0, uint8_t type = 0, void (*func)() = NULL);
      DecodeStatus getMessage(void);
      void clearSlots(uint8_t type = 0xFF)
      {
        for (int i = 0; i < NUM_SENSORS; i++)
        {
          if ((type == 0xFF) || (sensor[i].s_type == type))
          {
            sensor[i].valid = false;
            sensor[i].complete = false;
            sensor[i].temp_ok = false;
            sensor[i].humidity_ok = false;
            sensor[i].uv_ok = false;
            sensor[i].wind_ok = false;
            sensor[i].rain_ok = false;
            sensor[i].moisture_ok = false;
          }
        }
      };

      int findId(uint32_t id);
      int findType(uint8_t type, uint8_t channel = 0xFF);
      struct Sensor *pData;
      int findSlot(uint32_t id, DecodeStatus *status);
      DecodeStatus decodeBresser5In1Payload(uint8_t *msg, uint8_t msgSize);
      DecodeStatus decodeBresser6In1Payload(uint8_t *msg, uint8_t msgSize);
      uint16_t lfsr_digest16(uint8_t const message[], unsigned bytes, uint16_t gen, uint16_t key);
      int add_bytes(uint8_t const message[], unsigned num_bytes);
    };

  } // namespace weatherstation
} // namespace esphome
