#pragma once

#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>

class D3DManager
{
    private:
        Microsoft::WRL::ComPtr<ID3D11Device1> m_Device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_DeviceContext;

    public:
        static D3DManager& Get();

        bool CreateDevice();
        void DestroyDevice();
        Microsoft::WRL::ComPtr<ID3D11Texture2D> CreateOverlayTexture(UINT width, UINT height);

        ID3D11Device1* GetDevice() const;
        ID3D11DeviceContext* GetDeviceContext() const;
};
