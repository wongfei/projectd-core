#pragma once

#include "Core/Math.h"
#include <unordered_map>

namespace D {

struct VertexHashNode
{
	vec3f location;
	size_t id;
};

struct VertexHash
{
	size_t tableSize = 1024;
	float cellSize = 10;
	float invCellSize = 0;
	std::unordered_map<size_t, std::vector<VertexHashNode>> buckets;

	void init(float _cellSize, size_t _tableSize)
	{
		tableSize = _tableSize;
		cellSize = _cellSize;
		invCellSize = 1.0f / _cellSize;
		clear();
	}

	void clear()
	{
		buckets.clear();
	}

	void add(const vec3f& location, size_t vertexId)
	{
		const size_t hash = getHash(location);

		auto iter = buckets.find(hash);
		if (iter == buckets.end())
			iter = buckets.insert({hash, {}}).first;

		iter->second.push_back({ location, vertexId });
	}

	void queryNeighbours(const vec3f& origin, std::vector<size_t>& outNeighbours, float maxDistance = 0.0f)
	{
		if (maxDistance <= 0.0f)
			maxDistance = cellSize;

		const auto maxDistanceSq = maxDistance * maxDistance;
		const auto offset = cellSize;

		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					const auto pos = origin + vec3f(x * offset, y * offset, z * offset);
					const size_t hash = getHash(pos);

					priv_queryNeighboursByHash(hash, origin, maxDistanceSq, outNeighbours);
				}
			}
		}
	}

	void priv_queryNeighboursByHash(size_t hash, const vec3f& origin, float maxDistanceSq, std::vector<size_t>& outNeighbours)
	{
		auto bucketIter = buckets.find(hash);
		if (bucketIter != buckets.end())
		{
			for (const auto& node : bucketIter->second)
			{
				const auto distSq = (origin - node.location).sqlen();
				if (distSq < maxDistanceSq)
					outNeighbours.push_back(node.id);
			}
		}
	}

	inline size_t getHash(const vec3f& location) const
	{
		// Optimized Spatial Hashing for Collision Detection of Deformable Objects
		// https://matthias-research.github.io/pages/publications/tetraederCollision.pdf
		// https://stackoverflow.com/questions/5928725/hashing-2d-3d-and-nd-vectors
		
		// The coordinates of the vertex position (x, y, z) are divided
		// by the given grid cell size l and rounded down to
		// the next integer (i, j, k): i = x/l, j = y/l, k = z/l

		const float s = invCellSize;
		const uint32_t x = (uint32_t)roundToInt(location.x * s);
		const uint32_t y = (uint32_t)roundToInt(location.y * s);
		const uint32_t z = (uint32_t)roundToInt(location.z * s);

		// hash(x,y,z) = ( x p1 xor y p2 xor z p3) mod n
		// p1, p2, p3 are large prime numbers, 73856093, 19349663, 83492791
		// n is the hash table size

		const size_t hash = (size_t)((x * 73856093) ^ (y * 19349663) ^ (z * 83492791)) % (size_t)tableSize;

		return hash;
	}
};

}
