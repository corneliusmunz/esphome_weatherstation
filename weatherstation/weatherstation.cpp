#include "weatherstation.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace weatherstation
  {

    static const char *const TAG = "weatherstation.sensor";

    static CC1101 radio = new Module(15, 5, 2, 4);

    // List of sensor IDs to be excluded - can be empty
    uint32_t const sensor_ids_exc[] = SENSOR_IDS_EXC;

    // List of sensor IDs to be included - if empty, handle all available sensors
    uint32_t const sensor_ids_inc[] = SENSOR_IDS_INC;

    void WeatherStationComponent::setup()
    {
      ESP_LOGCONFIG(TAG, "Setting up Weatherstation...");
      this->begin();

      // int state = radio.begin(868.3, 8.21, 57.136417, 270, 10, 32);
      // if (state == RADIOLIB_ERR_NONE)
      // {
      //   ESP_LOGD(TAG, "Radio successfully configured");
      // } else {
      //   ESP_LOGD(TAG, "Radio initialization failed");
      // }
      ESP_LOGCONFIG(TAG, "Finish Setting up Weatherstation...");
    }

    void WeatherStationComponent::dump_config()
    {
      ESP_LOGCONFIG(TAG, "WeatherStation Config:");
    }

    float WeatherStationComponent::get_setup_priority() const { return setup_priority::DATA; }

    void WeatherStationComponent::loop()
    {

      ESP_LOGD(TAG, "WeatherStation update");

      this->clearSlots();
      int decode_status = this->getMessage();
      //bool decode_ok = this->getData(80000, DATA_COMPLETE);

      if (decode_status == 0)
      {
        ESP_LOGD(TAG, "WeatherSensor successfully configured");
        this->temperature_sensor_->publish_state(sensor[0].temp_c);
        this->humidity_sensor_->publish_state(sensor[0].humidity);
        this->windspeed_sensor_->publish_state(sensor[0].wind_avg_meter_sec * 3.6);
        this->winddirection_sensor_->publish_state(sensor[0].wind_direction_deg);
        this->rain_sensor_->publish_state(sensor[0].rain_mm);
      }
      else
      {
        //ESP_LOGD(TAG, "WeatherSensor initialization failed with code %d", decode_status);
        ESP_LOGD(TAG, "WeatherSensor initialization failed");
      }

      // this->temperature_sensor_->publish_state(12.3);
      // this->humidity_sensor_->publish_state(45.6);
      // this->windspeed_sensor_->publish_state(7.1);
      // this->winddirection_sensor_->publish_state(10.7);

      // This example uses only a single slot in the sensor data array
      // int const i = 0;

      // // Clear all sensor data
      // this->clearSlots();

      // // Tries to receive radio message (non-blocking) and to decode it.
      // // Timeout occurs after a small multiple of expected time-on-air.
      // //int decode_status = this->getMessage();
      // bool decode_ok = this->getData(60000, DATA_COMPLETE);

      // //if (decode_status == 0)
      // if (decode_ok)
      // {
      //   ESP_LOGD(TAG, "WeatherSensor successfully configured");
      //   this->temperature_sensor_->publish_state(sensor[0].temp_c);
      //   this->humidity_sensor_->publish_state(sensor[0].humidity);
      // }
      // else
      // {
      //   //ESP_LOGD(TAG, "WeatherSensor initialization failed with code %d", decode_status);
      //   ESP_LOGD(TAG, "WeatherSensor initialization failed");

      // }

      // int state = radio.begin(868.3, 8.21, 57.136417, 270, 10, 32);
      // if (state == RADIOLIB_ERR_NONE)
      // {
      //   ESP_LOGD(TAG, "Radio successfully configured");
      // }
      // else
      // {
      //   ESP_LOGD(TAG, "Radio initialization failed");
      // }

      // Clear all sensor data
      //weatherSensor.clearSlots();

      // Tries to receive radio message (non-blocking) and to decode it.
      // Timeout occurs after a small multiple of expected time-on-air.
      // int decode_status = weatherSensor.getMessage();

      // if (decode_status == DECODE_OK)
      // {
      //   ESP_LOGD(TAG, "WeatherSensor successfully configured");
      // }
      // else
      // {
      //   ESP_LOGD(TAG, "WeatherSensor initialization failed");
      // }

      //this->temperature_sensor_->publish_state(12.3);
      //this->humidity_sensor_->publish_state(34.5);
    }

    int16_t WeatherStationComponent::begin(void)
    {
      // Freq: 868.300 MHz, Bandwidth: 203 KHz, rAmpl: 33 dB, sens: 8 dB, DataRate: 8207.32 Baud
      ESP_LOGD(TAG, "Initializing ...");
      // carrier frequency:                   868.3 MHz
      // bit rate:                            8.22 kbps
      // frequency deviation:                 57.136417 kHz
      // Rx bandwidth:                        270.0 kHz (CC1101) / 250 kHz (SX1276)
      // output power:                        10 dBm
      // preamble length:                     40 bits
      int state = radio.begin(868.3, 8.21, 57.136417, 270, 10, 32);

      if (state == RADIOLIB_ERR_NONE)
      {
        ESP_LOGD(TAG, "Success ...");
        state = radio.setCrcFiltering(false);
        if (state != RADIOLIB_ERR_NONE)
        {
          ESP_LOGE(TAG, "Error disabling crc filtering: [%d]", state);
          while (true)
            ;
        }
        state = radio.fixedPacketLengthMode(27);
        if (state != RADIOLIB_ERR_NONE)
        {
          ESP_LOGE(TAG, "Error setting fixed packet length: [%d]", state);
          while (true)
            ;
        }
        // Preamble: AA AA AA AA AA
        // Sync is: 2D D4
        // Preamble 40 bits but the CC1101 doesn't allow us to set that
        // so we use a preamble of 32 bits and then use the sync as AA 2D
        // which then uses the last byte of the preamble - we recieve the last sync byte
        // as the 1st byte of the payload.
        state = radio.setSyncWord(0xAA, 0x2D, 0, false);

        if (state != RADIOLIB_ERR_NONE)
        {
          ESP_LOGE(TAG, "Error setting sync words: [%d]", state);
          while (true)
            ;
        }
      }
      else
      {
        ESP_LOGE(TAG, "Error initialising: [%d]", state);
        while (true)
          ;
      }
      ESP_LOGD(TAG, "Setup complete - awaiting incoming messages...");
      rssi = radio.getRSSI();

      return state;
    }

    bool WeatherStationComponent::getData(uint32_t timeout, uint8_t flags, uint8_t type, void (*func)())
    {
      const uint32_t timestamp = millis();

      while ((millis() - timestamp) < timeout)
      {
        int decode_status = getMessage();

        // Callback function (see https://www.geeksforgeeks.org/callbacks-in-c/)
        if (func)
        {
          (*func)();
        }

        if (decode_status == DECODE_OK)
        {
          bool all_slots_valid = true;
          bool all_slots_complete = true;
          for (int i = 0; i < NUM_SENSORS; i++)
          {
            if (!sensor[i].valid)
            {
              all_slots_valid = false;
              continue;
            }

            // No special requirements, one valid message is sufficient
            if (flags == 0)
              return true;

            // Specific sensor type required
            if (((flags & DATA_TYPE) != 0) && (sensor[i].s_type == type))
            {
              if (sensor[i].complete || !(flags & DATA_COMPLETE))
              {
                return true;
              }
            }
            // All slots required (valid AND complete) - must check all slots
            else if (flags & DATA_ALL_SLOTS)
            {
              all_slots_valid &= sensor[i].valid;
              all_slots_complete &= sensor[i].complete;
            }
            // At least one sensor valid and complete
            else if (sensor[i].complete)
            {
              return true;
            }
          } // for (int i=0; i<NUM_SENSORS; i++)

          // All slots required (valid AND complete)
          if ((flags & DATA_ALL_SLOTS) && all_slots_valid && all_slots_complete)
          {
            return true;
          }

        } // if (decode_status == DECODE_OK)
      }   //  while ((millis() - timestamp) < timeout)

      // Timeout
      return false;
    }

    DecodeStatus WeatherStationComponent::getMessage(void)
    {
      uint8_t recvData[27];
      DecodeStatus decode_res = DECODE_INVALID;

      // Receive data
      //     1. flush RX buffer
      //     2. switch to RX mode
      //     3. wait for expected RX packet or timeout [~500us in this configuration]
      //     4. flush RX buffer
      //     5. switch to standby
      int state = radio.receive(recvData, 27);
      ESP_LOGD(TAG, "radio receive state %d", state);

      rssi = radio.getRSSI();
      ESP_LOGD(TAG, "radio receive rssi %f", rssi);

      if (state == RADIOLIB_ERR_NONE)
      {
        // Verify last syncword is 1st byte of payload (see setSyncWord() above)
        if (recvData[0] == 0xD4)
        {

          ESP_LOGD(TAG, "R [%02X] RSSI: %0.1f", recvData[0], rssi);
          decode_res = decodeBresser6In1Payload(&recvData[1], sizeof(recvData) - 1);

          if (decode_res == DECODE_INVALID ||
              decode_res == DECODE_PAR_ERR ||
              decode_res == DECODE_CHK_ERR ||
              decode_res == DECODE_DIG_ERR)
          {
            decode_res = decodeBresser5In1Payload(&recvData[1], sizeof(recvData) - 1);
          }
        } // if (recvData[0] == 0xD4)
        else if (state == RADIOLIB_ERR_RX_TIMEOUT)
        {
          ESP_LOGD(TAG, "T");
        } // if (state == RADIOLIB_ERR_RX_TIMEOUT)
        else
        {
          // some other error occurred
          ESP_LOGD(TAG, "Receive failed: [%d]", state);
        }
      } // if (state == RADIOLIB_ERR_NONE)

      return decode_res;
    }

    int WeatherStationComponent::findSlot(uint32_t id, DecodeStatus *status)
    {

      // log_v("find_slot(): ID=%08X", id);

      // Skip sensors from exclude-list (if any)
      uint8_t n_exc = sizeof(sensor_ids_exc) / 4;
      if (n_exc != 0)
      {
        for (int i = 0; i < n_exc; i++)
        {
          if (id == sensor_ids_exc[i])
          {
            // log_v("In Exclude-List, skipping!");
            *status = DECODE_SKIP;
            return -1;
          }
        }
      }

      // Handle sensors from include-list (if not empty)
      uint8_t n_inc = sizeof(sensor_ids_inc) / 4;
      if (n_inc != 0)
      {
        bool found = false;
        for (int i = 0; i < n_inc; i++)
        {
          if (id == sensor_ids_inc[i])
          {
            found = true;
            break;
          }
        }
        if (!found)
        {
          // log_v("Not in Include-List, skipping!");
          *status = DECODE_SKIP;
          return -1;
        }
      }

      // Search all slots
      int free_slot = -1;
      int update_slot = -1;
      for (int i = 0; i < NUM_SENSORS; i++)
      {
        // Save first free slot
        if (!sensor[i].valid && (free_slot < 0))
        {
          free_slot = i;
        }

        // Check if sensor has already been stored
        else if (sensor[i].valid && (sensor[i].sensor_id == id))
        {
          update_slot = i;
        }
      }

      if (update_slot > -1)
      {
        // Update slot
        // log_v("find_slot(): Updating slot #%d", update_slot);
        *status = DECODE_OK;
        return update_slot;
      }
      else if (free_slot > -1)
      {
        // Store to free slot
        // log_v("find_slot(): Storing into slot #%d", free_slot);
        *status = DECODE_OK;
        return free_slot;
      }
      else
      {
        // log_v("find_slot(): No slot left");
        // No slot left
        *status = DECODE_FULL;
        return -1;
      }
    }

    //
    // Find required sensor data by ID
    //
    int WeatherStationComponent::findId(uint32_t id)
    {
      for (int i = 0; i < NUM_SENSORS; i++)
      {
        if (sensor[i].valid && (sensor[i].sensor_id == id))
          return i;
      }
      return -1;
    }

    //
    // Find required sensor data by type and (optionally) channel
    //
    int WeatherStationComponent::findType(uint8_t type, uint8_t ch)
    {
      for (int i = 0; i < NUM_SENSORS; i++)
      {
        if (sensor[i].valid && (sensor[i].s_type == type) &&
            ((ch == 0xFF) || (sensor[i].chan = ch)))
          return i;
      }
      return -1;
    }

    //
    // From from rtl_433 project - https://github.com/merbanan/rtl_433/blob/master/src/util.c
    //
    uint16_t WeatherStationComponent::lfsr_digest16(uint8_t const message[], unsigned bytes, uint16_t gen, uint16_t key)
    {
      uint16_t sum = 0;
      for (unsigned k = 0; k < bytes; ++k)
      {
        uint8_t data = message[k];
        for (int i = 7; i >= 0; --i)
        {
          // fprintf(stderr, "key at bit %d : %04x\n", i, key);
          // if data bit is set then xor with key
          if ((data >> i) & 1)
            sum ^= key;

          // roll the key right (actually the lsb is dropped here)
          // and apply the gen (needs to include the dropped lsb as msb)
          if (key & 1)
            key = (key >> 1) ^ gen;
          else
            key = (key >> 1);
        }
      }
      return sum;
    }

    //
    // From from rtl_433 project - https://github.com/merbanan/rtl_433/blob/master/src/util.c
    //
    int WeatherStationComponent::add_bytes(uint8_t const message[], unsigned num_bytes)
    {
      int result = 0;
      for (unsigned i = 0; i < num_bytes; ++i)
      {
        result += message[i];
      }
      return result;
    }

    DecodeStatus WeatherStationComponent::decodeBresser5In1Payload(uint8_t *msg, uint8_t msgSize)
    {
      // First 13 bytes need to match inverse of last 13 bytes
      for (unsigned col = 0; col < msgSize / 2; ++col)
      {
        if ((msg[col] ^ msg[col + 13]) != 0xff)
        {
          // log_d("Parity wrong at column %d", col);
          return DECODE_PAR_ERR;
        }
      }

      // Verify checksum (number number bits set in bytes 14-25)
      uint8_t bitsSet = 0;
      uint8_t expectedBitsSet = msg[13];

      for (uint8_t p = 14; p < msgSize; p++)
      {
        uint8_t currentByte = msg[p];
        while (currentByte)
        {
          bitsSet += (currentByte & 1);
          currentByte >>= 1;
        }
      }

      if (bitsSet != expectedBitsSet)
      {
        // log_d("Checksum wrong - actual [%02X] != [%02X]", bitsSet, expectedBitsSet);
        return DECODE_CHK_ERR;
      }

      uint8_t id_tmp = msg[14];
      uint8_t type_tmp = msg[15] & 0xF;
      DecodeStatus status;

      // Find appropriate slot in sensor data array and update <status>
      int slot = findSlot(id_tmp, &status);

      if (status != DECODE_OK)
        return status;

      sensor[slot].sensor_id = id_tmp;
      sensor[slot].s_type = type_tmp;

      int temp_raw = (msg[20] & 0x0f) + ((msg[20] & 0xf0) >> 4) * 10 + (msg[21] & 0x0f) * 100;
      if (msg[25] & 0x0f)
      {
        temp_raw = -temp_raw;
      }
      sensor[slot].temp_c = temp_raw * 0.1f;

      sensor[slot].humidity = (msg[22] & 0x0f) + ((msg[22] & 0xf0) >> 4) * 10;

      int wind_direction_raw = ((msg[17] & 0xf0) >> 4) * 225;
      int gust_raw = ((msg[17] & 0x0f) << 8) + msg[16];
      int wind_raw = (msg[18] & 0x0f) + ((msg[18] & 0xf0) >> 4) * 10 + (msg[19] & 0x0f) * 100;

      sensor[slot].wind_direction_deg = wind_direction_raw * 0.1f;
      sensor[slot].wind_gust_meter_sec = gust_raw * 0.1f;
      sensor[slot].wind_avg_meter_sec = wind_raw * 0.1f;

      int rain_raw = (msg[23] & 0x0f) + ((msg[23] & 0xf0) >> 4) * 10 + (msg[24] & 0x0f) * 100 + ((msg[24] & 0xf0) >> 4) * 1000;
      sensor[slot].rain_mm = rain_raw * 0.1f;

      sensor[slot].battery_ok = (msg[25] & 0x80) ? false : true;

      /* check if the message is from a Bresser Professional Rain Gauge */
      if ((msg[15] & 0xF) == 0x9)
      {
        // rescale the rain sensor readings
        sensor[slot].rain_mm *= 2.5;

        // Rain Gauge has no humidity (according to description) and no wind sensor (obviously)
        sensor[slot].humidity_ok = false;
        sensor[slot].wind_ok = false;
      }
      else
      {
        sensor[slot].humidity_ok = true;
        sensor[slot].wind_ok = true;
      }

      sensor[slot].temp_ok = true;
      sensor[slot].uv_ok = false;
      sensor[slot].rain_ok = true;
      sensor[slot].moisture_ok = false;
      sensor[slot].valid = true;
      sensor[slot].complete = true;

      // Save rssi to sensor specific data set
      sensor[slot].rssi = rssi;

      return DECODE_OK;
    }

    DecodeStatus WeatherStationComponent::decodeBresser6In1Payload(uint8_t *msg, uint8_t msgSize)
    {
      (void)msgSize;                                                                             // unused parameter - kept for consistency with other decoders; avoid warning
      int const moisture_map[] = {0, 7, 13, 20, 27, 33, 40, 47, 53, 60, 67, 73, 80, 87, 93, 99}; // scale is 20/3

      // Per-message status flags
      bool temp_ok = false;
      bool humidity_ok = false;
      bool uv_ok = false;
      bool wind_ok = false;
      bool rain_ok = false;
      bool moisture_ok = false;

      // LFSR-16 digest, generator 0x8810 init 0x5412
      int chkdgst = (msg[0] << 8) | msg[1];
      int digest = lfsr_digest16(&msg[2], 15, 0x8810, 0x5412);
      if (chkdgst != digest)
      {
        // log_d("Digest check failed - [%02X] != [%02X]", chkdgst, digest);
        return DECODE_DIG_ERR;
      }
      // Checksum, add with carry
      int sum = add_bytes(&msg[2], 16); // msg[2] to msg[17]
      if ((sum & 0xff) != 0xff)
      {
        // log_d("Checksum failed");
        return DECODE_CHK_ERR;
      }

      uint32_t id_tmp = ((uint32_t)msg[2] << 24) | (msg[3] << 16) | (msg[4] << 8) | (msg[5]);
      uint8_t type_tmp = (msg[6] >> 4); // 1: weather station, 2: indoor?, 4: soil probe
      uint8_t chan_tmp = (msg[6] & 0x7);
      uint8_t flags = (msg[16] & 0x0f);
      DecodeStatus status;

      // Find appropriate slot in sensor data array and update <status>
      int slot = findSlot(id_tmp, &status);

      if (status != DECODE_OK)
        return status;

      // unused...
      //int startup = (msg[6] >> 3) & 1; // s.a. #1214

      sensor[slot].sensor_id = id_tmp;
      sensor[slot].s_type = type_tmp;
      sensor[slot].chan = chan_tmp;

      // temperature, humidity(, uv) - shared with rain counter
      temp_ok = humidity_ok = (flags == 0);
      if (temp_ok)
      {
        bool sign = (msg[13] >> 3) & 1;
        int temp_raw = (msg[12] >> 4) * 100 + (msg[12] & 0x0f) * 10 + (msg[13] >> 4);
        float temp = ((sign) ? (temp_raw - 1000) : temp_raw) * 0.1f;

        sensor[slot].temp_c = temp;
        sensor[slot].battery_ok = (msg[13] >> 1) & 1; // b[13] & 0x02 is battery_good, s.a. #1993
        sensor[slot].humidity = (msg[14] >> 4) * 10 + (msg[14] & 0x0f);

        // apparently ff01 or 0000 if not available, ???0 if valid, inverted BCD
        uv_ok = (~msg[15] & 0xff) <= 0x99 && (~msg[16] & 0xf0) <= 0x90;
        if (uv_ok)
        {
          int uv_raw = ((~msg[15] & 0xf0) >> 4) * 100 + (~msg[15] & 0x0f) * 10 + ((~msg[16] & 0xf0) >> 4);
          sensor[slot].uv = uv_raw * 0.1f;
        }
      }

      //int unk_ok  = (msg[16] & 0xf0) == 0xf0;
      //int unk_raw = ((msg[15] & 0xf0) >> 4) * 10 + (msg[15] & 0x0f);

      // invert 3 bytes wind speeds
      msg[7] ^= 0xff;
      msg[8] ^= 0xff;
      msg[9] ^= 0xff;
      wind_ok = (msg[7] <= 0x99) && (msg[8] <= 0x99) && (msg[9] <= 0x99);
      if (wind_ok)
      {
        int gust_raw = (msg[7] >> 4) * 100 + (msg[7] & 0x0f) * 10 + (msg[8] >> 4);
        int wavg_raw = (msg[9] >> 4) * 100 + (msg[9] & 0x0f) * 10 + (msg[8] & 0x0f);
        int wind_dir_raw = ((msg[10] & 0xf0) >> 4) * 100 + (msg[10] & 0x0f) * 10 + ((msg[11] & 0xf0) >> 4);

        sensor[slot].wind_gust_meter_sec = gust_raw * 0.1f;
        sensor[slot].wind_avg_meter_sec = wavg_raw * 0.1f;
        sensor[slot].wind_direction_deg = wind_dir_raw * 1.0f;
      }

      // rain counter, inverted 3 bytes BCD - shared with temp/hum
      msg[12] ^= 0xff;
      msg[13] ^= 0xff;
      msg[14] ^= 0xff;

      rain_ok = (flags == 1) && (type_tmp == 1);
      if (rain_ok)
      {
        int rain_raw = (msg[12] >> 4) * 100000 + (msg[12] & 0x0f) * 10000 + (msg[13] >> 4) * 1000 + (msg[13] & 0x0f) * 100 + (msg[14] >> 4) * 10 + (msg[14] & 0x0f);
        sensor[slot].rain_mm = rain_raw * 0.1f;
      }

      moisture_ok = false;

      // the moisture sensor might present valid readings but does not have the hardware
      if (sensor[slot].s_type == 4)
      {
        wind_ok = 0;
        uv_ok = 0;
      }

      if (sensor[slot].s_type == 4 && temp_ok && sensor[slot].humidity >= 1 && sensor[slot].humidity <= 16)
      {
        moisture_ok = true;
        humidity_ok = false;
        sensor[slot].moisture = moisture_map[sensor[slot].humidity - 1];
      }

      // Update per-slot status flags
      sensor[slot].temp_ok |= temp_ok;
      sensor[slot].humidity_ok |= humidity_ok;
      sensor[slot].uv_ok |= uv_ok;
      sensor[slot].wind_ok |= wind_ok;
      sensor[slot].rain_ok |= rain_ok;

      sensor[slot].moisture_ok |= moisture_ok;

      ESP_LOGD(TAG, "Temp: %d  Hum: %d  UV: %d  Wind: %d  Rain: %d  Moist: %d", temp_ok, humidity_ok, uv_ok, wind_ok, rain_ok, moisture_ok);
      ESP_LOGD(TAG, "Temp: %f  Hum: %f Wind: %f", sensor[slot].temp_c, sensor[slot].humidity, sensor[slot].wind_gust_meter_sec);

      sensor[slot].valid = true;

      // Weather station data is split into two separate messages
      sensor[slot].complete = ((sensor[slot].s_type == SENSOR_TYPE_WEATHER1) && sensor[slot].temp_ok && sensor[slot].rain_ok) || (sensor[slot].s_type != SENSOR_TYPE_WEATHER1);

      // Save rssi to sensor specific data set
      sensor[slot].rssi = rssi;

      return DECODE_OK;
    }

  } // namespace weatherstation
} // namespace esphome
