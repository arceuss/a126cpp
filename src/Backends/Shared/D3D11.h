#pragma once

// Shared D3D11 state between platform and rendering backends.
// The SDL2+D3D11 platform backend creates the device/context/swap chain;
// the D3D11 rendering backend uses them for drawing.

#include <d3d11_1.h>

namespace D3D11_Shared
{
    ID3D11Device* getDevice();
    ID3D11Device1* getDevice1();
    ID3D11DeviceContext* getContext();
    ID3D11DeviceContext1* getContext1();
    IDXGISwapChain* getSwapChain();
    ID3D11RenderTargetView* getRenderTargetView();
    ID3D11DepthStencilView* getDepthStencilView();

    void setDevice(ID3D11Device* device);
    void setContext(ID3D11DeviceContext* context);
    void setSwapChain(IDXGISwapChain* swapChain);
    void setRenderTargetView(ID3D11RenderTargetView* rtv);
    void setDepthStencilView(ID3D11DepthStencilView* dsv);

    int getBackbufferWidth();
    int getBackbufferHeight();
    void setBackbufferSize(int w, int h);
}
