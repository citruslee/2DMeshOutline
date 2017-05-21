#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <unordered_map>
#include "imgui.h"
#include "imgui_impl_dx11.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobj.hpp"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "D3DCompiler.lib")

#define SCREEN_WIDTH  1600
#define SCREEN_HEIGHT 900

struct VERTEX
{
	FLOAT x, y, z;
	DirectX::XMFLOAT4 Color;
};

// Define the constant data used to communicate with shaders.
typedef struct ConstantBuffer
{
	DirectX::XMMATRIX mWorldViewProj;
	DirectX::XMFLOAT4 vSomeVectorThatMayBeNeededByASpecificShader;
	float scale;
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
ID3D11Buffer *pIBuffer = nullptr;
ID3D11Buffer*   g_pConstantBuffer11 = nullptr;
D3D11_VIEWPORT viewport = D3D11_VIEWPORT();

ID3D11RasterizerState* wireframe = nullptr;
ID3D11RasterizerState* fillsolid = nullptr;

DirectX::XMMATRIX WVP;
DirectX::XMMATRIX camView;
DirectX::XMMATRIX camProjection;
DirectX::XMVECTOR camPosition;
DirectX::XMVECTOR camTarget;
DirectX::XMVECTOR camUp;
DirectX::XMVECTOR DefaultForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
DirectX::XMVECTOR DefaultRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
DirectX::XMVECTOR camForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
DirectX::XMVECTOR camRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

float moveLeftRight = 0.0f;
float moveBackForward = 0.0f;

float camYaw = 0.0f;
float camPitch = 0.0f;

DirectX::XMMATRIX camRotationMatrix;

tinyobj::attrib_t attrib;
std::vector<tinyobj::shape_t> shapes;
std::vector<VERTEX> vertices;
std::vector<int> indices;

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

using namespace DirectX;

void UpdateCamera()
{
	camRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);
	camTarget = DirectX::XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	camTarget = DirectX::XMVector3Normalize(camTarget);
	camRight = DirectX::XMVector3TransformCoord(DefaultRight, camRotationMatrix);
	camForward = DirectX::XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	camUp = DirectX::XMVector3Cross(camForward, camRight);
	camPosition += moveLeftRight * camRight;
	camPosition += moveBackForward * camForward;
	moveLeftRight = 0.0f;
	moveBackForward = 0.0f;
	camTarget = camPosition + camTarget;
	camView = DirectX::XMMatrixLookAtLH(camPosition, camTarget, camUp);
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

	UINT creationFlags = 0;

#if defined(_DEBUG)
		// If the project is in a debug build, enable the debug layer.
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, nullptr, 0, D3D11_SDK_VERSION, &scd, &swapchain, &dev, nullptr, &devcon);
	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	dev->CreateRenderTargetView(pBackBuffer, nullptr, &backbuffer);
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

	static bool renderwireframe = false;
	static float scale = 1.0;;

	ImGui_ImplDX11_NewFrame();
	{
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Outline", &show_another_window);
		ImGui::Text("Hello");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::Text("left/right %f", moveLeftRight);
		ImGui::Text("back/forward %f", moveBackForward);

		ImGui::SliderFloat("2D Mesh Scale", &scale, 0.0001f, 1.0f);
		ImGui::Checkbox("Render Wireframe?", &renderwireframe);

		ImGui::End();
	}

	float speed = 0.01f;

	if (GetAsyncKeyState(0x41))
	{
		moveLeftRight -= speed;
	}
	if (GetAsyncKeyState(0x44))
	{
		moveLeftRight += speed;
	}
	if (GetAsyncKeyState(0x57))
	{
		moveBackForward += speed;
	}
	if (GetAsyncKeyState(0x53))
	{
		moveBackForward -= speed;
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	devcon->Map(g_pConstantBuffer11, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	VS_CONSTANT_BUFFER* dataPtr = (VS_CONSTANT_BUFFER*)mappedResource.pData;

	static float a = 0.0f;
	a += 0.0001f;

	
	WVP = XMMatrixIdentity();
	WVP = DirectX::XMMatrixIdentity() * camView * camProjection;

	UpdateCamera();

	dataPtr->mWorldViewProj = XMMatrixTranspose(WVP);
	dataPtr->vSomeVectorThatMayBeNeededByASpecificShader = DirectX::XMFLOAT4(1, sin(a), 1, 1);
	dataPtr->scale = scale;
	dataPtr->fTime = 1.0f;
	dataPtr->fSomeFloatThatMayBeNeededByASpecificShader2 = 2.0f;
	dataPtr->fSomeFloatThatMayBeNeededByASpecificShader3 = 4.0f;
	devcon->Unmap(g_pConstantBuffer11, 0);

	float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);
	devcon->RSSetViewports(1, &viewport);

	if (renderwireframe)
	{
		devcon->RSSetState(wireframe);
	}
	else
	{
		devcon->RSSetState(fillsolid);
	}
	devcon->ClearRenderTargetView(backbuffer, clearColor);
	devcon->VSSetShader(pVS, 0, 0);
	devcon->IASetInputLayout(pLayout);
	devcon->PSSetShader(pPS, 0, 0);
	devcon->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);
	
	devcon->IASetIndexBuffer(pIBuffer, DXGI_FORMAT_R32_UINT, 0);
	devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

	devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devcon->DrawIndexed(indices.size(), 0, 0);
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

	const char *filename = "C:\\Users\\Citrus\\Documents\\Visual Studio 2015\\Projects\\Outliner\\Outliner\\assets\\testmesh.obj";
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename)) 
	{
		throw std::runtime_error(err);
	}

	int idx = 0;

	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const tinyobj::index_t& index : shape.mesh.indices) 
		{
			VERTEX vertex = {};

			vertex.x = attrib.vertices[3 * index.vertex_index + 0];
			vertex.y = attrib.vertices[3 * index.vertex_index + 1];
			vertex.z = attrib.vertices[3 * index.vertex_index + 2];

			
			vertex.Color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			vertices.push_back(vertex);
			

			indices.push_back(idx);
			idx += 1;
		}
	}

	//Camera information
	camPosition = DirectX::XMVectorSet(0.0f, 5.0f, -8.0f, 0.0f);
	camTarget = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	camUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//Set the View matrix
	camView = DirectX::XMMatrixLookAtLH(camPosition, camTarget, camUp);

	//Set the Projection matrix
	camProjection = DirectX::XMMatrixPerspectiveFovLH(0.4f*3.14f, (float)SCREEN_WIDTH / SCREEN_HEIGHT, 1.0f, 1000.0f);
	

	D3D11_BUFFER_DESC bd = D3D11_BUFFER_DESC();
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX) * vertices.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_BUFFER_DESC indexBufferDesc = D3D11_BUFFER_DESC();
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned int) * indices.size();
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	dev->CreateBuffer(&bd, NULL, &pVBuffer); 

	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, vertices.data(), vertices.size() * sizeof(VERTEX));
	devcon->Unmap(pVBuffer, NULL);

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = indices.data();
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	dev->CreateBuffer(&indexBufferDesc, &InitData, &pIBuffer);
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
	
	VS_CONSTANT_BUFFER cbuffer = VS_CONSTANT_BUFFER();
	cbuffer.vSomeVectorThatMayBeNeededByASpecificShader = DirectX::XMFLOAT4(1, 1, 0, 1);
	cbuffer.scale = 1.0f;
	cbuffer.fTime = 1.0f;
	cbuffer.fSomeFloatThatMayBeNeededByASpecificShader2 = 2.0f;
	cbuffer.fSomeFloatThatMayBeNeededByASpecificShader3 = 4.0f;

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(VS_CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &cbuffer;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	dev->CreateBuffer(&cbDesc, &InitData, &g_pConstantBuffer11);

	D3D11_RASTERIZER_DESC rastdesc = D3D11_RASTERIZER_DESC();
	rastdesc.FillMode = D3D11_FILL_WIREFRAME;
	rastdesc.CullMode = D3D11_CULL_NONE;
	dev->CreateRasterizerState(&rastdesc, &wireframe);
	rastdesc.FillMode = D3D11_FILL_SOLID;
	dev->CreateRasterizerState(&rastdesc, &fillsolid);
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

		if ((mouseX - oldMouseX) > 0)             // mouse moved to the right
			angle += 3.0f;
		else if ((mouseX - oldMouseX) < 0)     // mouse moved to the left
			angle -= 3.0f;

		return 0;
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}