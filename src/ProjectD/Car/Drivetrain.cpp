#include "Car/Drivetrain.h"
#include "Car/Car.h"
#include "Car/Engine.h"
#include "Car/Tyre.h"
#include "Car/ISuspension.h"
#include "Car/ITorqueGenerator.h"
#include "Car/ICoastGenerator.h"
#include "Sim/Simulator.h"

namespace D {

Drivetrain::Drivetrain()
{}

Drivetrain::~Drivetrain()
{}

void Drivetrain::init(Car* _car)
{
	car = _car;

	engine.inertia = 0.01f;
	drive.inertia = 0.01f;

	tyreLeft = car->tyres[2].get();
	tyreRight = car->tyres[3].get();
	outShaftL.inertia = tyreLeft->data.angularInertia;
	outShaftR.inertia = tyreRight->data.angularInertia;

	engineModel.reset(new Engine());
	engineModel->init(car);

	auto ini(std::make_unique<INIReader>(car->carDataPath + L"drivetrain.ini"));
	GUARD_FATAL(ini->ready);

	auto strTracType = ini->getString(L"TRACTION", L"TYPE");
	if (strTracType == L"AWD" || strTracType == L"AWD2")
	{
		TODO_NOT_IMPLEMENTED_FATAL;
	}

	damageRpmWindow = ini->getFloat(L"DAMAGE", L"RPM_WINDOW_K");

	gears.clear();
	addGear(L"R", ini->getFloat(L"GEARS", L"GEAR_R"));
	addGear(L"N", 0.0f);

	int iNumGears = ini->getInt(L"GEARS", L"COUNT");
	for (int i = 1; i <= iNumGears; ++i)
	{
		auto name = strwf(L"%d", i);
		float fRatio = ini->getFloat(L"GEARS", strwf(L"GEAR_%d", i));
		addGear(name, fRatio);
	}

	finalRatio = ini->getFloat(L"GEARS", L"FINAL");
	setCurrentGear(1, true);

	diffPowerRamp = ini->getFloat(L"DIFFERENTIAL", L"POWER");
	diffCoastRamp = ini->getFloat(L"DIFFERENTIAL", L"COAST");
	diffPreLoad = ini->getFloat(L"DIFFERENTIAL", L"PRELOAD");

	if (diffPowerRamp >= 1.0f && diffCoastRamp >= 1.0f)
	{
		diffType = DifferentialType::Spool;
	}

	if (strTracType == L"RWD")
	{
		tractionType = TractionType::RWD;
		car->tyres[0]->driven = false;
		car->tyres[1]->driven = false;
		car->tyres[2]->driven = true;
		car->tyres[3]->driven = true;
	}
	else if (strTracType == L"FWD")
	{
		tractionType = TractionType::FWD;
		car->tyres[0]->driven = true;
		car->tyres[1]->driven = true;
		car->tyres[2]->driven = false;
		car->tyres[3]->driven = false;

		tyreLeft = car->tyres[0].get();
		tyreRight = car->tyres[1].get();
		outShaftL.inertia = tyreLeft->data.angularInertia;
		outShaftR.inertia = tyreRight->data.angularInertia;
	}
	else if (strTracType == L"AWD")
	{
		tractionType = TractionType::AWD;
		car->tyres[0]->driven = true;
		car->tyres[1]->driven = true;
		car->tyres[2]->driven = true;
		car->tyres[3]->driven = true;

		TODO_NOT_IMPLEMENTED_FATAL;
	}
	else if (strTracType == L"AWD2")
	{
		tractionType = TractionType::AWD_NEW;
		car->tyres[0]->driven = true;
		car->tyres[1]->driven = true;
		car->tyres[2]->driven = true;
		car->tyres[3]->driven = true;

		TODO_NOT_IMPLEMENTED_FATAL;
	}
	else
	{
		SHOULD_NOT_REACH_FATAL;
	}

	gearUpTime = ini->getFloat(L"GEARBOX", L"CHANGE_UP_TIME") * 0.001f;
	if (gearUpTime == 0.0f)
		gearUpTime = 0.1f;

	gearDnTime = ini->getFloat(L"GEARBOX", L"CHANGE_DN_TIME") * 0.001f;
	if (gearDnTime == 0.0f)
		gearDnTime = 0.15f;

	autoCutOffTime = ini->getFloat(L"GEARBOX", L"AUTO_CUTOFF_TIME") * 0.001f;
	isShifterSupported = (ini->getInt(L"GEARBOX", L"SUPPORTS_SHIFTER") != 0);

	validShiftRPMWindow = ini->getFloat(L"GEARBOX", L"VALID_SHIFT_RPM_WINDOW");
	if (validShiftRPMWindow == 0.0)
		validShiftRPMWindow = 500.0;

	orgRpmWindow = validShiftRPMWindow;

	controlsWindowGain = ini->getFloat(L"GEARBOX", L"CONTROLS_WINDOW_GAIN");

	float fGearboxInertia = ini->getFloat(L"GEARBOX", L"INERTIA");
	if (fGearboxInertia != 0.0f)
	{
		clutchInertia = fGearboxInertia;
		drive.inertia = fGearboxInertia;
	}

	clutchMaxTorque = ini->getFloat(L"CLUTCH", L"MAX_TORQUE");
	if (clutchMaxTorque == 0.0)
		clutchMaxTorque = 450.0;

	if (tractionType == TractionType::RWD) // TODO: check
	{
		auto strIniPath(car->carDataPath + L"ctrl_single_lock.ini");
		if (osFileExists(strIniPath)) // TODO: what cars?
		{
			controllers.singleDiffLock.reset(new DynamicController(car, strIniPath));
		}
	}
}

void Drivetrain::reset()
{
	clutchOpenState = true;
	rootVelocity = 0;
	engine.velocity = 0;
	outShaftL.velocity = 0;
	outShaftR.velocity = 0;
	outShaftLF.velocity = 0;
	outShaftRF.velocity = 0;
	drive.velocity = 0;
	gearRequest.request = GearChangeRequest::eNoGearRequest;
	validShiftRPMWindow = orgRpmWindow;
	engineModel->reset();
}

void Drivetrain::addGear(const std::wstring& name, double gearRatio)
{
	gears.emplace_back(GearRatio{gearRatio});
}

void Drivetrain::setCurrentGear(int index, bool force)
{
	if (index != 1 && isGearboxLocked)
	{
		currentGear = 1;
	}
	else
	{
		isGearGrinding = false;

		if (index >= 0 && index < (int)gears.size() && index != currentGear)
		{
			double v8 = fabs(
				engine.velocity - gears[index].ratio * drive.velocity * finalRatio);

			double v9 = ((car->controls.gas * locClutch * v8 - locClutch * v8)
				* controlsWindowGain + locClutch * v8)
				* (1.0f / (2.0f * M_PI)) * 60.0f;

			if (index == 1 || force || v9 < validShiftRPMWindow)
			{
				currentGear = index;
			}
			else
			{
				isGearGrinding = true;

				if (validShiftRPMWindow > 0.0)
				{
					double fRate = car->sim->mechanicalDamageRate;
					if (fRate > 0.0)
						validShiftRPMWindow -= damageRpmWindow * fRate * 0.003; // TODO: dt?
				}
			}
		}
	}
}

bool Drivetrain::gearUp()
{
	if (isGearboxLocked)
		return false;

	int iCurGear = currentGear;
	int iReqGear = iCurGear + 1;

	if (iReqGear >= (int)gears.size())
		return false;

	if (gearRequest.request != GearChangeRequest::eNoGearRequest)
		return false;

	gearRequest.request = GearChangeRequest::eChangeUp;
	gearRequest.timeAccumulator = 0;
	gearRequest.timeout = gearUpTime;
	gearRequest.requestedGear = iReqGear;

	OnGearRequestEvent e;
	e.request = gearRequest.request;
	e.nextGear = iReqGear;

	for (auto& h : evOnGearRequest.handlers)
	{
		if (h.second)
			(h.second)(e);
	}

	if (autoCutOffTime != 0.0f)
		cutOff = autoCutOffTime;

	currentGear = 1;
	return true;
}

bool Drivetrain::gearDown()
{
	if (isGearboxLocked)
		return false;

	int iCurGear = currentGear;
	int iReqGear = iCurGear - 1;

	if (iCurGear <= 0)
		return false;

	if (gearRequest.request != GearChangeRequest::eNoGearRequest)
		return false;

	gearRequest.request = GearChangeRequest::eChangeDown;
	gearRequest.timeAccumulator = 0;
	gearRequest.timeout = gearDnTime;
	gearRequest.requestedGear = iReqGear;

	OnGearRequestEvent e;
	e.request = gearRequest.request;
	e.nextGear = iReqGear;

	for (auto& h : evOnGearRequest.handlers)
	{
		if (h.second)
			(h.second)(e);
	}

	currentGear = 1;
	return true;
}

void Drivetrain::step(float dt)
{
	outShaftLF.oldVelocity = outShaftLF.velocity;
	outShaftRF.oldVelocity = outShaftRF.velocity;
	outShaftL.oldVelocity = outShaftL.velocity;
	outShaftR.oldVelocity = outShaftR.velocity;

	locClutch = powf(car->controls.clutch, 1.5f);
	currentClutchTorque = 0;

	stepControllers(dt);

	switch (tractionType)
	{
		case TractionType::RWD:
		case TractionType::FWD:
			step2WD(dt);
			break;

		case TractionType::AWD:
			TODO_NOT_IMPLEMENTED_FATAL; //step4WD(dt);
			break;

		case TractionType::AWD_NEW:
			TODO_NOT_IMPLEMENTED_FATAL; //step4WD_new(dt);
			break;

		default:
			SHOULD_NOT_REACH_FATAL;
			break;
	}
}

void Drivetrain::step2WD(float dt)
{
	int iGearRequest = (int)gearRequest.request - 1;
	if ((!iGearRequest || iGearRequest == 1) && (gearRequest.timeout < gearRequest.timeAccumulator))
	{
		currentGear = gearRequest.requestedGear;
		gearRequest.request = GearChangeRequest::eNoGearRequest;
	}

	if (gearRequest.request != GearChangeRequest::eNoGearRequest)
	{
		gearRequest.timeAccumulator += dt;
	}

	const GearRatio& curGear = gears[currentGear];
	ratio = finalRatio * curGear.ratio;
	engine.inertia = engineModel->inertia;

	for (auto* pTorqGen : wheelTorqueGenerators)
	{
		float fTorq = pTorqGen->getOutputTorque() * 0.5f;
		tyreLeft->status.feedbackTorque += fTorq;
		tyreRight->status.feedbackTorque += fTorq;
	}

	if (lastRatio != ratio)
	{
		reallignSpeeds(dt);
		lastRatio = ratio;
	}

	EngineInput input;

	if (cutOff > 0.0)
	{
		cutOff -= dt;
	}
	else
	{
		input.gasInput = car->controls.gas;
	}

	input.rpm = (float)((engine.velocity * 0.15915507) * 60.0);
	engineModel->step(input, dt);

	if (locClutch < 1.0f)
	{
		clutchOpenState = true;
	}
	else if (engine.velocity != 0.0)
	{
		clutchOpenState = (fabs(rootVelocity / engine.velocity - 1.0) >= 0.1);
	}
	else
	{
		clutchOpenState = (rootVelocity != 0.0);
	}

	double fEngineInertia = engine.inertia;
	double fNewEngineInertia = fEngineInertia;

	if (ratio != 0.0)
	{
		double fInertiaSum = drive.inertia + outShaftL.inertia + outShaftR.inertia;

		if (tractionType == TractionType::AWD) // TODO: AWD in step2WD???
		{
			fInertiaSum += (outShaftLF.inertia + outShaftRF.inertia);
		}

		fNewEngineInertia = fInertiaSum / (ratio * ratio) + clutchInertia + fEngineInertia;
	}

	double fInertiaFromWheels = getInertiaFromWheels();
	double fDeltaDriveV = 0;
	double fClutchTorq = 0;

	if (!clutchOpenState)
	{
		double fDeltaRootV = (engineModel->status.outTorque / fNewEngineInertia) * dt;
		rootVelocity += fDeltaRootV;

		if (ratio == 0.0)
		{
			fDeltaDriveV = (tyreRight->status.feedbackTorque + tyreLeft->status.feedbackTorque) / fInertiaFromWheels * dt;
			drive.velocity += fDeltaDriveV;
		}
		else
		{
			accelerateDrivetrainBlock(fDeltaRootV / ratio, true);

			fDeltaDriveV = (tyreRight->status.feedbackTorque + tyreLeft->status.feedbackTorque) / fInertiaFromWheels * dt;
			rootVelocity += fDeltaDriveV * ratio;
			drive.velocity += fDeltaDriveV;
		}
	}
	else
	{
		fClutchTorq = -((engine.velocity - rootVelocity) / (fabs(engine.velocity - rootVelocity) + 4.0) * (locClutch * clutchMaxTorque));
		currentClutchTorque = fClutchTorq;

		if (ratio != 0.0)
		{
			engine.velocity += (fClutchTorq + engineModel->status.outTorque) / fEngineInertia * dt;

			double fDeltaRootV = (-fClutchTorq / (fNewEngineInertia - fEngineInertia)) * dt;
			rootVelocity += fDeltaRootV;

			accelerateDrivetrainBlock(fDeltaRootV / ratio, true);

			fDeltaDriveV = (tyreRight->status.feedbackTorque + tyreLeft->status.feedbackTorque) / fInertiaFromWheels * dt;
			rootVelocity += fDeltaDriveV * ratio;
			drive.velocity += fDeltaDriveV;
		}
		else
		{
			double fNewEngineVelocity = engine.velocity + engineModel->status.outTorque / fEngineInertia * dt;
			engine.velocity = fNewEngineVelocity;
			rootVelocity = fNewEngineVelocity;

			fDeltaDriveV = (tyreRight->status.feedbackTorque + tyreLeft->status.feedbackTorque) / fInertiaFromWheels * dt;
			drive.velocity += fDeltaDriveV;
		}
	}

	if (tractionType == TractionType::AWD) // TODO: AWD in step2WD???
	{
		fDeltaDriveV = fDeltaDriveV * 0.5 * 2.0;
		outShaftLF.velocity += fDeltaDriveV;
		outShaftRF.velocity += fDeltaDriveV;
	}

	outShaftL.velocity += fDeltaDriveV;
	outShaftR.velocity += fDeltaDriveV;

	if (diffType == DifferentialType::Spool)
	{
		outShaftL.velocity = drive.velocity;
		outShaftR.velocity = drive.velocity;
	}
	else if (diffType == DifferentialType::LSD)
	{
		double fOutClutchTorq, fDiffLoad;

		if (fClutchTorq != 0.0)
			fOutClutchTorq = -fClutchTorq;
		else
			fOutClutchTorq = locClutch * engineModel->status.outTorque;

		if (fOutClutchTorq <= 0.0)
			fDiffLoad = fabs(ratio * diffCoastRamp * fOutClutchTorq);
		else
			fDiffLoad = fabs(ratio) * (diffPowerRamp * fOutClutchTorq);

		double fDiffTotalLoad = fDiffLoad + diffPreLoad;

		if (fabs(outShaftL.velocity - drive.velocity) >= 0.1
			|| fabs(tyreRight->status.feedbackTorque - tyreLeft->status.feedbackTorque) > fDiffTotalLoad)
		{
			double fUnk1 = -((outShaftL.velocity - outShaftR.velocity) / (fabs(outShaftL.velocity - outShaftR.velocity) + 0.01) * fDiffTotalLoad);
			double fDeltaV1 = dt * (fUnk1 / outShaftL.inertia * 0.5);
			outShaftL.velocity += fDeltaV1;
			outShaftR.velocity -= fDeltaV1;

			double fDeltaV2 = dt * ((tyreRight->status.feedbackTorque - tyreLeft->status.feedbackTorque) / outShaftR.inertia * 0.5);
			outShaftL.velocity -= fDeltaV2;
			outShaftR.velocity += fDeltaV2;
		}
		else
		{
			outShaftL.velocity = drive.velocity;
			outShaftR.velocity = drive.velocity;
		}
	}
	else
	{
		SHOULD_NOT_REACH_FATAL;
	}

	if (tyreLeft->status.isLocked && tyreRight->status.isLocked)
	{
		float fTorqL = (tyreLeft->absOverride * tyreLeft->inputs.brakeTorque) + tyreLeft->inputs.handBrakeTorque;
		float fTorqR = (tyreRight->absOverride * tyreRight->inputs.brakeTorque) + tyreRight->inputs.handBrakeTorque;
		bool bFlag = true;

		if (fabs(ratio * engineModel->status.outTorque) <= (fTorqL + fTorqR))
		{
			if (car->speed.ms() <= 1.0f)
				bFlag = false;
		}

		if (bFlag)
		{
			tyreLeft->status.isLocked = false;
			tyreRight->status.isLocked = false;
		}
		else if (clutchOpenState)
		{
			rootVelocity = 0;
			drive.velocity = 0;
			outShaftL.velocity = 0;
			outShaftR.velocity = 0;
		}
	}

	if (tractionType != TractionType::AWD_NEW) // TODO: AWD_NEW in step2WD???
	{
		if (!clutchOpenState)
			engine.velocity = rootVelocity;

		if (ratio != 0.0)
		{
			DEBUG_GUARD_WARN((fabs(drive.velocity - (rootVelocity / ratio)) <= 0.5));
		}

		tyreLeft->status.angularVelocity = (float)outShaftL.velocity;
		tyreRight->status.angularVelocity = (float)outShaftR.velocity;

		tyreLeft->stepRotationMatrix(dt);
		tyreRight->stepRotationMatrix(dt);
	}

	if (ratio == 0.0)
	{
		totalTorque = fabs(engineModel->status.outTorque * locClutch);
	}
	else
	{
		totalTorque = fabs((fabs(ratio) * (engineModel->status.outTorque * locClutch)) - (tyreLeft->status.feedbackTorque + tyreRight->status.feedbackTorque));
	}

	float fGearTorque = (float)(locClutch * engineModel->status.outTorque * curGear.ratio);

	if (car->suspensionTypeR == SuspensionType::Axle)
	{
		float fAxleTorq = fGearTorque * car->axleTorqueReaction;

		car->body->addLocalTorque(vec3f(0, 0, fAxleTorq));
		car->rigidAxle->addLocalTorque(vec3f(0, 0, -fAxleTorq));

		if (car->torqueModeEx == TorqueModeEX::reactionTorques)
		{
			vec3f vTorq = vec3f(-fGearTorque, 0, 0);

			if (car->axleTorqueReaction == 0.0f)
				car->body->addLocalTorque(vTorq);
			else
				car->rigidAxle->addLocalTorque(vTorq);
		}
	}
	else if (car->torqueModeEx == TorqueModeEX::reactionTorques && !tyreLeft->status.isLocked && !tyreRight->status.isLocked)
	{
		int suspId[2];
		if (tractionType == TractionType::FWD)
		{
			suspId[0] = 0;
			suspId[1] = 1;
		}
		else
		{
			suspId[0] = 2;
			suspId[1] = 3;
		}

		for (int i = 0; i < 2; ++i)
		{
			mat44f mxHubWorld = car->suspensions[suspId[i]]->getHubWorldMatrix();
			vec3f vTorq = vec3f(&mxHubWorld.M11) * -(fGearTorque * 0.5f);
			car->body->addTorque(vTorq);
		}
	}
}

void Drivetrain::stepControllers(float dt)
{
	if (controllers.awdFrontShare.get())
	{
		awdFrontShare = controllers.awdFrontShare->eval();
	}

	if (controllers.awdCenterLock.get())
	{
		float fCenterLock = controllers.awdCenterLock->eval();
		float fScale = tclamp(((car->speed.kmh() - 5.0f) * 0.05f), 0.0f, 1.0f);

		awdCenterDiff.preload = ((fCenterLock - 20.0f) * fScale) + 20.0f;
		awdCenterDiff.power = 0;
	}

	if (controllers.singleDiffLock.get())
	{
		diffPreLoad = controllers.singleDiffLock->eval();
		diffPowerRamp = 0;
	}
}

void Drivetrain::reallignSpeeds(float dt)
{
	double fRatio = ratio;
	if (fRatio != 0.0)
	{
		double fDriveVel = drive.velocity;
		if (locClutch <= 0.9f)
		{
			rootVelocity = fDriveVel * fRatio;
		}
		else
		{
			rootVelocity -=
				(1.0 - engine.inertia / getInertiaFromEngine())
				* (rootVelocity / fRatio - fDriveVel)
				* fabs(fRatio);
		}

		accelerateDrivetrainBlock((rootVelocity / fRatio - fDriveVel), false);

		if (!clutchOpenState)
			engine.velocity = rootVelocity;

		DEBUG_GUARD_WARN((fabs(drive.velocity - (rootVelocity / fRatio)) <= 0.5));
	}
}

void Drivetrain::accelerateDrivetrainBlock(double acc, bool fromEngine) // TODO: check
{
	drive.velocity += acc;

	if (tractionType == TractionType::AWD)
	{
		double fShare = 0.5;
		if (fromEngine)
			fShare = awdFrontShare;

		double fDeltaVel = fShare * acc * 2.0;
		outShaftRF.velocity += fDeltaVel;
		outShaftLF.velocity += fDeltaVel;

		fDeltaVel = (1.0 - fShare) * acc * 2.0;
		outShaftR.velocity += fDeltaVel;
		outShaftL.velocity += fDeltaVel;
	}
	else if (diffType == DifferentialType::LSD || diffType == DifferentialType::Spool)
	{
		outShaftR.velocity += acc;
		outShaftL.velocity += acc;
	}
}

double Drivetrain::getInertiaFromWheels()
{
	double fRatio = ratio;
	double fRatioSq = fRatio * fRatio;
	double fRWD = drive.inertia + (outShaftL.inertia + outShaftR.inertia);

	switch (tractionType)
	{
		case TractionType::RWD:
		case TractionType::FWD:
		case TractionType::AWD_NEW: // TODO: why not with AWD?
		{
			if (fRatio == 0.0)
				return fRWD;

			if (clutchOpenState)
				return fRWD + (clutchInertia * fRatioSq);
			else
				return fRWD + ((clutchInertia + engine.inertia) * fRatioSq);
		}

		case TractionType::AWD:
		{
			double fAWD = fRWD + (outShaftLF.inertia + outShaftRF.inertia);

			if (fRatio == 0.0)
				return fAWD;

			if (clutchOpenState)
				return fAWD + (clutchInertia * fRatioSq);
			else
				return fAWD + ((clutchInertia + engine.inertia) * fRatioSq);
		}
	}

	SHOULD_NOT_REACH_FATAL;
}

double Drivetrain::getInertiaFromEngine()
{
	double fRatio = ratio;
	if (fRatio == 0.0)
		return engine.inertia;

	double fRWD = drive.inertia + outShaftL.inertia + outShaftR.inertia;

	switch (tractionType)
	{
		case TractionType::RWD:
		case TractionType::FWD:
		case TractionType::AWD_NEW: // TODO: why not with AWD?
		{
			return fRWD / (fRatio * fRatio) + clutchInertia + engine.inertia;
		}

		case TractionType::AWD:
		{
			double fAWD = fRWD + (outShaftLF.inertia + outShaftRF.inertia);
			return fAWD / (fRatio * fRatio) + clutchInertia + engine.inertia;
		}
	}

	SHOULD_NOT_REACH_FATAL;
}

float Drivetrain::getEngineRPM() const
{
	return (float)((engine.velocity * 0.15915507) * 60.0);
}

float Drivetrain::getDrivetrainSpeed() const
{
	return (float)drive.velocity;
}

bool Drivetrain::isChangingGear() const
{
	return gearRequest.request != GearChangeRequest::eNoGearRequest;
}

}
