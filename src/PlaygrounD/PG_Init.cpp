#include "PlaygrounD.h"
#include "Audio/FmodContext.h"

namespace D {

//=======================================================================================

PlaygrounD::PlaygrounD() { TRACE_CTOR(PlaygrounD); }
PlaygrounD::~PlaygrounD() { TRACE_DTOR(PlaygrounD); shut(); }

void PlaygrounD::init(const std::string& basePath, bool loadedByPython)
{
	log_printf(L"PlaygrounD: init");

	initEngine(basePath, loadedByPython);
	initCharts();
	initNuk();

	tick0_ = clock::now();
}

//=======================================================================================

void PlaygrounD::shut()
{
	sim_.reset();
	controlsProvider_.reset();

	shutNuk();
	closeWindow();
}

//=======================================================================================

void PlaygrounD::initEngine(const std::string& basePath, bool loadedByPython)
{
	if (!loadedByPython)
		srand(666);

	auto exePath = osGetModuleFullPath();
	auto exeDir = osGetDirPath(exePath);

	if (loadedByPython)
		appDir_ = strw(basePath);
	else
		appDir_ = osCombinePath(exeDir, L"..\\");

	replace(appDir_, L'\\', L'/');
	if (!ends_with(appDir_, L'/'))
		appDir_.append(L"/");

	if (loadedByPython)
	{
		SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		AddDllDirectory(osCombinePath(appDir_, L"bin").c_str());
	}

	if (!loadedByPython)
	{
		const std::wstring logPath = appDir_ + L"projectd.log";
		log_init(logPath.c_str());
	}

	log_printf(L"PlaygrounD: initEngine");
	log_printf(L"exePath: %s", exePath.c_str());
	log_printf(L"appDir: %s", appDir_.c_str());

	#if defined(DEBUG)
	INIReader::_debug = true;
	#endif

	auto ini(std::make_unique<INIReader>(appDir_ + L"cfg/demo.ini"));
	if (ini->ready)
	{
		posx_ = ini->getInt(L"SYS", L"POSX");
		posy_ = ini->getInt(L"SYS", L"POSY");
		width_ = ini->getInt(L"SYS", L"RESX");
		height_ = ini->getInt(L"SYS", L"RESY");
		fullscreen_ = ini->getInt(L"SYS", L"FULLSCREEN") != 0;
		drawHz_ = tclamp(ini->getInt(L"SYS", L"FPS_LIMIT"), 0, 1000);
		swapInterval_ = tclamp(ini->getInt(L"SYS", L"SWAP_INTERVAL"), -1, 1);
		mouseSens_ = ini->getFloat(L"SYS", L"MOUSE_SENS");
	}

	ini = (std::make_unique<INIReader>(appDir_ + L"cfg/sim.ini"));
	if (ini->ready)
	{
		simHz_ = tclamp(ini->getInt(L"SIM", L"STEP_HZ"), 0, 1000);
	}

	initWindow();

	auto rc = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	log_printf(L"IMG_Init: %d", rc);

	font_.initDefault(appDir_);

	fmodContext_.reset(new FmodContext());
	fmodContext_->init(stra(appDir_));

	clearViewport();
	swap();
}

//=======================================================================================

void PlaygrounD::initCarController()
{
	log_printf(L"initCarController");

	bool forceKeyboard = false;
	auto dinputIni(std::make_unique<INIReader>(appDir_ + L"cfg/dinput.ini"));
	if (dinputIni->ready)
	{
		auto inputDev = dinputIni->getString(L"STEER", L"DEVICE");
		forceKeyboard = (inputDev == L"KEYBOARD");
	}

	if (forceKeyboard)
	{
		controlsProvider_ = std::make_shared<KeyboardCarController>(appDir_, sysWindow_);
	}
	else
	{
		auto controlsProvider = std::make_shared<DICarController>(appDir_, sysWindow_);
		if (controlsProvider->diWheel)
		{
			controlsProvider_ = controlsProvider;
		}
		else
		{
			controlsProvider_ = std::make_shared<KeyboardCarController>(appDir_, sysWindow_);
		}
	}

	CarControlsInput input;
	CarControls controls;

	for (int i = 0; i < 3; ++i)
		controlsProvider_->acquireControls(&input, &controls, (float)simTickRate_);
}

//=======================================================================================

void PlaygrounD::setSimulator(SimulatorPtr newSim)
{
	log_printf(L"setSimulator %p -> %p", sim_.get(), newSim.get());

	if (sim_ == newSim)
		return;

	sim_ = newSim;
	if (!sim_)
		return;

	if (!sim_->avatar)
		sim_->avatar.reset(new GLSimulator(sim_.get()));

	track_ = sim_->track.get();

	if (track_ && !track_->pits.empty())
	{
		pitPos_ = track_->pits[0];
		camPos_ = glm::vec3(pitPos_.M41, pitPos_.M42, pitPos_.M43);
	}
}

//=======================================================================================

void PlaygrounD::setActiveCar(int carId, bool takeControls, bool enableSound)
{
	log_printf(L"setActiveCar id=%d takeControls=%d enableSound=%d", carId, (int)takeControls, (int)enableSound);

	if (sim_ && carId >= 0 && carId < (int)sim_->cars.size())
	{
		auto* old = car_;
		car_ = sim_->cars[carId];
		inpCamMode_ = (int)ECamMode::Eye;

		if (old)
		{
			old->lockControls = true;
			old->controlsProvider.reset();

			if (old->avatar)
				((GLCar*)old->avatar.get())->drawBody = true;

			old->audioRenderer.reset();
		}

		if (takeControls && enableCarController_)
		{
			if (!controlsProvider_)
				initCarController();

			controlsProvider_->setCar(car_);
			car_->controlsProvider = controlsProvider_;
			car_->lockControls = false;
		}

		if (enableSound && enableCarSound_)
		{
			if (!car_->audioRenderer)
				car_->audioRenderer = std::make_shared<FmodAudioRenderer>(fmodContext_, stra(appDir_), car_);
		}
	}
}

}
