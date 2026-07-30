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
#include "thymio_ground.h"
#include "../../../../../main/stm32_spi.h"
#include "../../../../../main/settings.h"
#include "../../../../../main/common.h"

int16_t calib_temp[4]; // left black, right black, left white, right white

/// \moduleref thymio
/// \class GROUND - GROUND object
///
/// The GROUND object get data from ground sensors.

typedef struct _thymio_ground_obj_t {
    mp_obj_base_t base;
    mp_uint_t ground_id;
} thymio_ground_obj_t;

STATIC const thymio_ground_obj_t thymio_ground_obj[] = {
    {{&thymio_ground_type}, 0},
    {{&thymio_ground_type}, 1}                   
};
#define NUM_GROUND MP_ARRAY_SIZE(thymio_ground_obj)

STATIC int16_t tempAmbient[2];
STATIC int16_t tempReflected[2];
STATIC T_Settings Setting;

void ground_init(void) {
    Settings_GetGroundBlackSettings(Setting.GroundBlack);
    Settings_GetGroundWhiteSettings(Setting.GroundWhite);
}

int ground_get_value(int ground) {
    return GetGroundValue(ground);
}

void ground_get_calibration(int16_t *values)
{
    Settings_GetGroundBlackSettings(values);
    Settings_GetGroundWhiteSettings(&values[2]);
}

void ground_set_and_save_calibration_from_values(int16_t *values)
{
    Settings_WriteGroundBlack(values);
    Settings_SetGroundBlackSettings(values);
    //Common_SetGroundThr(values);
    Settings_WriteGroundWhite(&values[2]);
    Settings_SetGroundWhiteSettings(&values[2]);
    STM32_SetGroundRange(&values[2], values);
}

void ground_set_calibration_from_values(int16_t *values)
{
    Settings_SetGroundBlackSettings(values);
    //Common_SetGroundThr(values);
    Settings_SetGroundWhiteSettings(&values[2]);
    STM32_SetGroundRange(&values[2], values);   
}

void ground_save_calibration_from_values(void)
{
    int16_t tempGround[2];
    Settings_GetGroundBlackSettings(tempGround);
    Settings_WriteGroundBlack(tempGround);
    Settings_GetGroundWhiteSettings(tempGround);
    Settings_WriteGroundWhite(tempGround);
}

void ground_reset_calibration(void)
{
    WriteFactoryGroundBlack();
    WriteFactoryGroundWhite();
}

/******************************************************************************/
/* MicroPython bindings                                                       */

void ground_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    thymio_ground_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "GROUND(%u)", self->ground_id);
}

/// \classmethod \constructor(id)
/// Create an GROUND object associated with the given GROUND:
///
///   - `id` is the GROUND number, 0-1.
STATIC mp_obj_t ground_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    // get ground number
    mp_int_t ground_id = mp_obj_get_int(args[0]);

    // check ground number
    if (ground_id > (NUM_GROUND-1)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("GROUND(%d) doesn't exist"), ground_id);
    }

    ground_init();

    // return static ground object
    return MP_OBJ_FROM_PTR(&thymio_ground_obj[ground_id]);
}

/// \method value()
/// Get ground sensor value (the lower the value, the darker the object).
mp_obj_t ground_value(mp_obj_t self_in) {
    thymio_ground_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(ground_get_value(self->ground_id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ground_value_obj, ground_value);

/// \method normalized_value()
/// Get normalized (between 0 and 100) ground sensor value (the lower the value, the darker the object).
mp_obj_t ground_normalized_value(mp_obj_t self_in) {
    thymio_ground_obj_t *self = MP_OBJ_TO_PTR(self_in);
    float temp = ground_get_value(self->ground_id);
    float normalized = temp*100.0/1024.0;
    if(normalized > 100) {
        normalized = 100;
    }
    return mp_obj_new_int((int)normalized);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ground_normalized_value_obj, ground_normalized_value);

/// \method get_ambient()
/// Get ground sensor ambient value (the higher the value, the brighter the ambient light).
mp_obj_t ground_ambient(mp_obj_t self_in) {
    thymio_ground_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(GetGroundAmbient(self->ground_id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ground_ambient_obj, ground_ambient);

/// \method reflected()
/// Get ground sensor reflected value (the lower the value, the darker the object).
mp_obj_t ground_reflected(mp_obj_t self_in) {
    thymio_ground_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(GetGroundReflected(self->ground_id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ground_reflected_obj, ground_reflected);

/// \method ground_get_calibration()
/// Get ground calibration values [black, white].
mp_obj_t ground_get_calibration_(mp_obj_t self_in) {
    thymio_ground_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_list_t *data = MP_OBJ_TO_PTR(mp_obj_new_list(2, NULL));
    ground_get_calibration(calib_temp);
    if(self->ground_id == 0)
    {
        data->items[0] = mp_obj_new_int(calib_temp[0]); // black
        data->items[1] = mp_obj_new_int(calib_temp[2]); // white
    }
    else
    {
        data->items[0] = mp_obj_new_int(calib_temp[1]); // black
        data->items[1] = mp_obj_new_int(calib_temp[3]); // white
    }
    return MP_OBJ_FROM_PTR(data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ground_get_calibration_obj, ground_get_calibration_);

/// \method set_and_save_calibration_from_values()
/// Set both ground calibration values [black left, black right, white left, white right] and save to flash
/// based on the given values as parameters. These values will be used right away on all the behaviors.
mp_obj_t ground_set_and_save_calibration_from_values_(mp_obj_t self_in, mp_obj_t values) {
    mp_obj_t *items;
    size_t len;
    int16_t calib_values[4];

    mp_obj_get_array(values, &len, &items);
    if (len != 4) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Expected exactly 4 values"));
    }

    for (size_t i = 0; i < len; i++) {
        calib_values[i] = mp_obj_get_int(items[i]);
    }

    ground_set_and_save_calibration_from_values(calib_values);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(ground_set_and_save_calibration_from_values_obj, ground_set_and_save_calibration_from_values_);

/// \method set_calibration_from_values()
/// Set both ground calibration values [black left, black right, white left, white right] without saving to flash
/// based on the given values as parameters. These values will be used right away on all the behaviors.
mp_obj_t ground_set_calibration_from_values_(mp_obj_t self_in, mp_obj_t values) {
    mp_obj_t *items;
    size_t len;
    int16_t calib_values[4];

    mp_obj_get_array(values, &len, &items);
    if (len != 4) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Expected exactly 4 values"));
    }

    for (size_t i = 0; i < len; i++) {
        calib_values[i] = mp_obj_get_int(items[i]);
    }

    ground_set_calibration_from_values(calib_values);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(ground_set_calibration_from_values_obj, ground_set_calibration_from_values_);

/// \method save_calibration_from_values()
/// Save calibration values for both ground sensors (white and black) based on previously set values with "ground_set_calibration_from_values" or "ground_set_and_save_calibration_from_values".
/// The defualt values are loaded from flash at init, thus if "ground_set_calibration_from_values" is not called, the previous saved values will be used.
mp_obj_t ground_save_calibration_from_values_(mp_obj_t self_in) {
    ground_save_calibration_from_values();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ground_save_calibration_from_values_obj, ground_save_calibration_from_values_);

/// \method calibrate_white()
/// Calibrate both ground sensors on a white surface.
/// These values will be used until power off.
mp_obj_t ground_calibrate_white(mp_obj_t self_in) {
    GetGroundAmbients(tempAmbient);
    GetGroundReflecteds(tempReflected);
    Setting.GroundWhite[0] = tempReflected[0] - tempAmbient[0];
    if(Setting.GroundWhite[0] < 0)
    {
        Setting.GroundWhite[0] = 0;
    }
    Setting.GroundWhite[1] = tempReflected[1] - tempAmbient[1];
    if(Setting.GroundWhite[1] < 0)
    {
        Setting.GroundWhite[1] = 0;
    } 
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ground_calibrate_white_obj, ground_calibrate_white);

/// \method calibrate_black()
/// Calibrate both ground sensors on a black surface.
/// These values will be used until power off.
mp_obj_t ground_calibrate_black(mp_obj_t self_in) {
    GetGroundAmbients(tempAmbient);
    GetGroundReflecteds(tempReflected);
    Setting.GroundBlack[0] = tempReflected[0] - tempAmbient[0];
    if(Setting.GroundBlack[0] < 0)
    {
        Setting.GroundBlack[0] = 0;
    }
    Setting.GroundBlack[1] = tempReflected[1] - tempAmbient[1];
    if(Setting.GroundBlack[1] < 0)
    {
        Setting.GroundBlack[1] = 0;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ground_calibrate_black_obj, ground_calibrate_black);

/// \method save_calibration()
/// Save calibration values for both ground sensors (white and black) previously set with "ground_calibrate_white" and "ground_calibrate_black".
/// The defualt values are loaded from flash at init, thus if "ground_calibrate_white" or "ground_calibrate_black" are not called, the previous saved values will be used.
mp_obj_t ground_save_calibration(mp_obj_t self_in) {
    Settings_WriteGroundBlack(Setting.GroundBlack);
    Settings_SetGroundBlackSettings(Setting.GroundBlack);
    //Common_SetGroundThr(Setting.GroundBlack);
    Settings_WriteGroundWhite(Setting.GroundWhite);
    Settings_SetGroundWhiteSettings(Setting.GroundWhite);
    STM32_SetGroundRange(Setting.GroundWhite, Setting.GroundBlack);    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ground_save_calibration_obj, ground_save_calibration);

STATIC const mp_rom_map_elem_t ground_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&ground_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_normalized_value), MP_ROM_PTR(&ground_normalized_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_ambient), MP_ROM_PTR(&ground_ambient_obj) },
    { MP_ROM_QSTR(MP_QSTR_reflected), MP_ROM_PTR(&ground_reflected_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_calibration), MP_ROM_PTR(&ground_get_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_and_save_calibration_from_values), MP_ROM_PTR(&ground_set_and_save_calibration_from_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_calibration_from_values), MP_ROM_PTR(&ground_set_calibration_from_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_calibration_from_values), MP_ROM_PTR(&ground_save_calibration_from_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibrate_white), MP_ROM_PTR(&ground_calibrate_white_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibrate_black), MP_ROM_PTR(&ground_calibrate_black_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_calibration), MP_ROM_PTR(&ground_save_calibration_obj) },    
};

STATIC MP_DEFINE_CONST_DICT(ground_locals_dict, ground_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_ground_type,
    MP_QSTR_GROUND,
    MP_TYPE_FLAG_NONE,
    make_new, ground_make_new,
    print, ground_print,
    locals_dict, &ground_locals_dict
    );

