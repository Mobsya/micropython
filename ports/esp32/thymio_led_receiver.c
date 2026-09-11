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
#include "thymio_led_receiver.h"
#include "../../../../../main/leds.h"


/// \moduleref thymio
/// \class INFRARED RECEIVER LED
///
/// The Thymio 3 has a TV remote receiver LED near the receiver that provides visual feedback to the user. If you want to use this LED, be sure to first disable the "led receiver" onboard behavior.

typedef struct _thymio_led_receiver_obj_t {
    mp_obj_base_t base;
} thymio_led_receiver_obj_t;

void led_receiver_init(void) {
    /* Turn off LEDs and initialize */
    Leds_SetSingleBrightness(E_Led_RC5, 0);
}

/******************************************************************************/
/* MicroPython bindings                                                       */

void led_receiver_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "LED RECEIVER");
}

/// \classmethod \constructor()
/// Create an object referencing the infrared receiver LED.
/// \example Create an infrared receiver LED object:
///     import thymio
///     receiver_led = thymio.LED_RECEIVER()
/// \endexample
STATIC mp_obj_t led_receiver_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    thymio_led_receiver_obj_t *led = m_new_obj(thymio_led_receiver_obj_t);
    led->base.type = &thymio_led_receiver_type;
    return MP_OBJ_FROM_PTR(led);
}

/// \method on()
/// Turn the LED on at maximum brightness.
/// \example Turn on the LED at maximum brightness; first disable the "leds receiver" behavior to be able to control the LED.
///     import thymio
///     behav = thymio.BEHAVIORS()
///     behav.disable_led_receiver()
///     receiver_led.on()
/// \endexample; 
mp_obj_t led_receiver_obj_on(mp_obj_t self_in) {
    Leds_SetSingleBrightness(E_Led_RC5, MAX_BRIGHTNESS);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_receiver_obj_on_obj, led_receiver_obj_on);

/// \method off()
/// Turn the LED off.
/// \example Turn off the LED; first disable the "leds receiver" behavior to be able to control the LED.
///     import thymio
///     behav = thymio.BEHAVIORS()
///     behav.disable_led_receiver()
///     receiver_led.off()
/// \endexample; 
mp_obj_t led_receiver_obj_off(mp_obj_t self_in) {
    Leds_SetSingleBrightness(E_Led_RC5, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(led_receiver_obj_off_obj, led_receiver_obj_off);

/// \method intensity([value])
/// Get or set the LED intensity.  Intensity ranges between 0 (off) and 16 (full on).
/// If no argument is given, return the current LED intensity.
/// If an argument is given, set the LED intensity and return `None`.
/// \example Set the LED at half brightness; first disable the "leds receiver" behavior to be able to control the LED.
///     import thymio
///     behav = thymio.BEHAVIORS()
///     behav.disable_led_receiver()
///     receiver_led.intensity(8)
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

STATIC const mp_rom_map_elem_t led_receiver_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&led_receiver_obj_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&led_receiver_obj_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_intensity), MP_ROM_PTR(&led_receiver_obj_intensity_obj) },
};

STATIC MP_DEFINE_CONST_DICT(led_receiver_locals_dict, led_receiver_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_led_receiver_type,
    MP_QSTR_LED_RECEIVER,
    MP_TYPE_FLAG_NONE,
    make_new, led_receiver_obj_make_new,
    print, led_receiver_obj_print,
    locals_dict, &led_receiver_locals_dict
    );

