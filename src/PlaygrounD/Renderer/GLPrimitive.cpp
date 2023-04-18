#include "Renderer/GLPrimitive.h"
#include "Physics/ITriMesh.h"

namespace D {

void GLPrimitive::draw()
{
	if (wireframe)
	{
		glPolygonMode(GL_FRONT, GL_LINE);
		glPolygonMode(GL_BACK, GL_LINE);
	}

	glColor3fv(&color.x);
	list.draw();
	
	if (wireframe)
	{
		glPolygonMode(GL_FRONT, GL_FILL);
		glPolygonMode(GL_BACK, GL_FILL);
	}
}

void GLPrimitive::initBox()
{
	{ GLCompileScoped compile(list);
		glxBox();
	}
}

void GLPrimitive::initTriMesh(ITriMesh* trimesh)
{
	{ GLCompileScoped compile(list);
		glxTriMesh(trimesh);
	}
}

void glxBox()
{
	glBegin(GL_QUADS);
	// Front Face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);  // Bottom Left
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5f, -0.5f,  0.5f);  // Bottom Right
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5f,  0.5f,  0.5f);  // Top Right
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f,  0.5f);  // Top Left
	// Back Face
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);  // Bottom Right
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);  // Top Right
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5f,  0.5f, -0.5f);  // Top Left
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5f, -0.5f, -0.5f);  // Bottom Left
	// Top Face
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);  // Top Left
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f,  0.5f,  0.5f);  // Bottom Left
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5f,  0.5f,  0.5f);  // Bottom Right
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5f,  0.5f, -0.5f);  // Top Right
	// Bottom Face
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f, -0.5f, -0.5f);  // Top Right
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5f, -0.5f, -0.5f);  // Top Left
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5f, -0.5f,  0.5f);  // Bottom Left
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);  // Bottom Right
	// Right face
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5f, -0.5f, -0.5f);  // Bottom Right
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5f,  0.5f, -0.5f);  // Top Right
	glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5f,  0.5f,  0.5f);  // Top Left
	glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5f, -0.5f,  0.5f);  // Bottom Left
	// Left Face
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5f, -0.5f, -0.5f);  // Bottom Left
	glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5f, -0.5f,  0.5f);  // Bottom Right
	glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5f,  0.5f,  0.5f);  // Top Right
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5f,  0.5f, -0.5f);  // Top Left
	glEnd();
}

inline vec3f conv(const TriMeshVertex& in) { return vec3f(in.x, in.y, in.z); }

void glxTriMesh(ITriMesh* trimesh)
{
	const auto* VB = trimesh->getVB();
	const auto* IB = trimesh->getIB();
	const size_t n = trimesh->getIndexCount();

	glBegin(GL_TRIANGLES);
	for (size_t i = 0; i < n; i += 3)
	{
		const auto& v0 = VB[IB[i]];
		const auto& v1 = VB[IB[i + 1]];
		const auto& v2 = VB[IB[i + 2]];

		auto a = conv(v0) - conv(v1);
		auto b = conv(v1) - conv(v2);
		auto vn = a.cross(b).get_norm();

		glNormal3fv(&vn.x);
		glVertex3fv(&v0.x);
		glNormal3fv(&vn.x);
		glVertex3fv(&v1.x);
		glNormal3fv(&vn.x);
		glVertex3fv(&v2.x);
	}
	glEnd();
}

}
