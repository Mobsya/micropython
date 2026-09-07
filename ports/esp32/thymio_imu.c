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
#include "thymio_imu.h"
#include "../../../../../main/accelerometer.h"
#include "../../../../../main/gyroscope.h"
#include "../../../../../main/angle_controller.h"
#include "../../../../../main/aseba_esp32.h"
#include "../../../../../main/settings.h"

T_Axis acc_temp, gyro_temp;
int16_t calib_temp[3];
STATIC T_Settings Setting;

/// \moduleref thymio
/// \class IMU - IMU object
///
/// The IMU object get imu accelerometer and gyroscope values.

typedef struct _thymio_imu_obj_t {
    mp_obj_base_t base;
} thymio_imu_obj_t;

void imu_init(void) {
    Setting.GyroRotFactor = Settings_GetGyroRotFactorSettings();
}

T_Axis imu_get_acc(void) {
    return Accelerometer_GetAcceleration();
}

T_Axis imu_get_gyro(void) {
    return Gyroscope_GetAngularVelocity();
}

int16_t imu_get_angle_deg(void) {
    return Gyroscope_GetAngleZ_deg();
}

void imu_reset_angle(void) {
    Gyroscope_ResetAngle();
}

void imu_get_gyro_calibration(int16_t *values) {
    return Gyroscope_GetCalibration(values);
}

void imu_reset_gyro_calibration(void) {
    Gyroscope_ResetCalibration();
}

void imu_calibrate_gyro(void) {
    Gyroscope_Calibrate();
}

void imu_rotate_deg(int16_t angle, int16_t speed) {
    AngleController_Start(angle, speed);
}

bool imu_rotation_is_complete(void) {
    return AngleController_Completed();
}

void imu_enable_gyro_continuous_calibration(void) {
    Gyroscope_EnableContinuousCalib();
}

void imu_disable_gyro_continuous_calibration(void) {
    Gyroscope_DisableContinuousCalib();
}

bool imu_tap_detected(void) {
    //return Accelerometer_IsTapDetected(); // This call will clear also the flag
    return IS_EVENT(EVENT_TAP);
}

void imu_clear_tap_event(void) {
    CLEAR_EVENT(EVENT_TAP);
}

bool imu_freefall_detected(void) {    
    //return Accelerometer_IsFreeFallDetected(); // This call will clear also the flag
    return IS_EVENT(EVENT_FREEFALL);
}

void imu_clear_freefall_event(void) {
    CLEAR_EVENT(EVENT_FREEFALL);
}

/******************************************************************************/
/* MicroPython bindings                                                       */

void imu_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "IMU");
}

/// \classmethod \constructor()
/// Create a IMU object:
STATIC mp_obj_t imu_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    thymio_imu_obj_t *imu = m_new_obj(thymio_imu_obj_t);
    imu->base.type = &thymio_imu_type;
    imu_init();
    return MP_OBJ_FROM_PTR(imu);
}

/// \method imu_get_acc()
/// Get raw accelerometer values. Values between -32768 and 32767 (scale = +- 2g).
mp_obj_t imu_get_acceleration(mp_obj_t self_in) {
    mp_obj_list_t *data = MP_OBJ_TO_PTR(mp_obj_new_list(3, NULL));
    acc_temp = imu_get_acc();
    data->items[0] = mp_obj_new_int(acc_temp.X);
    data->items[1] = mp_obj_new_int(acc_temp.Y);
    data->items[2] = mp_obj_new_int(acc_temp.Z);
    return MP_OBJ_FROM_PTR(data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_get_acceleration_obj, imu_get_acceleration);

/// \method imu_get_gyro()
/// Get raw gyroscope values. Values between -32768 and 32767 (scale = +- 500 dps).
mp_obj_t imu_get_gyroscope(mp_obj_t self_in) {
    mp_obj_list_t *data = MP_OBJ_TO_PTR(mp_obj_new_list(3, NULL));
    gyro_temp = imu_get_gyro();
    data->items[0] = mp_obj_new_int(gyro_temp.X);
    data->items[1] = mp_obj_new_int(gyro_temp.Y);
    data->items[2] = mp_obj_new_int(gyro_temp.Z);
    return MP_OBJ_FROM_PTR(data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_get_gyroscope_obj, imu_get_gyroscope);

/// \method imu_get_angle_deg()
/// Get yaw angle in degrees.
mp_obj_t imu_get_angle_deg_(mp_obj_t self_in) {
    return mp_obj_new_int(imu_get_angle_deg());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_get_angle_deg_obj, imu_get_angle_deg_);

/// \method imu_reset_angle()
/// Reset the angle (set degrees to 0).
mp_obj_t imu_reset_angle_(mp_obj_t self_in) {
    imu_reset_angle();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_reset_angle_obj, imu_reset_angle_);

/// \method imu_get_gyro_calibration()
/// Get gyroscope offsets calibration values [x, y, z].
mp_obj_t imu_get_gyro_calibration_(mp_obj_t self_in) {
    mp_obj_list_t *data = MP_OBJ_TO_PTR(mp_obj_new_list(3, NULL));
    imu_get_gyro_calibration(calib_temp);
    data->items[0] = mp_obj_new_int(calib_temp[0]);
    data->items[1] = mp_obj_new_int(calib_temp[1]);
    data->items[2] = mp_obj_new_int(calib_temp[2]);
    return MP_OBJ_FROM_PTR(data);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_get_gyro_calibration_obj, imu_get_gyro_calibration_);

/// \method imu_reset_gyro_calibration()
/// Reset the gyroscope offsets calibration values (set to 0).
mp_obj_t imu_reset_gyro_calibration_(mp_obj_t self_in) {
    imu_reset_gyro_calibration();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_reset_gyro_calibration_obj, imu_reset_gyro_calibration_);

/// \method imu_calibrate_gyro()
/// Make a gyroscope offsets calibration, automatic calibration must be disabled before calling this function.
mp_obj_t imu_calibrate_gyro_(mp_obj_t self_in) {
    imu_calibrate_gyro();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_calibrate_gyro_obj, imu_calibrate_gyro_);

/// \method imu_rotate_deg()
/// Tell the robot to rotate by "angle" degrees (-360..360) at maximum speed "speed" (0..1000).
/// The robot spins in place; the sign of the angle gives the direction.
mp_obj_t imu_rotate_deg_(mp_obj_t self_in, mp_obj_t angle, mp_obj_t speed) {
    int a = mp_obj_get_int(angle);
    int s = mp_obj_get_int(speed);
    // check paramters range
    if ((a > 360) || (a < -360)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Target angle must be between -360 and 360"));
    }
    if ((s > 1000) || (s < 0)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Maximum speed must be between 0 and 1000"));
    }    
    AngleController_Start(a, s);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(imu_rotate_deg_obj, imu_rotate_deg_);

/// \method imu_rotate_deg_LR_speeds()
/// Tell the robot to rotate by "angle" degrees (-360..360) using an arbitrary
/// speed pair instead of a spin in place. Speeds are in the range -1000..1000
/// and must differ from each other, otherwise the robot would never turn.
/// The pair is applied to the motors as given: its sign gives the travel
/// direction (both positive drives forward, both negative drives backward) and
/// the difference "right - left" gives the turn, counterclockwise when positive.
/// (300, 0) is a forward arc to the right, (-300, -100) a backward arc to the
/// left, (-300, 300) a spin in place.
/// When the requested angle lies the other way, the pair wins and the robot
/// takes the long way around: rotate_deg_LR_speeds(-90, 0, 200) travels +270
/// degrees along a forward arc to the left, ending at the requested heading.
mp_obj_t imu_rotate_deg_LR_speeds_(size_t n_args, const mp_obj_t *args) {
    int a = mp_obj_get_int(args[1]);
    int l = mp_obj_get_int(args[2]);
    int r = mp_obj_get_int(args[3]);
    // check parameters range
    if ((a > 360) || (a < -360)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Target angle must be between -360 and 360"));
    }
    if ((l > 1000) || (l < -1000) || (r > 1000) || (r < -1000)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Speeds must be between -1000 and 1000"));
    }
    if (l == r) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Left and right speeds must differ"));
    }
    AngleController_StartWithSpeeds(a, l, r);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(imu_rotate_deg_LR_speeds_obj, 4, 4, imu_rotate_deg_LR_speeds_);

/// \method imu_rotate_deg_absolute()
/// Tell the robot to rotate to an absolute heading of "angle" degrees at maximum
/// speed "speed" (0..1000). The heading is referred to the origin set by the last
/// "reset_angle" call, not to the current orientation, and is not folded into a
/// single turn: a heading beyond -360..360 is a legitimate request when the robot
/// accumulated several turns since the last reset.
mp_obj_t imu_rotate_deg_absolute_(mp_obj_t self_in, mp_obj_t angle, mp_obj_t speed) {
    int a = mp_obj_get_int(angle);
    int s = mp_obj_get_int(speed);
    // check parameters range
    if ((a > 32767) || (a < -32768)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Target angle must be between -32768 and 32767"));
    }
    if ((s > 1000) || (s < 0)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Maximum speed must be between 0 and 1000"));
    }
    AngleController_StartAbsolute(a, s);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(imu_rotate_deg_absolute_obj, imu_rotate_deg_absolute_);

/// \method imu_rotate_deg_absolute_LR_speeds()
/// Same as "rotate_deg_absolute", but using an arbitrary speed pair instead of a
/// spin in place. Speeds are in the range -1000..1000 and must differ from each
/// other, otherwise the robot would never turn. Passing (-speed, speed) is
/// equivalent to "rotate_deg_absolute".
/// The pair is applied to the motors as given; when the target heading lies the
/// way the pair does not turn to, the robot takes the long way around and still
/// ends at the requested heading.
mp_obj_t imu_rotate_deg_absolute_LR_speeds_(size_t n_args, const mp_obj_t *args) {
    int a = mp_obj_get_int(args[1]);
    int l = mp_obj_get_int(args[2]);
    int r = mp_obj_get_int(args[3]);
    // check parameters range
    if ((a > 32767) || (a < -32768)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Target angle must be between -32768 and 32767"));
    }
    if ((l > 1000) || (l < -1000) || (r > 1000) || (r < -1000)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Speeds must be between -1000 and 1000"));
    }
    if (l == r) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Left and right speeds must differ"));
    }
    AngleController_StartAbsoluteWithSpeeds(a, l, r);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(imu_rotate_deg_absolute_LR_speeds_obj, 4, 4, imu_rotate_deg_absolute_LR_speeds_);

/// \method imu_rotation_is_complete()
/// Tell if the angle controller completed the rotation, i.e. reached the target position given with "rotate_deg".
/// Return true if the robot reached the target position given by "rotate_deg".
mp_obj_t imu_rotation_is_complete_(mp_obj_t self_in) {
    return mp_obj_new_bool(AngleController_Completed());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_rotation_is_complete_obj, imu_rotation_is_complete_);

/// \method imu_enable_gyro_continuous_calibration()
/// Enable gyroscope automatic calibration (by default is on).
mp_obj_t imu_enable_gyro_continuous_calibration_(mp_obj_t self_in) {
    imu_enable_gyro_continuous_calibration();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_enable_gyro_continuous_calibration_obj, imu_enable_gyro_continuous_calibration_);

/// \method imu_disable_gyro_continuous_calibration()
/// Disable gyroscope automatic calibration (by default is on).
mp_obj_t imu_disable_gyro_continuous_calibration_(mp_obj_t self_in) {
    imu_disable_gyro_continuous_calibration();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_disable_gyro_continuous_calibration_obj, imu_disable_gyro_continuous_calibration_);

/// \method imu_tap_detected()
/// Return true if a tap is detected, false otherwise. This flag must be cleared using "clear_tap_event".
mp_obj_t imu_tap_detected_(mp_obj_t self_in) {
    return mp_obj_new_bool(imu_tap_detected());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_tap_detected_obj, imu_tap_detected_);

/// \method clear_tap_event
/// Clear the tap event flag. This must be used after a tap detection event in order to detect following taps.
mp_obj_t imu_clear_tap_event_(mp_obj_t self_in) {
    imu_clear_tap_event();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_clear_tap_event_obj, imu_clear_tap_event_);

/// \method imu_freefall_detected()
/// Return true if a free fall is detected, false otherwise. This flag must be cleared using "clear_freefall_event".
mp_obj_t imu_freefall_detected_(mp_obj_t self_in) {
    return mp_obj_new_bool(imu_freefall_detected());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_freefall_detected_obj, imu_freefall_detected_);

/// \method clear_freefall_event
/// Clear the free fall event flag. This must be used after a free fall detection event in order to detect following free falls.
mp_obj_t imu_clear_freefall_event_(mp_obj_t self_in) {
    imu_clear_freefall_event();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_clear_freefall_event_obj, imu_clear_freefall_event_);

/// \method imu_gyro_calibration_save()
/// Save gyroscope offsets calibration values.
mp_obj_t imu_gyro_calibration_save(mp_obj_t self_in) {
    Gyroscope_SaveCalibrationOffsets();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_gyro_calibration_save_obj, imu_gyro_calibration_save);

/// \method imu_set_gyro_scale_calibration()
/// Set gyroscope scale calibration value; 4096 corresponds to 90 degrees (each tick is about 0.022 degrees).
/// These values will be used until power off.
mp_obj_t imu_set_gyro_scale_calibration(mp_obj_t self_in, mp_obj_t scale)
{
    Setting.GyroRotFactor = mp_obj_get_int(scale);
    Settings_SetGyroRotFactorSettings(Setting.GyroRotFactor);
    AngleController_UpdateRotFactor(Setting.GyroRotFactor);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(imu_set_gyro_scale_calibration_obj, imu_set_gyro_scale_calibration);

/// \method imu_get_gyro_scale_calibration()
/// Get gyroscope scale calibration value.
mp_obj_t imu_get_gyro_scale_calibration(mp_obj_t self_in)
{
    Setting.GyroRotFactor = Settings_GetGyroRotFactorSettings();
    return mp_obj_new_int(Setting.GyroRotFactor);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_get_gyro_scale_calibration_obj, imu_get_gyro_scale_calibration);

/// \method imu_save_gyro_scale_calibration()
/// Save gyroscope scale calibration value in flash.
mp_obj_t imu_save_gyro_scale_calibration(mp_obj_t self_in)
{
    Settings_WriteGyroRotFactor(Setting.GyroRotFactor);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_save_gyro_scale_calibration_obj, imu_save_gyro_scale_calibration);

/// \method imu_get_angle_raw()
/// Get raw yaw angle.
mp_obj_t imu_get_angle_raw(mp_obj_t self_in) {
    return mp_obj_new_int(Gyroscope_GetAngleZ());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(imu_get_angle_raw_obj, imu_get_angle_raw);

STATIC const mp_rom_map_elem_t imu_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_acc), MP_ROM_PTR(&imu_get_acceleration_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_gyro), MP_ROM_PTR(&imu_get_gyroscope_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_angle_deg), MP_ROM_PTR(&imu_get_angle_deg_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_angle), MP_ROM_PTR(&imu_reset_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_gyro_calib), MP_ROM_PTR(&imu_reset_gyro_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_calibrate_gyro), MP_ROM_PTR(&imu_calibrate_gyro_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotate_deg), MP_ROM_PTR(&imu_rotate_deg_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotate_deg_LR_speeds), MP_ROM_PTR(&imu_rotate_deg_LR_speeds_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotate_deg_absolute), MP_ROM_PTR(&imu_rotate_deg_absolute_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotate_deg_absolute_LR_speeds), MP_ROM_PTR(&imu_rotate_deg_absolute_LR_speeds_obj) },
    { MP_ROM_QSTR(MP_QSTR_rotation_completed), MP_ROM_PTR(&imu_rotation_is_complete_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_gyro_auto_calib), MP_ROM_PTR(&imu_enable_gyro_continuous_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_gyro_auto_calib), MP_ROM_PTR(&imu_disable_gyro_continuous_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_gyro_calib), MP_ROM_PTR(&imu_get_gyro_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_tap_detected), MP_ROM_PTR(&imu_tap_detected_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear_tap_event), MP_ROM_PTR(&imu_clear_tap_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_freefall_detected), MP_ROM_PTR(&imu_freefall_detected_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear_freefall_event), MP_ROM_PTR(&imu_clear_freefall_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_gyro_calib), MP_ROM_PTR(&imu_gyro_calibration_save_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_gyro_scale_calib), MP_ROM_PTR(&imu_get_gyro_scale_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_gyro_scale_calib), MP_ROM_PTR(&imu_set_gyro_scale_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_gyro_scale_calib), MP_ROM_PTR(&imu_save_gyro_scale_calibration_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_angle_raw), MP_ROM_PTR(&imu_get_angle_raw_obj) },
};

STATIC MP_DEFINE_CONST_DICT(imu_locals_dict, imu_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_imu_type,
    MP_QSTR_IMU,
    MP_TYPE_FLAG_NONE,
    make_new, imu_make_new,
    print, imu_print,
    locals_dict, &imu_locals_dict
    );

