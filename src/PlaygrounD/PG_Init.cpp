#include "PlaygrounD.h"
#include "Audio/FmodContext.h"
#include <thread>

namespace D {

//=======================================================================================

PlaygrounD::PlaygrounD() { TRACE_CTOR(PlaygrounD); }
PlaygrounD::~PlaygrounD() { TRACE_DTOR(PlaygrounD); shut(); }

//=======================================================================================

void PlaygrounD::run(const std::wstring& basePath, bool loadedByPython, bool bLoadDemo)
{
	log_printf(L"ENTER PlaygrounD::run");

	init(basePath, loadedByPython);

	if (bLoadDemo)
		loadDemo();

	log_printf(L"MAIN LOOP");
	while (!exitFlag_)
	{
		tick();
	}

	shut();

	log_printf(L"LEAVE PlaygrounD::run");
}

//=======================================================================================

void PlaygrounD::init(const std::wstring& basePath, bool loadedByPython)
{
	log_printf(L"PlaygrounD: init");

	initEngine(basePath, loadedByPython);
	initCharts();
	initNuk();

	gameStartTicks_ = clock::now();

	initializedFlag_ = true;
}

//=======================================================================================

void PlaygrounD::shut()
{
	if (simManager_)
	{
		const int n = simManager_->getSimCount();
		for (int i = 0; i < n; ++i)
		{
			auto sim = simManager_->getSim(i);
			if (sim)
			{
				for (auto* car : sim->cars)
				{
					car->controlsProvider.reset();
					car->audioRenderer.reset();
				}
			}
		}
	}

	sim_.reset();
	controlsProvider_.reset();
	fmodContext_.reset();

	shutNuk();
	closeWindow();

	initializedFlag_ = false;
}

//=======================================================================================

void PlaygrounD::initEngine(const std::wstring& basePath, bool loadedByPython)
{
	log_printf(L"PlaygrounD: initEngine");

	appDir_ = basePath;
	replace(appDir_, L'\\', L'/');
	if (!ends_with(appDir_, L'/')) { appDir_.append(L"/"); }

	log_printf(L"baseDir=%s", appDir_.c_str());

	if (loadedByPython)
	{
		SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		AddDllDirectory(osCombinePath(appDir_, L"bin").c_str());
	}
	else
	{
		srand(666);
	}

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
		enableSleep_ = ini->getInt(L"SYS", L"ENABLE_IDLE_SLEEP") != 0;
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

	initCarController();

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

	sim_ = newSim;
	if (sim_)
	{
		if (!sim_->avatar)
			sim_->avatar.reset(new GLSimulator(sim_.get()));

		auto* track = sim_->track.get();

		if (track && !track->pits.empty())
		{
			pitPos_ = track->pits[0];
			camPos_ = glm::vec3(pitPos_.M41, pitPos_.M42, pitPos_.M43);
		}
	}

	activeSimId_ = sim_ ? sim_->simulatorId : -1;

	setActiveCar(0);
}

//=======================================================================================

void PlaygrounD::setActiveCar(int carId)
{
	log_printf(L"setActiveCar id=%d", carId);

	Car* newCar = nullptr;
	if (sim_ && carId >= 0 && carId < (int)sim_->cars.size())
	{
		newCar = sim_->cars[carId];
	}

	auto* oldCar = car_;
	car_ = newCar;

	if (oldCar && newCar != oldCar)
	{
		if (oldCar->controlsProvider)
		{
			oldCar->controlsProvider->setCar(nullptr);
			oldCar->controlsProvider.reset();
		}

		oldCar->audioRenderer.reset();

		if (oldCar->avatar)
			((GLCar*)oldCar->avatar.get())->drawBody = true;
	}

	activeCarId_ = car_ ? car_->physicsGUID : -1;
	inpCamMode_ = car_ ? (int)ECamMode::Eye : (int)ECamMode::Free;

	enableCarControls(newCarControls_);
	enableCarSound(newCarSound_);
}

void PlaygrounD::enableCarControls(bool enable)
{
	if (car_ && controlsProvider_)
	{
		if (enable)
		{
			controlsProvider_->setCar(car_);
			car_->controlsProvider = controlsProvider_;
			car_->lockControls = false;
		}
		else
		{
			controlsProvider_->setCar(nullptr);
			car_->controlsProvider.reset();
		}
	}
}

void PlaygrounD::enableCarSound(bool enable)
{
	if (car_ && fmodContext_)
	{
		if (enable)
		{
			if (!car_->audioRenderer)
				car_->audioRenderer = std::make_shared<FmodAudioRenderer>(fmodContext_, stra(appDir_), car_);
		}
		else
		{
			car_->audioRenderer.reset();
		}
	}
}

}
