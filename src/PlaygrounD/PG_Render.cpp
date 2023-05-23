#include "PlaygrounD.h"

namespace D {

void PlaygrounD::render()
{
	const auto renderThreadId = osGetCurrentThreadId();

	#if 1
		if (sim_ && sim_->track && sim_->physicsThreadId == renderThreadId)
		{
			TrackRayCastHit hit;
			sim_->track->rayCast(v(camPos_), v(camFront_), 1000.0f, hit);
			lookatSurf_ = hit.surface;
			lookatHit_ = hit.pos;
		}
	#endif

	clearViewport();

	if (drawWorld_)
	{
		renderWorld();
	}

	if (draw2D_)
	{
		render2D();
	}
}

void PlaygrounD::renderWorld()
{
	projPerspective();

	if (sim_ && sim_->avatar)
	{
		auto simAvatar = (GLSimulator*)sim_->avatar.get();
		simAvatar->drawSkybox = drawSky_;

		glEnable(GL_DEPTH_TEST);
		GLint oldDepthFunc = 0;
		glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
		glDepthFunc(GL_LESS);

		if (sim_->track)
		{
			auto trackAvatar = (GLTrack*)sim_->track->avatar.get();
			if (trackAvatar)
			{
				trackAvatar->setCamera(v(camPos_), v(camFront_));
				trackAvatar->wireframe = wireframeMode_;
				trackAvatar->drawFatPoints = drawTrackPoints_;
				trackAvatar->drawNearbyPoints = drawNearbyPoints_;
				trackAvatar->drawSenseiPoints = drawTrackSensei_;
			}
		}

		if (car_ && car_->avatar)
		{
			auto carAvatar = (GLCar*)car_->avatar.get();
			carAvatar->drawBody = (inpCamMode_ != (int)ECamMode::Eye);
			carAvatar->drawProbes = drawCarProbes_;
			carAvatar->drawSenseiPoints = drawCarSensei_;
			carAvatar->drawSplineLocation = drawCarSplineLoc_;
		}

		sim_->avatar->draw();

		auto& debugGL = DebugGL::get();
		
		glBegin(GL_POINTS);
		for (const auto& prim : debugGL.points)
		{
			glColor3fv(&prim.color.x);
			glVertex3fv(&prim.p.x);
		}
		glEnd();

		glBegin(GL_LINES);
		for (const auto& prim : debugGL.lines)
		{
			glColor3fv(&prim.color.x);
			glVertex3fv(&prim.p1.x);
			glVertex3fv(&prim.p2.x);
		}
		glEnd();

		glDisable(GL_DEPTH_TEST);
		glDepthFunc(oldDepthFunc);
	}

	if (sim_ && !sim_->dbgCollisions.empty())
	{
		glPointSize(3);
		glBegin(GL_POINTS);
		for (auto& c : sim_->dbgCollisions)
		{
			glColor3f(1.0f, 1.0f, 0.0f);
			glVertex3fv(&c.pos.x);
		}
		glEnd();
	}

	if (inpCamMode_ == (int)ECamMode::Free)
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		glPointSize(3);
		glBegin(GL_POINTS);
		{
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3fv(&lookatHit_.x);
		}
		glEnd();
	}

	glPointSize(1);
	glLineWidth(1);
}

void PlaygrounD::render2D()
{
	projOrtho();

	glEnable(GL_TEXTURE_2D);
	renderCharts();
	renderStats();
	glDisable(GL_TEXTURE_2D);

	renderNuk();
}

}
