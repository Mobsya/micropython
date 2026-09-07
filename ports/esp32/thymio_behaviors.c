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
#include "thymio_behaviors.h"
#include "../../../../../main/mode.h"
#include "../../../../../main/behavior.h"
#include "../../../../../main/leds.h"
#include "../../../../../main/stm32_spi.h"


/// \moduleref thymio
/// \class BEHAVIORS - BEHAVIORS object
///
/// The BEHAVIORS object enable and disable behaviors.

typedef struct _thymio_behaviors_obj_t {
    mp_obj_base_t base;
} thymio_behaviors_obj_t;

//STATIC const thymio_behaviors_obj_t thymio_behaviors_obj;

void behaviors_init(void) {
}

/******************************************************************************/
/* MicroPython bindings                                                       */

void behaviors_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "BEHAVIORS");
}

/// \classmethod \constructor()
/// Create a BEHAVIORS object associated with the given BEHAVIORS:
/// \example Create a BEHAVIORS object
///     import thymio
///     behav = thymio.BEHAVIORS()
/// \endexample
STATIC mp_obj_t behaviors_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    thymio_behaviors_obj_t *behav = m_new_obj(thymio_behaviors_obj_t);
    behav->base.type = &thymio_behaviors_type;
    return MP_OBJ_FROM_PTR(behav);
}

// Disable onboard behaviors. All sensors and actuators are released for user scripts.
/// \example
///     behav.disable_behaviors()
/// \endexample
STATIC mp_obj_t disable_behaviors(mp_obj_t self_in) {
    enter_micropython_mode();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_behaviors_obj, disable_behaviors);

// Enable onboard behaviors. Some sensors and actuators are locked depending on the current behavior.
/// \example
///     behav.enable_behaviors()
/// \endexample
STATIC mp_obj_t enable_behaviors(mp_obj_t self_in) {
    exit_micropython_mode();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_behaviors_obj, enable_behaviors);

// Disable "sound button" behavior. No sound will be played when touching the buttons.
/// \example
///     behav.disable_sound_button()
/// \endexample
STATIC mp_obj_t disable_sound_button(mp_obj_t self_in) {
    Behavior_Disable(B_SOUND_BUTTON);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_sound_button_obj, disable_sound_button);

// Enable "sound button" behavior. A sound will be played when touching the buttons.
/// \example
///     behav.enable_sound_button()
/// \endexample
STATIC mp_obj_t enable_sound_button(mp_obj_t self_in) {
    Behavior_Enable(B_SOUND_BUTTON);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_sound_button_obj, enable_sound_button);

// Disable "leds button" behavior. No buttons LEDs will be turned on when touching the buttons.
/// \example
///     behav.disable_leds_button()
/// \endexample
STATIC mp_obj_t disable_leds_button(mp_obj_t self_in) {
    Behavior_Disable(B_LEDS_BUTTON);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_leds_button_obj, disable_leds_button);

// Enable "leds button" behavior. When touching a button then the related button LED will be turned on.
/// \example
///     behav.enable_leds_button()
/// \endexample
STATIC mp_obj_t enable_leds_button(mp_obj_t self_in) {
    Behavior_Enable(B_LEDS_BUTTON);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_leds_button_obj, enable_leds_button);

// Disable "leds lego gyro" behavior. Front lego LEDs aren't used to show rotation speed.
/// \example
///     behav.disable_leds_lego_gyro()
/// \endexample
STATIC mp_obj_t disable_leds_lego_gyro(mp_obj_t self_in) {
    Behavior_Disable(B_LEDS_LEGO_GYRO);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_leds_lego_gyro_obj, disable_leds_lego_gyro);

// Enable "leds lego gyro" behavior. Front lego LEDs are used to show rotation speed.
/// \example
///     behav.enable_leds_lego_gyro()
/// \endexample
STATIC mp_obj_t enable_leds_lego_gyro(mp_obj_t self_in) {
    Behavior_Enable(B_LEDS_LEGO_GYRO);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_leds_lego_gyro_obj, enable_leds_lego_gyro);

// Disable "leds acc" behavior. Circle LEDs aren't used to show robot 3D orientation.
/// \example
///     behav.disable_leds_acc()
/// \endexample
STATIC mp_obj_t disable_leds_acc(mp_obj_t self_in) {
    Behavior_Disable(B_LEDS_ACC);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_leds_acc_obj, disable_leds_acc);

// Enable "leds acc" behavior. Circle LEDs are used to show robot 3D orientation.
/// \example
///     behav.enable_leds_acc()
/// \endexample
STATIC mp_obj_t enable_leds_acc(mp_obj_t self_in) {
    Behavior_Enable(B_LEDS_ACC);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_leds_acc_obj, enable_leds_acc);

// Disable "led receiver" behavior. The receiver LED is not toggled when a new command is received.
/// \example
///     behav.disable_led_rc5()
/// \endexample
STATIC mp_obj_t disable_led_rc5(mp_obj_t self_in) {
    Behavior_Disable(B_LED_RC5);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_led_rc5_obj, disable_led_rc5);

// Enable "leds receiver" behavior. The receiver LED is toggled when a new command is received.
/// \example
///     behav.enable_led_rc5()
/// \endexample
STATIC mp_obj_t enable_led_rc5(mp_obj_t self_in) {
    Behavior_Enable(B_LED_RC5);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_led_rc5_obj, enable_led_rc5);

// Disable "leds lego animation" behavior. The lego LEDs do not show any animation.
/// \example
///     behav.disable_leds_lego_animation()
/// \endexample
STATIC mp_obj_t disable_leds_lego_animation(mp_obj_t self_in) {
    Behavior_Disable(B_LEDS_LEGO_KITT);
    Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
    Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_leds_lego_animation_obj, disable_leds_lego_animation);

// Enable "leds lego animation" behavior. The lego LEDs show an animation.
/// \example
///     behav.enable_leds_lego_animation()
/// \endexample
STATIC mp_obj_t enable_leds_lego_animation(mp_obj_t self_in) {
    Behavior_Enable(B_LEDS_LEGO_KITT);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_leds_lego_animation_obj, enable_leds_lego_animation);

// Disable "leds proximity" behavior. The front and back LEDs are turned off.
/// \example
///     behav.disable_leds_proximity()
/// \endexample
STATIC mp_obj_t disable_leds_proximity(mp_obj_t self_in) {
    Behavior_Disable(B_LEDS_PROX);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_leds_proximity_obj, disable_leds_proximity);

// Enable "leds proximity" behavior. The front and back LEDs will turn on based on proximities values, the near the object the birghter the LEDs.
/// \example
///     behav.enable_leds_proximity()
/// \endexample
STATIC mp_obj_t enable_leds_proximity(mp_obj_t self_in) {
    Behavior_Enable(B_LEDS_PROX);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_leds_proximity_obj, enable_leds_proximity);

// Disable "leds battery" behavior. The central back LEDs are turned off.
/// \example
///     behav.disable_leds_battery()
/// \endexample
STATIC mp_obj_t disable_leds_battery(mp_obj_t self_in) {
    Behavior_Disable(B_LEDS_BATTERY);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_leds_battery_obj, disable_leds_battery);

// Enable "leds battery" behavior. The central back LEDs show the battery status.
/// \example
///     behav.enable_leds_battery()
/// \endexample
STATIC mp_obj_t enable_leds_battery(mp_obj_t self_in) {
    Behavior_Enable(B_LEDS_BATTERY);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_leds_battery_obj, enable_leds_battery);

// Disable "led microphone" behavior. The microphone LED is not used.
/// \example
///     behav.disable_led_microphone()
/// \endexample
STATIC mp_obj_t disable_led_microphone(mp_obj_t self_in) {
    Behavior_Disable(B_LED_MIC);
    Behavior_Disable(B_LED_MIC_STATE); // Turn off mic led
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_led_microphone_obj, disable_led_microphone);

// Enable "led microphone" behavior. The microphone LED show the sound volume detected by the robot, the more noise the brighter the LED.
/// \example
///     behav.enable_led_microphone()
/// \endexample
STATIC mp_obj_t enable_led_microphone(mp_obj_t self_in) {
    Behavior_Enable(B_LED_MIC);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_led_microphone_obj, enable_led_microphone);

/// \method set_led_mic_threshold(value)
/// Set the threshold for the "led microphone" behavior. If the volume < threshold then LED is off; if it is 0 then the behavior is disabled.
/// \param     value - Threshold between 0 and 1023. The higher the value the more noise is needed to turn on the LED.
/// \example
///     behav.set_led_mic_threshold(50)
/// \endexample
mp_obj_t set_led_mic_threshold(mp_obj_t self_in, mp_obj_t threshold) {
    int thr = mp_obj_get_int(threshold);
    STM32_SetMicrophoneThreshold(thr);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(set_led_mic_threshold_obj, set_led_mic_threshold);

// Enable "leds testing" behavior. The LEDs handled by the STM32 will be turned on and off sequentially (for testing purposes).
/// \example
///     behav.enable_leds_test()
/// \endexample
STATIC mp_obj_t enable_leds_test(mp_obj_t self_in) {
    Behavior_Enable(B_LEDS_TEST);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(enable_leds_test_obj, enable_leds_test);

// Disable "leds testing" behavior.
/// \example
///     behav.disable_leds_test()
/// \endexample
STATIC mp_obj_t disable_leds_test(mp_obj_t self_in) {
    Behavior_Disable(B_LEDS_TEST);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(disable_leds_test_obj, disable_leds_test);

STATIC const mp_rom_map_elem_t behaviors_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_disable_behaviors), MP_ROM_PTR(&disable_behaviors_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_behaviors), MP_ROM_PTR(&enable_behaviors_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_sound_button), MP_ROM_PTR(&disable_sound_button_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_sound_button), MP_ROM_PTR(&enable_sound_button_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_leds_button), MP_ROM_PTR(&disable_leds_button_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_leds_button), MP_ROM_PTR(&enable_leds_button_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_leds_lego_gyro), MP_ROM_PTR(&disable_leds_lego_gyro_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_leds_lego_gyro), MP_ROM_PTR(&enable_leds_lego_gyro_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_leds_acc), MP_ROM_PTR(&disable_leds_acc_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_leds_acc), MP_ROM_PTR(&enable_leds_acc_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_led_receiver), MP_ROM_PTR(&disable_led_rc5_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_led_receiver), MP_ROM_PTR(&enable_led_rc5_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_disable_leds_lego_animation), MP_ROM_PTR(&disable_leds_lego_animation_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_leds_lego_animation), MP_ROM_PTR(&enable_leds_lego_animation_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_disable_leds_proximity), MP_ROM_PTR(&disable_leds_proximity_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_leds_proximity), MP_ROM_PTR(&enable_leds_proximity_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_disable_leds_battery), MP_ROM_PTR(&disable_leds_battery_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_leds_battery), MP_ROM_PTR(&enable_leds_battery_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_disable_led_microphone), MP_ROM_PTR(&disable_led_microphone_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_led_microphone), MP_ROM_PTR(&enable_led_microphone_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_set_led_mic_threshold), MP_ROM_PTR(&set_led_mic_threshold_obj) }, 
    { MP_ROM_QSTR(MP_QSTR_enable_leds_test), MP_ROM_PTR(&enable_leds_test_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_leds_test), MP_ROM_PTR(&disable_leds_test_obj) }, 
};

STATIC MP_DEFINE_CONST_DICT(behaviors_locals_dict, behaviors_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_behaviors_type,
    MP_QSTR_BEHAVIORS,
    MP_TYPE_FLAG_NONE,
    make_new, behaviors_make_new,
    print, behaviors_print,
    locals_dict, &behaviors_locals_dict
    );

