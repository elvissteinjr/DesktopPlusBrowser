#include "D3DManager.h"

#include "openvr.h"

static D3DManager g_D3DManager;

D3DManager& D3DManager::Get()
{
    return g_D3DManager;
}

bool D3DManager::CreateDevice()
{
    //Get the adapter recommended by OpenVR if it's loaded
    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter_ptr_vr;

    if (vr::VRSystem() != nullptr)
    {
        Microsoft::WRL::ComPtr<IDXGIFactory1> factory_ptr;
        int32_t vr_gpu_id;
        vr::VRSystem()->GetDXGIOutputInfo(&vr_gpu_id);

        HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory_ptr);
        if (!FAILED(hr))
        {
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter_ptr;
            UINT i = 0;

            while (factory_ptr->EnumAdapters(i, &adapter_ptr) != DXGI_ERROR_NOT_FOUND)
            {
                if ((int32_t)i == vr_gpu_id)
                {
                    adapter_ptr_vr = adapter_ptr;
                    break;
                }

                ++i;
            }
        }
    }

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    Microsoft::WRL::ComPtr<ID3D11Device> d3d_device;

    //No swap chain needed for VR
    if (adapter_ptr_vr != nullptr)
    {
        if (D3D11CreateDevice(adapter_ptr_vr.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &d3d_device, &featureLevel, &m_DeviceContext) != S_OK)
        {
            return false;
        }
    }
    else //Still try /something/, but it probably won't work
    {
        if (D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevelArray, 2, D3D11_SDK_VERSION, &d3d_device, &featureLevel, &m_DeviceContext) != S_OK)
        {
            return false;
        }
    }

    HRESULT res = d3d_device.As(&m_Device);

    if (FAILED(res))
        return false;

    return true;
}

void D3DManager::DestroyDevice()
{
    m_Device.Reset();
    m_DeviceContext.Reset();
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> D3DManager::CreateOverlayTexture(UINT width, UINT height)
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;

    if (m_Device == nullptr)
        return tex;

    D3D11_TEXTURE2D_DESC desc = {0};
    desc.Width            = width;
    desc.Height           = height;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;
    desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags   = 0;
    desc.MiscFlags        = D3D11_RESOURCE_MISC_SHARED;

    m_Device->CreateTexture2D(&desc, nullptr, &tex);

    return tex;
}

ID3D11Device1* D3DManager::GetDevice() const
{
    return m_Device.Get();
}

ID3D11DeviceContext* D3DManager::GetDeviceContext() const
{
    return m_DeviceContext.Get();
}
