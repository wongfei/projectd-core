#include "GLSimulator.h"
#include "GLTrack.h"
#include "GLCar.h"
#include "GLSkyBox.h"

#include "Sim/Simulator.h"
#include "Sim/Track.h"
#include "Car/Car.h"

namespace D {

GLSimulator::GLSimulator(Simulator* _sim)
{
	sim = _sim;
}

GLSimulator::~GLSimulator()
{
}

void GLSimulator::draw()
{
	if (drawSkybox)
	{
		if (skybox)
		{
			skybox->draw();
		}
		else
		{
			skybox.reset(new GLSkyBox());
			skybox->load(10000, sim->basePath + L"content/demo/sky%d.jpg");
		}
	}

	if (sim->track)
	{
		if (sim->track->avatar)
			sim->track->avatar->draw();
		else
			sim->track->avatar.reset(new GLTrack(sim->track.get()));
	}

	for (auto* car : sim->cars)
	{
		if (car->avatar)
			car->avatar->draw();
		else
			car->avatar.reset(new GLCar(car));
	}
}

}
