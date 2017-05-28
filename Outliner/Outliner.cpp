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
			int minval = std::min(vert1, vert2);
			int maxval = std::max(vert1, vert2);

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
