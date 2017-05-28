#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "imgui.h"
#include "imgui_impl_dx11.hpp"
#include "LogWindow.hpp"
#include "clipper.hpp"  
#include "Mesh.hpp"
#include "Outliner.hpp"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "D3DCompiler.lib")

#define SCREEN_WIDTH  1600
#define SCREEN_HEIGHT 900

using namespace ClipperLib;
using namespace DirectX;

static LogWindow logger;

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

ID3D11Buffer *g_pConstantBuffer11 = nullptr;
D3D11_VIEWPORT viewport = D3D11_VIEWPORT();

ID3D11RasterizerState *wireframe = nullptr;
ID3D11RasterizerState *fillsolid = nullptr;

DirectX::XMMATRIX WVP;
DirectX::XMMATRIX camRotationMatrix;
DirectX::XMMATRIX camView;
DirectX::XMMATRIX camProjection;
DirectX::XMVECTOR camPosition;
DirectX::XMVECTOR camTarget;
DirectX::XMVECTOR camUp;
DirectX::XMVECTOR DefaultForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
DirectX::XMVECTOR DefaultRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
DirectX::XMVECTOR camForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
DirectX::XMVECTOR camRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

ID3D11Buffer *outlinevbuffer = nullptr;
std::vector<VERTEX> outlineverts;

float moveLeftRight = 0.0f;
float moveBackForward = 0.0f;

float camYaw = 0.0f;
float camPitch = 0.0f;

WORD oldMouseX, oldMouseY, mouseX, mouseY;
float angle = 0;

void InitD3D(HWND hWnd);
void RenderFrame(void);
void CleanD3D(void);
void InitGraphics(void);
void InitPipeline(void);

#pragma warning(disable:4018)

inline XMFLOAT3& operator+=(XMFLOAT3 &l, const XMFLOAT3 &r) { l.x += r.x; l.y += r.y; l.z += r.z; return l; }
inline XMFLOAT3& operator*=(XMFLOAT3 &l, float r) { l.x *= r; l.y *= r; l.z *= r; return l; }
inline XMFLOAT3 operator-(const XMFLOAT3 &l, const XMFLOAT3 &r) { return XMFLOAT3(l.x - r.x, l.y - r.y, l.z - r.z); }
inline XMFLOAT3 operator*(const XMFLOAT3 &l, float r) { return XMFLOAT3(l.x*r, l.y*r, l.z*r); }
inline XMFLOAT3 operator/(const XMFLOAT3 &l, float r) { return XMFLOAT3(l.x / r, l.y / r, l.z / r); }

inline float Dot(const XMFLOAT3 a, const XMFLOAT3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline float Len(const XMFLOAT3 a)
{
	return sqrt(Dot(a, a));
}

inline XMFLOAT3 Normalize(XMFLOAT3 a)
{
	return a / Len(a);
}

Mesh obj;

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
	static float hullN = 0.1f;
	static float scale = 0.1f;

	ImGui_ImplDX11_NewFrame();
	{
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Outline", &show_another_window);
		ImGui::Text("Hello");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::Text("left/right %f", moveLeftRight);
		ImGui::Text("back/forward %f", moveBackForward);

		ImGui::SliderFloat("N (Hull)", (float *)&hullN, -100.0f, 100.0f);
		ImGui::SliderFloat("Scale", (float *)&scale, 0.0f, 100.0f);
		ImGui::Checkbox("Render Wireframe?", &renderwireframe);

		if (ImGui::Button("remap"))
		{
			
		}
		logger.Draw("Log");

	
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

	{
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
		dataPtr->scale = 1.0f;
		dataPtr->fTime = 1.0f;
		dataPtr->fSomeFloatThatMayBeNeededByASpecificShader2 = 2.0f;
		dataPtr->fSomeFloatThatMayBeNeededByASpecificShader3 = 4.0f;
		devcon->Unmap(g_pConstantBuffer11, 0);
	}

	float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
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
	
	obj.DrawMesh(devcon);

	UINT stride = sizeof(VERTEX);
	UINT offset = 0;

	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		devcon->Map(g_pConstantBuffer11, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

		VS_CONSTANT_BUFFER* dataPtr = (VS_CONSTANT_BUFFER*)mappedResource.pData;

		dataPtr->mWorldViewProj = XMMatrixTranspose(WVP);
		dataPtr->scale = scale;
		dataPtr->fTime = 1.0f;
		dataPtr->fSomeFloatThatMayBeNeededByASpecificShader2 = 2.0f;
		dataPtr->fSomeFloatThatMayBeNeededByASpecificShader3 = 4.0f;
		devcon->Unmap(g_pConstantBuffer11, 0);
	}

	devcon->IASetVertexBuffers(0, 1, &outlinevbuffer, &stride, &offset);
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	devcon->Draw(outlineverts.size(), 0);
	
	ImGui::Render();
	swapchain->Present(0, 0);
}

void CleanD3D(void)
{
	swapchain->SetFullscreenState(FALSE, NULL);
	pLayout->Release();
	pVS->Release();
	pPS->Release();
	
	swapchain->Release();
	backbuffer->Release();
	dev->Release();
	devcon->Release();
}

void InitGraphics()
{
	const char *filename = "C:\\Users\\Citrus\\Documents\\Visual Studio 2015\\Projects\\Outliner\\Outliner\\assets\\testmesh.obj";//"C:\\Users\\Citrus\\Source\\Repos\\2DMeshOutline\\Outliner\\assets\\testmesh.obj";

	obj.LoadObj(dev, devcon, filename);

	//Camera information
	camPosition = DirectX::XMVectorSet(0.0f, 5.0f, -8.0f, 0.0f);
	camTarget = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	camUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//Set the View matrix
	camView = DirectX::XMMatrixLookAtLH(camPosition, camTarget, camUp);

	//Set the Projection matrix
	camProjection = DirectX::XMMatrixPerspectiveFovLH(0.4f*3.14f, (float)SCREEN_WIDTH / SCREEN_HEIGHT, 1.0f, 1000.0f);

	Outliner outl;
	auto outline = outl.GetOutlines(obj.GetTriangles(), obj.GetVertexPositions());
	for (auto &o : outline[0].positions)
	{
		VERTEX vx;
		vx.pos.x = o.x;
		vx.pos.y = o.y;
		vx.pos.z = 0.0f;
		vx.color = DirectX::XMFLOAT4(0, 1, 0, 1);
		outlineverts.push_back(vx);
	}

	std::vector<XMFLOAT3> normals;
	for (int i = 0; i < outlineverts.size() - 1; i++)
	{
		auto &current = outlineverts[i];
		auto &next = outlineverts[i + 1];
		auto dir = Normalize(XMFLOAT3(next.pos.x - current.pos.x, next.pos.y - current.pos.y, 0.0f));
		XMFLOAT3 normal = XMFLOAT3(-dir.y, dir.x, 0.0f);
		normals.push_back(normal);
	}

	for (int i = 0; i < normals.size(); i++)
	{
		auto &n0 = normals[(i + normals.size() - 1) % normals.size()];
		auto &n1 = normals[i];
		auto v = Normalize(XMFLOAT3(n0.x + n1.x, n0.y + n1.y, 0.0f));
		v.x = fabsf(n0.x) > fabsf(n1.x) ? n0.x : n1.x;
		v.y = fabsf(n0.y) > fabsf(n1.y) ? n0.y : n1.y;
		outlineverts[i].pos += v;
		outlineverts[i].color = XMFLOAT4(v.x, v.y, 0.0f, 1.0f);
	}
	outlineverts.back() = outlineverts[0];

	D3D11_BUFFER_DESC bd = D3D11_BUFFER_DESC();
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX) * outlineverts.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	dev->CreateBuffer(&bd, NULL, &outlinevbuffer);
	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map(outlinevbuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, outlineverts.data(), outlineverts.size() * sizeof(VERTEX));
	devcon->Unmap(outlinevbuffer, NULL);
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