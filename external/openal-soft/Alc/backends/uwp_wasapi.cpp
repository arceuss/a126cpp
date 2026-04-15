/**
 * OpenAL cross platform audio library
 * UWP WASAPI playback backend — only active when WINAPI_FAMILY is not
 * WINAPI_FAMILY_DESKTOP_APP (i.e. Windows Store / UWP targets).
 *
 * Device activation uses ActivateAudioInterfaceAsync + WinRT
 * MediaDevice::GetDefaultAudioRenderId.  The rendering thread and stream
 * configuration are a faithful port of the mmdevapi.c patterns.
 */
#include "config.h"

#include <winapifamily.h>
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP)

/* Pull in audio and WinRT headers before any OpenAL headers so that the
 * C++ ABI COM interface declarations (virtual methods) are in scope.
 * COBJMACROS must NOT be defined here — we use C++ vtable calls. */
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#ifndef _WAVEFORMATEXTENSIBLE_
#include <ks.h>
#include <ksmedia.h>
#endif
#include <roapi.h>
#include <winstring.h>
#include <windows.media.devices.h>
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <new>

/* OpenAL headers are pure C — wrap them so symbols get C linkage in the
 * C++ translation unit.  alu.h is excluded because its ALvoid typedefs
 * conflict with Windows headers in C++ mode; we only need aluMixData. */
extern "C" {
#include "alMain.h"
void aluMixData(ALCdevice *device, ALvoid *buffer, ALsizei size);
void aluHandleDisconnect(ALCdevice *device);
}

using namespace Microsoft::WRL;

/* ---------------------------------------------------------------------------
 * KSDATAFORMAT_SUBTYPE GUIDs — defined as file-local statics to avoid
 * duplicate-symbol conflicts with mmdevapi.c which also DEFINE_GUIDs them.
 * --------------------------------------------------------------------------- */
static const GUID LOCAL_KSDATAFORMAT_SUBTYPE_PCM =
    { 0x00000001, 0x0000, 0x0010,
      { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

static const GUID LOCAL_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT =
    { 0x00000003, 0x0000, 0x0010,
      { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

/* Speaker-mask shorthands (mirror mmdevapi.c) */
#define MONO        SPEAKER_FRONT_CENTER
#define STEREO      (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT)
#define QUAD        (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT)
#define X5DOT1      (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT)
#define X5DOT1SIDE  (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT)
#define X6DOT1      (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_CENTER|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT)
#define X7DOT1      (SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT|SPEAKER_FRONT_CENTER|SPEAKER_LOW_FREQUENCY|SPEAKER_BACK_LEFT|SPEAKER_BACK_RIGHT|SPEAKER_SIDE_LEFT|SPEAKER_SIDE_RIGHT)

/* Per-device state */
typedef struct {
    IAudioClient        *client;
    IAudioRenderClient  *render;
    HANDLE               NotifyEvent;
    volatile UINT32      Padding;
    volatile int         killNow;
    ALvoid              *thread;
} UwpWasapiData;


/* ===========================================================================
 * Async-to-sync activation bridge
 *
 * IActivateAudioInterfaceCompletionHandler fires ActivateCompleted on the
 * WASAPI thread pool.  We implement IAgileObject so the callback is allowed
 * from any apartment without marshalling.
 * =========================================================================== */

class AudioActivationHandler final
    : public IActivateAudioInterfaceCompletionHandler
    , public IAgileObject              /* required for cross-apartment callback */
{
public:
    AudioActivationHandler()
        : m_refCount(1)
        , m_event(CreateEventW(nullptr, FALSE, FALSE, nullptr))
        , m_activateHr(E_PENDING)
        , m_client(nullptr)
    {}

    ~AudioActivationHandler()
    {
        if (m_event)
            CloseHandle(m_event);
        if (m_client)
            m_client->Release();
    }

    /* IUnknown */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override
    {
        if (riid == __uuidof(IActivateAudioInterfaceCompletionHandler) ||
            riid == __uuidof(IAgileObject) ||
            riid == __uuidof(IUnknown))
        {
            *ppv = static_cast<IActivateAudioInterfaceCompletionHandler*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        ULONG rc = InterlockedDecrement(&m_refCount);
        if (rc == 0) delete this;
        return rc;
    }

    /* IActivateAudioInterfaceCompletionHandler */
    STDMETHODIMP ActivateCompleted(
        IActivateAudioInterfaceAsyncOperation *op) override
    {
        HRESULT hrActivate = E_FAIL;
        IUnknown *pUnk = nullptr;
        op->GetActivateResult(&hrActivate, &pUnk);

        m_activateHr = hrActivate;

        if (SUCCEEDED(hrActivate) && pUnk != nullptr)
        {
            /* The activated interface is IAudioClient; QI for explicit type. */
            pUnk->QueryInterface(IID_PPV_ARGS(&m_client));
            pUnk->Release();
        }
        else if (pUnk != nullptr)
        {
            pUnk->Release();
        }

        SetEvent(m_event);
        return S_OK;
    }

    /**
     * Block the calling thread until ActivateCompleted fires.
     * On success, *ppClient receives ownership of a ref-counted IAudioClient.
     * The caller is responsible for Release().
     */
    HRESULT WaitForResult(IAudioClient **ppClient)
    {
        WaitForSingleObjectEx(m_event, INFINITE, FALSE);
        *ppClient = m_client;
        m_client = nullptr;     /* transfer ownership to caller */
        return m_activateHr;
    }

private:
    volatile LONG  m_refCount;
    HANDLE         m_event;
    HRESULT        m_activateHr;
    IAudioClient  *m_client;
};


/* ===========================================================================
 * ActivateDefaultAudioClient
 *
 * Synchronously obtains an IAudioClient for the system default render device
 * using the UWP-legal ActivateAudioInterfaceAsync path.
 * Returns a ref-counted IAudioClient*, or nullptr on any failure.
 * =========================================================================== */

static IAudioClient *ActivateDefaultAudioClient(void)
{
    /* 1. Ask WinRT MediaDevice for the default render device ID string. */
    HSTRING hstrClass = nullptr;
    const wchar_t *kMediaDeviceClass = L"Windows.Media.Devices.MediaDevice";
    HRESULT hr = WindowsCreateString(
        kMediaDeviceClass,
        static_cast<UINT32>(wcslen(kMediaDeviceClass)),
        &hstrClass);
    if (FAILED(hr))
        return nullptr;

    ComPtr<ABI::Windows::Media::Devices::IMediaDeviceStatics> pStatics;
    hr = RoGetActivationFactory(hstrClass,
        IID_PPV_ARGS(pStatics.GetAddressOf()));
    WindowsDeleteString(hstrClass);
    if (FAILED(hr))
    {
        ERR("RoGetActivationFactory(MediaDevice) failed: 0x%08lx\n", hr);
        return nullptr;
    }

    HSTRING hstrDevId = nullptr;
    hr = pStatics->GetDefaultAudioRenderId(
        ABI::Windows::Media::Devices::AudioDeviceRole_Default, &hstrDevId);
    if (FAILED(hr) || hstrDevId == nullptr)
    {
        ERR("GetDefaultAudioRenderId failed: 0x%08lx\n", hr);
        return nullptr;
    }

    /* 2. Asynchronously activate IAudioClient, then wait synchronously. */
    AudioActivationHandler *pHandler =
        new (std::nothrow) AudioActivationHandler();
    if (pHandler == nullptr)
    {
        WindowsDeleteString(hstrDevId);
        return nullptr;
    }

    IActivateAudioInterfaceAsyncOperation *pAsyncOp = nullptr;
    hr = ActivateAudioInterfaceAsync(
        WindowsGetStringRawBuffer(hstrDevId, nullptr),
        __uuidof(IAudioClient),
        nullptr,
        pHandler,
        &pAsyncOp);
    WindowsDeleteString(hstrDevId);

    if (FAILED(hr))
    {
        ERR("ActivateAudioInterfaceAsync failed: 0x%08lx\n", hr);
        pHandler->Release();
        if (pAsyncOp) pAsyncOp->Release();
        return nullptr;
    }
    /* pAsyncOp is only needed to track/cancel the operation; we don't need it. */
    if (pAsyncOp)
        pAsyncOp->Release();

    IAudioClient *pClient = nullptr;
    hr = pHandler->WaitForResult(&pClient);
    pHandler->Release();

    if (FAILED(hr))
    {
        ERR("Audio interface activation failed: 0x%08lx\n", hr);
        return nullptr;
    }

    return pClient;
}


/* ===========================================================================
 * MakeExtensible — identical logic to mmdevapi.c
 * =========================================================================== */

static ALCboolean MakeExtensible(WAVEFORMATEXTENSIBLE *out,
                                 const WAVEFORMATEX *in)
{
    memset(out, 0, sizeof(*out));

    if (in->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        *out = *(const WAVEFORMATEXTENSIBLE*)in;
    }
    else if (in->wFormatTag == WAVE_FORMAT_PCM)
    {
        out->Format = *in;
        out->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        out->Format.cbSize = sizeof(*out) - sizeof(*in);
        if (out->Format.nChannels == 1)
            out->dwChannelMask = MONO;
        else if (out->Format.nChannels == 2)
            out->dwChannelMask = STEREO;
        else
            ERR("Unhandled PCM channel count: %d\n", out->Format.nChannels);
        out->SubFormat = LOCAL_KSDATAFORMAT_SUBTYPE_PCM;
    }
    else if (in->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
    {
        out->Format = *in;
        out->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        out->Format.cbSize = sizeof(*out) - sizeof(*in);
        if (out->Format.nChannels == 1)
            out->dwChannelMask = MONO;
        else if (out->Format.nChannels == 2)
            out->dwChannelMask = STEREO;
        else
            ERR("Unhandled IEEE float channel count: %d\n",
                out->Format.nChannels);
        out->SubFormat = LOCAL_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    }
    else
    {
        ERR("Unhandled format tag: 0x%04x\n", in->wFormatTag);
        return ALC_FALSE;
    }

    return ALC_TRUE;
}


/* ===========================================================================
 * DoReset — configure and initialize the audio stream.
 * Mirrors mmdevapi.c DoReset (lines 332-561) exactly.
 * =========================================================================== */

static HRESULT DoReset(ALCdevice *device)
{
    UwpWasapiData *data = (UwpWasapiData*)device->ExtraData;
    WAVEFORMATEXTENSIBLE OutputType;
    WAVEFORMATEX *wfx = nullptr;
    REFERENCE_TIME min_per, buf_time;
    UINT32 buffer_len, min_len;
    HRESULT hr;

    hr = data->client->GetMixFormat(&wfx);
    if (FAILED(hr))
    {
        ERR("Failed to get mix format: 0x%08lx\n", hr);
        return hr;
    }

    if (!MakeExtensible(&OutputType, wfx))
    {
        CoTaskMemFree(wfx);
        return E_FAIL;
    }
    CoTaskMemFree(wfx);
    wfx = nullptr;

    buf_time = ((REFERENCE_TIME)device->UpdateSize * device->NumUpdates *
                10000000 + device->Frequency - 1) / device->Frequency;

    if (!(device->Flags & DEVICE_FREQUENCY_REQUEST))
        device->Frequency = OutputType.Format.nSamplesPerSec;

    if (!(device->Flags & DEVICE_CHANNELS_REQUEST))
    {
        if (OutputType.Format.nChannels == 1 &&
                OutputType.dwChannelMask == MONO)
            device->FmtChans = DevFmtMono;
        else if (OutputType.Format.nChannels == 2 &&
                OutputType.dwChannelMask == STEREO)
            device->FmtChans = DevFmtStereo;
        else if (OutputType.Format.nChannels == 4 &&
                OutputType.dwChannelMask == QUAD)
            device->FmtChans = DevFmtQuad;
        else if (OutputType.Format.nChannels == 6 &&
                OutputType.dwChannelMask == X5DOT1)
            device->FmtChans = DevFmtX51;
        else if (OutputType.Format.nChannels == 6 &&
                OutputType.dwChannelMask == X5DOT1SIDE)
            device->FmtChans = DevFmtX51Side;
        else if (OutputType.Format.nChannels == 7 &&
                OutputType.dwChannelMask == X6DOT1)
            device->FmtChans = DevFmtX61;
        else if (OutputType.Format.nChannels == 8 &&
                OutputType.dwChannelMask == X7DOT1)
            device->FmtChans = DevFmtX71;
        else
            ERR("Unhandled channel config: %d -- 0x%08lx\n",
                OutputType.Format.nChannels, OutputType.dwChannelMask);
    }

    switch (device->FmtChans)
    {
        case DevFmtMono:
            OutputType.Format.nChannels = 1;
            OutputType.dwChannelMask = MONO;
            break;
        case DevFmtStereo:
            OutputType.Format.nChannels = 2;
            OutputType.dwChannelMask = STEREO;
            break;
        case DevFmtQuad:
            OutputType.Format.nChannels = 4;
            OutputType.dwChannelMask = QUAD;
            break;
        case DevFmtX51:
            OutputType.Format.nChannels = 6;
            OutputType.dwChannelMask = X5DOT1;
            break;
        case DevFmtX51Side:
            OutputType.Format.nChannels = 6;
            OutputType.dwChannelMask = X5DOT1SIDE;
            break;
        case DevFmtX61:
            OutputType.Format.nChannels = 7;
            OutputType.dwChannelMask = X6DOT1;
            break;
        case DevFmtX71:
            OutputType.Format.nChannels = 8;
            OutputType.dwChannelMask = X7DOT1;
            break;
    }

    switch (device->FmtType)
    {
        case DevFmtByte:
            device->FmtType = DevFmtUByte;
            /* fall-through */
        case DevFmtUByte:
            OutputType.Format.wBitsPerSample = 8;
            OutputType.Samples.wValidBitsPerSample = 8;
            OutputType.SubFormat = LOCAL_KSDATAFORMAT_SUBTYPE_PCM;
            break;
        case DevFmtUShort:
            device->FmtType = DevFmtShort;
            /* fall-through */
        case DevFmtShort:
            OutputType.Format.wBitsPerSample = 16;
            OutputType.Samples.wValidBitsPerSample = 16;
            OutputType.SubFormat = LOCAL_KSDATAFORMAT_SUBTYPE_PCM;
            break;
        case DevFmtUInt:
            device->FmtType = DevFmtInt;
            /* fall-through */
        case DevFmtInt:
            OutputType.Format.wBitsPerSample = 32;
            OutputType.Samples.wValidBitsPerSample = 32;
            OutputType.SubFormat = LOCAL_KSDATAFORMAT_SUBTYPE_PCM;
            break;
        case DevFmtFloat:
            OutputType.Format.wBitsPerSample = 32;
            OutputType.Samples.wValidBitsPerSample = 32;
            OutputType.SubFormat = LOCAL_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
            break;
    }

    OutputType.Format.nSamplesPerSec = device->Frequency;
    OutputType.Format.nBlockAlign =
        OutputType.Format.nChannels * OutputType.Format.wBitsPerSample / 8;
    OutputType.Format.nAvgBytesPerSec =
        OutputType.Format.nSamplesPerSec * OutputType.Format.nBlockAlign;

    hr = data->client->IsFormatSupported(
        AUDCLNT_SHAREMODE_SHARED, &OutputType.Format, &wfx);
    if (FAILED(hr))
    {
        ERR("Failed to check format support: 0x%08lx\n", hr);
        hr = data->client->GetMixFormat(&wfx);
    }
    if (FAILED(hr))
    {
        ERR("Failed to find a supported format: 0x%08lx\n", hr);
        return hr;
    }

    if (wfx != nullptr)
    {
        if (!MakeExtensible(&OutputType, wfx))
        {
            CoTaskMemFree(wfx);
            return E_FAIL;
        }
        CoTaskMemFree(wfx);
        wfx = nullptr;

        device->Frequency = OutputType.Format.nSamplesPerSec;

        if (OutputType.Format.nChannels == 1 &&
                OutputType.dwChannelMask == MONO)
            device->FmtChans = DevFmtMono;
        else if (OutputType.Format.nChannels == 2 &&
                OutputType.dwChannelMask == STEREO)
            device->FmtChans = DevFmtStereo;
        else if (OutputType.Format.nChannels == 4 &&
                OutputType.dwChannelMask == QUAD)
            device->FmtChans = DevFmtQuad;
        else if (OutputType.Format.nChannels == 6 &&
                OutputType.dwChannelMask == X5DOT1)
            device->FmtChans = DevFmtX51;
        else if (OutputType.Format.nChannels == 6 &&
                OutputType.dwChannelMask == X5DOT1SIDE)
            device->FmtChans = DevFmtX51Side;
        else if (OutputType.Format.nChannels == 7 &&
                OutputType.dwChannelMask == X6DOT1)
            device->FmtChans = DevFmtX61;
        else if (OutputType.Format.nChannels == 8 &&
                OutputType.dwChannelMask == X7DOT1)
            device->FmtChans = DevFmtX71;
        else
        {
            ERR("Unhandled extensible channels: %d -- 0x%08lx\n",
                OutputType.Format.nChannels, OutputType.dwChannelMask);
            device->FmtChans = DevFmtStereo;
            OutputType.Format.nChannels = 2;
            OutputType.dwChannelMask = STEREO;
        }

        if (IsEqualGUID(OutputType.SubFormat,
                        LOCAL_KSDATAFORMAT_SUBTYPE_PCM))
        {
            if (OutputType.Format.wBitsPerSample == 8)
                device->FmtType = DevFmtUByte;
            else if (OutputType.Format.wBitsPerSample == 16)
                device->FmtType = DevFmtShort;
            else if (OutputType.Format.wBitsPerSample == 32)
                device->FmtType = DevFmtInt;
            else
            {
                device->FmtType = DevFmtShort;
                OutputType.Format.wBitsPerSample = 16;
            }
        }
        else if (IsEqualGUID(OutputType.SubFormat,
                             LOCAL_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
        {
            device->FmtType = DevFmtFloat;
            OutputType.Format.wBitsPerSample = 32;
        }
        else
        {
            ERR("Unhandled format sub-type\n");
            device->FmtType = DevFmtShort;
            OutputType.Format.wBitsPerSample = 16;
            OutputType.SubFormat = LOCAL_KSDATAFORMAT_SUBTYPE_PCM;
        }
        OutputType.Samples.wValidBitsPerSample =
            OutputType.Format.wBitsPerSample;
    }

    SetDefaultWFXChannelOrder(device);

    hr = data->client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        buf_time, 0, &OutputType.Format, nullptr);
    if (FAILED(hr))
    {
        ERR("Failed to initialize audio client: 0x%08lx\n", hr);
        return hr;
    }

    hr = data->client->GetDevicePeriod(&min_per, nullptr);
    if (SUCCEEDED(hr))
    {
        min_len = (UINT32)((min_per * device->Frequency + 10000000 - 1) /
                           10000000);
        /* Round UpdateSize up to the nearest multiple of the hardware period. */
        if (min_len < device->UpdateSize)
            min_len *= (device->UpdateSize + min_len / 2) / min_len;
        hr = data->client->GetBufferSize(&buffer_len);
    }
    if (FAILED(hr))
    {
        ERR("Failed to get audio buffer info: 0x%08lx\n", hr);
        return hr;
    }

    device->UpdateSize = min_len;
    device->NumUpdates = buffer_len / device->UpdateSize;
    if (device->NumUpdates <= 1)
    {
        ERR("Audio client returned buffer_len < period*2; expect break up\n");
        device->NumUpdates = 2;
        device->UpdateSize = buffer_len / device->NumUpdates;
    }

    return hr;
}


/* ===========================================================================
 * Rendering thread — mirrors mmdevapi.c MMDevApiProc (lines 221-290)
 * =========================================================================== */

static ALuint UwpWasapiProc(ALvoid *ptr)
{
    ALCdevice *device = (ALCdevice*)ptr;
    UwpWasapiData *data = (UwpWasapiData*)device->ExtraData;
    UINT32 buffer_len, written;
    ALuint update_size, len;
    BYTE *buffer;
    HRESULT hr;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        ERR("CoInitialize(NULL) failed: 0x%08lx\n", hr);
        ALCdevice_Lock(device);
        aluHandleDisconnect(device);
        ALCdevice_Unlock(device);
        return 0;
    }

    SetRTPriority();

    update_size = device->UpdateSize;
    buffer_len  = update_size * device->NumUpdates;

    while (!data->killNow)
    {
        hr = data->client->GetCurrentPadding(&written);
        if (FAILED(hr))
        {
            ERR("Failed to get padding: 0x%08lx\n", hr);
            ALCdevice_Lock(device);
            aluHandleDisconnect(device);
            ALCdevice_Unlock(device);
            break;
        }
        data->Padding = written;

        len = buffer_len - written;
        if (len < update_size)
        {
            DWORD res = WaitForSingleObjectEx(data->NotifyEvent, 2000, FALSE);
            if (res != WAIT_OBJECT_0)
                ERR("WaitForSingleObjectEx error: 0x%lx\n", res);
            continue;
        }
        len -= len % update_size;

        hr = data->render->GetBuffer(len, &buffer);
        if (SUCCEEDED(hr))
        {
            ALCdevice_Lock(device);
            aluMixData(device, buffer, len);
            data->Padding = written + len;
            ALCdevice_Unlock(device);
            hr = data->render->ReleaseBuffer(len, 0);
        }
        if (FAILED(hr))
        {
            ERR("Failed to buffer data: 0x%08lx\n", hr);
            ALCdevice_Lock(device);
            aluHandleDisconnect(device);
            ALCdevice_Unlock(device);
            break;
        }
    }
    data->Padding = 0;

    CoUninitialize();
    return 0;
}


/* ===========================================================================
 * Backend function table entries
 * =========================================================================== */

static ALCenum UwpWasapiOpenPlayback(ALCdevice *device,
                                     const ALCchar * /*deviceName*/)
{
    /* deviceName is ignored — UWP can only activate the default device via
     * ActivateAudioInterfaceAsync; there is no enumeration API. */
    UwpWasapiData *data =
        (UwpWasapiData*)calloc(1, sizeof(UwpWasapiData));
    if (!data)
        return ALC_OUT_OF_MEMORY;
    device->ExtraData = data;

    data->NotifyEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!data->NotifyEvent)
    {
        free(data);
        device->ExtraData = nullptr;
        return ALC_INVALID_VALUE;
    }

    data->client = ActivateDefaultAudioClient();
    if (!data->client)
    {
        CloseHandle(data->NotifyEvent);
        free(data);
        device->ExtraData = nullptr;
        return ALC_INVALID_VALUE;
    }

    device->DeviceName = _strdup("Default Audio Device");
    return ALC_NO_ERROR;
}

static void UwpWasapiClosePlayback(ALCdevice *device)
{
    UwpWasapiData *data = (UwpWasapiData*)device->ExtraData;

    if (data->client)
    {
        data->client->Release();
        data->client = nullptr;
    }
    if (data->NotifyEvent)
    {
        CloseHandle(data->NotifyEvent);
        data->NotifyEvent = nullptr;
    }

    free(data);
    device->ExtraData = nullptr;
}

static ALCboolean UwpWasapiResetPlayback(ALCdevice *device)
{
    return SUCCEEDED(DoReset(device)) ? ALC_TRUE : ALC_FALSE;
}

static ALCboolean UwpWasapiStartPlayback(ALCdevice *device)
{
    UwpWasapiData *data = (UwpWasapiData*)device->ExtraData;
    void *ptr;
    HRESULT hr;

    ResetEvent(data->NotifyEvent);

    hr = data->client->SetEventHandle(data->NotifyEvent);
    if (FAILED(hr))
    {
        ERR("Failed to set event handle: 0x%08lx\n", hr);
        return ALC_FALSE;
    }

    hr = data->client->Start();
    if (FAILED(hr))
    {
        ERR("Failed to start audio client: 0x%08lx\n", hr);
        return ALC_FALSE;
    }

    hr = data->client->GetService(__uuidof(IAudioRenderClient), &ptr);
    if (FAILED(hr))
    {
        ERR("Failed to get render client: 0x%08lx\n", hr);
        data->client->Stop();
        return ALC_FALSE;
    }
    data->render = (IAudioRenderClient*)ptr;

    data->killNow = 0;
    data->thread  = StartThread(UwpWasapiProc, device);
    if (!data->thread)
    {
        ERR("Failed to start render thread\n");
        data->render->Release();
        data->render = nullptr;
        data->client->Stop();
        return ALC_FALSE;
    }

    return ALC_TRUE;
}

static void UwpWasapiStopPlayback(ALCdevice *device)
{
    UwpWasapiData *data = (UwpWasapiData*)device->ExtraData;

    if (!data->thread)
        return;

    data->killNow = 1;
    StopThread(data->thread);
    data->thread  = nullptr;
    data->killNow = 0;

    data->render->Release();
    data->render = nullptr;

    data->client->Stop();
}

static ALint64 UwpWasapiGetLatency(ALCdevice *device)
{
    UwpWasapiData *data = (UwpWasapiData*)device->ExtraData;
    return (ALint64)data->Padding * 1000000000 / device->Frequency;
}

static const BackendFuncs UwpWasapiFuncs = {
    UwpWasapiOpenPlayback,
    UwpWasapiClosePlayback,
    UwpWasapiResetPlayback,
    UwpWasapiStartPlayback,
    UwpWasapiStopPlayback,
    nullptr,    /* OpenCapture  */
    nullptr,    /* CloseCapture */
    nullptr,    /* StartCapture */
    nullptr,    /* StopCapture  */
    nullptr,    /* CaptureSamples */
    nullptr,    /* AvailableSamples */
    ALCdevice_LockDefault,
    ALCdevice_UnlockDefault,
    UwpWasapiGetLatency
};


/* ===========================================================================
 * Public C-linkage entry points (called by OpenAL32/alc.c backend table)
 * =========================================================================== */

extern "C" {

ALCboolean alcUwpWasapiInit(BackendFuncs *FuncList)
{
    *FuncList = UwpWasapiFuncs;
    return ALC_TRUE;
}

void alcUwpWasapiDeinit(void)
{
    /* Nothing to release at library level; per-device cleanup is in Close. */
}

void alcUwpWasapiProbe(enum DevProbe type)
{
    if (type == ALL_DEVICE_PROBE)
        AppendAllDevicesList("Default Audio Device");
    /* CAPTURE_DEVICE_PROBE: no capture support */
}

} /* extern "C" */

#endif /* WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP */
