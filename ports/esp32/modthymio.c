/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 "Eric Poulsen" <eric@zyxod.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>
#include "soc/rtc_cntl_reg.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_heap_caps.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/timeutils/timeutils.h"
#include "modmachine.h"
#include "modthymio.h"
#include "thymio_leds_circle.h"
#include "thymio_proximity.h"
#include "thymio_ground.h"
#include "thymio_motors.h"
#include "thymio_sound.h"
#include "thymio_leds_lego_front.h"
#include "thymio_leds_lego_back.h"
#include "thymio_leds_rgb.h"
#include "thymio_rc5.h"
#include "thymio_imu.h"
#include "thymio_color_sensor.h"
#include "thymio_buttons.h"
#include "thymio_behaviors.h"
#include "thymio_leds_buttons.h"
#include "thymio_led_mic.h"
#include "thymio_i2c.h"
#include "../../../../../main/mode.h"
#include "../../../../../main/utility.h"
#include "../../../../../main/stm32_spi.h"

char api_version[6] = "XX.XX\0";

// Turn off all LEDs and stop the motors.
STATIC mp_obj_t turn_off_all(void) {
    turnOffAllSensors();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(turn_off_all_obj, turn_off_all);

// Get current battery voltage in millivolts.
STATIC mp_obj_t get_battery_voltage(void) {
    return mp_obj_new_int(STM32_GetBatteryVoltage());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_battery_voltage_obj, get_battery_voltage);

// Get the sum of left and right motors measured voltage (adc value).
STATIC mp_obj_t get_motors_voltage(void) {
    return mp_obj_new_int(STM32_GetBatteryMotorVoltage());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_motors_voltage_obj, get_motors_voltage);

// Get current Thymio 3 Micropython API version.
STATIC mp_obj_t get_api_version(void) {
    snprintf(api_version, 6, "%2d.%-2d", THYMIO_API_MAJOR_VERSION, THYMIO_API_MINOR_VERSION);
    return mp_obj_new_str(api_version, strlen(api_version));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_api_version_obj, get_api_version);

STATIC const mp_rom_map_elem_t thymio_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_thymio) },

    { MP_ROM_QSTR(MP_QSTR_turn_off_all), MP_ROM_PTR(&turn_off_all_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_battery_voltage), MP_ROM_PTR(&get_battery_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_motors_voltage), MP_ROM_PTR(&get_motors_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_api_version), MP_ROM_PTR(&get_api_version_obj) },

    { MP_ROM_QSTR(MP_QSTR_LEDS_CIRCLE), MP_ROM_PTR(&thymio_leds_circle_type) },
    { MP_ROM_QSTR(MP_QSTR_PROXIMITY), MP_ROM_PTR(&thymio_proximity_type) },
    { MP_ROM_QSTR(MP_QSTR_GROUND), MP_ROM_PTR(&thymio_ground_type) },
    { MP_ROM_QSTR(MP_QSTR_MOTORS), MP_ROM_PTR(&thymio_motors_type) },
    { MP_ROM_QSTR(MP_QSTR_SOUND), MP_ROM_PTR(&thymio_sound_type) },
    { MP_ROM_QSTR(MP_QSTR_LEDS_LEGO_FRONT), MP_ROM_PTR(&thymio_leds_lego_front_type) },
    { MP_ROM_QSTR(MP_QSTR_LEDS_LEGO_BACK), MP_ROM_PTR(&thymio_leds_lego_back_type) },
    { MP_ROM_QSTR(MP_QSTR_LEDS_RGB), MP_ROM_PTR(&thymio_leds_rgb_type) },
    { MP_ROM_QSTR(MP_QSTR_RC5), MP_ROM_PTR(&thymio_rc5_type) },
    { MP_ROM_QSTR(MP_QSTR_IMU), MP_ROM_PTR(&thymio_imu_type) },
    { MP_ROM_QSTR(MP_QSTR_COLOR_SENSOR), MP_ROM_PTR(&thymio_color_sensor_type) },
    { MP_ROM_QSTR(MP_QSTR_BUTTONS), MP_ROM_PTR(&thymio_buttons_type) },
    { MP_ROM_QSTR(MP_QSTR_BEHAVIORS), MP_ROM_PTR(&thymio_behaviors_type) },
    { MP_ROM_QSTR(MP_QSTR_LEDS_BUTTONS), MP_ROM_PTR(&thymio_leds_buttons_type) },
    { MP_ROM_QSTR(MP_QSTR_LED_MICROPHONE), MP_ROM_PTR(&thymio_led_microphone_type) },
    { MP_ROM_QSTR(MP_QSTR_I2C), MP_ROM_PTR(&thymio_i2c_type) },
};

STATIC MP_DEFINE_CONST_DICT(tyhmio_module_globals, thymio_module_globals_table);

const mp_obj_module_t thymio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&tyhmio_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_thymio, thymio_module);
