#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
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
#include "Renderer/GLSimulator.h"
#include "Renderer/GLTrack.h"
#include "Renderer/GLCar.h"

#include "Audio/FmodAudioRenderer.h"
#include "Input/DICarController.h"
#include "Input/KeyboardCarController.h"

#include <chrono>

struct nk_context;

namespace D {

inline D::vec3f v(const glm::vec3& in) { return D::vec3f(in.x, in.y, in.z); }
inline glm::vec3 v(const D::vec3f& in) { return glm::vec3(in.x, in.y, in.z); }

struct ISimulatorManager
{
	virtual int getSimCount() const = 0;
	virtual SimulatorPtr getSim(int simId) = 0;
};

struct PlaygrounD
{
	PlaygrounD();
	~PlaygrounD();

	void run(const std::wstring& basePath, bool loadedByPython = false, bool bLoadDemo = false);

	// Init
	void init(const std::wstring& basePath, bool loadedByPython);
	void loadDemo();
	void shut();
	void initEngine(const std::wstring& basePath, bool loadedByPython);
	void initCarController();
	void setSimulator(SimulatorPtr newSim);
	void setActiveCar(int carId);
	void enableCarControls(bool enable);
	void enableCarSound(bool enable);

	// Tick
	void tick();
	void updateSimStats(float dt, float gameTime);
	void processInput(float dt);
	void updateCamera(float dt);
	void freeFlyCamera(float dt);

	// Render
	void render();
	void renderWorld();
	void render2D();

	// Window
	void initWindow();
	void closeWindow();
	void moveWindow(int x, int y);
	void resizeWindow(int w, int h);
	void clearViewport();
	void projPerspective();
	void projOrtho();
	void swap();
	void processEvents();

	// Debug
	void initCharts();
	void renderCharts();
	void renderStats();
	void renderCarStats(float x, float y, float dy);

	// Nuklear
	void initNuk();
	void shutNuk();
	void beginInputNuk();
	void processEventNuk(SDL_Event* ev);
	void endInputNuk();
	void renderNuk();

	inline bool getkey(SDL_Scancode scan) { return keys_[scan]; }
	inline bool getkeyf(SDL_Scancode scan) { return keys_[scan] ? 1.0f : 0.0f; }

	inline bool asyncKeydown(SDL_Scancode scan) { auto k = keys_[scan]; auto a = asynckeys_[scan]; asynckeys_[scan] = k; return (k && k != a); }
	inline bool asyncKeyup(SDL_Scancode scan) { auto k = keys_[scan]; auto a = asynckeys_[scan]; asynckeys_[scan] = k; return (!k && k != a); }

	//=============================================================================================

	typedef std::chrono::high_resolution_clock clock;

	std::unordered_map<std::string, std::string> options_;

	int simHz_ = 333;
	int drawHz_ = 111;
	double simTickRate_ = 0;
	double drawTickRate_ = 0;
	double hitchRate = 5.0;

	int posx_ = 0;
	int posy_ = 0;
	int width_ = 1280;
	int height_ = 720;
	int swapInterval_ = -1;
	bool fullscreen_ = false;
	bool enableSleep_ = true;

	std::wstring appDir_;
	SDL_Window* appWindow_ = nullptr;
	SDL_GLContext glContext_ = nullptr;
	HWND sysWindow_ = nullptr;
	nk_context* nukContext_ = nullptr;
	GLFont font_;
	bool initializedFlag_ = false;

	bool exitFlag_ = false;
	bool keys_[SDL_NUM_SCANCODES] = {};
	bool asynckeys_[SDL_NUM_SCANCODES] = {};

	std::shared_ptr<FmodContext> fmodContext_;
	std::shared_ptr<ICarControlsProvider> controlsProvider_;

	ISimulatorManager* simManager_ = nullptr; // managed by PyProjectD.cpp
	SimulatorPtr sim_;
	Car* car_ = nullptr;
	Surface* lookatSurf_ = nullptr;
	vec3f lookatHit_;
	mat44f pitPos_;
	int activeSimId_ = -1;
	int activeCarId_ = -1;

	SimulatorPtr newSim_;
	int newSimId_ = -1;
	int newCarId_ = -1;
	bool newCarControls_ = false;
	bool newCarSound_ = false;

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
	float mouseSens_ = 0.2f;

	int simEnabled_ = true;
	int simStepOnce_ = false;
	int wireframeMode_ = false;
	int drawWorld_ = true;
	int drawSky_ = true;
	int drawTrackPoints_ = false;
	int drawNearbyPoints_ = false;
	int drawCarProbes_ = false;
	int draw2D_ = true;

	clock::time_point tick0_;
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
	double trainId_ = 0;

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
