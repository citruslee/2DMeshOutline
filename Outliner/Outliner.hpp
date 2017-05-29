#pragma once
#include <d3d11.h>
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <iterator>
#include <DirectXMath.h>
#include "MathExtension.hpp"
#include "Mesh.hpp"

template <typename T>
struct identity { typedef T type; };

template <typename A, typename B>
inline bool contains(const std::map<A, B>& m, const typename identity<A>::type& str)
{
	return m.find(str) != m.end();
}

struct LineLoop 
{
	std::vector<DirectX::XMFLOAT3> positions;
};

struct ExtrudedOutline
{
	std::vector<VERTEX> vertices;
	ID3D11Buffer *vertexBuffer;
};

class Outliner
{
public:
	std::vector<LineLoop> GetOutlines(const std::vector<int> &triangles, const std::vector<DirectX::XMFLOAT3> &vertices);
	std::vector<ExtrudedOutline> GenerateExtrudedOutlines(ID3D11Device *dev, ID3D11DeviceContext *devcon, float offset, int repetitions);

	ID3D11Buffer *CreateVertexBufferFromData(ID3D11Device *dev, ID3D11DeviceContext *devcon, UINT bytewidth, std::vector<VERTEX> vertices);
private:
	std::vector<LineLoop> loops;
};