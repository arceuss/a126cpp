/**
 * OpenAL cross platform audio library
 * Copyright (C) 2010 by Chris Robinson
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <switch.h>

#include "alMain.h"
#include "alu.h"


/* Number of buffer slots handed to the audout service. This is also written
 * into Device->NumUpdates, which ALc.c clamps to the range [2,16]. Four rather
 * than two: chunk generation can stall the mixer thread long enough to drain a
 * shorter queue, which is audible as a gap.
 */
#define SWITCH_NUM_BUFFERS 4

/* Guard rails on the per-buffer size. The lower bound keeps the service from
 * being fed pathologically short updates, the upper bound bounds the
 * allocation when a config file asks for a very large update.
 */
#define SWITCH_MIN_UPDATE_FRAMES 256
#define SWITCH_MAX_UPDATE_FRAMES 4096

/* audout requires both the sample buffer pointer and the sample buffer size
 * to be aligned to 0x1000 bytes. There is no libnx macro for this constant;
 * the audren mempool alignment must not be used here.
 */
#define SWITCH_BUFFER_ALIGN 0x1000


typedef struct {
    volatile int killNow;
    ALvoid *thread;

    void *rawbuf;
    AudioOutBuffer buffers[SWITCH_NUM_BUFFERS];

    ALuint frameSize;
    ALuint dataSize;
    ALuint alignedSize;
} switch_data;


static const ALCchar switchDevice[] = "Switch Audio Out";

/* audout only ever hands back a buffer this backend appended, so a foreign
 * pointer means the service state is not what we think it is.
 */
static ALCboolean switch_owns_buffer(const switch_data *data,
                                     const AudioOutBuffer *buffer)
{
    ALuint i;

    for(i = 0;i < SWITCH_NUM_BUFFERS;i++)
    {
        if(buffer == &data->buffers[i])
            return ALC_TRUE;
    }
    return ALC_FALSE;
}


static ALuint SwitchProc(ALvoid *ptr)
{
    ALCdevice *Device = (ALCdevice*)ptr;
    switch_data *data = (switch_data*)Device->ExtraData;
    AudioOutBuffer *released;
    u32 released_count;
    ALuint i;

    /* Prime every slot so the service always has work queued. */
    for(i = 0;i < SWITCH_NUM_BUFFERS;i++)
    {
        aluMixData(Device, data->buffers[i].buffer, Device->UpdateSize);
        audoutAppendAudioOutBuffer(&data->buffers[i]);
    }

    while(!data->killNow && Device->Connected)
    {
        released = NULL;
        released_count = 0;

        /* AUDOUT_MAX_DELAY (1 second) rather than UINT64_MAX: StopThread is a
         * blocking pthread_join, so an infinite wait here would deadlock
         * switch_stop_playback if the buffer event never fires again. A finite
         * timeout keeps killNow observable.
         */
        if(R_FAILED(audoutWaitPlayFinish(&released, &released_count,
                                         AUDOUT_MAX_DELAY)))
        {
            ERR("audoutWaitPlayFinish failed\n");
            ALCdevice_Lock(Device);
            aluHandleDisconnect(Device);
            ALCdevice_Unlock(Device);
            break;
        }

        /* audoutWaitPlayFinish clears the event and polls for released
         * buffers first, so it can legitimately succeed with nothing
         * released. Retry rather than dereferencing a NULL buffer.
         */
        if(!released_count || !released)
            continue;

        if(!switch_owns_buffer(data, released))
        {
            ERR("audout returned an unknown buffer\n");
            ALCdevice_Lock(Device);
            aluHandleDisconnect(Device);
            ALCdevice_Unlock(Device);
            break;
        }

        aluMixData(Device, released->buffer, Device->UpdateSize);
        audoutAppendAudioOutBuffer(released);
    }

    return 0;
}


static ALCenum switch_open_playback(ALCdevice *device, const ALCchar *deviceName)
{
    switch_data *data;

    if(!deviceName)
        deviceName = switchDevice;
    else if(strcmp(deviceName, switchDevice) != 0)
        return ALC_INVALID_VALUE;

    data = (switch_data*)calloc(1, sizeof(*data));
    if(!data)
        return ALC_OUT_OF_MEMORY;

    /* audoutInitialize already opens the default audio output device. */
    if(R_FAILED(audoutInitialize()))
    {
        ERR("audoutInitialize failed\n");
        free(data);
        return ALC_INVALID_VALUE;
    }

    /* The mixer below is hard-wired to interleaved stereo 16-bit. Fail loudly
     * instead of emitting audio in the wrong format.
     */
    if(audoutGetPcmFormat() != PcmFormat_Int16 || audoutGetChannelCount() != 2)
    {
        ERR("Unsupported audout format (pcm %d, channels %d)\n",
            (int)audoutGetPcmFormat(), (int)audoutGetChannelCount());
        audoutExit();
        free(data);
        return ALC_INVALID_VALUE;
    }

    device->DeviceName = strdup(deviceName);
    device->ExtraData = data;
    return ALC_NO_ERROR;
}

static void switch_close_playback(ALCdevice *device)
{
    switch_data *data = (switch_data*)device->ExtraData;

    audoutExit();

    free(data);
    device->ExtraData = NULL;
}

static ALCboolean switch_reset_playback(ALCdevice *device)
{
    ALuint oldFrequency = device->Frequency ? device->Frequency : 44100;
    ALuint oldUpdateSize = device->UpdateSize ? device->UpdateSize : 1024;
    ALuint oldNumUpdates = device->NumUpdates ? device->NumUpdates : 4;
    ALuint64 oldTotalFrames, newTotalFrames;
    ALuint updateFrames;

    /* The audout device format is fixed, so force it here. 1.15.1 resamples
     * every source to the device rate unconditionally, so a 44.1kHz buffer
     * still plays correctly at 48kHz. No libnx state is touched: reset may be
     * called repeatedly.
     *
     * Preserve the requested total buffering duration rather than a per-buffer
     * size, because the buffer count is being changed as well as the rate.
     */
    oldTotalFrames = (ALuint64)oldUpdateSize * oldNumUpdates;
    newTotalFrames = (oldTotalFrames * audoutGetSampleRate() +
                      oldFrequency - 1) / oldFrequency;
    updateFrames = (ALuint)((newTotalFrames + SWITCH_NUM_BUFFERS - 1) /
                            SWITCH_NUM_BUFFERS);

    if(updateFrames < SWITCH_MIN_UPDATE_FRAMES)
        updateFrames = SWITCH_MIN_UPDATE_FRAMES;
    else if(updateFrames > SWITCH_MAX_UPDATE_FRAMES)
        updateFrames = SWITCH_MAX_UPDATE_FRAMES;

    device->UpdateSize = updateFrames;
    device->NumUpdates = SWITCH_NUM_BUFFERS;

    device->Frequency = audoutGetSampleRate();
    device->FmtChans = DevFmtStereo;
    device->FmtType = DevFmtShort;

    SetDefaultWFXChannelOrder(device);

    return ALC_TRUE;
}

static ALCboolean switch_start_playback(ALCdevice *device)
{
    switch_data *data = (switch_data*)device->ExtraData;
    ALuint i;

    data->frameSize = FrameSizeFromDevFmt(device->FmtChans, device->FmtType);
    data->dataSize = device->UpdateSize * data->frameSize;
    data->alignedSize = (data->dataSize + (SWITCH_BUFFER_ALIGN-1)) &
                        ~(SWITCH_BUFFER_ALIGN-1);

    /* Ordinary heap memory is correct here: audout transfers the samples over
     * IPC, so there is no mempool to register and no cache maintenance to do.
     */
    data->rawbuf = memalign(SWITCH_BUFFER_ALIGN,
                            data->alignedSize * SWITCH_NUM_BUFFERS);
    if(!data->rawbuf)
    {
        ERR("Buffer memalign failed\n");
        return ALC_FALSE;
    }
    memset(data->rawbuf, 0, data->alignedSize * SWITCH_NUM_BUFFERS);

    for(i = 0;i < SWITCH_NUM_BUFFERS;i++)
    {
        data->buffers[i].next = NULL;
        data->buffers[i].buffer = (u8*)data->rawbuf + i*data->alignedSize;
        data->buffers[i].buffer_size = data->alignedSize;
        data->buffers[i].data_size = data->dataSize;
        data->buffers[i].data_offset = 0;
    }

    if(R_FAILED(audoutStartAudioOut()))
    {
        ERR("audoutStartAudioOut failed\n");
        free(data->rawbuf);
        data->rawbuf = NULL;
        return ALC_FALSE;
    }

    data->thread = StartThread(SwitchProc, device);
    if(data->thread == NULL)
    {
        audoutStopAudioOut();
        free(data->rawbuf);
        data->rawbuf = NULL;
        return ALC_FALSE;
    }

    return ALC_TRUE;
}

static void switch_stop_playback(ALCdevice *device)
{
    switch_data *data = (switch_data*)device->ExtraData;

    if(!data->thread)
        return;

    /* The order below is load-bearing: the thread must be joined before
     * audoutStopAudioOut so no append races the stop, and the sample memory
     * must only be freed after the stop, so the service is no longer holding
     * any queued buffer descriptors.
     */
    data->killNow = 1;
    StopThread(data->thread);
    data->thread = NULL;

    data->killNow = 0;

    audoutStopAudioOut();

    free(data->rawbuf);
    data->rawbuf = NULL;
}


static const BackendFuncs switch_funcs = {
    switch_open_playback,
    switch_close_playback,
    switch_reset_playback,
    switch_start_playback,
    switch_stop_playback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ALCdevice_LockDefault,
    ALCdevice_UnlockDefault,
    ALCdevice_GetLatencyDefault
};

ALCboolean alc_switch_init(BackendFuncs *func_list)
{
    *func_list = switch_funcs;
    return ALC_TRUE;
}

void alc_switch_deinit(void)
{
}

void alc_switch_probe(enum DevProbe type)
{
    /* Deliberately does not touch audout: probing can run while a device is
     * already open.
     */
    switch(type)
    {
        case ALL_DEVICE_PROBE:
            AppendAllDevicesList(switchDevice);
            break;
        case CAPTURE_DEVICE_PROBE:
            break;
    }
}
