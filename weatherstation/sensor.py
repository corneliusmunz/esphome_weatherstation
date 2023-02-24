import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_HUMIDITY,
    CONF_ID,
    CONF_TEMPERATURE,
    CONF_WIND_SPEED,
    CONF_WIND_DIRECTION_DEGREES,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_KILOMETER_PER_HOUR,
    ICON_SIGN_DIRECTION,
    ICON_WEATHER_WINDY,
    UNIT_DEGREES
)

DEPENDENCIES = []

weatherstation_ns = cg.esphome_ns.namespace("weatherstation")

WeatherStationComponent = weatherstation_ns.class_(
    "WeatherStationComponent", cg.PollingComponent
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(WeatherStationComponent),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_WIND_SPEED): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOMETER_PER_HOUR,
                icon=ICON_WEATHER_WINDY,
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_WIND_DIRECTION_DEGREES): sensor.sensor_schema(
                unit_of_measurement=UNIT_DEGREES,
                icon=ICON_SIGN_DIRECTION,
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT
            ),
            cv.Optional("rain"): sensor.sensor_schema(
                unit_of_measurement="mm",
                accuracy_decimals=1,
                state_class=STATE_CLASS_MEASUREMENT
            )
        }
    ).extend(cv.polling_component_schema("120s"))
    #.extend(cv.polling_component_schema("10s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add_library("SPI", None)
    cg.add_library("RadioLib", "5.6.0")


    if CONF_TEMPERATURE in config:
        conf = config[CONF_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_temperature_sensor(sens))


    if CONF_HUMIDITY in config:
        conf = config[CONF_HUMIDITY]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_humidity_sensor(sens))

    if CONF_WIND_SPEED in config:
        conf = config[CONF_WIND_SPEED]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_wind_speed_sensor(sens))

    if CONF_WIND_DIRECTION_DEGREES in config:
        conf = config[CONF_WIND_DIRECTION_DEGREES]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_wind_direction_sensor(sens))

    if "rain" in config:
        conf = config["rain"]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_rain_sensor(sens))
