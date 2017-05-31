#include "Outliner.hpp"

std::vector<LineLoop> Outliner::GetOutlines(const std::vector<int>& triangles, const std::vector<DirectX::XMFLOAT3>& vertices)
{
	// Get just the outer edges from the mesh's triangles (ignore or remove any shared edges)
	std::map<std::string, std::pair<int, int>> edges;
	for (int i = 0; i < triangles.size(); i += 3)
	{
		for (int e = 0; e < 3; e++)
		{
			int vert1 = triangles[i + e];
			int vert2 = triangles[i + ((e + 1) % 3)];
			int minval = min(vert1, vert2);
			int maxval = max(vert1, vert2);

			char buff[100];
			snprintf(buff, sizeof(buff), "%d:%d", minval, maxval);
			std::string edge = buff;
			if (contains(edges, edge))
			{
				edges.erase(edge);
			}
			else
			{
				edges[edge] = std::pair<int, int>(vert1, vert2);
			}
		}
	}

	// Create edge lookup Dictionary
	std::map<int, int> lookup;
	for (auto &edge : edges)
	{
		if (contains(lookup, edge.second.first) == false)
		{
			lookup[edge.second.first] = edge.second.second;
		}
	}

	LineLoop line;
	// Loop through edge vertices in order
	auto i = lookup.begin();
	while (!lookup.empty())
	{
		line.positions.push_back(vertices[i->first]);
		auto nextVert = i->second;
		lookup.erase(i);
		i = lookup.find(nextVert);

		// Shape complete
		if (i == lookup.end())
		{
			line.positions.push_back(vertices[nextVert]);
			loops.push_back(line);
			line.positions.clear();
			i = lookup.begin();
		}
	}
	return loops;
}

bool Outliner::CheckIfEdgesOverlap(std::vector<VERTEX> firstmesh, std::vector<VERTEX> secondmesh)
{
	for (int i = 0; i < secondmesh.size(); i++)
	{
		auto &u0 = secondmesh[i];
		auto &u1 = secondmesh[(i + 1) % secondmesh.size()];
		for (int i = 0; i < firstmesh.size(); i++)
		{
			auto &v0 = firstmesh[i];
			auto &v1 = firstmesh[(i + 1) % firstmesh.size()];

			if (DoLinesIntersect(u0.pos, u1.pos, v0.pos, v1.pos))
			{
				return true;
			}
		}
	}
	return false;
}

std::vector<VERTEX> Outliner::ExtrudeFromLoops(std::vector<VERTEX> inner, std::vector<VERTEX> outer)
{
	std::vector<VERTEX> extruded;

	for (int i = 0; i < inner.size() - 1; i++)
	{
		auto &u0 = inner[i];
		auto &u1 = inner[(i + 1) % inner.size()];
		auto &v0 = outer[i];
		auto &v1 = outer[(i + 1) % outer.size()];

		extruded.push_back(u0);
		extruded.push_back(v0);
		extruded.push_back(v1);

		extruded.push_back(v1);
		extruded.push_back(u0);
		extruded.push_back(u1);
	}
	return extruded;
}

std::vector<ExtrudedOutline> Outliner::GenerateExtrudedOutlines(ID3D11Device *dev, ID3D11DeviceContext *devcon, float offset, int repetitions)
{
	std::vector<ExtrudedOutline> offsetmesh;
	//for each loop
	for (int loopindex = 0; loopindex < loops.size(); loopindex++)
	{
		//for N repetitions (user defined)
		for (int rep = 1; rep < repetitions * 2; rep += 2)
		{
			std::vector<VERTEX> outline;

			//we store first the outline vertices
			for (auto &o : loops[loopindex].positions)
			{
				VERTEX vx = VERTEX();
				vx.pos.x = o.x;
				vx.pos.y = o.y;
				vx.pos.z = 0.0f;
				vx.color = DirectX::XMFLOAT4(0, 1, 0, 1);
				outline.push_back(vx);
			}

			std::vector<XMFLOAT3> normals;
			//then we precalc the normal vectors to each line segment
			for (int i = 0; i < outline.size() - 1; i++)
			{
				auto &current = outline[i];
				auto &next = outline[i + 1];
				auto dir = Normalize(XMFLOAT3(next.pos.x - current.pos.x, next.pos.y - current.pos.y, 0.0f));
				auto normal = XMFLOAT3(-dir.y, dir.x, 0.0f);
				normals.push_back(normal);
			}

			//then we apply the respective normal to respective vertex
			for (int i = 0; i < normals.size(); i++)
			{
				auto &n0 = normals[(i + normals.size() - 1) % normals.size()];
				auto &n1 = normals[i];
				auto v = Normalize(XMFLOAT3(n0.x + n1.x, n0.y + n1.y, 0.0f));
				//here be dragons
				v.x = fabsf(n0.x) > fabsf(n1.x) ? n0.x : n1.x;
				v.y = fabsf(n0.y) > fabsf(n1.y) ? n0.y : n1.y;
				outline[i].pos += (v * (offset * rep));
				outline[i].color = XMFLOAT4(v.x, v.y, 0.0f, 1.0f);
			}
			outline.back() = outline[0];

			std::vector<VERTEX> extrusion = outline;
			//then we apply the respective normal to respective vertex
			for (int i = 0; i < normals.size(); i++)
			{
				auto &n0 = normals[(i + normals.size() - 1) % normals.size()];
				auto &n1 = normals[i];
				auto v = Normalize(XMFLOAT3(n0.x + n1.x, n0.y + n1.y, 0.0f));
				//here be dragons
				v.x = fabsf(n0.x) > fabsf(n1.x) ? n0.x : n1.x;
				v.y = fabsf(n0.y) > fabsf(n1.y) ? n0.y : n1.y;
				extrusion[i].pos += (v * (offset));
				extrusion[i].color = XMFLOAT4(v.x, v.y, 0.0f, 1.0f);
			}
			extrusion.back() = extrusion[0];

			std::vector<VERTEX> extruded = ExtrudeFromLoops(outline, extrusion);
			
			if (offsetmesh.size() > 0)
			{
				bool canAddThisOutline = true;
				for (int i = 0; i < offsetmesh.size(); i++)
				{
					if (CheckIfEdgesOverlap(extruded, offsetmesh[i].vertices))
					{
						canAddThisOutline = false;
						break;
					}
				}
				if (!canAddThisOutline)
				{
					break;
				}
			}

			ExtrudedOutline out1;
			out1.vertices = extruded;
			out1.vertexBuffer = this->CreateVertexBufferFromData(dev, devcon, extruded.size() * sizeof(VERTEX), extruded);
			offsetmesh.push_back(out1);
		}
	}
	return offsetmesh;
}

ID3D11Buffer *Outliner::CreateVertexBufferFromData(ID3D11Device * dev, ID3D11DeviceContext *devcon, UINT bytewidth, std::vector<VERTEX> vertices)
{
	ID3D11Buffer *buf = nullptr;
	D3D11_BUFFER_DESC bd = D3D11_BUFFER_DESC();
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = bytewidth;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	dev->CreateBuffer(&bd, NULL, &buf);
	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map(buf, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
	memcpy(ms.pData, vertices.data(), bytewidth);
	devcon->Unmap(buf, NULL);
	return buf;
}

bool Outliner::DoLinesIntersect(DirectX::XMFLOAT3 p1, DirectX::XMFLOAT3 p2, DirectX::XMFLOAT3 p3, DirectX::XMFLOAT3 p4)
{
	// Store the values for fast access and easy
	// equations-to-code conversion
	float x1 = p1.x, x2 = p2.x, x3 = p3.x, x4 = p4.x;
	float y1 = p1.y, y2 = p2.y, y3 = p3.y, y4 = p4.y;

	float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	// If d is zero, there is no intersection
	if (d == 0)
	{
		return false;
	}
	// Get the x and y
	float pre = (x1*y2 - y1*x2), post = (x3*y4 - y3*x4);
	float x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
	float y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

	// Check if the x and y coordinates are within both lines
	if (x < min(x1, x2) || x > max(x1, x2) || x < min(x3, x4) || x > max(x3, x4))
	{
		return false;
	}
	if (y < min(y1, y2) || y > max(y1, y2) || y < min(y3, y4) || y > max(y3, y4))
	{
		return false;
	}

	return true;
}
