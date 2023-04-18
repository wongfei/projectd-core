#include "Renderer/GLCar.h"
#include "Car/Car.h"
#include "Car/Tyre.h"
#include "Car/CarState.h"
#include "Car/CarDebug.h"

namespace D {

void GLCar::init(Car* _car)
{
	car = _car;

	bodyMesh.initTriMesh(car->collider.get());
	bodyMesh.color = vec3f(0, 1, 0);
	bodyMesh.wireframe = true;

	wheelMesh.initBox();
	wheelMesh.color = vec3f(0, 0, 1);
	wheelMesh.wireframe = true;
}

void GLCar::draw(bool drawBody, bool drawProbes)
{
	// body
	if (drawBody)
	{
		auto gm = car->getGraphicsOffsetMatrix();
		glPushMatrix();
		glMultMatrixf(&gm.M11);
		bodyMesh.draw();
		glPopMatrix();
	}

	// wheels
	glPolygonMode(GL_FRONT, GL_LINE);
	glPolygonMode(GL_BACK, GL_LINE);
	//glColor3f(0, 0, 1);
	for (int i = 0; i < 4; ++i)
	{
		auto* pTyre = car->tyres[i].get();
		auto p = pTyre->worldPosition;
		auto r = pTyre->worldRotation;
		auto lwr = pTyre->localWheelRotation;
		r.M41 = p.x;
		r.M42 = p.y;
		r.M43 = p.z;
		glPushMatrix();
		glMultMatrixf(&r.M11);
		glMultMatrixf(&lwr.M11);
		float sz = 0.3f;
		glScalef(sz, sz, sz);
		wheelMesh.draw();
		glPopMatrix();
	}
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	// hubs
	vec3f bodyPos(&car->state->bodyMatrix.M41);
	vec3f hub[4];
	vec3f contact[4];

	for (int i = 0; i < 4; ++i)
	{
		const auto& hm = car->state->hubMatrix[i];
		hub[i] = {hm.M41, hm.M42, hm.M43};
		contact[i] = car->state->tyreContacts[i];
	}

	glBegin(GL_LINES);
	{
		// hubs to center
		glColor3f(1.0f, 1.0f, 0.0f);
		for (int i = 0; i < 4; ++i)
		{
			glVertex3fv(&hub[i].x);
			glVertex3fv(&bodyPos.x);
		}

		// hubs to raycast hit
		glColor3f(1.0f, 0.0f, 0.0f);
		for (int i = 0; i < 4; ++i)
		{
			glVertex3fv(&hub[i].x);
			glVertex3fv(&contact[i].x);
		}

		// front/rear axes
		glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3fv(&hub[0].x);
		glVertex3fv(&hub[1].x);
		glVertex3fv(&hub[2].x);
		glVertex3fv(&hub[3].x);

		// car origin
		float fLen = 0.3f;
		auto carBase = car->body->localToWorld(vec3f(0, 0, 0));
		auto v = car->body->localToWorld(vec3f(fLen, 0, 0));
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3fv(&carBase.x);
		glVertex3fv(&v.x);
		v = car->body->localToWorld(vec3f(0, fLen, 0));
		glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3fv(&carBase.x);
		glVertex3fv(&v.x);
		v = car->body->localToWorld(vec3f(0, 0, fLen));
		glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3fv(&carBase.x);
		glVertex3fv(&v.x);
	}
	glEnd();

	glPointSize(3);
	glBegin(GL_POINTS);
	{
		// car origin
		glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3fv(&bodyPos.x);

		// hubs
		glColor3f(0.0f, 0.0f, 1.0f);
		for (int i = 0; i < 4; ++i)
			glVertex3fv(&hub[i].x);

		// contacts
		glColor3f(1.0f, 0.0f, 0.0f);
		for (int i = 0; i < 4; ++i)
			glVertex3fv(&contact[i].x);
	}
	glEnd();

	// suspension
	CarDebug dbg;
	for (int i = 0; i < 4; ++i)
	{
		car->suspensions[i]->getDebugState(&dbg);
	}
	glBegin(GL_LINES);
	for (auto &n : dbg.lines)
	{
		glColor3fv(&n.color.x);
		glVertex3fv(&n.p1.x);
		glVertex3fv(&n.p2.x);
	}
	glEnd();
	glBegin(GL_POINTS);
	for (auto &n : dbg.points)
	{
		glColor3fv(&n.color.x);
		glVertex3fv(&n.p.x);
	}
	glEnd();

	// probes
	const auto numRays = car->probes.size();
	if (drawProbes && numRays > 0)
	{
		glBegin(GL_LINES);
		glColor3f(1.0f, 1.0f, 0.0f);
		for (size_t rayId = 0; rayId < numRays; ++rayId)
		{
			const auto& r = car->probes[rayId];
			const auto rayStart = car->body->localToWorld(r.pos);
			const auto rayEnd = car->body->localToWorld(r.pos + r.dir * r.length);
			glVertex3fv(&rayStart.x);
			glVertex3fv(&rayEnd.x);
		}
		glEnd();

		glPointSize(6);
		glBegin(GL_POINTS);
		glColor3f(1.0f, 0.0f, 0.0f);
		for (size_t rayId = 0; rayId < numRays; ++rayId)
		{
			//if (car->probeHits[rayId] > 0.0f)
			{
				const auto& r = car->probes[rayId];
				const auto hitPos = car->body->localToWorld(r.pos + r.dir * car->probeHits[rayId]);
				glVertex3fv(&hitPos.x);
			}
		}
		glEnd();
	}
}

void GLCar::drawInstance(Car* instance, bool drawBody, bool drawProbes) // TODO: lame hack
{
	auto tmp = car;
	car = instance;
	draw(drawBody, drawProbes);
	car = tmp;
}

}
