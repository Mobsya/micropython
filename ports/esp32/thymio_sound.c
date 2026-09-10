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
#include <string.h>

#include "extmod/vfs.h"
#include "py/stream.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "thymio_sound.h"
#include "../../../../../main/codec.h"
#include "../../../../../main/aseba_esp32.h"
#include "../../../../../main/stm32_spi.h"
#include "../../../../../main/settings.h"


/// \moduleref thymio
/// \class SOUND
///
/// The Thymio 3 robot has audio capabilities that allow it to record and play sounds.

typedef struct _thymio_sound_obj_t {
    mp_obj_base_t base;
} thymio_sound_obj_t;

//STATIC const thymio_sound_obj_t thymio_sound_obj;

void sound_init(void) {
}

int sound_get_mic_volume(void) {
    return STM32_GetMicrophoneIntensity();
}

bool sound_clap_detected(void) {
    return IS_EVENT(EVENT_MIC);
}

void sound_clear_clap_event(void) {
    CLEAR_EVENT(EVENT_MIC);
}

/******************************************************************************/
/* MicroPython bindings                                                       */

void sound_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "SOUND");
}

/// \classmethod \constructor()
/// Create an sound object associated with the speaker and microphone.
/// \example Create a SOUND object
///     import thymio
///     sound = thymio.SOUND()
/// \endexample
STATIC mp_obj_t sound_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    thymio_sound_obj_t *sound = m_new_obj(thymio_sound_obj_t);
    sound->base.type = &thymio_sound_type;
    return MP_OBJ_FROM_PTR(sound);
}

/// \method record(duration)
/// Start a recording in wav format. The data are saved in RAM memory. Max duration is 10 seconds. During the recording the microphone led (blue led on top) is turned on.
/// \param duration given in seconds (integer)
/// \example Record a sound for 5 seconds
///     sound.record(5)
/// \endexample
mp_obj_t sound_record_wav_(mp_obj_t self_in, mp_obj_t sec) {
    int duration = mp_obj_get_int(sec);
    if(duration > 10) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Max recording is 10 seconds"));
        return mp_const_none;
    }
    Codec_RecordWAVFile(duration);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(sound_record_wav_obj, sound_record_wav_);

/// \method get_mic_volume
/// Return the volume computed from the microphone. Range is from 0 to 1023.
/// \example Print the current microphone volume
///     print(str(sound.get_mic_volume()))
/// \endexample
mp_obj_t sound_get_mic_volume_(mp_obj_t self_in) {
    return mp_obj_new_int(sound_get_mic_volume());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_get_mic_volume_obj, sound_get_mic_volume_);

/// \method clap_detected
/// Return true if a clap is detected, false otherwise. This flag must be cleared using "clear_clap_event".
mp_obj_t sound_clap_detected_(mp_obj_t self_in) {
    return mp_obj_new_bool(sound_clap_detected());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_clap_detected_obj, sound_clap_detected_);

/// \method clear_clap_event
/// Clear the clap event flag. This must be used after a clap detection event in order to detect following claps.
mp_obj_t sound_clear_clap_event_(mp_obj_t self_in) {
    sound_clear_clap_event();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_clear_clap_event_obj, sound_clear_clap_event_);

/// \method record_completed()
/// Tell if the recording is completed.
/// Return true if the recording is completed.
/// The onevent decorator can be used to attach custom callbacks at the "record completed" event, specifying the decorator parameter RECORD_COMPLETED.
/// \example Record a sound for 5 seconds and play it back when the recording is completed using decorator:
///     import thymio
///     import uasyncio as asyncio
///     from thymio_events import ThymioEvents
///     from thymio_events import onevent
/// 
///     te = ThymioEvents()
///     sound = thymio.SOUND()
/// 
///     @onevent("RECORD_COMPLETED")
///     def record_end():
///         print("record complete")
///         sound.play_recorded()
/// 
///     async def main():
///         sound.record(5)
///         while True:
///             try:
///                 await asyncio.sleep(1)
///             except:
///                 None
/// 
///     asyncio.run(main())
/// \endexample
mp_obj_t sound_record_is_complete(mp_obj_t self_in) {
    return mp_obj_new_bool(Codec_IsRecordFinished());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_record_is_complete_obj, sound_record_is_complete);

/// \method play_completed()
/// Tell if the last sound is complete.
/// Return true if the last sound played is completed. If no sound was played it returns false.
mp_obj_t sound_play_is_complete(mp_obj_t self_in) {
    return mp_obj_new_bool(Codec_IsSoundFinished());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_play_is_complete_obj, sound_play_is_complete);

/// \method record_get()
/// Get the recording data so that you can save them to robot storage.
/// example Save the last recorded sound to a file in robot storage:
///     f = open('my.wav', 'w')
///     size = f.write(bytearray(sound.record_get()))  # return number of bytes written
///     f.close()
/// \endexample
STATIC mp_obj_t sound_record_get(mp_obj_t self_in) {
    byte *buf;
    uint32_t recSize = Codec_GetRecordSize();
    buf = m_new(byte, recSize);
    memcpy(buf, Codec_GetRecordPtr(), recSize);
    return mp_obj_new_bytearray_by_ref(recSize, buf);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_record_get_obj, sound_record_get);

/// \method play_from_file(name)
/// \brief  Play sound file (wav or mp3) from robot storage. The sound file must have either ".wav" or ".mp3" extension and in the format: 12KHz sample rate, 16 bits per sample, mono channel.
/// \param  name Name of the file
/// \return None if ok, RuntimeError exception if another sound or recording is already running, ValueError if file not supported.
/// \example Play a wav:
///     sound.play_from_file('my.wav')
/// \endexample
mp_obj_t sound_play_from_file(mp_obj_t self_in, mp_obj_t name) {
    mp_obj_t file;
    mp_obj_t args[2] = {
        name,
        MP_OBJ_NEW_QSTR(MP_QSTR_rb),
    };
    uint32_t sampleRate = 0;
    uint16_t numChannels = 0;
    uint16_t bitsPerChannel = 0;
    uint16_t mp3Ver = 0;
    uint32_t id3Size = 0;
    uint32_t len;
    int errcode;
    uint32_t fileSize = 0;
    byte *buf;
    uint8_t fileType = 0;
    char* fileName = mp_obj_str_get_str(args[0]);
    int fileNameLen = strlen(fileName);
    //printf("last char filename = %c\n", fileName[fileNameLen-1]);
    if(fileName[fileNameLen-1] == 'v') { // Wav
        fileType = 1;
    } else if(fileName[fileNameLen-1] == '3') { // Mp3
        fileType = 2;
    } else {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("File format not supported"));
        return mp_const_none;
    }

    mp_obj_t stat = mp_vfs_stat(name);
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(stat);
    // print tuple
    fileSize = mp_obj_get_int(t->items[6]);
    //for (int i = 1; i <= 9; ++i) {
    //    printf("tuple[%d]=%d\n", i, mp_obj_get_int(t->items[i]));  // dev, nlink, uid, gid, size, atime, mtime, ctime
    //}
    //printf("filesize=%d\n", fileSize);
    buf = m_new(byte, fileSize);
    file = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);
    len = mp_stream_rw(file, buf, fileSize, &errcode, MP_STREAM_RW_READ | MP_STREAM_RW_ONCE);
    mp_stream_close(file);   

    // Check sound format by interpreting the header.
    if(fileType == 1) { // Wav
        numChannels = buf[22]+(buf[23]<<8);
        sampleRate = buf[24]+(buf[25]<<8)+(buf[26]<<16)+(buf[27]<<24);        
        bitsPerChannel = buf[34]+(buf[35]<<8);
        printf("wav ch=%d, rate=%d, bits=%d\n", numChannels, sampleRate, bitsPerChannel);
        if((numChannels==1) && (sampleRate==12000) && (bitsPerChannel==16)) {
            if(Codec_PlayWAVFile(buf, fileSize) != ESP_OK) {
                mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Cannot play"));
                return mp_const_none;    
            }
        } else {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("File format not supported"));
            return mp_const_none;
        }
    } else if(fileType == 2) { // Mp3         
        if(buf[0] == 0x49) { // ID3 header detected
            id3Size = buf[9] + (buf[8]<<7) + (buf[7]<<14) + (buf[6]<<21);
            //printf("id3size = %d\n", id3Size);
            //printf("%x %x %x %x\n", buf[10+id3Size], buf[11+id3Size], buf[12+id3Size], buf[13+id3Size]);
            mp3Ver = (buf[11+id3Size]&0x18)>>3;
            numChannels = (buf[13+id3Size]&0xC0)>>6;
            sampleRate = (buf[12+id3Size]&0x0C)>>2;            
        } else if(buf[0] == 0xFF) { // Mp3 header (no ID3 included)
            //printf("%x %x %x %x\n", buf[0], buf[1], buf[2], buf[3]); // Expected something like: FF E3 84 C4
            mp3Ver = (buf[1]&0x18)>>3;
            numChannels = (buf[3]&0xC0)>>6;
            sampleRate = (buf[2]&0x0C)>>2;
        } else {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("File format not supported"));
            return mp_const_none;
        }
        //printf("mp3 ver=%d, rate=%d, ch=%d\n", mp3Ver, sampleRate, numChannels);
        if((mp3Ver==0) && (numChannels==3) && (sampleRate==1)) {
            if(Codec_PlayMP3File(buf, fileSize) != ESP_OK) {
                mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Cannot play"));
                return mp_const_none;                
            }
        } else {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("File format not supported"));
            return mp_const_none;            
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(sound_play_from_file_obj, sound_play_from_file);

/// \method play_recorded()
/// \brief     Play the last recorded sound directly from memory.
/// \param     None
/// \return    None if ok, RuntimeError exception if another sound or recording is already running.
mp_obj_t sound_play_recorded(mp_obj_t self_in) {
    if(Codec_PlayRecorded() != ESP_OK) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Cannot play"));
        return mp_const_none;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_play_recorded_obj, sound_play_recorded);

/// \method play_onboard(id)
/// \brief  Play onboard sound (the ones pre-built in the robot firmware).
/// \param  id The id parameter identifies the correct sound to play: 0=alarm, 1=bad, 2=battery low, 3=beep, 4=sequence delete all, 5=sequence delete last, 6=sequence end path, 7=bluetooth connection, 8=code error, 9=code exec success, 10=detect, 11=draw end, 12=arrows, 13-18=balafon notes, 19-24=flute notes, 25-30=guitar notes, 31-36=orchestra notes, 37-42=piano notes, 43-48=violin notes, 49=intro, 50=notify, 51=outro, 52=red freefall, 53=red prisoner, 54=red tap, 55=rotate
/// \return None if ok, RuntimeError exception if another sound or recording is already running.
mp_obj_t sound_play_onboard(mp_obj_t self_in, mp_obj_t ind) {
    int index = mp_obj_get_int(ind);
    if(Codec_PlayOnboardSound(index) != ESP_OK) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Cannot play"));
        return mp_const_none;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(sound_play_onboard_obj, sound_play_onboard);

/// \method pause()
/// \brief     Pause any running sound (can be resumed with "resume").
/// \param     None
/// \return    None if ok, RuntimeError exception if no sound is running.
mp_obj_t sound_pause(mp_obj_t self_in) {
    if(Codec_Pause() != ESP_OK) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Cannot pause"));
        return mp_const_none;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_pause_obj, sound_pause);

/// \method resume()
/// \brief     Resume a previously paused (with "pause") sound play.
/// \param     None
/// \return    None if ok, RuntimeError exception if no sound was paused.
mp_obj_t sound_resume(mp_obj_t self_in) {
    if(Codec_Resume() != ESP_OK) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Cannot resume"));
        return mp_const_none;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_resume_obj, sound_resume);

/// \method set_volume(volume)
/// \brief     Set the play volume.
/// \param     volume Between 0 and 10.
/// \return    None
mp_obj_t sound_set_volume(mp_obj_t self_in, mp_obj_t vol) {
    int volume = mp_obj_get_int(vol);
    if((volume > 10) || (volume < 0)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Volume must be between 0 and 10"));
        return mp_const_none;
    }

    Codec_SetVolume(volume*10);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(sound_set_volume_obj, sound_set_volume);

/// \method save_volume()
/// \brief     Store the current volume value in the permanent settings. The volume setting will be used when the robot is turned on next times.
/// \param     None
/// \return    None
mp_obj_t sound_save_volume(mp_obj_t self_in) {
    Settings_WriteVolume(Settings_GetVolumeSettings());
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_save_volume_obj, sound_save_volume);

/// \method clear_events()
/// \brief     Clear all audio events ("play completed" and "recording completed").
/// \param     None
/// \return    None
mp_obj_t sound_clear_events(mp_obj_t self_in) {
    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_clear_events_obj, sound_clear_events);

/// \method stop()
/// \brief     Stop any running sound play (cannot be resumed).
/// \param     None
/// \return    None.
mp_obj_t sound_stop(mp_obj_t self_in) {
    Codec_Stop();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sound_stop_obj, sound_stop);

/// \method play_tone(frequency, duration)
/// \brief  Play a single tone.
/// \param  frequency - frequency in [Hz], limited to 3 KHz; 0 means silence
/// \param  duration - duration in tenths of a second; 0 means play forever (until "stop")
/// \return None if ok, RuntimeError exception if another sound or recording is already running.
/// \example Play a 440 Hz tone for 1 second
///     sound.play_tone(440, 10)
/// \endexample
mp_obj_t sound_play_tone(mp_obj_t self_in, mp_obj_t freq, mp_obj_t duration) {
    T_ToneNote note;
    int dur = mp_obj_get_int(duration);
    note.freq_Hz = mp_obj_get_float(freq);
    note.duration_ms = (dur > 0) ? (uint32_t)(dur * 100) : 0; // Convert from tenths of a second to milliseconds
    if(Codec_PlayToneMelody(&note, 1) != ESP_OK) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Cannot play"));
        return mp_const_none;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(sound_play_tone_obj, sound_play_tone);

/// \method play_melody(freqs, durations)
/// \brief  Play a melody of up to TONE_MELODY_MAX_NOTES notes; the melody is played up to the end without interruptions.
///         The two lists must have the same length, for instance:
///           play_melody([262, 330, 392], [2, 2, 4])
/// \param  freqs - list (or tuple) of frequencies in [Hz], limited to 3 KHz; 0 means silence (rest)
/// \param  durations - list (or tuple) of durations in tenths of a second; 0 means play forever
///                     (only meaningful for the last note)
/// \return None if ok, RuntimeError exception if another sound or recording is already running,
///         ValueError if the lists are not correctly specified.
/// \example Play a melody of 3 notes: C4, E4, G4
///     sound.play_melody([262, 330, 392], [2, 2, 4])
/// \endexample
STATIC mp_obj_t sound_play_melody(mp_obj_t self_in, mp_obj_t freqs, mp_obj_t durations) {
    T_ToneNote notes[TONE_MELODY_MAX_NOTES];
    size_t freqsLen = 0, durationsLen = 0;
    mp_obj_t *freqsItems = NULL, *durationsItems = NULL;

    if((!mp_obj_is_type(freqs, &mp_type_list) && !mp_obj_is_type(freqs, &mp_type_tuple)) ||
       (!mp_obj_is_type(durations, &mp_type_list) && !mp_obj_is_type(durations, &mp_type_tuple))) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Frequencies and durations must be lists"));
        return mp_const_none;
    }

    mp_obj_get_array(freqs, &freqsLen, &freqsItems);
    mp_obj_get_array(durations, &durationsLen, &durationsItems);

    if(freqsLen != durationsLen) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Frequencies and durations must have the same length"));
        return mp_const_none;
    }

    if((freqsLen == 0) || (freqsLen > TONE_MELODY_MAX_NOTES)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("From 1 to %d notes are expected"), TONE_MELODY_MAX_NOTES);
        return mp_const_none;
    }

    for(size_t i=0; i<freqsLen; i++) {
        int dur = mp_obj_get_int(durationsItems[i]);
        notes[i].freq_Hz = mp_obj_get_float(freqsItems[i]);
        notes[i].duration_ms = (dur > 0) ? (uint32_t)(dur * 100) : 0; // Convert from tenths of a second to milliseconds
    }

    if(Codec_PlayToneMelody(notes, (uint8_t)freqsLen) != ESP_OK) {
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Cannot play"));
        return mp_const_none;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(sound_play_melody_obj, sound_play_melody);

STATIC const mp_rom_map_elem_t sound_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&sound_record_wav_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_mic_volume), MP_ROM_PTR(&sound_get_mic_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_clap_detected), MP_ROM_PTR(&sound_clap_detected_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear_clap_event), MP_ROM_PTR(&sound_clear_clap_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_record_completed), MP_ROM_PTR(&sound_record_is_complete_obj) },
    { MP_ROM_QSTR(MP_QSTR_record_get), MP_ROM_PTR(&sound_record_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_from_file), MP_ROM_PTR(&sound_play_from_file_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_recorded), MP_ROM_PTR(&sound_play_recorded_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_onboard), MP_ROM_PTR(&sound_play_onboard_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_completed), MP_ROM_PTR(&sound_play_is_complete_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&sound_pause_obj) },    
    { MP_ROM_QSTR(MP_QSTR_resume), MP_ROM_PTR(&sound_resume_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_volume), MP_ROM_PTR(&sound_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_volume), MP_ROM_PTR(&sound_save_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear_events), MP_ROM_PTR(&sound_clear_events_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&sound_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_tone), MP_ROM_PTR(&sound_play_tone_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_melody), MP_ROM_PTR(&sound_play_melody_obj) },
};

STATIC MP_DEFINE_CONST_DICT(sound_locals_dict, sound_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    thymio_sound_type,
    MP_QSTR_SOUND,
    MP_TYPE_FLAG_NONE,
    make_new, sound_make_new,
    print, sound_print,
    locals_dict, &sound_locals_dict
    );

