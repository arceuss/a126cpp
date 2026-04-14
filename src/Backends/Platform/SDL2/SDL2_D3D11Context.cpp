// SDL2 platform backend - D3D11 context creation
// Replaces SDL2_GLContext.cpp when using the D3D11 rendering backend.
// Creates SDL2 window (no OpenGL), extracts HWND, creates D3D11 device/swap chain.

#include "lwjgl/GLContext.h"
#include "Backends/Shared/SDL2.h"
#include "Backends/Shared/D3D11.h"

#include <iostream>
#include <stdexcept>
#include <string>

#include <d3d11_1.h>
#include <dxgi.h>

#include "external/SDLException.h"

#include "SDL.h"
#include "SDL_syswm.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#define IDI_ICON1 1
#endif

// D3D11_Shared storage
namespace D3D11_Shared
{
    static ID3D11Device* s_device = nullptr;
    static ID3D11DeviceContext* s_context = nullptr;
    static IDXGISwapChain* s_swapChain = nullptr;
    static ID3D11RenderTargetView* s_rtv = nullptr;
    static ID3D11DepthStencilView* s_dsv = nullptr;
    static int s_bbWidth = 0;
    static int s_bbHeight = 0;

    ID3D11Device* getDevice() { return s_device; }
    ID3D11Device1* getDevice1()
    {
        ID3D11Device1* d1 = nullptr;
        if (s_device)
            s_device->QueryInterface(__uuidof(ID3D11Device1), (void**)&d1);
        return d1;
    }
    ID3D11DeviceContext* getContext() { return s_context; }
    ID3D11DeviceContext1* getContext1()
    {
        ID3D11DeviceContext1* c1 = nullptr;
        if (s_context)
            s_context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&c1);
        return c1;
    }
    IDXGISwapChain* getSwapChain() { return s_swapChain; }
    ID3D11RenderTargetView* getRenderTargetView() { return s_rtv; }
    ID3D11DepthStencilView* getDepthStencilView() { return s_dsv; }

    void setDevice(ID3D11Device* device) { s_device = device; }
    void setContext(ID3D11DeviceContext* context) { s_context = context; }
    void setSwapChain(IDXGISwapChain* swapChain) { s_swapChain = swapChain; }
    void setRenderTargetView(ID3D11RenderTargetView* rtv) { s_rtv = rtv; }
    void setDepthStencilView(ID3D11DepthStencilView* dsv) { s_dsv = dsv; }

    int getBackbufferWidth() { return s_bbWidth; }
    int getBackbufferHeight() { return s_bbHeight; }
    void setBackbufferSize(int w, int h) { s_bbWidth = w; s_bbHeight = h; }
}

// SDL2_Shared storage
namespace SDL2_Shared
{
    static SDL_Window* s_window = nullptr;

    SDL_Window* getWindow() { return s_window; }
    SDL_GLContext getGLContext() { return nullptr; } // No GL context in D3D11 mode
    void setWindow(SDL_Window* window) { s_window = window; }
    void setGLContext(SDL_GLContext) {} // No-op
}

namespace
{

static bool s_vsyncEnabled = false;

static void createRenderTargetView()
{
    ID3D11Texture2D* backBuffer = nullptr;
    D3D11_Shared::s_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (!backBuffer) return;

    D3D11_Shared::s_device->CreateRenderTargetView(backBuffer, nullptr, &D3D11_Shared::s_rtv);

    D3D11_TEXTURE2D_DESC bbDesc;
    backBuffer->GetDesc(&bbDesc);
    D3D11_Shared::s_bbWidth = bbDesc.Width;
    D3D11_Shared::s_bbHeight = bbDesc.Height;

    backBuffer->Release();
}

static void createDepthStencilView()
{
    D3D11_TEXTURE2D_DESC dsDesc = {};
    dsDesc.Width = D3D11_Shared::s_bbWidth;
    dsDesc.Height = D3D11_Shared::s_bbHeight;
    dsDesc.MipLevels = 1;
    dsDesc.ArraySize = 1;
    dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsDesc.SampleDesc.Count = 1;
    dsDesc.Usage = D3D11_USAGE_DEFAULT;
    dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* dsTex = nullptr;
    D3D11_Shared::s_device->CreateTexture2D(&dsDesc, nullptr, &dsTex);
    if (!dsTex) return;

    D3D11_Shared::s_device->CreateDepthStencilView(dsTex, nullptr, &D3D11_Shared::s_dsv);
    dsTex->Release();
}

} // anonymous namespace

namespace lwjgl
{
namespace GLContext
{

namespace detail
{

class D3D11Context
{
private:
    SDL_Window* window = nullptr;
    GLCapabilities capabilities;
    int lastWidth = 0;
    int lastHeight = 0;

public:
    D3D11Context()
    {
        // Create SDL window without OpenGL flag
        window = SDL_CreateWindow("Minecraft Alpha v1.2.6",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            854, 480,
            SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
        if (window == nullptr)
            throw SDLException();

        // Load and set window icon (same as GL path)
#ifdef _WIN32
        {
            HICON hIcon = NULL;
            bool fromResource = false;

            hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
            if (hIcon != NULL)
            {
                fromResource = true;
            }
            else
            {
                const char* iconPaths[] = {
                    "src/mc.ico",
                    "mc.ico",
                    "../src/mc.ico",
                    "../../src/mc.ico"
                };

                for (int i = 0; i < 4 && hIcon == NULL; i++)
                {
                    hIcon = (HICON)LoadImageA(NULL, iconPaths[i], IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
                }
            }

            if (hIcon != NULL)
            {
                ICONINFO iconInfo;
                if (GetIconInfo(hIcon, &iconInfo))
                {
                    BITMAP bmp;
                    if (GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp))
                    {
                        int width = bmp.bmWidth;
                        int height = bmp.bmHeight;

                        SDL_Surface* iconSurface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_BGRA32);
                        if (iconSurface != nullptr)
                        {
                            HDC hDC = CreateCompatibleDC(NULL);
                            HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, iconInfo.hbmColor);

                            BITMAPINFO bmi;
                            ZeroMemory(&bmi, sizeof(BITMAPINFO));
                            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                            bmi.bmiHeader.biWidth = width;
                            bmi.bmiHeader.biHeight = -height;
                            bmi.bmiHeader.biPlanes = 1;
                            bmi.bmiHeader.biBitCount = 32;
                            bmi.bmiHeader.biCompression = BI_RGB;

                            if (GetDIBits(hDC, iconInfo.hbmColor, 0, height, iconSurface->pixels, &bmi, DIB_RGB_COLORS))
                            {
                                SDL_SetWindowIcon(window, iconSurface);
                            }

                            SelectObject(hDC, hOldBmp);
                            DeleteDC(hDC);
                            SDL_FreeSurface(iconSurface);
                        }
                    }

                    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
                    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
                }

                if (!fromResource)
                {
                    DestroyIcon(hIcon);
                }
            }
        }
#endif

        // Extract HWND from SDL window
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (!SDL_GetWindowWMInfo(window, &wmInfo))
            throw std::runtime_error("SDL_GetWindowWMInfo failed: " + std::string(SDL_GetError()));

        HWND hwnd = wmInfo.info.win.window;

        // Create D3D11 device and swap chain
        DXGI_SWAP_CHAIN_DESC scDesc = {};
        scDesc.BufferCount = 1;
        scDesc.BufferDesc.Width = 854;
        scDesc.BufferDesc.Height = 480;
        scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scDesc.BufferDesc.RefreshRate.Numerator = 0;
        scDesc.BufferDesc.RefreshRate.Denominator = 1;
        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.OutputWindow = hwnd;
        scDesc.SampleDesc.Count = 1;
        scDesc.SampleDesc.Quality = 0;
        scDesc.Windowed = TRUE;
        scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel;
        D3D_FEATURE_LEVEL requestedFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        UINT createFlags = 0;
#ifdef MC_DEBUG_D3D11
        createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        IDXGISwapChain* swapChain = nullptr;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createFlags,
            &requestedFeatureLevel, 1,
            D3D11_SDK_VERSION,
            &scDesc,
            &swapChain,
            &device,
            &featureLevel,
            &context
        );

        if (FAILED(hr))
            throw std::runtime_error("D3D11CreateDeviceAndSwapChain failed");

        // Store in shared state
        D3D11_Shared::s_device = device;
        D3D11_Shared::s_context = context;
        D3D11_Shared::s_swapChain = swapChain;

        // Create render target view
        createRenderTargetView();

        // Create depth stencil view
        createDepthStencilView();

        // Store window in shared state
        SDL2_Shared::setWindow(window);

        SDL_GetWindowSize(window, &lastWidth, &lastHeight);

        std::cout << "D3D11 initialized (feature level " << std::hex << featureLevel << std::dec << ")" << std::endl;
    }

    SDL_Window* getWindow() const { return window; }
    const GLCapabilities& getCapabilities() const { return capabilities; }

    void swapBuffers()
    {
		D3D11_Shared::s_swapChain->Present(s_vsyncEnabled ? 1 : 0, 0);

        // Check for window resize
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        if (w != lastWidth || h != lastHeight)
        {
            lastWidth = w;
            lastHeight = h;

            // Release old views
            if (D3D11_Shared::s_rtv) { D3D11_Shared::s_rtv->Release(); D3D11_Shared::s_rtv = nullptr; }
            if (D3D11_Shared::s_dsv) { D3D11_Shared::s_dsv->Release(); D3D11_Shared::s_dsv = nullptr; }

            // Clear any references to the old back buffer
            D3D11_Shared::s_context->OMSetRenderTargets(0, nullptr, nullptr);

            // Resize swap chain
            D3D11_Shared::s_swapChain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);

            // Recreate views
            createRenderTargetView();
            createDepthStencilView();
        }
    }
};

static D3D11Context& getContext()
{
    static D3D11Context context;
    return context;
}

SDL_Window* getWindow()
{
    return getContext().getWindow();
}

SDL_GLContext getGLContext()
{
    return nullptr; // No GL context
}

void swapBuffers()
{
    getContext().swapBuffers();
}

void setVSyncEnabled(bool enabled)
{
	s_vsyncEnabled = enabled;
}

} // namespace detail

void instantiate()
{
    detail::getContext();
}

const detail::GLCapabilities& getCapabilities()
{
    return detail::getContext().getCapabilities();
}

} // namespace GLContext
} // namespace lwjgl
