#include "Renderer/GLTrack.h"
#include "Renderer/GLPrimitive.h"
#include "Sim/Track.h"
#include "Sim/Surface.h"

namespace D {

void GLTrack::init(Track* _track)
{
	track = _track;

	{ GLCompileScoped compile(surfaces);
		for (auto& s : track->surfaces)
		{
			auto color = rgb(128, 128, 128) + randV(-32, 32) / 255.0f;
			glColor3fv(&color.x);
			glxTriMesh(s->trimesh.get());
		}
	}

	{ GLCompileScoped compile(walls);
		glColor3f(0.2f, 0, 0);
		for (auto& s : track->surfaces)
		{
			if (s->collisionCategory == C_CATEGORY_WALL)
			{
				glxTriMesh(s->trimesh.get());
			}
		}
	}
}

void GLTrack::drawSurfaces(const vec3f& lightPos, const vec3f& lightDir)
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glShadeModel(GL_FLAT);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	GLfloat ambientLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat diffuseLight[] = { 0.95f, 0.95f, 0.95f, 1.0f };
	GLfloat specularLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat position[] = { lightPos.x, lightPos.y, lightPos.z, 1.0f };
	GLfloat direction[] = { lightDir.x, lightDir.y, lightDir.z, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, direction);

	surfaces.draw();

	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
}

void GLTrack::drawWalls()
{
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glLineWidth(2);

	glPolygonMode(GL_FRONT, GL_LINE);
	glPolygonMode(GL_BACK, GL_LINE);

	walls.draw();

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);
}

void GLTrack::drawOrigin()
{
	// world origin
	glBegin(GL_LINES);
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(0, 0, 0);
		glVertex3f(1, 0, 0);
		glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 1, 0);
		glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 0, 1);
	glEnd();

	glPointSize(3);
	glBegin(GL_POINTS);
		glColor3f(1.0f, 1.0f, 0.0f);
		glVertex3f(0, 0, 0);
	glEnd();

	// pit origin
	if (!track->pits.empty())
	{
		vec3f p(&track->pits[0].M41);

		glBegin(GL_LINES);
			float fLen = 0.5f;
			auto v = p + vec3f(fLen, 0, 0);
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex3fv(&p.x);
			glVertex3fv(&v.x);

			v = p + vec3f(0, fLen, 0);
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3fv(&p.x);
			glVertex3fv(&v.x);

			v = p + vec3f(0, 0, fLen);
			glColor3f(0.0f, 0.0f, 1.0f);
			glVertex3fv(&p.x);
			glVertex3fv(&v.x);
		glEnd();

		glPointSize(3);
		glBegin(GL_POINTS);
			glColor3f(1.0f, 1.0f, 0.0f);
			glVertex3fv(&p.x);
		glEnd();
	}
}

}
