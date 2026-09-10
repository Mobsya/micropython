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
#include "thymio_rc5.h"
#include "../../../../../main/rc5.h"
#include "../../../../../main/leds.h"

int16_t toggle = -1;

/// \moduleref thymio
/// \class RC5
///
/// Thymio 3 has an infrared receiver that allows it to receive commands from standard TV remote controls and it has also a TV remote receiver LED near the receiver that provides visual feedback to the user. If you want to use this LED, be sure to first disable the "led receiver" onboard behavior.

typedef struct _thymio_rc5_obj_t {
    mp_obj_base_t base;
} thymio_rc5_obj_t;

void rc5_init(void) {
}

int rc5_get_command(void) {
    return RC5_GetCommand(&toggle);
}

/******************************************************************************/
/* MicroPython bindings                                                       */

void rc5_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "RC5");
}

/// \classmethod \constructor(id)
/// Create an RC5 object associated with the IR remote receiver.
/// \example Create an RC5 object
///     import thymio
///     rc5 = thymio.RC5()
/// \endexample
STATIC mp_obj_t rc5_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    thymio_rc5_obj_t *rc5 = m_new_obj(thymio_rc5_obj_t);
    rc5->base.type = &thymio_rc5_type;
    return MP_OBJ_FROM_PTR(rc5);
}

/// \method get_command()
/// Get last new command received or -1 if nothing received or no new commands received.
/// \example Print the last received command:
///     print(str(rc5.get_command()))
/// \endexample
mp_obj_t rc5_obj_get_command(mp_obj_t self_in) {
    return mp_obj_new_int(rc5_get_command());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rc5_obj_get_command_obj, rc5_obj_get_command);

/// \method get_address()
/// Get RC5 address.
/// \example Print the RC5 address:
///     print(str(rc5.get_address()))
/// \endexample
mp_obj_t rc5_obj_get_address(mp_obj_t self_in) {
    return mp_obj_new_int(RC5_GetAddress());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rc5_obj_get_address_obj, rc5_obj_get_address);

/// \method led_on()
/// Turn the LED on at maximum brightness.
/// \example Turn on the LED at maximum brightness; first disable the "leds receiver" behavior to be able to control the LED.
///     import thymio
///     behav = thymio.BEHAVIORS()
///     behav.disable_led_receiver()
///     rc5.led_on()
/// \endexample; 
mp_obj_t led_receiver_obj_on(mp_obj_t self_in) {
    Leds_SetSingleBrightness(E_Led_RC5, MAX_BRIGHTNESS);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_receiver_obj_on_obj, led_receiver_obj_on);

/// \method led_off()
/// Turn the LED off.
/// \example Turn off the LED; first disable the "leds receiver" behavior to be able to control the LED.
///     import thymio
///     behav = thymio.BEHAVIORS()
///     behav.disable_led_receiver()
///     rc5.led_off()
/// \endexample; 
mp_obj_t led_receiver_obj_off(mp_obj_t self_in) {
    Leds_SetSingleBrightness(E_Led_RC5, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_receiver_obj_off_obj, led_receiver_obj_off);

/// \method led_intensity([value])
/// Get or set the LED intensity.  Intensity ranges between 0 (off) and 16 (full on).
/// If no argument is given, return the current LED intensity.
/// If an argument is given, set the LED intensity and return `None`.
/// \example Set the LED at half brightness; first disable the "leds receiver" behavior to be able to control the LED.
///     import thymio
///     behav = thymio.BEHAVIORS()
///     behav.disable_led_receiver()
///     rc5.led_intensity(8)
/// \endexample; 
mp_obj_t led_receiver_obj_intensity(size_t n_args, const mp_obj_t *args) {
    if (n_args == 1) {
        return mp_obj_new_int(Leds_GetBrightness(E_Led_RC5));
    } else {
        int intensity = mp_obj_get_int(args[1]);
        if(intensity > MAX_BRIGHTNESS) {
            intensity = MAX_BRIGHTNESS;
        }
        Leds_SetSingleBrightness(E_Led_RC5, intensity);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_receiver_obj_intensity_obj, 1, 2, led_receiver_obj_intensity);

STATIC const mp_rom_map_elem_t rc5_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_command), MP_ROM_PTR(&rc5_obj_get_command_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_address), MP_ROM_PTR(&rc5_obj_get_address_obj) },
    { MP_ROM_QSTR(MP_QSTR_led_on), MP_ROM_PTR(&led_receiver_obj_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_led_off), MP_ROM_PTR(&led_receiver_obj_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_led_intensity), MP_ROM_PTR(&led_receiver_obj_intensity_obj) },    
};

STATIC MP_DEFINE_CONST_DICT(rc5_locals_dict, rc5_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_rc5_type,
    MP_QSTR_RC5,
    MP_TYPE_FLAG_NONE,
    make_new, rc5_obj_make_new,
    print, rc5_obj_print,
    locals_dict, &rc5_locals_dict
    );

