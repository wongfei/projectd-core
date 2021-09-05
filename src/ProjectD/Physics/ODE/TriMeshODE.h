#pragma once

#include "Physics/ITriMesh.h"

namespace D {

struct TriMeshODE : public ITriMesh
{
	TriMeshODE();
	~TriMeshODE();

	void resize(size_t vertexCount, size_t indexCount) override;
	TriMeshVertex* getVB() override;
	size_t getVertexCount() override;
	TriMeshIndex* getIB() override;
	size_t getIndexCount() override;

	std::vector<TriMeshVertex> vertices;
	std::vector<TriMeshIndex> indices;
};

}
