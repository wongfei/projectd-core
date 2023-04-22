#pragma once

#include "Renderer/GLCore.h"

namespace D {

struct Simulator;

struct GLSimulator : public IAvatar
{
	GLSimulator(Simulator* sim);
	virtual ~GLSimulator();

	virtual void draw() override;

	Simulator* sim = nullptr;
	std::unique_ptr<struct GLSkyBox> skybox;
	bool drawSkybox = true;
};

}
