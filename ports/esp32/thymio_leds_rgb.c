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
#include "thymio_leds_rgb.h"
#include "../../../../../main/leds.h"


/// \moduleref thymio
/// \class LEDS_RGB - LED object
///
/// The LEDS_RGB object controls an individual RGB LED (Light Emitting Diode).

typedef struct _thymio_leds_rgb_obj_t {
    mp_obj_base_t base;
    mp_uint_t led_id;
} thymio_leds_rgb_obj_t;

STATIC const thymio_leds_rgb_obj_t thymio_leds_rgb_obj[] = {
    {{&thymio_leds_rgb_type}, 0}, // front-left
    {{&thymio_leds_rgb_type}, 1}, // front-right
    {{&thymio_leds_rgb_type}, 2}, // back-left
    {{&thymio_leds_rgb_type}, 3}, // back-right
    {{&thymio_leds_rgb_type}, 4}, // bottom
    {{&thymio_leds_rgb_type}, 5}, // small rgb led on the top-back (debug)
};
#define NUM_LEDS_RGB MP_ARRAY_SIZE(thymio_leds_rgb_obj)

void leds_rgb_init(void) {
    /* Turn off LEDs and initialize */
    Leds_SetFrontLeftBrightness(0, 0, 0);
    Leds_SetFrontRightBrightness(0, 0, 0);
    Leds_SetBackLeftBrightness(0, 0, 0);
    Leds_SetBackRightBrightness(0, 0, 0);
    Leds_SetColorSensorBrightness(0, 0, 0);
    Leds_SetDebugBrightness(0, 0, 0);
}

int leds_rgb_get_intensity_red(int led) {
    switch(led) {
        case 0: // front-left
            return Leds_GetBrightness(E_Led_R_Front_Left);
            break;
        case 1: // front-right
            return Leds_GetBrightness(E_Led_R_Front_Right);
            break;
        case 2: // back-left
            return Leds_GetBrightness(E_Led_R_Back_Left);
            break;
        case 3: // back-right
            return Leds_GetBrightness(E_Led_R_Back_Right);
            break;
        case 4: // bottom
            return Leds_GetBrightness(E_Led_R_Color_Sensor);
            break;   
        case 5: // top-back (debug)
            return Leds_GetBrightness(E_Led_R_Debug);
            break;                      
    }
    return 0;
}

int leds_rgb_get_intensity_green(int led) {
    switch(led) {
        case 0: // front-left
            return Leds_GetBrightness(E_Led_G_Front_Left);
            break;
        case 1: // front-right
            return Leds_GetBrightness(E_Led_G_Front_Right);
            break;
        case 2: // back-left
            return Leds_GetBrightness(E_Led_G_Back_Left);
            break;
        case 3: // back-right
            return Leds_GetBrightness(E_Led_G_Back_Right);
            break;
        case 4: // bottom
            return Leds_GetBrightness(E_Led_G_Color_Sensor);
            break;   
        case 5: // top-back (debug)
            return Leds_GetBrightness(E_Led_G_Debug);
            break;             
    }
    return 0;
}

int leds_rgb_get_intensity_blue(int led) {
    switch(led) {
        case 0: // front-left
            return Leds_GetBrightness(E_Led_B_Front_Left);
            break;
        case 1: // front-right
            return Leds_GetBrightness(E_Led_B_Front_Right);
            break;
        case 2: // back-left
            return Leds_GetBrightness(E_Led_B_Back_Left);
            break;
        case 3: // back-right
            return Leds_GetBrightness(E_Led_B_Back_Right);
            break;
        case 4: // bottom
            return Leds_GetBrightness(E_Led_B_Color_Sensor);
            break;   
        case 5: // top-back (debug)
            return Leds_GetBrightness(E_Led_B_Debug);
            break;             
    }
    return 0;
}

void leds_rgb_set_intensity(int led, int red, int green, int blue) {
    if (led > (NUM_LEDS_RGB-1)) {
        return;
    }
    if(red > MAX_BRIGHTNESS) {
        red = MAX_BRIGHTNESS;
    }
    if(green > MAX_BRIGHTNESS) {
        green = MAX_BRIGHTNESS;
    }
    if(blue > MAX_BRIGHTNESS) {
        blue = MAX_BRIGHTNESS;
    }
    switch(led) {
        case 0: // front-left
            Leds_SetFrontLeftBrightness(red, green, blue);
            break;
        case 1: // front-right
            Leds_SetFrontRightBrightness(red, green, blue);
            break;
        case 2: // back-left
            Leds_SetBackLeftBrightness(red, green, blue);
            break;
        case 3: // back-right
            Leds_SetBackRightBrightness(red, green, blue);
            break;
        case 4: // bottom
            Leds_SetColorSensorBrightness(red, green, blue);
            break;   
        case 5: // top-back (debug)
            Leds_SetDebugBrightness(red, green, blue);
            break;             
    }
}

/******************************************************************************/
/* MicroPython bindings                                                       */

void leds_rgb_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    thymio_leds_rgb_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "LEDS RGB(%u)", self->led_id);
}

/// \classmethod \constructor(id)
/// Create an LEDS RGB associated with the given RGB LED:
/// \param id The LED number, 0-5 that corresponds to front-left, front-right, back-left, back-right, bottom, small top-back.
/// \example Create a RGB LED object for all the RGB LEDs:
///     import thymio
///     rgb_fl = thymio.LEDS_RGB(0) # RGB front-left
///     rgb_fr = thymio.LEDS_RGB(1) # RGB front-right
///     rgb_bl = thymio.LEDS_RGB(2) # RGB back-left
///     rgb_br = thymio.LEDS_RGB(3) # RGB back-right
///     rgb_btm = thymio.LEDS_RGB(4)  # RGB bottom
///     rgb_conn_status = thymio.LEDS_RGB(5)  # RGB small top-back
/// \endexample
STATIC mp_obj_t leds_rgb_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    // get led number
    mp_int_t led_id = mp_obj_get_int(args[0]);

    // check led number
    if (led_id > (NUM_LEDS_RGB-1)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("LED RGB(%d) doesn't exist"), led_id);
    }

    // return static led object
    return MP_OBJ_FROM_PTR(&thymio_leds_rgb_obj[led_id]);
}

/// \method on()
/// Turn the RGB LED on (white) at maximum brightness.
/// \example Turn on the front-left RGB LED white at maximum brightness:
///     rgb_fl.on()
/// \endexample
mp_obj_t leds_rgb_obj_on(mp_obj_t self_in) {
    thymio_leds_rgb_obj_t *self = MP_OBJ_TO_PTR(self_in);
    leds_rgb_set_intensity(self->led_id, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(leds_rgb_obj_on_obj, leds_rgb_obj_on);

/// \method off()
/// Turn the RGB LED off.
/// \example Turn off the front-left RGB LED:
///     rgb_fl.off()
/// \endexample
mp_obj_t leds_rgb_obj_off(mp_obj_t self_in) {
    thymio_leds_rgb_obj_t *self = MP_OBJ_TO_PTR(self_in);
    leds_rgb_set_intensity(self->led_id, 0, 0, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(leds_rgb_obj_off_obj, leds_rgb_obj_off);

/// \method set_intensity([value red, value green, value blue])
/// Set the RGB LED intensities.  Intensity ranges between 0 (off) and 16 (full on).
/// \param    values red, green, blue: intensities between 0 and 16. The higher the value the brighter the LED.
/// \example Turn the front-left RGB LED red at maximum brightness:
///     rgb_fl.set_intensity(16, 0, 0)
/// \endexample
/// Return `None`.
mp_obj_t leds_rgb_obj_set_intensity(size_t n_args, const mp_obj_t *args) {
    thymio_leds_rgb_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t red = mp_obj_get_int(args[1]);
    mp_int_t green = mp_obj_get_int(args[2]);
    mp_int_t blue = mp_obj_get_int(args[3]);
    leds_rgb_set_intensity(self->led_id, red, green, blue);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(leds_rgb_obj_set_intensity_obj, 4, 4, leds_rgb_obj_set_intensity);

/// \method get_intensity_red()
/// Return current red intensity [0..16].
/// \example Get the current front-left RGB LED red value:
///     rgb_fl.get_intensity_red()
/// \endexample
mp_obj_t leds_rgb_obj_get_intensity_red(mp_obj_t self_in) {
    thymio_leds_rgb_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(leds_rgb_get_intensity_red(self->led_id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(leds_rgb_obj_get_intensity_red_obj, leds_rgb_obj_get_intensity_red);

/// \method get_intensity_green()
/// Return current green intensity [0..16].
/// \example Get the current front-left RGB LED green value:
///     rgb_fl.get_intensity_green()
/// \endexample
mp_obj_t leds_rgb_obj_get_intensity_green(mp_obj_t self_in) {
    thymio_leds_rgb_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(leds_rgb_get_intensity_green(self->led_id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(leds_rgb_obj_get_intensity_green_obj, leds_rgb_obj_get_intensity_green);

/// \method get_intensity_blue()
/// Return current blue intensity [0..16].
/// \example Get the current front-left RGB LED blue value:
///     rgb_fl.get_intensity_blue()
/// \endexample
mp_obj_t leds_rgb_obj_get_intensity_blue(mp_obj_t self_in) {
    thymio_leds_rgb_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(leds_rgb_get_intensity_blue(self->led_id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(leds_rgb_obj_get_intensity_blue_obj, leds_rgb_obj_get_intensity_blue);

STATIC const mp_rom_map_elem_t leds_rgb_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&leds_rgb_obj_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&leds_rgb_obj_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_intensity), MP_ROM_PTR(&leds_rgb_obj_set_intensity_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_intensity_red), MP_ROM_PTR(&leds_rgb_obj_get_intensity_red_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_intensity_green), MP_ROM_PTR(&leds_rgb_obj_get_intensity_green_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_intensity_blue), MP_ROM_PTR(&leds_rgb_obj_get_intensity_blue_obj) },
};

STATIC MP_DEFINE_CONST_DICT(leds_rgb_locals_dict, leds_rgb_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_leds_rgb_type,
    MP_QSTR_LEDS_RGB,
    MP_TYPE_FLAG_NONE,
    make_new, leds_rgb_obj_make_new,
    print, leds_rgb_obj_print,
    locals_dict, &leds_rgb_locals_dict
    );

