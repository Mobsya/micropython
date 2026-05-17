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
#include "thymio_buttons.h"
#include "../../../../../main/buttons.h"

/// \moduleref thymio
/// \class BUTTONS - BUTTONS object
///
/// The BUTTONS object get touchpad buttons status.

typedef struct _thymio_buttons_obj_t {
    mp_obj_base_t base;
} thymio_buttons_obj_t;

void buttons_init(void) {
}

// uint8_t* buttons_get_status(void) {
//     return Buttons_GetStatus();
// }

/******************************************************************************/
/* MicroPython bindings                                                       */

void buttons_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "BUTTONS");
}

/// \classmethod \constructor()
/// Create a BUTTONS object:
STATIC mp_obj_t buttons_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    thymio_buttons_obj_t *buttons = m_new_obj(thymio_buttons_obj_t);
    buttons->base.type = &thymio_buttons_type;
    return MP_OBJ_FROM_PTR(buttons);
}

/// \method buttons_get_status()
/// Get buttons status (1 = pressed, 0 = not pressed). The returned list correspond to [BACKWARD, LEFT, CENTER, FORWARD, RIGHT].
mp_obj_t buttons_get_status_values(mp_obj_t self_in) {
    mp_obj_list_t *data = MP_OBJ_TO_PTR(mp_obj_new_list(5, NULL));
    uint8_t* status = Buttons_GetStatus();
    for (int i = 0; i < 5; i++) {
        data->items[i] = mp_obj_new_int(status[i]);
    }
    return MP_OBJ_FROM_PTR(data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(buttons_get_status_values_obj, buttons_get_status_values);

/// \method buttons_get_raw()
/// Get raw value buttons. The returned list correspond to [BACKWARD, LEFT, CENTER, FORWARD, RIGHT].
mp_obj_t buttons_get_raw_values(mp_obj_t self_in) {
    mp_obj_list_t *data = MP_OBJ_TO_PTR(mp_obj_new_list(5, NULL));
    uint16_t* raw = Buttons_GetRaw();
    for (int i = 0; i < 5; i++) {
        data->items[i] = mp_obj_new_int(raw[i]);
    }
    return MP_OBJ_FROM_PTR(data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(buttons_get_raw_values_obj, buttons_get_raw_values);

/// \method buttons_get_filtered()
/// Get filtered value buttons. The returned list correspond to [BACKWARD, LEFT, CENTER, FORWARD, RIGHT].
mp_obj_t buttons_get_filtered_values(mp_obj_t self_in) {
    mp_obj_list_t *data = MP_OBJ_TO_PTR(mp_obj_new_list(5, NULL));
    uint16_t* filtered = Buttons_GetFiltered();
    for (int i = 0; i < 5; i++) {
        data->items[i] = mp_obj_new_int(filtered[i]);
    }
    return MP_OBJ_FROM_PTR(data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(buttons_get_filtered_values_obj, buttons_get_filtered_values);

STATIC const mp_rom_map_elem_t buttons_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_status), MP_ROM_PTR(&buttons_get_status_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_raw), MP_ROM_PTR(&buttons_get_raw_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_filtered), MP_ROM_PTR(&buttons_get_filtered_values_obj) },
};

STATIC MP_DEFINE_CONST_DICT(buttons_locals_dict, buttons_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_buttons_type,
    MP_QSTR_BUTTONS,
    MP_TYPE_FLAG_NONE,
    make_new, buttons_make_new,
    print, buttons_print,
    locals_dict, &buttons_locals_dict
    );

