#include "Car/CarImpl.h"
#include "Sim/Simulator.h"
#include "Sim/Track.h"

namespace D {

Car::Car(TrackPtr _track)
:
	track(_track)
{
	TRACE_CTOR(Car);

	sim = track->sim.get();

	bounds.length = 4.0f;
	bounds.width = 2.0f;
	bounds.lengthFront = 2.0f;
	bounds.lengthRear = 2.0f;

	lastBodyMassUpdateTime = -100000000.0;
}

Car::~Car()
{
	TRACE_DTOR(Car);

	sim->unregisterCar(this);
}

//=============================================================================
// INIT
//=============================================================================

bool Car::init(const std::wstring& modelName)
{
	log_printf(L"Car: init: modelName=\"%s\"", modelName.c_str());

	physicsGUID = (unsigned int)sim->cars.size();

	auto& pCore = sim->physics;
	body = pCore->createRigidBody();
	fuelTankBody = pCore->createRigidBody();

	unixName = modelName;
	carDataPath = sim->basePath + L"content/cars/" + unixName + L"/data/";
	log_printf(L"carDataPath=\"%s\"", carDataPath.c_str());

	initCarData();

	fuelTankBody->setMassBox(1.0f, 0.5f, 0.5f, 0.5f); // TODO: check
	fuelTankBody->setPosition(fuelTankPos);
	fuelTankJoint = sim->physics->createFixedJoint(fuelTankBody, body);

	log_printf(L"car body=%p", body.get());
	log_printf(L"car fuelTankBody=%p", fuelTankBody.get());

	// Suspensions

	auto ini(std::make_unique<INIReader>(carDataPath + L"suspensions.ini"));

	auto strRearType = ini->getString(L"REAR", L"TYPE");
	if (strRearType == L"AXLE")
	{
		rigidAxle = pCore->createRigidBody();
		axleTorqueReaction = ini->getFloat(L"AXLE", L"TORQUE_REACTION");
		log_printf(L"car rigidAxle=%p", rigidAxle.get());
	}

	for (int index = 0; index < 4; ++index)
	{
		std::wstring strSuspType;
		SuspensionType eSuspType;
		std::unique_ptr<SuspensionBase> pSusp;

		if (index >= 2)
			strSuspType = ini->getString(L"REAR", L"TYPE");
		else
			strSuspType = ini->getString(L"FRONT", L"TYPE");

		log_printf(L"create suspension id=%d type=%s", index, strSuspType.c_str());

		if (strSuspType == L"STRUT")
		{
			eSuspType = SuspensionType::Strut;
			auto pImpl = new SuspensionStrut(); pSusp.reset(pImpl);
			pImpl->init(pCore, body, index, carDataPath);
		}
		else if (strSuspType == L"DWB")
		{
			eSuspType = SuspensionType::DoubleWishbone;
			auto pImpl = new SuspensionDW(); pSusp.reset(pImpl);
			pImpl->init(pCore, body, antirollBars[0].get(), antirollBars[1].get(), index, carDataPath);
		}
		else if (strSuspType == L"ML")
		{
			eSuspType = SuspensionType::Multilink;
			auto pImpl = new SuspensionML(); pSusp.reset(pImpl);
			pImpl->init(pCore, body, index, carDataPath);
		}
		else if (strSuspType == L"AXLE" && index >= 2)
		{
			eSuspType = SuspensionType::Axle;
			auto eSide = (index == 2) ? RigidAxleSide::Left : RigidAxleSide::Right;
			auto pImpl = new SuspensionAxle(); pSusp.reset(pImpl);
			pImpl->init(pCore, body, rigidAxle, index, eSide, carDataPath);
		}
		else
		{
			SHOULD_NOT_REACH_FATAL;
		}

		if (index >= 2)
			suspensionTypeR = eSuspType;
		else
			suspensionTypeF = eSuspType;

		ISuspension* pSuspInterface = pSusp.get();
		suspensions.emplace_back(pSuspInterface);
		suspensionsImpl.emplace_back(std::move(pSusp));

		auto pTyre = std::make_unique<Tyre>();
		pTyre->init(this, pSuspInterface, sim->track, index, carDataPath);
		tyres.emplace_back(std::move(pTyre));
	}

	for (const auto& compound : tyres[0]->compoundDefs)
	{
		tyreCompounds.emplace_back(compound->name);
	}

	for (int i = 0; i < 4; i += 2)
	{
		if (suspensions[i]->getType() == SuspensionType::DoubleWishbone)
		{
			auto s1 = (SuspensionDW*)suspensions[i];
			auto s2 = (SuspensionDW*)suspensions[i + 1];
			bool isFront = (i == 0);

			auto pSpring = std::make_unique<HeaveSpring>();
			pSpring->init(body.get(), s1, s2, isFront, carDataPath);
			heaveSprings.emplace_back(std::move(pSpring));
		}
	}

	ridePickupPoint[0].z = suspensions[0]->getBasePosition().z;
	ridePickupPoint[1].z = suspensions[2]->getBasePosition().z;

	antirollBars.emplace_back(std::make_unique<AntirollBar>());
	antirollBars.emplace_back(std::make_unique<AntirollBar>());
	antirollBars[0]->init(body.get(), suspensions[0], suspensions[1]);
	antirollBars[1]->init(body.get(), suspensions[2], suspensions[3]);
	antirollBars[0]->k = ini->getFloat(L"ARB", L"FRONT");
	antirollBars[1]->k = ini->getFloat(L"ARB", L"REAR");
	auto strArb0 = carDataPath + L"ctrl_arb_front.ini";
	auto strArb1 = carDataPath + L"ctrl_arb_rear.ini";
	if (osFileExists(strArb0))
	{
		antirollBars[0]->ctrl.init(this, strArb0);
	}
	if (osFileExists(strArb1))
	{
		antirollBars[1]->ctrl.init(this, strArb1);
	}

	// Components

	colliderManager.reset(new CarColliderManager());
	colliderManager->init(this);
	loadColliderBlob();

	slipStream.reset(new SlipStream());
	aeroMap.reset(new AeroMap());
	auto vFrontPos = suspensions[0]->getBasePosition();
	auto vRearPos = suspensions[2]->getBasePosition();
	aeroMap->init(this, vFrontPos, vRearPos);

	water.reset(new ThermalObject());
	water->tmass = 20.0f;
	water->coolSpeedK = 0.002f;

	brakeSystem.reset(new BrakeSystem());
	brakeSystem->init(this);

	steeringSystem.reset(new SteeringSystem());
	steeringSystem->init(this);
	steeringSystem->linearRatio = steerLinearRatio;

	drivetrain.reset(new Drivetrain());
	drivetrain->init(this);
	controls.isShifterSupported = drivetrain->isShifterSupported;

	gearChanger.reset(new GearChanger());
	gearChanger->init(this);

	state.reset(new CarState());

	updateBodyMass();

	auto pThis = this;
	sim->evOnStepCompleted.add(this, [pThis](double dt) {
		pThis->postStep((float)dt);
	});

	return true;
}

//=============================================================================

void Car::initCarData()
{
	auto ini(std::make_unique<INIReader>(carDataPath + L"car.ini"));
	GUARD_FATAL(ini->ready);

	screenName = ini->getString(L"INFO", L"SCREEN_NAME");
	mass = ini->getFloat(L"BASIC", L"TOTALMASS");

	if (ini->hasSection(L"EXPLICIT_INERTIA"))
	{
		explicitInertia = ini->getFloat3(L"EXPLICIT_INERTIA", L"INERTIA");
		body->setMassExplicitInertia(mass, explicitInertia.x, explicitInertia.y, explicitInertia.z);
	}
	else
	{
		bodyInertia = ini->getFloat3(L"BASIC", L"INERTIA");
		body->setMassBox(mass, bodyInertia.x, bodyInertia.y, bodyInertia.z);
	}

	if (ini->hasSection(L"FUEL_EXT"))
	{
		fuelKG = ini->getFloat(L"FUEL_EXT", L"KG_PER_LITER");
	}

	ffMult = ini->getFloat(L"CONTROLS", L"FFMULT") * 0.001f;
	steerLock = ini->getFloat(L"CONTROLS", L"STEER_LOCK");
	steerRatio = ini->getFloat(L"CONTROLS", L"STEER_RATIO");

	steerLinearRatio = ini->getFloat(L"CONTROLS", L"LINEAR_STEER_ROD_RATIO");
	if (steerLinearRatio == 0.0f)
		steerLinearRatio = 0.003f;

	steerAssist = ini->getFloat(L"CONTROLS", L"STEER_ASSIST");
	if (steerAssist == 0.0f)
		steerAssist = 1.0f;

	fuelConsumptionK = ini->getFloat(L"FUEL", L"CONSUMPTION");
	fuel = ini->getFloat(L"FUEL", L"FUEL");
	maxFuel = ini->getFloat(L"FUEL", L"MAX_FUEL");

	if (maxFuel == 0.0f)
		maxFuel = 30.0f;
	if (fuel == 0.0f)
		fuel = 30.0f;
	requestedFuel = (float)fuel;

	float fPickup = ini->getFloat(L"RIDE", L"PICKUP_FRONT_HEIGHT");
	ridePickupPoint[0] = vec3f(0, fPickup, 0);

	fPickup = ini->getFloat(L"RIDE", L"PICKUP_REAR_HEIGHT");
	ridePickupPoint[1] = vec3f(0, fPickup, 0);

	fuelTankPos = ini->getFloat3(L"FUELTANK", L"POSITION");

	graphicsOffset = ini->getFloat3(L"BASIC", L"GRAPHICS_OFFSET");
	graphicsPitchRotation = ini->getFloat(L"BASIC", L"GRAPHICS_PITCH_ROTATION") * (M_PI / 180.0f);
}

//=============================================================================

void Car::loadColliderBlob()
{
	#pragma pack(push, 1)
	struct BlobCollider
	{
		uint32_t magic = 0;
		uint32_t numVertices = 0;
		uint32_t numIndices = 0;
	};
	#pragma pack(pop)

	FileHandle file;
	auto strPath = carDataPath + L"collider.bin";
	GUARD_FATAL(file.open(strPath.c_str(), L"rb"));

	BlobCollider raw;
	GUARD_FATAL(fread(&raw, sizeof(raw), 1, file.fd) == 1);
	GUARD_FATAL(raw.numVertices && raw.numIndices);

	auto mesh = sim->physics->createTriMesh();
	mesh->resize(raw.numVertices, raw.numIndices);
	GUARD_FATAL(fread(mesh->getVB(), raw.numVertices * sizeof(TriMeshVertex), 1, file.fd) == 1);
	GUARD_FATAL(fread(mesh->getIB(), raw.numIndices * sizeof(TriMeshIndex), 1, file.fd) == 1);

	auto gm = getGraphicsOffsetMatrix();
	initColliderMesh(mesh, gm);
}

//=============================================================================

void Car::initColliderMesh(ITriMeshPtr mesh, const mat44f& bodyMatrix)
{
	vec3f vMin(FLT_MAX, FLT_MAX, FLT_MAX);
	vec3f vMax(vMin * -1.0f);

	auto* pVertices = mesh->getVB();
	auto nVertexCount = mesh->getVertexCount();

	for (auto i = 0; i < nVertexCount; ++i)
	{
		auto& v = pVertices[i];

		vMin.x = tmin(vMin.x, v.x);
		vMin.y = tmin(vMin.y, v.y);
		vMin.z = tmin(vMin.z, v.z);

		vMax.x = tmax(vMax.x, v.x);
		vMax.y = tmax(vMax.y, v.y);
		vMax.z = tmax(vMax.z, v.z);
	}

	vec3f vPos(&bodyMatrix.M41);
	bounds.min = vPos + vMin;
	bounds.max = vPos + vMax;
	bounds.length = fabsf(bounds.max.z - bounds.min.z);
	bounds.width = fabsf(bounds.max.x - bounds.min.x);
	bounds.lengthFront = fabsf(bounds.max.z);
	bounds.lengthRear = fabsf(bounds.min.z);

	body->addMeshCollider(mesh, bodyMatrix, physicsGUID + 1, C_CATEGORY_CAR, C_MASK_CAR_MESH);
	collider = mesh;
}

//=============================================================================
// STEP
//=============================================================================

void Car::stepPreCacheValues(float dt)
{
	speed.value = body->getVelocity().len();
}

//=============================================================================

void Car::step(float dt)
{
	if (!physicsGUID)
	{
		vec3f vBodyVelocity = body->getVelocity();
		float fVelSq = vBodyVelocity.sqlen();
		float fERP;

		if (fVelSq >= 1.0f)
		{
			fERP = 0.3f;
			for (auto* pSusp : suspensions)
			{
				auto* pImpl = (SuspensionBase*)pSusp;
				pSusp->setERPCFM(fERP, pImpl->baseCFM);
			}
		}
		else
		{
			fERP = 0.9f;
			for (auto& pSusp : suspensions)
			{
				pSusp->setERPCFM(fERP, 0.0000001f);
			}
		}

		fuelTankJoint->setERPCFM(fERP, -1.0f);
	}

	if (controlsProvider)
	{
		pollControls(dt);
	}

	updateAirPressure();

	float fRpmAbs = fabsf(drivetrain->getEngineRPM());
	float fTurboBoost = tmax(0.0f, drivetrain->engineModel->status.turboBoost);
	double fNewFuel = fuel - (fRpmAbs * dt * drivetrain->engineModel->gasUsage) * (fTurboBoost + 1.0) * fuelConsumptionK * 0.001 * sim->fuelConsumptionRate;
	fuel = fNewFuel;

	if (fNewFuel > 0.0f)
	{
		drivetrain->engineModel->fuelPressure = 1.0f;
	}
	else
	{
		fuel = 0;
		drivetrain->engineModel->fuelPressure = 0;
	}

	updateBodyMass();

	float fSteerAngleSig = (steerLock * controls.steer) / steerRatio;
	if (!isfinite(fSteerAngleSig))
	{
		SHOULD_NOT_REACH_WARN;
		fSteerAngleSig = 0;
	}

	finalSteerAngleSignal = fSteerAngleSig;

	bool bAllTyresLoaded = true;
	for (int i = 0; i < 4; ++i)
	{
		if (tyres[i]->status.load <= 0.0f)
		{
			bAllTyresLoaded = false;
			break;
		}
	}

	//autoClutch.step(dt);

	float fSpeed = speed.ms();
	vec3f fAngVel = body->getAngularVelocity();
	float fAngVelSq = fAngVel.sqlen();

	if (fSpeed >= 0.5f || fAngVelSq >= 1.0f)
	{
		sleepingFrames = 0;
	}
	else
	{
		if (bAllTyresLoaded 
			&& (controls.gas <= 0.01f 
				|| controls.clutch <= 0.01f 
				|| drivetrain->currentGear == 1))
		{
			sleepingFrames++;
		}
		else
		{
			sleepingFrames = 0;
		}

		if (sleepingFrames > framesToSleep)
		{
			body->stop();
			fuelTankBody->stop();
		}
	}

	vec3f vBodyVel = body->getVelocity();
	vec3f vAccel = (vBodyVel - lastVelocity) * (1.0f / dt) * 0.10197838f;
	lastVelocity = vBodyVel;
	accG = body->worldToLocalNormal(vAccel);

	stepThermalObjects(dt);
	stepComponents(dt);
	//updateColliderStatus(dt); // TODO
	//if (!physicsGUID) stepJumpStart(dt); // TODO
}

//=============================================================================

void Car::updateAirPressure()
{
	float fAirDensity = sim->getAirDensity();

	if (slipStreamEffectGain > 0.0f)
	{
		vec3f vPos = body->getPosition(0.0f);
		float fMinSlip = 1.0f;

		for (SlipStream* pSS : sim->slipStreams)
		{
			if (pSS != slipStream.get())
			{
				float fSlip = tclamp((1.0f - (pSS->getSlipEffect(vPos) * slipStreamEffectGain)), 0.0f, 1.0f);

				if (fMinSlip > fSlip)
					fMinSlip = fSlip;
			}
		}

		fAirDensity = ((fAirDensity - (fMinSlip * fAirDensity)) * (0.75f / slipStreamEffectGain)) + (fMinSlip * fAirDensity);
	}

	aeroMap->airDensity = fAirDensity;
}

//=============================================================================

void Car::updateBodyMass()
{
	if (sim->physicsTime - lastBodyMassUpdateTime > 1000.0)
	{
		if (bodyInertia.x != 0.0f || bodyInertia.y != 0.0f || bodyInertia.z != 0.0f)
		{
			float fBodyMass = calcBodyMass();
			body->setMassBox(fBodyMass, bodyInertia.x, bodyInertia.y, bodyInertia.z);
		}
		else
		{
			body->setMassExplicitInertia(mass, explicitInertia.x, explicitInertia.y, explicitInertia.z);
			log_printf(L"setMassExplicitInertia");
		}

		float fFuelMass = tmax(0.1f, (fuelKG * (float)fuel));
		fuelTankBody->setMassBox(fFuelMass, 0.5f, 0.5f, 0.5f); // TODO: check

		lastBodyMassUpdateTime = sim->physicsTime;
	}
}

//=============================================================================

float Car::calcBodyMass()
{
	float fSuspMass = 0;
	for (int i = 0; i < 4; ++i)
		fSuspMass += suspensions[i]->getMass();

	return (mass - fSuspMass) + ballastKG;
}

//=============================================================================

void Car::stepThermalObjects(float dt)
{
	float fRpm = drivetrain->getEngineRPM();
	if (fRpm > (drivetrain->engineModel->data.minimum * 0.8f))
	{
		float fLimiter = (float)drivetrain->engineModel->getLimiterRPM();
		water->addHeadSource((((fRpm / fLimiter) * 20.0f) * controls.gas) + 85.0f);
	}
	
	water->step(dt, sim->ambientTemperature, speed);
}

//=============================================================================

void Car::stepComponents(float dt)
{
	brakeSystem->step(dt);
	//edl.step(dt);

	for (auto& iter : suspensions)
	{
		iter->step(dt);
	}

	for (auto& iter : tyres)
	{
		iter->step(dt);
	}
	sendFF(dt);

	for (auto& iter : heaveSprings)
	{
		if (iter->k != 0.0f)
			iter->step(dt);
	}

	//drs.step(dt);
	aeroMap->step(dt);
	//kers.step(dt);
	//ers.step(dt);
	steeringSystem->step(dt);
	//autoBlip.step(dt);
	//autoShift.step(dt);
	gearChanger->step(dt);
	drivetrain->step(dt);

	for (auto& iter : antirollBars)
	{
		iter->step(dt);
	}

	//abs.step(dt);
	//tractionControl.step(dt);
	//speedLimiter.step(dt);
	//colliderManager.step(dt);
	//stabilityControl.step(dt);
	//fuelLapEvaluator.step(dt);
}

//=============================================================================

void Car::postStep(float dt)
{
	OnStepCompleteEvent e;
	e.car = this;
	e.physicsTime = sim->physicsTime;
	evOnStepComplete.fire(e);

	vec3f vBodyVel = body->getVelocity();
	vec3f vBodyPos = body->getPosition(0);
	slipStream->setPosition(vBodyPos, vBodyVel);

	updateCarState();

	if (audioRenderer)
		audioRenderer->update(dt);
}

//=============================================================================

void Car::updateCarState()
{
	state->controls = controls;

	auto bodyM = body->getWorldMatrix(0);
	state->bodyMatrix = (bodyM);
	state->bodyPos = {bodyM.M41, bodyM.M42, bodyM.M43};
	state->bodyEuler = (bodyM.getEulerAngles());

	auto bodyGM = getGraphicsOffsetMatrix();
	state->graphicsMatrix = (bodyGM);
	state->graphicsPos = {bodyGM.M41, bodyGM.M42, bodyGM.M43};
	state->graphicsEuler = (bodyGM.getEulerAngles());

	for (int i = 0; i < 4; ++i)
	{
		state->hubMatrix[i] = (suspensions[i]->getHubWorldMatrix());
		state->tyreContacts[i] = (tyres[i]->contactPoint);
	}

	state->speedMS = speed.ms();
	state->engineRPM = getEngineRpm();
	state->gear = drivetrain->currentGear;
}

//=============================================================================
// COLLISION
//=============================================================================

void Car::onCollisionCallback(
	void* userData0, void* shape0, 
	void* userData1, void* shape1, 
	const vec3f& normal, const vec3f& pos, float depth)
{
	if (!(body.get() == userData0 || body.get() == userData1))
		return;

	IRigidBody* pOtherBody;
	ICollisionObject* pOtherShape;

	if (body.get() == userData0)
	{
		pOtherBody = (IRigidBody*)userData1;
		pOtherShape = (ICollisionObject*)shape1;
	}
	else
	{
		pOtherBody = (IRigidBody*)userData0;
		pOtherShape = (ICollisionObject*)shape0;
	}

	unsigned long ulOtherGroup = pOtherShape ? pOtherShape->getGroup() : 0;
	unsigned long ulGroup0 = 0;
	unsigned long ulGroup1 = 0;
	bool bFlag0 = false;
	bool bFlag1 = false;

	if (shape0)
	{
		ulGroup0 = ((ICollisionObject*)shape0)->getGroup();
		bFlag0 = (ulGroup0 == 1 || ulGroup0 == 16);
	}

	if (shape1)
	{
		ulGroup1 = ((ICollisionObject*)shape1)->getGroup();
		bFlag1 = (ulGroup1 == 1 || ulGroup1 == 16);
	}

	lastCollisionTime = sim->physicsTime;

	vec3f vPosLocal = body->worldToLocal(pos);
	vec3f vVelocity = body->getPointVelocity(pos);

	vec3f vOtherVelocity(0, 0, 0);
	if (pOtherBody)
	{
		vOtherVelocity = pOtherBody->getPointVelocity(pos);
	}

	vec3f vDeltaVelocity = vVelocity - vOtherVelocity;
	float fRelativeSpeed = -((vDeltaVelocity * normal) * 3.6f);
	float fDamage = fRelativeSpeed * sim->mechanicalDamageRate;

	if (userData0 && userData1 && !bFlag0 && !bFlag1)
		lastCollisionWithCarTime = sim->physicsTime;

	float* pDmg = damageZoneLevel;

	if (fRelativeSpeed > 0.0f && !bFlag0 && !bFlag1)
	{
		if (fRelativeSpeed * sim->mechanicalDamageRate > 150.0f)
			drivetrain->engineModel->blowUp();

		vec3f vNorm = vPosLocal.get_norm();
		int iZoneId;

		if (fabsf(vNorm.z) <= 0.70700002f)
		{
			iZoneId = (vPosLocal.x >= 0.0f) ? 2 : 3;
		}
		else
		{
			iZoneId = (vPosLocal.z <= 0.0f) ? 1 : 0;
		}
		
		pDmg[iZoneId] = tmax(pDmg[iZoneId], fDamage);
		pDmg[4] = tmax(pDmg[4], fDamage);
	}

	#if 0
	if (pDmg[0] > 0.0f && pDmg[2] > 0.0f)
		suspensions[0]->setDamage((pDmg[0] + pDmg[2]) * 0.5f);

	if (pDmg[0] > 0.0f && pDmg[3] > 0.0f)
		suspensions[1]->setDamage((pDmg[0] + pDmg[3]) * 0.5f);

	if (pDmg[1] > 0.0f && pDmg[2] > 0.0f)
		suspensions[2]->setDamage((pDmg[1] + pDmg[2]) * 0.5f);

	if (pDmg[1] > 0.0f && pDmg[3] > 0.0f)
		suspensions[3]->setDamage((pDmg[1] + pDmg[3]) * 0.5f);
	#endif

	#if 0
	ACPhysicsEvent pe;
	pe.type = eACEventType::acEvent_OnCollision;
	pe.param1 = (float)physicsGUID;
	pe.param2 = depth;
	pe.param3 = -1;
	pe.param4 = fRelativeSpeed;
	pe.vParam1 = pos;
	pe.vParam2 = normal;
	pe.voidParam0 = nullptr;
	pe.voidParam1 = nullptr;
	pe.ulParam0 = ulOtherGroup; // TODO: ulGroup1 OR ulOtherGroup ?
	sim->eventQueue.push(pe); // processed by Sim::stepPhysicsEvent
	#endif

	if (fRelativeSpeed > 0.0f && !bFlag0 && !bFlag1)
	{
		//log_printf(L"collision %u %u %.3f", (UINT)ulGroup0, (UINT)ulGroup1, fRelativeSpeed);

		OnCollisionEvent ce;
		ce.body = pOtherBody;
		ce.relativeSpeed = fRelativeSpeed;
		ce.worldPos = pos;
		ce.relPos = vPosLocal;
		ce.colliderGroup = ulOtherGroup; // TODO: ulGroup1 OR ulOtherGroup ?
		evOnCollisionEvent.fire(ce);
	}
}

//=============================================================================
// CONTROLS
//=============================================================================

void Car::pollControls(float dt)
{
	VibrationDef vd;

	CarControlsInput input = {steerLock, speed.value};
	controlsProvider->acquireControls(&input, &controls, dt);

	float fSpeedN = tclamp(speed.value, 0.0f, 1.0f);

	vibrationPhase += speed.value * dt;
	slipVibrationPhase += dt;

	float fVibSum = 0;
	float fMaxGain = 0;
	float fMaxSlip = 0;
	int iSurfCount = 0;

	for (int i = 0; i < 4; ++i)
	{
		auto* pSurf = tyres[i]->surfaceDef;
		if (pSurf)
		{
			fMaxGain = tmax(fMaxGain, pSurf->vibrationGain);
			if (pSurf->vibrationLength != 0.0f && pSurf->vibrationGain != 0.0f)
			{
				fVibSum += pSurf->vibrationLength;
				iSurfCount++;
			}
		}
		float fSlip = tyres[i]->status.ndSlip * 0.75f;
		fMaxSlip = tmax(fMaxSlip, fSlip);
	}

	if (!iSurfCount)
	{
		//SHOULD_NOT_REACH_WARN;
		return;
	}

	if (fMaxSlip <= 1.0f)
		fMaxSlip *= fMaxSlip;

	float fVibAvg = fVibSum / (float)iSurfCount;

	if (fVibAvg != 0.0f && fMaxGain != 0.0f)
	{
		float fLoad0 = tclamp(tyres[0]->status.load, 0.0f, 1.0f);
		float fLoad1 = tclamp(tyres[1]->status.load, 0.0f, 1.0f);
		vd.curbs = (((sawToothWave(vibrationPhase, fVibAvg) * fLoad0) * fLoad1) * fMaxGain) * fSpeedN;
	}

	float fPhase30 = sinf(vibrationPhase * 30.0f);
	vd.gforce = tclamp(fabsf(accG.y), 0.0f, 1.0f) * fPhase30;

	float fPhase120 = sinf(vibrationPhase * 120.0f);
	vd.slips = tclamp(fMaxSlip * 0.4f, 0.0f, 1.0f) * fPhase120;

	float v36 = ((float)drivetrain->engine.velocity * 0.15915507f) * 60.0f;
	float v37 = v36 / drivetrain->engineModel->getLimiterRPM();
	vd.engine = tclamp(v37, 0.0f, 1.0f);

	#if 0
	if (abs.isPresent && abs->isInAction())
		vd.abs = ksSquareWave(pksPhysics->physicsTime, 100.0f);
	else
		vd.abs = 0;
	#endif
	
	// TODO: WTF?
	vd.curbs *= fSpeedN;
	vd.slips *= fSpeedN;
	vd.abs *= fSpeedN;

	controlsProvider->setVibrations(&vd);
}

void Car::sendFF(float dt)
{
	if (controlsProvider)
	{
		lastFF = getSteerFF(dt);

		#if 1
		float fLspSpeed = sim->mzLowSpeedReduction.speedKMH;
		float fLspMin = sim->mzLowSpeedReduction.minValue;

		if (fLspSpeed != 0.0f)
		{
			float fReduct = tclamp((speed.value * 3.6f) / fLspSpeed, 0.0f, 1.0f);
			lastFF *= (((1.0f - fLspMin) * fReduct) + fLspMin);
		}
		#endif

		// TODO: mess here!!!
		float fVar4 = tclamp((1.0f - (speed.value * 3.6f * 0.1f)), 0.0f, 1.0f);
		float dmin = sim->ffDamperMinValue;
		float d = ((1.0f - dmin) * fVar4 + dmin) * sim->ffDamperGain;
		lastDamp = d;

		controlsProvider->sendFF(lastFF, d, userFFGain);
	}
	else
	{
		mzCurrent = 0;
	}
}

float Car::getSteerFF(float dt)
{
	float fTorq0 = suspensions[0]->getSteerTorque();
	float fTorq1 = suspensions[1]->getSteerTorque();
	float fTorqFront = fTorq0 + fTorq1;

	float fSteerPos = controls.steer * steerLock;
	float fSteerDelta = (fSteerPos - lastSteerPosition) * 333.33334f;
	float fMz = ((mzCurrent - fTorqFront) * controlsProvider->getFeedbackFilter()) + fTorqFront;
	mzCurrent = fMz;

	float fAv0 = fabsf(tyres[0]->status.angularVelocity);
	float fAv1 = fabsf(tyres[1]->status.angularVelocity);
	float fAvFront = fAv0 + fAv1;

	float fGyroFF = ((fSteerDelta / fabsf(steerRatio))
		* (fAvFront * tyres[0]->data.angularInertia))
		* sim->ffGyroWheelGain;

	lastSteerPosition = fSteerPos;
	lastPureMZFF = fMz * 1.4f;

	float fSignal = (-(fMz + fGyroFF)) * 1.4f;

	float fBlisterF = (float)tmax(tyres[0]->status.blister, tyres[1]->status.blister);
	float fBlisterR = (float)tmax(tyres[2]->status.blister, tyres[3]->status.blister);
	float fBlister = tmax(fBlisterF, fBlisterR) * 0.003f;

	float fFlatSpot = (float)tmax(tyres[0]->status.flatSpot, tyres[1]->status.flatSpot);
	if (fFlatSpot < fBlister)
		fFlatSpot = fBlister;

	flatSpotPhase += fAvFront * dt;

	if (fAvFront > 7.0f)
	{
		float fWave = sawToothWave(flatSpotPhase, 12.56636f) * fFlatSpot;
		float fLoad = tyres[0]->status.load + tyres[1]->status.load;
		fSignal += ((fWave * sim->ffFlatSpotGain * fLoad * 0.5f) + 1.0f);
	}

	float fSteerFF;
	if (steerAssist == 1.0f)
	{
		fSteerFF = fSignal * ffMult;
	}
	else
	{
		float fAssist = powf(fabsf(fSignal * ffMult), steerAssist);
		fSteerFF = fAssist * signf(fSignal);
	}

	float fResult = -fSteerFF;
	lastGyroFF = fGyroFF * 1.4f;

	#if 0
	if (controlsProvider->useFakeUndersteerFF)
	{
		// TODO
	}
	#endif

	return fResult;
}

//=============================================================================
// UTILS
//=============================================================================

void Car::teleport(const mat44f& m)
{
	vec3f pos(&m.M41);

	framesToSleep = 50;

	body->stop();
	fuelTankBody->stop();
	for (auto& susp : suspensions)
	{
		susp->stop();
	}

	auto hit = sim->physics->rayCast(pos, vec3f(0, -1, 0), 1000);
	if (hit.hasContact)
	{
		pos.y = hit.pos.y + 0.7f; // TODO: raceEngineer->getBaseCarHeight()
	}

	body->setPosition(pos);
	body->setRotation(m);

	fuelTankBody->setPosition(body->localToWorld(fuelTankPos));
	fuelTankBody->setRotation(m);

	for (auto& susp : suspensions)
	{
		susp->attach();
	}

	//body->stop();
	//fuelTankBody->stop();
}

vec3f Car::getGroundWindVector() const
{
	plane4f ground(
		tyres[0]->unmodifiedContactPoint,
		tyres[1]->unmodifiedContactPoint,
		tyres[2]->unmodifiedContactPoint);

	auto wind = sim->wind.vector;
	float dot = (wind * ground.normal);

	return (wind - (ground.normal * dot)) * 0.44f;
}

float Car::getPointGroundHeight(const vec3f& pt) const
{
	plane4f pl1(
		tyres[0]->contactPoint,
		tyres[1]->contactPoint,
		tyres[2]->contactPoint);

	plane4f pl2(
		tyres[0]->contactPoint,
		tyres[1]->contactPoint,
		tyres[3]->contactPoint);

	vec3f axis(0, -1, 0);
	float dot1 = pl1.normal * axis;
	float dot2 = pl2.normal * axis;

	float y1 = 0, y2 = 0;

	if (dot1 != 0.0f)
		y1 = pt.y + ((pt * pl1.normal + pl1.d) / dot1);

	if (dot2 != 0.0f)
		y2 = pt.y + ((pt * pl2.normal + pl2.d) / dot2);

	return ((pt.y - y2) + (pt.y - y1)) * 0.5f;
}

mat44f Car::getGraphicsOffsetMatrix() const
{
	auto m = body->getWorldMatrix(0);
	auto off = graphicsOffset * m;

	auto gm = m;
	gm.M41 = off.x;
	gm.M42 = off.y;
	gm.M43 = off.z;

	if (graphicsPitchRotation != 0.0f)
	{
		auto rotator = mat44f::createFromAxisAngle(vec3f(1, 0, 0), graphicsPitchRotation);
		gm = mat44f::mult(rotator, gm);
	}

	return gm;
}

bool Car::isSleeping() const
{
	return sleepingFrames > framesToSleep;
}

float Car::getEngineRpm() const
{
	return ((float)drivetrain->engine.velocity * 0.15915507f * 60.0f);
}

inline float getLoad(Car* car, int tyreId)
{
	auto* pTyre = car->tyres[tyreId].get();
	float fDX = pTyre->getDX(pTyre->status.load);
	float fD = pTyre->getCorrectedD(fDX, nullptr);
	return fD * pTyre->status.load;
}

float Car::getOptimalBrake()
{
	float fLoad0 = getLoad(this, 0);
	float fLoad1 = getLoad(this, 1);
	float fLoad2 = getLoad(this, 2);
	float fLoad3 = getLoad(this, 3);

	float fLoadFront = (fLoad0 + fLoad1) * 0.5f * tyres[0]->status.loadedRadius;
	float fLoadRear = (fLoad2 + fLoad3) * 0.5f * tyres[2]->status.loadedRadius;

	float fBrakeFront = fLoadFront / (brakeSystem->brakePower * brakeSystem->frontBias);
	float fBrakeRear = fLoadRear / (brakeSystem->brakePower * (1.0f - brakeSystem->frontBias));

	return tmin(fBrakeFront, fBrakeRear);
}

}
