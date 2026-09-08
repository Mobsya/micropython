/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2016 Damien P. George
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

#include "py/runtime.h"
#include "py/mphal.h"
#include "thymio_color_sensor.h"
#include "../../../../../main/leds.h"

T_HSV hsv_temp;
T_RawColor raw_temp;

/// \moduleref thymio
/// \class COLOR_SENSOR - COLOR_SENSOR object
///
/// The Thymio 3 robot is equipped with a color sensor on its bottom. This sensor detects colors when in direct contact with a surface.

typedef struct _thymio_color_sensor_obj_t {
    mp_obj_base_t base;
} thymio_color_sensor_obj_t;

void color_sensor_init(void) {
}

T_HSV color_sensor_get_hsv(void) {
    return ColorSensor_GetHsv();
}

T_RawColor color_sensor_get_raw(void) {
    return ColorSensor_GetRaw();
}

T_RawColor color_sensor_get_calib_white(void) {
    return ColorSensor_GetWhiteCalibration();
}

T_RawColor color_sensor_get_calib_black(void) {
    return ColorSensor_GetBlackCalibration();
}


/******************************************************************************/
/* MicroPython bindings                                                       */

void color_sensor_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "COLOR_SENSOR");
}

/// \classmethod \constructor()
/// Create a COLOR SENSOR object:
/// \example Create a COLOR SENSOR object
///     import thymio
///     color = thymio.COLOR_SENSOR()
/// \endexample
STATIC mp_obj_t color_sensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    thymio_color_sensor_obj_t *color_sensor = m_new_obj(thymio_color_sensor_obj_t);
    color_sensor->base.type = &thymio_color_sensor_type;
    return MP_OBJ_FROM_PTR(color_sensor);
}

/// \method get_hsv()
/// Get the HSV values detected by the color sensor. Hue range is [0..360], saturation range is [0..100], value range is [0..100].
/// \example Print HSV values detected by the color sensor:
///     hue, saturation, value = color.get_hsv()
///     print(f"Hue: {hue}, Saturation: {saturation}, Value: {value}") 
/// \endexample
mp_obj_t color_sensor_get_hsv_values(mp_obj_t self_in) {
    mp_obj_t items[3];
    hsv_temp = color_sensor_get_hsv();
    items[0] = mp_obj_new_int(hsv_temp.Hue);
    items[1] = mp_obj_new_int(hsv_temp.Saturation);
    items[2] = mp_obj_new_int(hsv_temp.Value);
    return mp_obj_new_list(3, items);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(color_sensor_get_hsv_values_obj, color_sensor_get_hsv_values);

/// \method get_raw()
/// Get color sensor raw values. The returned list corresponds to [red, green, blue, clear].
/// \example Print raw values detected by the color sensor:
///     red, green, blue, clear = color.get_raw()
///     print(f"Red: {red}, Green: {green}, Blue: {blue}, Clear: {clear}") 
/// \endexample
mp_obj_t color_sensor_get_raw_values(mp_obj_t self_in) {
    mp_obj_t items[4];
    raw_temp = color_sensor_get_raw();
    items[0] = mp_obj_new_int(raw_temp.Red);
    items[1] = mp_obj_new_int(raw_temp.Green);
    items[2] = mp_obj_new_int(raw_temp.Blue);
    items[3] = mp_obj_new_int(raw_temp.Clear);
    return mp_obj_new_list(4, items);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(color_sensor_get_raw_values_obj, color_sensor_get_raw_values);

/// \method get_calibration()
/// Get calibration values (calibration done in both white and black surfaces). The returned list corresponds to [red white, green white, blue white, red black, green black, blue black].
/// \example Print calibration values of the color sensor:
///     red_white, green_white, blue_white, red_black, green_black, blue_black = color.get_calibration()
///     print(f"Red White: {red_white}, Green White: {green_white}, Blue White: {blue_white}, Red Black: {red_black}, Green Black: {green_black}, Blue Black: {blue_black}") 
/// \endexample
mp_obj_t color_sensor_get_calibration(mp_obj_t self_in) {
    T_RawColor white = color_sensor_get_calib_white();
    T_RawColor black = color_sensor_get_calib_black();
    mp_obj_t items[6];
    items[0] = mp_obj_new_int(white.Red);
    items[1] = mp_obj_new_int(white.Green);
    items[2] = mp_obj_new_int(white.Blue);
    items[3] = mp_obj_new_int(black.Red);
    items[4] = mp_obj_new_int(black.Green);
    items[5] = mp_obj_new_int(black.Blue);
    return mp_obj_new_list(6, items);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(color_sensor_get_calibration_obj, color_sensor_get_calibration);

/// \method calibrate_and_save_white()
/// Calibrate on white surface and save calibration values to flash.
/// \example Calibrate the color sensor on a white surface and save the calibration values to flash:
///     color.calibrate_and_save_white()
/// \endexample
mp_obj_t color_sensor_calibrate_and_save_white(mp_obj_t self_in) {
    ColorSensor_CalibrateWhite();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(color_sensor_calibrate_and_save_white_obj, color_sensor_calibrate_and_save_white);

/// \method calibrate_and_save_black()
/// Calibrate on black surface and save calibration values to flash.
/// \example Calibrate the color sensor on a black surface and save the calibration values to flash:
///     color.calibrate_and_save_black()
/// \endexample
mp_obj_t color_sensor_calibrate_and_save_black(mp_obj_t self_in) {
    ColorSensor_CalibrateBlack();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(color_sensor_calibrate_and_save_black_obj, color_sensor_calibrate_and_save_black);

/// \method led_on()
/// Turn the color sensor LED on at maximum brightness. Beware that the color sensor need the LED to be on to detect colors.
mp_obj_t led_color_obj_on(mp_obj_t self_in) {
    Leds_SetSingleBrightness(E_Led_White_Sensor, MAX_BRIGHTNESS);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_color_obj_on_obj, led_color_obj_on);

/// \method led_off()
/// Turn the color sensor LED off. Beware that the color sensor need the LED to be on to detect colors.
mp_obj_t led_color_obj_off(mp_obj_t self_in) {
    Leds_SetSingleBrightness(E_Led_White_Sensor, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_color_obj_off_obj, led_color_obj_off);

/// \method led_intensity([value])
/// Get or set the color sensor LED intensity.  Intensity ranges between 0 (off) and 16 (full on).
/// If no argument is given, return the current LED intensity.
/// If an argument is given, set the LED intensity and return `None`.
/// Beware that the color sensor need the LED to be on to detect colors.
mp_obj_t led_color_obj_intensity(size_t n_args, const mp_obj_t *args) {
    if (n_args == 1) {
        return mp_obj_new_int(Leds_GetBrightness(E_Led_White_Sensor));
    } else {
        int intensity = mp_obj_get_int(args[1]);
        if(intensity > MAX_BRIGHTNESS) {
            intensity = MAX_BRIGHTNESS;
        }
        Leds_SetSingleBrightness(E_Led_White_Sensor, intensity);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_color_obj_intensity_obj, 1, 2, led_color_obj_intensity);

STATIC const mp_rom_map_elem_t color_sensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_hsv), MP_ROM_PTR(&color_sensor_get_hsv_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_raw), MP_ROM_PTR(&color_sensor_get_raw_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_calibration), MP_ROM_PTR(&color_sensor_get_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibrate_and_save_white), MP_ROM_PTR(&color_sensor_calibrate_and_save_white_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibrate_and_save_black), MP_ROM_PTR(&color_sensor_calibrate_and_save_black_obj) },
    { MP_ROM_QSTR(MP_QSTR_led_on), MP_ROM_PTR(&led_color_obj_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_led_off), MP_ROM_PTR(&led_color_obj_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_led_intensity), MP_ROM_PTR(&led_color_obj_intensity_obj) },
};

STATIC MP_DEFINE_CONST_DICT(color_sensor_locals_dict, color_sensor_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_color_sensor_type,
    MP_QSTR_COLOR_SENSOR,
    MP_TYPE_FLAG_NONE,
    make_new, color_sensor_make_new,
    print, color_sensor_print,
    locals_dict, &color_sensor_locals_dict
    );

