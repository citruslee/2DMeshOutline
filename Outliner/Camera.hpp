#pragma once
#include <DirectXMath.h>

class Camera
{
public:

	void Init(int screenwidth, int screenheight)
	{
		//Camera information
		camPosition = DirectX::XMVectorSet(0.0f, 5.0f, -8.0f, 0.0f);
		camTarget = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		camUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		//Set the View matrix
		camView = DirectX::XMMatrixLookAtLH(camPosition, camTarget, camUp);

		//Set the Projection matrix
		camProjection = DirectX::XMMatrixPerspectiveFovLH(0.4f*3.14f, (float)screenwidth / screenheight, 1.0f, 1000.0f);
	}

	void UpdateInput()
	{
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

	DirectX::XMMATRIX &GetViewMatrix()
	{
		return this->camView;
	}

	DirectX::XMMATRIX &GetProjectionMatrix()
	{
		return this->camProjection;
	}

private:
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

	float moveLeftRight = 0.0f;
	float moveBackForward = 0.0f;

	float camYaw = 0.0f;
	float camPitch = 0.0f;

	float speed = 0.01f;
};