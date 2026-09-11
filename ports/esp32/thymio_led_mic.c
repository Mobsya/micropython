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
#include "thymio_led_mic.h"
#include "../../../../../main/behavior.h"


/// \moduleref thymio
/// \class MICROPHONE LED
///
/// The Thymio 3 has a LED near the microphone that can be controlled. If you want to use this LED, be sure to first disable the "led microphone" onboard behavior.

typedef struct _thymio_led_microphone_obj_t {
    mp_obj_base_t base;
} thymio_led_microphone_obj_t;

void led_microphone_init(void) {
    /* Turn off LEDs and initialize */
    Behavior_Disable(B_LED_MIC_STATE); // Turn off mic led
}

/******************************************************************************/
/* MicroPython bindings                                                       */

void led_microphone_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "LED MICROPHONE");
}

/// \classmethod \constructor(id)
/// Create an object referencing the microphone LED.
/// \example Create a microphone LED object:
///     import thymio
///     mic_led = thymio.LED_MICROPHONE()
/// \endexample
STATIC mp_obj_t led_microphone_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    thymio_led_microphone_obj_t *led = m_new_obj(thymio_led_microphone_obj_t);
    led->base.type = &thymio_led_microphone_type;
    return MP_OBJ_FROM_PTR(led);
}

/// \method on()
/// Turn the microphone LED on at maximum brightness; first disable the "led microphone" behavior to be able to control the LED.
mp_obj_t led_microphone_obj_on(mp_obj_t self_in) {
    Behavior_Disable(B_LED_MIC);
    Behavior_Enable(B_LED_MIC_STATE); // Turn off mic led
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_microphone_obj_on_obj, led_microphone_obj_on);

/// \method off()
/// Turn the color sensor LED off; first disable the "led microphone" behavior to be able to control the LED.
mp_obj_t led_microphone_obj_off(mp_obj_t self_in) {
    Behavior_Disable(B_LED_MIC);
    Behavior_Disable(B_LED_MIC_STATE); // Turn off mic led
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_microphone_obj_off_obj, led_microphone_obj_off);

STATIC const mp_rom_map_elem_t led_microphone_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&led_microphone_obj_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&led_microphone_obj_off_obj) },
};

STATIC MP_DEFINE_CONST_DICT(led_microphone_locals_dict, led_microphone_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_led_microphone_type,
    MP_QSTR_LED_MICROPHONE,
    MP_TYPE_FLAG_NONE,
    make_new, led_microphone_obj_make_new,
    print, led_microphone_obj_print,
    locals_dict, &led_microphone_locals_dict
    );

