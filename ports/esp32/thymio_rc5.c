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
/// Thymio 3 has an infrared receiver that allows it to receive commands from standard TV remote controls.

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

STATIC const mp_rom_map_elem_t rc5_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_command), MP_ROM_PTR(&rc5_obj_get_command_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_address), MP_ROM_PTR(&rc5_obj_get_address_obj) },   
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

