#include "Renderer/GLTrack.h"
#include "Renderer/GLPrimitive.h"
#include "Sim/Track.h"
#include "Sim/Surface.h"

namespace D {

GLTrack::GLTrack(Track* _track)
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
		GLCompileScoped compile(fatPointsBatch);
		glPointSize(6);
		
		glBegin(GL_POINTS);
		glColor3f(0.0f, 0.0f, 0.0f);
		glVertex3fv(&track->fatPoints[0].center.x);

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

GLTrack::~GLTrack()
{
}

void GLTrack::draw()
{
	drawSurfaces();

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

	if (drawFatPoints && !track->fatPoints.empty())
	{
		fatPointsBatch.draw();
	}

	if (drawNearbyPoints)
	{
		nearbyPoints.clear();
		track->fatPointsHash.queryNeighbours(camPos, nearbyPoints);

		glPointSize(10);
		glBegin(GL_POINTS);
		glColor3f(1.0f, 1.0f, 1.0f);
		for (const auto& id : nearbyPoints)
		{
			glVertex3fv(&track->fatPoints[id].left.x);
			glVertex3fv(&track->fatPoints[id].right.x);
		}
		glEnd();

		const float obstacleDist = track->rayCastTrackBounds(camPos, camDir);
		if (obstacleDist > 0.0f)
		{
			const auto interPos = camPos + camDir * obstacleDist;

			glPointSize(6);
			glBegin(GL_POINTS);
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex3fv(&interPos.x);
			glEnd();

			TrackRayCastHit hit;
			if (track->rayCast(interPos + vec3f(0, 50, 0), vec3f(0, -1, 0), 100, hit))
			{
				glBegin(GL_LINES);
				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3fv(&interPos.x);
				glVertex3fv(&hit.pos.x);
				glEnd();

				glPointSize(6);
				glBegin(GL_POINTS);
				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3fv(&hit.pos.x);
				glEnd();
			}
		}
	}
}

void GLTrack::drawSurfaces() // LOL LEGACY OPENGL IS CRAZY :D
{
	GLint oldShadeModel = 0;
	glGetIntegerv(GL_SHADE_MODEL, &oldShadeModel);

	if (wireframe)
	{
		glLineWidth(2);
		glPolygonMode(GL_FRONT, GL_LINE);
		glPolygonMode(GL_BACK, GL_LINE);
	}
	else
	{
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glShadeModel(GL_FLAT);
		
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

		GLfloat ambientLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		GLfloat diffuseLight[] = { 0.95f, 0.95f, 0.95f, 1.0f };
		GLfloat specularLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		GLfloat position[] = { camPos.x, camPos.y, camPos.z, 1.0f };
		GLfloat direction[] = { camDir.x, camDir.y, camDir.z, 1.0f };
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, direction);
	}

	trackBatch.draw();
	wallsBatch.draw();

	if (wireframe)
	{
		glLineWidth(1);
		glPolygonMode(GL_FRONT, GL_FILL);
		glPolygonMode(GL_BACK, GL_FILL);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_COLOR_MATERIAL);
		glShadeModel(oldShadeModel);
	}
}

}
