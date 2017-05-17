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

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600

struct VERTEX
{
	FLOAT X, Y, Z;
	DirectX::XMFLOAT4 Color;
};

IDXGISwapChain *swapchain;
ID3D11Device *dev;
ID3D11DeviceContext *devcon;
ID3D11RenderTargetView *backbuffer;
ID3D11InputLayout *pLayout;
ID3D11VertexShader *pVS; 
ID3D11PixelShader *pPS;
ID3D11Buffer *pVBuffer;
D3D11_VIEWPORT viewport;

tinyobj::attrib_t attrib;
std::vector<tinyobj::shape_t> shapes;
std::vector<VERTEX> vertices;



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

	float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);
	devcon->RSSetViewports(1, &viewport);
	devcon->ClearRenderTargetView(backbuffer, clearColor);
	devcon->VSSetShader(pVS, 0, 0);
	devcon->IASetInputLayout(pLayout);
	devcon->PSSetShader(pPS, 0, 0);
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
	
	
	for (int i = 0; i < attrib.vertices.size() / 3; i++)
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
	}
	

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
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}