#pragma once

#include "Car/CarCommon.h"
#include "Car/DynamicController.h"
#include "Core/Event.h"

namespace D {

enum class TractionType
{
	RWD = 0x0,
	FWD = 0x1,
	AWD = 0x2,
	AWD_NEW = 0x3,
};

enum class DifferentialType
{
	LSD = 0x0,
	Spool = 0x1,
};

enum class GearChangeRequest
{
	eNoGearRequest = 0x0,
	eChangeUp = 0x1,
	eChangeDown = 0x2,
	eChangeToGear = 0x3,
};

struct GearRatio
{
	double ratio = 0;
};

struct GearElement
{
	double inertia = 0;
	double velocity = 0;
	double oldVelocity = 0;
};

struct GearRequestStatus
{
	GearChangeRequest request = GearChangeRequest(0);
	double timeAccumulator = 0;
	double timeout = 200;
	int requestedGear = -1;
};

struct DifferentialSetting
{
	DifferentialType type = DifferentialType(0);
	double power = 0;
	double coast = 0;
	double preload = 0;
};

struct DrivetrainControllers
{
	std::unique_ptr<DynamicController> singleDiffLock;
	std::unique_ptr<DynamicController> awdFrontShare;
	std::unique_ptr<DynamicController> awdCenterLock;
	std::unique_ptr<DynamicController> awd2;
};

struct OnGearRequestEvent
{
	GearChangeRequest request = GearChangeRequest(0);
	int nextGear = 0;
};

struct Drivetrain : public NonCopyable
{
	Drivetrain();
	~Drivetrain();
	void init(Car* pCar);
	void addGear(const std::wstring& name, double gearRatio);
	void setCurrentGear(int index, bool force);
	bool gearUp();
	bool gearDown();
	void step(float dt);
	void stepControllers(float dt);
	void step2WD(float dt);
	void reallignSpeeds(float dt);
	void accelerateDrivetrainBlock(double acc, bool fromEngine);
	double getInertiaFromWheels();
	double getInertiaFromEngine();
	float getEngineRPM() const;
	float getDrivetrainSpeed() const;
	bool isChangingGear() const;

	// config
	TractionType tractionType = TractionType(0);
	DifferentialType diffType = DifferentialType(0);
	std::vector<GearRatio> gears;
	double diffPowerRamp = 0.7;
	double diffCoastRamp = 0.2;
	double diffPreLoad = 0;
	double finalRatio = 4.0;
	double gearUpTime = 0.1;
	double gearDnTime = 0.15;
	double autoCutOffTime = 0;
	double controlsWindowGain = 0;
	double validShiftRPMWindow = 0;
	double orgRpmWindow = 0;
	double damageRpmWindow = 0;
	double clutchMaxTorque = 0;
	double clutchInertia = 1.0;
	bool isShifterSupported = true;
	bool isGearboxLocked = false;

	// runtime
	Car* car = nullptr;
	Tyre* tyreLeft = nullptr;
	Tyre* tyreRight = nullptr;

	std::unique_ptr<Engine> engineModel;
	std::vector<ITorqueGenerator*> wheelTorqueGenerators;

	GearRequestStatus gearRequest;
	Event<OnGearRequestEvent> evOnGearRequest;

	GearElement engine;
	GearElement drive;
	GearElement outShaftL;
	GearElement outShaftR;
	GearElement outShaftLF;
	GearElement outShaftRF;
	
	DrivetrainControllers controllers;
	DifferentialSetting awdFrontDiff;
	DifferentialSetting awdRearDiff;
	DifferentialSetting awdCenterDiff;

	double locClutch = 1.0;
	double currentClutchTorque = 0;
	double ratio = 12.0;
	double lastRatio = -1.0;
	double cutOff = 0;
	double rootVelocity = 0;
	double totalTorque = 0;
	double awdFrontShare = 0.3;
	int currentGear = 0;
	bool isGearGrinding = false;
	bool clutchOpenState = false;
};

}
