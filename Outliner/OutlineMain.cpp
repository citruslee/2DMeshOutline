#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "imgui.h"
#include "imgui_impl_dx11.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobj.hpp"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "D3DCompiler.lib")

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 300

struct VERTEX
{
	FLOAT X, Y, Z;
	DirectX::XMFLOAT4 Color;
};

// Define the constant data used to communicate with shaders.
typedef struct ConstantBuffer
{
	DirectX::XMFLOAT4X4 mWorldViewProj;
	DirectX::XMFLOAT4 vSomeVectorThatMayBeNeededByASpecificShader;
	float fSomeFloatThatMayBeNeededByASpecificShader;
	float fTime;
	float fSomeFloatThatMayBeNeededByASpecificShader2;
	float fSomeFloatThatMayBeNeededByASpecificShader3;
} VS_CONSTANT_BUFFER;


IDXGISwapChain *swapchain = nullptr;
ID3D11Device *dev = nullptr;
ID3D11DeviceContext *devcon = nullptr;
ID3D11RenderTargetView *backbuffer = nullptr;
ID3D11InputLayout *pLayout = nullptr;
ID3D11VertexShader *pVS = nullptr;
ID3D11PixelShader *pPS = nullptr;
ID3D11Buffer *pVBuffer = nullptr;
ID3D11Buffer*   g_pConstantBuffer11 = nullptr;
D3D11_VIEWPORT viewport = D3D11_VIEWPORT();

tinyobj::attrib_t attrib;
std::vector<tinyobj::shape_t> shapes;
std::vector<VERTEX> vertices;

WORD oldMouseX, oldMouseY, mouseX, mouseY;
float angle = 0;

void InitD3D(HWND hWnd);
void RenderFrame(void);
void CleanD3D(void);
void InitGraphics(void);
void InitPipeline(void);

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc = WNDCLASSEX();
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";
	RegisterClassEx(&wc);

	RECT wr = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	hWnd = CreateWindowEx(NULL, L"WindowClass", L"Outline - Made by Citrus", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top, NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);


	InitD3D(hWnd);
	ImGui_ImplDX11_Init(hWnd, dev, devcon);

	MSG msg;
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				break;
			}
		}
		RenderFrame();
	}
	CleanD3D();
	return msg.wParam;
}

void InitD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC scd = DXGI_SWAP_CHAIN_DESC();
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	scd.BufferDesc.Width = SCREEN_WIDTH;
	scd.BufferDesc.Height = SCREEN_HEIGHT;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = 4;
	scd.Windowed = TRUE;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION, &scd, &swapchain, &dev, NULL, &devcon);
	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	pBackBuffer->Release();

	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = SCREEN_WIDTH;
	viewport.Height = SCREEN_HEIGHT;

	InitPipeline();
	InitGraphics();
}

void RenderFrame(void)
{
	bool show_test_window = true;
	bool show_another_window = false;
	ImVec4 clear_col = ImColor(114, 144, 154);

	ImGui_ImplDX11_NewFrame();

	{
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Outline", &show_another_window);
		ImGui::Text("Hello");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	devcon->Map(g_pConstantBuffer11, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	VS_CONSTANT_BUFFER* dataPtr = (VS_CONSTANT_BUFFER*)mappedResource.pData;

	static float a = 0.0f;
	a += 0.0001f;

	auto radians = float(3.14f * (angle - 90.0f) / 180.0f);

	// calculate the camera's position
	auto cameraX = 0 + sin(radians)*mouseY;     // multiplying by mouseY makes the
	auto cameraZ = 0 + cos(radians)*mouseY;    // camera get closer/farther away with mouseY
	auto cameraY = 0 + mouseY / 2.0f;


	//dataPtr->mWorldViewProj = view;
	dataPtr->vSomeVectorThatMayBeNeededByASpecificShader = DirectX::XMFLOAT4(1, sin(a), 1, 1);
	dataPtr->fSomeFloatThatMayBeNeededByASpecificShader = 3.0f;
	dataPtr->fTime = 1.0f;
	dataPtr->fSomeFloatThatMayBeNeededByASpecificShader2 = 2.0f;
	dataPtr->fSomeFloatThatMayBeNeededByASpecificShader3 = 4.0f;
	devcon->Unmap(g_pConstantBuffer11, 0);

	float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);
	devcon->RSSetViewports(1, &viewport);
	devcon->ClearRenderTargetView(backbuffer, clearColor);
	devcon->VSSetShader(pVS, 0, 0);
	devcon->IASetInputLayout(pLayout);
	devcon->PSSetShader(pPS, 0, 0);
	devcon->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);
	devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
	devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devcon->Draw(vertices.size(), 0);
	ImGui::Render();
	swapchain->Present(0, 0);
}

void CleanD3D(void)
{
	swapchain->SetFullscreenState(FALSE, NULL);
	pLayout->Release();
	pVS->Release();
	pPS->Release();
	pVBuffer->Release();
	swapchain->Release();
	backbuffer->Release();
	dev->Release();
	devcon->Release();
}

void InitGraphics()
{

	const char *filename = "C:\\Users\\Citrus\\Source\\Repos\\2DMeshOutline\\Outliner\\assets\\testmesh.obj";

	std::string err;
	bool ret =
		tinyobj::LoadObj(&attrib, &shapes, nullptr, &err, filename);
	
	VERTEX OurVertices[] =
	{
		{ 0.0f, 0.5f, 0.0f, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ 0.45f, -0.5, 0.0f,  DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ -0.45f, -0.5f, 0.0f,  DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};

	vertices.push_back(OurVertices[0]);
	vertices.push_back(OurVertices[1]);
	vertices.push_back(OurVertices[2]);

	/*for (int i = 0; i < attrib.vertices.size() / 3; i++)
	{
		VERTEX v = VERTEX();
		v.X = attrib.vertices[3 * i] * 0.001f;
		v.Y = attrib.vertices[3 * i + 1] * 0.001f;
		v.Z = attrib.vertices[3 * i + 2];

		v.Color.x = 1.0f;//attrib.normals[i];
		v.Color.y = 1.0f;//attrib.normals[i + 1];
		v.Color.z = 1.0f;//attrib.normals[i + 2];
		v.Color.w = 1.0f;

		vertices.push_back(v);
	}*/
	

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX) * vertices.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	dev->CreateBuffer(&bd, NULL, &pVBuffer); 

	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, vertices.data(), vertices.size() * sizeof(VERTEX));
	devcon->Unmap(pVBuffer, NULL);
}

void InitPipeline()
{
	ID3D10Blob *VS, *PS;
	D3DCompileFromFile(L"VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", 0, 0, &VS, nullptr);
	D3DCompileFromFile(L"PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", 0, 0, &PS, nullptr);

	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
	dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
	
	

	// Supply the vertex shader constant data.
	VS_CONSTANT_BUFFER cbuffer = VS_CONSTANT_BUFFER();
	//cbuffer.mWorldViewProj = { ... };
	cbuffer.vSomeVectorThatMayBeNeededByASpecificShader = DirectX::XMFLOAT4(1, 1, 0, 1);
	cbuffer.fSomeFloatThatMayBeNeededByASpecificShader = 3.0f;
	cbuffer.fTime = 1.0f;
	cbuffer.fSomeFloatThatMayBeNeededByASpecificShader2 = 2.0f;
	cbuffer.fSomeFloatThatMayBeNeededByASpecificShader3 = 4.0f;

	// Fill in a buffer description.
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(VS_CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &cbuffer;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// Create the buffer.
	auto hr = dev->CreateBuffer(&cbDesc, &InitData,	&g_pConstantBuffer11);

	if (FAILED(hr))
	{
		exit(0);
	}

	// Set the buffer.
	
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplDX11_WndProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}
	switch (message)
	{
	case WM_SIZE:
		if (dev != NULL && wParam != SIZE_MINIMIZED)
		{
			ImGui_ImplDX11_InvalidateDeviceObjects();
			swapchain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			ImGui_ImplDX11_CreateDeviceObjects();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_MOUSEMOVE:
		// save old mouse coordinates
		oldMouseX = mouseX;
		oldMouseY = mouseY;

		// get mouse coordinates from Windows
		mouseX = LOWORD(lParam);
		mouseY = HIWORD(lParam);

		// these lines limit the camera's range
		/*if (mouseY < 200)
			mouseY = 200;
		if (mouseY > 450)
			mouseY = 450;*/

		if ((mouseX - oldMouseX) > 0)             // mouse moved to the right
			angle += 3.0f;
		else if ((mouseX - oldMouseX) < 0)     // mouse moved to the left
			angle -= 3.0f;

		return 0;
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}