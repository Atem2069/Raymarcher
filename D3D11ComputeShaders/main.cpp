#include <iostream>

#define GLFW_EXPOSE_NATIVE_WIN32
#define WIDTH 640
#define HEIGHT 640

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

struct vec3
{
	vec3(float x, float y, float z) : x(x), y(y), z(z) {}
	float x;
	float y; 
	float z;
};

struct vec2
{
	vec2(float x, float y) : x(x), y(y) {}
	float x;
	float y;
};

struct Vertex
{
	Vertex(vec3 position, vec2 uv) : position(position), uv(uv) {}
	vec3 position;
	vec2 uv;
};
int main()
{
	if (!glfwInit())
	{
		std::cout << "Failed to init glfw.. " << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* m_window = glfwCreateWindow(WIDTH,HEIGHT, "Test", nullptr, nullptr);;
	if (!m_window)
	{
		std::cout << "Failed to create window.. " << std::endl;
		return -1;
	}

	HWND hwnd = glfwGetWin32Window(m_window);

	IDXGIFactory* m_factory;
	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&m_factory);

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_deviceContext;
	D3D_FEATURE_LEVEL featureLevels[1] = { D3D_FEATURE_LEVEL_11_0 };
	HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_DEBUG, featureLevels, 1, D3D11_SDK_VERSION, &m_device, nullptr, &m_deviceContext);;
	if (FAILED(result))
	{
		std::cout << "Failed to create D3D11 Device." << std::endl;
		return -1;
	}

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.Width = WIDTH;
	swapChainDesc.BufferDesc.Height = HEIGHT;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Flags = 0;
	swapChainDesc.OutputWindow = hwnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Windowed = TRUE;

	IDXGISwapChain* m_swapChain;
	result = m_factory->CreateSwapChain(m_device, &swapChainDesc, &m_swapChain);
	if (FAILED(result))
	{
		std::cout << "Failed to create swapchain. " << std::endl;
		return -1;
	}

	ID3D11Texture2D* m_backBuffer;
	m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&m_backBuffer);

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	ID3D11RenderTargetView* m_rtv;
	result = m_device->CreateRenderTargetView(m_backBuffer, &rtvDesc, &m_rtv);
	//m_backBuffer->Release();
	if (FAILED(result))
	{
		std::cout << "Failed to create rendertarget view." << std::endl;
		return -1;
	}

	//Making compute shader
	ID3D10Blob* computeShaderCode;
	result = D3DCompileFromFile(L"Shaders\\compute.hlsl", nullptr, nullptr, "main", "cs_5_0", 0, 0, &computeShaderCode, nullptr);
	if (FAILED(result))
	{
		std::cout << "Failed to compile compute shader. " << std::endl;
		return -1;
	}

	ID3D11ComputeShader* m_computeShader;
	result = m_device->CreateComputeShader(computeShaderCode->GetBufferPointer(), computeShaderCode->GetBufferSize(), nullptr, &m_computeShader);
	if (FAILED(result))
	{
		std::cout << "Failed to create compute shader. " << std::endl;
		return -1;
	}

	//Basic random texture which is shader resource and UAV
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Width = WIDTH;
	texDesc.Height = HEIGHT;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D* m_texture;
	result = m_device->CreateTexture2D(&texDesc, nullptr, &m_texture);
	if (FAILED(result))
	{
		std::cout << "Failed to create blank texture. " << std::endl;
		return -1;
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = texDesc.Format;
	uavDesc.Texture2D.MipSlice = 0;

	ID3D11UnorderedAccessView* m_textureUAV;
	result = m_device->CreateUnorderedAccessView(m_texture, &uavDesc, &m_textureUAV);
	if (FAILED(result))
	{
		std::cout << "Failed to create UAV. " << std::endl;
		return -1;
	}

	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		//Insert compute stuff here
		m_deviceContext->CSSetShader(m_computeShader, nullptr, 0);
		m_deviceContext->CSSetUnorderedAccessViews(0, 1, &m_textureUAV, nullptr);
		m_deviceContext->Dispatch(WIDTH/8, HEIGHT/8, 1);
		m_deviceContext->CopyResource(m_backBuffer, m_texture);
		m_swapChain->Present(1, 0);
	}

	m_device->Release();
	m_swapChain->Release();

	return 0;
}