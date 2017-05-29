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
				//edges.insert(std::pair<std::string, std::pair<int, int>>(edge, std::pair<int, int>(vert1, vert2)));
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

std::vector<ExtrudedOutline> Outliner::GenerateExtrudedOutlines(ID3D11Device *dev, ID3D11DeviceContext *devcon, float offset, int repetitions)
{
	std::vector<ExtrudedOutline> offsetmesh;
	//for each loop
	for (auto & loop : loops)
	{
		//for N repetitions (user defined)
		for (int rep = 0; rep < repetitions; rep++)
		{
			std::vector<VERTEX> outline;
			
			//we store first the outline vertices
			for (auto &o : loop.positions)
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
				outline[i].pos += (v * (offset * repetitions));
				outline[i].color = XMFLOAT4(v.x, v.y, 0.0f, 1.0f);
			}

			std::vector<VERTEX> extrusion(outline);
			//then we apply the respective normal to respective vertex
			for (int i = 0; i < normals.size(); i++)
			{
				auto &n0 = normals[(i + normals.size() - 1) % normals.size()];
				auto &n1 = normals[i];
				auto v = Normalize(XMFLOAT3(n0.x + n1.x, n0.y + n1.y, 0.0f));
				//here be dragons
				v.x = fabsf(n0.x) > fabsf(n1.x) ? n0.x : n1.x;
				v.y = fabsf(n0.y) > fabsf(n1.y) ? n0.y : n1.y;
				extrusion[i].pos += (v * (offset * repetitions));
				extrusion[i].color = XMFLOAT4(v.x, v.y, 0.0f, 1.0f);
			}
			//finish loop
			extrusion.back() = extrusion[0];

			ExtrudedOutline out1;
			out1.vertices = std::vector<VERTEX>(outline);
			out1.vertexBuffer = this->CreateVertexBufferFromData(dev, devcon, outline.size() * sizeof(VERTEX), outline);

			ExtrudedOutline out2;
			out2.vertices = std::vector<VERTEX>(extrusion);
			out2.vertexBuffer = this->CreateVertexBufferFromData(dev, devcon, extrusion.size() * sizeof(VERTEX), extrusion);

			offsetmesh.push_back(out2);
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
