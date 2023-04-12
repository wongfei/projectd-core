#include "Renderer/GLTrack.h"
#include "Renderer/GLPrimitive.h"
#include "Sim/Track.h"
#include "Sim/Surface.h"

namespace D {

void GLTrack::init(Track* _track)
{
	track = _track;

	vec3f color;

	{ GLCompileScoped compile(trackBatch);
		for (auto& s : track->surfaces)
		{
			if (s->collisionCategory == C_CATEGORY_TRACK)
			{
				if (s->isValidTrack)
				{
					//color = rgb(128, 128, 128) + randV(-32, 32) / 255.0f;
					color = rgb(0, 60, 100) + randV(-30, 30) / 255.0f;
				}
				else
				{
					color = rgb(0, 80, 00) + randV(-30, 30) / 255.0f;
				}

				glColor3fv(&color.x);
				glxTriMesh(s->trimesh.get());
			}
		}
	}

	{ GLCompileScoped compile(wallsBatch);
		for (auto& s : track->surfaces)
		{
			if (s->collisionCategory == C_CATEGORY_WALL)
			{
				color = rgb(160, 60, 0) + randV(-50, 50) / 255.0f;
				glColor3fv(&color.x);
				glxTriMesh(s->trimesh.get());
			}
		}
	}

	if (!track->fatPoints.empty())
	{
		GLCompileScoped compile(racePoints);
		glPointSize(6);
		
		glBegin(GL_POINTS);
		glColor3f(1.0f, 1.0f, 0.0f);
		for (const auto& pt : track->fatPoints)
		{
			glVertex3fv(&pt.best.x);
		}
		glEnd();
		
		glBegin(GL_POINTS);
		glColor3f(0.0f, 1.0f, 0.0f);
		for (const auto& pt : track->fatPoints)
		{
			glVertex3fv(&pt.left.x);
		}
		glEnd();
		
		glBegin(GL_POINTS);
		glColor3f(1.0f, 0.0f, 1.0f);
		for (const auto& pt : track->fatPoints)
		{
			glVertex3fv(&pt.right.x);
		}
		glEnd();

		glBegin(GL_LINES);
		glColor3f(0.0f, 1.0f, 1.0f);
		for (const auto& pt : track->fatPoints)
		{
			auto v1 = pt.best + vec3f(0, 0.01f, 0);
			auto v2 = v1 + pt.forwardDir * 0.5f;
			glVertex3fv(&v1.x);
			glVertex3fv(&v2.x);
		}
		glEnd();
	}
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

void GLTrack::drawTrack(bool wireframe)
{
	beginEnv(wireframe);

	trackBatch.draw();
	wallsBatch.draw();

	endEnv(wireframe);
}

void GLTrack::drawRacePoints()
{
	if (!track->fatPoints.empty())
		racePoints.draw();
}

void GLTrack::setLight(const vec3f& pos, const vec3f& dir)
{
	lightPos = pos;
	lightDir = dir;
}

#define ENABLE_LINE_AA 0

void GLTrack::beginEnv(bool wireframe) // LOL LEGACY OPENGL IS CRAZY :D
{
	if (wireframe)
	{
		#if (ENABLE_LINE_AA)
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		#endif
		glLineWidth(2);
		glPolygonMode(GL_FRONT, GL_LINE);
		glPolygonMode(GL_BACK, GL_LINE);
	}
	else
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
	}
}

void GLTrack::endEnv(bool wireframe)
{
	if (wireframe)
	{
		glPolygonMode(GL_FRONT, GL_FILL);
		glPolygonMode(GL_BACK, GL_FILL);

		#if (ENABLE_LINE_AA)
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
		#endif
	}
	else
	{
		glDisable(GL_COLOR_MATERIAL);
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHTING);
	}
}

}
