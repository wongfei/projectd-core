#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <gl/GLU.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Sim/Simulator.h"
#include "Sim/Track.h"
#include "Car/CarImpl.h"
#include "Core/SharedMemory.h"

#include "Renderer/GLPrimitive.h"
#include "Renderer/GLFont.h"
#include "Renderer/GLChart.h"
#include "Renderer/GLSkyBox.h"
#include "Renderer/GLTrack.h"
#include "Renderer/GLCar.h"

#include "Audio/FmodAudioRenderer.h"
#include "Input/DICarController.h"
#include "Input/KeyboardCarController.h"

#include <chrono>

namespace D {

inline D::vec3f v(const glm::vec3& in) { return D::vec3f(in.x, in.y, in.z); }
inline glm::vec3 v(const D::vec3f& in) { return glm::vec3(in.x, in.y, in.z); }

struct PlaygrounD
{
	PlaygrounD();
	~PlaygrounD();

	void init(int argc, char** argv);
	void initEngine(int argc, char** argv);
	void initGame();
	void initInput();

	void mainLoop();
	void processInput(float dt);
	void updateCamera(float dt);
	void freeFlyCamera(float dt);
	void render();
	void renderWorld();
	void renderUI();

	void initWindow();
	void closeWindow();
	void clearViewport();
	void projPerspective();
	void projOrtho();
	void swap();
	void processEvents();

	void initCharts();
	void renderCharts();
	void renderStats();

	inline bool getkey(SDL_Scancode scan) { return keys_[scan]; }
	inline bool getkeyf(SDL_Scancode scan) { return keys_[scan] ? 1.0f : 0.0f; }

	inline bool asyncKeydown(SDL_Scancode scan) { auto k = keys_[scan]; auto a = asynckeys_[scan]; asynckeys_[scan] = k; return (k && k != a); }
	inline bool asyncKeyup(SDL_Scancode scan) { auto k = keys_[scan]; auto a = asynckeys_[scan]; asynckeys_[scan] = k; return (!k && k != a); }

	//=============================================================================================

	typedef std::chrono::high_resolution_clock clock;

	std::unordered_map<std::string, std::string> options_;

	double simTickRate_ = 1.0 / 333.0;
	double drawTickRate_ = 1.0 / 111.0;
	double hitchRate = 5.0;

	int posx_ = 0;
	int posy_ = 0;
	int width_ = 1280;
	int height_ = 720;
	int swapInterval_ = -1;
	bool fullscreen_ = false;
	bool enableInput_ = true;
	bool enableSound_ = true;

	std::wstring appDir_;
	SDL_Window* appWindow_ = nullptr;
	SDL_GLContext glContext_ = nullptr;
	HWND sysWindow_ = nullptr;
	bool exitFlag_ = false;
	bool keys_[SDL_NUM_SCANCODES] = {};
	bool asynckeys_[SDL_NUM_SCANCODES] = {};

	SimulatorPtr sim_;
	Track* track_ = nullptr;
	Car* car_ = nullptr;
	mat44f pitPos_;

	GLFont font_;
	GLSkyBox sky_;
	GLTrack trackAvatar_;
	GLCar carAvatar_;

	Surface* lookatSurf_ = nullptr;
	vec3f lookatHit_;

	glm::vec3 camPos_ = {};
	glm::vec3 camYpr_ = {90, 0, 0};
	glm::vec3 camFront_ = {};
	glm::vec3 camRight_ = {};
	glm::vec3 camUp_ = {};
	glm::mat4 camView_ = {};

	enum class ECamMode {
		Free = 0,
		Eye,
		Rear,
		COUNT // LAST
	};
	int inpCamMode_ = (int)ECamMode::Eye;
	vec3f eyeOff_;
	float fov_ = 56.0f;
	float eyeTuneSpeed_ = 0.2f;
	float fovTuneSpeed_ = 5.0f;

	glm::vec3 camMove_ = {0, 0, 0};
	bool mouseBtn_[2] = {0, 0};
	float moveSpeed_ = 10.0f;
	float moveSpeedScale_ = 1.0f;
	float mouseSens_ = 0.35f;

	bool simEnabled_ = true;
	bool simStepOnce_ = false;
	bool wireframeMode_ = false;
	bool drawWorld_ = true;
	bool drawSky_ = true;
	bool drawTrackPoints_ = false;
	bool drawNearbyPoints_ = false;
	bool drawCarProbes_ = false;
	bool drawUI_ = true;

	double dt_ = 0;
	double gameTime_ = 0;
	double prevTime = 0;
	double physicsTime_ = 0;
	double simTime_ = 0;
	double drawTime_ = 0;
	double statTime_ = 0;
	double lastHitchTime_ = 0;
	uint64_t simId_ = 0;
	uint64_t drawId_ = 0;

	int skipFrames = 3;
	bool isFirstFrameDone = false;

	// DEBUG

	float maxDt = 0;
	float maxSim = 0;
	float maxDraw = 0;
	int simCount = 0;
	int drawCount = 0;

	float statMaxDt_ = 0;
	float statMaxSim_ = 0;
	float statMaxDraw_ = 0;
	int statSimRate_ = 0;
	int statDrawRate_ = 0;
	int statSimHitches_ = 0;
	int statDrawHitches_ = 0;

	float timeSinceShift_ = 0;
	int lastGear_ = 0;

	GLChart chartDT_;
	ChartSeries serSim_;
	ChartSeries serDraw_;
	ChartSeries serSwap_;
	ChartSeries serIdle_;

	GLChart chartInput_;
	ChartSeries serSteer_;
	ChartSeries serClutch_;
	ChartSeries serBrake_;
	ChartSeries serGas_;

	GLChart chartSA_;
	ChartSeries serSA_[4];

	GLChart chartFF_;
	ChartSeries serFF_;
};

}
