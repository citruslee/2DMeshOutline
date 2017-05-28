#pragma once
#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <iterator>
#include <DirectXMath.h>

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

class Outliner
{
public:
	std::vector<LineLoop> GetOutlines(const std::vector<int> &triangles, const std::vector<DirectX::XMFLOAT3> &vertices);
private:
	std::vector<LineLoop> loops;
};
