#include "Car/Tyre.h"
#include "Car/Car.h"
#include "Car/ISuspension.h"
#include "Car/TyreModel.h"
#include "Car/TyreThermalModel.h"
#include "Car/BrushSlipProvider.h"
#include "Car/BrushTyreModel.h"
#include "Sim/Simulator.h"
#include "Sim/Track.h"

#include "TyreUtils.inl"

namespace D {

Tyre::Tyre()
{
	memzero(status);
	//status.inflation = 1.0f;
	//status.wearMult = 1.0f;
	status.pressureStatic = 26.0f;
	status.pressureDynamic = 26.0f;
	status.lastTempIMO[0] = -200.0f;
	status.lastTempIMO[1] = -200.0f;
	status.lastTempIMO[2] = -200.0f;
}

Tyre::~Tyre()
{
}

void Tyre::init(Car* _car, ISuspension* _hub, ITrackRayCastProvider* _rayCastProvider, int _index, const std::wstring& dataPath)
{
	car = _car;
	hub = _hub;
	rayCastProvider = _rayCastProvider;
	index = _index;

	tyreModel.reset(new SCTM());
	thermalModel.reset(new TyreThermalModel());
	thermalModel->init(car, 12, 3);
	rayCaster = rayCastProvider->createRayCaster(3.0f);

	initCompounds(dataPath);
	setCompound(0);
	shakeGenerator.step(0.003f);
}

void Tyre::initCompounds(const std::wstring& dataPath)
{
	auto ini(std::make_unique<INIReader>(dataPath + L"tyres.ini"));
	GUARD_FATAL(ini->ready);

	int iVer = ini->getInt(L"HEADER", L"VERSION");
	GUARD_FATAL(iVer >= 10);

	std::wstring arrSelector[4] = {L"FRONT", L"FRONT", L"REAR", L"REAR"};
	const auto& strId = arrSelector[index];

	if (ini->hasSection(L"EXPLOSION"))
		explosionTemperature = ini->getFloat(L"EXPLOSION", L"TEMPERATURE");

	if (ini->hasSection(L"VIRTUALKM"))
		useLoadForVKM = (ini->getInt(L"VIRTUALKM", L"USE_LOAD") != 0);

	if (ini->hasSection(L"ADDITIONAL1"))
	{
		blanketTemperature = ini->getFloat(L"ADDITIONAL1", L"BLANKETS_TEMP");
		pressureTemperatureGain = ini->getFloat(L"ADDITIONAL1", L"PRESSURE_TEMPERATURE_GAIN");

		float fSpread = ini->getFloat(L"ADDITIONAL1", L"CAMBER_TEMP_SPREAD_K");
		if (fSpread != 0.0f)
			thermalModel->camberSpreadK = fSpread;
	}

	for (int id = 0; ; id++)
	{
		auto strCompound(strId);
		if (id > 0)
			strCompound.append(strwf(L"_%d", id));

		if (!ini->hasSection(strCompound))
			break;

		auto compound(std::make_unique<TyreCompound>());

		compound->index = id;
		compound->modelData.version = iVer;

		compound->name = ini->getString(strCompound, L"NAME");
		compound->shortName = ini->getString(strCompound, L"SHORT_NAME");
		compound->name.append(L" (");
		compound->name.append(compound->shortName);
		compound->name.append(L")");

		compound->data.width = ini->getFloat(strCompound, L"WIDTH");
		if (compound->data.width <= 0)
			compound->data.width = 0.15f;

		compound->data.radius = ini->getFloat(strCompound, L"RADIUS");
		compound->data.rimRadius = ini->getFloat(strCompound, L"RIM_RADIUS");
		compound->modelData.flexK = ini->getFloat(strCompound, L"FLEX");

		float fFLA = ini->getFloat(strCompound, L"FRICTION_LIMIT_ANGLE");
		if (fFLA == 0.0f)
			fFLA = 7.5f;

		auto bsp(std::make_unique<BrushSlipProvider>(fFLA, compound->modelData.flexK));

		compound->modelData.cfXmult = ini->getFloat(strCompound, L"CX_MULT");
		compound->data.radiusRaiseK = ini->getFloat(strCompound, L"RADIUS_ANGULAR_K") * 0.001f;
			
		if (ini->hasKey(strCompound, L"BRAKE_DX_MOD"))
		{
			compound->modelData.brakeDXMod = ini->getFloat(strCompound, L"BRAKE_DX_MOD");
			if (compound->modelData.brakeDXMod == 0.0f)
				compound->modelData.brakeDXMod = 1.0f;
			else
				compound->modelData.brakeDXMod += 1.0f;
		}

		if (ini->hasKey(strCompound, L"COMBINED_FACTOR"))
			compound->modelData.combinedFactor = ini->getFloat(strCompound, L"COMBINED_FACTOR");

		const float fFZ0 = ini->getFloat(strCompound, L"FZ0");
		const float fFlexGain = ini->getFloat(strCompound, L"FLEX_GAIN");

		compound->modelData.lsExpX = ini->getFloat(strCompound, L"LS_EXPX");
		compound->modelData.lsExpY = ini->getFloat(strCompound, L"LS_EXPY");
		compound->modelData.Dx0 = ini->getFloat(strCompound, L"DX_REF");
		compound->modelData.Dy0 = ini->getFloat(strCompound, L"DY_REF");

		compound->modelData.lsMultX = calcLoadSensMult(compound->modelData.Dx0, fFZ0, compound->modelData.lsExpX);
		compound->modelData.lsMultY = calcLoadSensMult(compound->modelData.Dy0, fFZ0, compound->modelData.lsExpY);

		if (ini->hasKey(strCompound, L"DY_CURVE"))
			compound->modelData.dyLoadCurve = ini->getCurve(strCompound, L"DY_CURVE");

		if (ini->hasKey(strCompound, L"DX_CURVE"))
			compound->modelData.dxLoadCurve = ini->getCurve(strCompound, L"DX_CURVE");

		bsp->version = 5;
		bsp->asy = 0.92f;
		bsp->brushModel->data.Fz0 = fFZ0;
		bsp->brushModel->data.maxSlip0 = tanf(fFLA * 0.017453f);
		bsp->brushModel->data.maxSlip1 = tanf(((fFlexGain + 1.0f) * fFLA) * 0.017453f);

		bsp->recomputeMaximum();

		bsp->asy = ini->getFloat(strCompound, L"FALLOFF_LEVEL");
		bsp->brushModel->data.falloffSpeed = ini->getFloat(strCompound, L"FALLOFF_SPEED");

		compound->modelData.speedSensitivity = ini->getFloat(strCompound, L"SPEED_SENSITIVITY");
		compound->modelData.relaxationLength = ini->getFloat(strCompound, L"RELAXATION_LENGTH");
		compound->modelData.rr0 = ini->getFloat(strCompound, L"ROLLING_RESISTANCE_0");
		compound->modelData.rr1 = ini->getFloat(strCompound, L"ROLLING_RESISTANCE_1");
		compound->modelData.rr_slip = ini->getFloat(strCompound, L"ROLLING_RESISTANCE_SLIP");

		compound->modelData.camberGain = ini->getFloat(strCompound, L"CAMBER_GAIN");
		compound->modelData.dcamber0 = ini->getFloat(strCompound, L"DCAMBER_0");
		compound->modelData.dcamber1 = ini->getFloat(strCompound, L"DCAMBER_1");

		if (compound->modelData.dcamber0 == 0.0f || compound->modelData.dcamber1 == 0.0f)
		{
			compound->modelData.dcamber0 = 0.1f;
			compound->modelData.dcamber1 = -0.8f;
		}

		if (ini->hasKey(strCompound, L"DCAMBER_LUT"))
		{
			compound->modelData.dCamberCurve = ini->getCurve(strCompound, L"DCAMBER_LUT");
			compound->modelData.useSmoothDCamberCurve = ini->getInt(strCompound, L"DCAMBER_LUT_SMOOTH") != 0;
		}

		compound->data.angularInertia = ini->getFloat(strCompound, L"ANGULAR_INERTIA");
		compound->data.d = ini->getFloat(strCompound, L"DAMP");
		compound->data.k = ini->getFloat(strCompound, L"RATE");

		if (compound->data.angularInertia == 0.0f)
			compound->data.angularInertia = 1.2f;
		if (compound->data.d == 0.0f)
			compound->data.d = 400.0f;
		if (compound->data.k == 0.0f)
			compound->data.k = 220000.0f;
		if (compound->modelData.Dx0 == 0.0f)
			compound->modelData.Dx0 = compound->modelData.Dy0 * 1.2f;
		if (compound->modelData.Dx1 == 0.0f)
			compound->modelData.Dx1 = compound->modelData.Dy1 * 0.1f;

		compound->pressureStatic = ini->getFloat(strCompound, L"PRESSURE_STATIC");
		if (compound->pressureStatic == 0.0f)
			compound->pressureStatic = 26.0f;

		compound->modelData.pressureRef = compound->pressureStatic;
		status.pressureDynamic = compound->pressureStatic;

		compound->modelData.pressureSpringGain = ini->getFloat(strCompound, L"PRESSURE_SPRING_GAIN");
		if (compound->modelData.pressureSpringGain == 0.0f)
			compound->modelData.pressureSpringGain = 1000.0f;

		compound->modelData.pressureFlexGain = ini->getFloat(strCompound, L"PRESSURE_FLEX_GAIN");
		compound->modelData.pressureRRGain = ini->getFloat(strCompound, L"PRESSURE_RR_GAIN");
		compound->modelData.pressureGainD = ini->getFloat(strCompound, L"PRESSURE_D_GAIN");

		compound->modelData.idealPressure = ini->getFloat(strCompound, L"PRESSURE_IDEAL");
		if (compound->modelData.idealPressure == 0.0f)
			compound->modelData.idealPressure = 26.0f;

		auto strThermal(L"THERMAL_" + strCompound);
		if (ini->hasSection(strThermal))
		{
			compound->thermalPatchData.surfaceTransfer = ini->getFloat(strThermal, L"SURFACE_TRANSFER");
			compound->thermalPatchData.patchTransfer = ini->getFloat(strThermal, L"PATCH_TRANSFER");
			compound->thermalPatchData.patchCoreTransfer = ini->getFloat(strThermal, L"CORE_TRANSFER");
			compound->thermalPatchData.internalCoreTransfer = ini->getFloat(strThermal, L"INTERNAL_CORE_TRANSFER");

			compound->data.thermalFrictionK = ini->getFloat(strThermal, L"FRICTION_K");
			compound->data.thermalRollingK = ini->getFloat(strThermal, L"ROLLING_K");
			compound->data.thermalRollingSurfaceK = ini->getFloat(strThermal, L"SURFACE_ROLLING_K");

			if (ini->hasKey(strThermal, L"COOL_FACTOR"))
				compound->thermalPatchData.coolFactorGain = (ini->getFloat(strThermal, L"COOL_FACTOR") - 1.0f) * 0.000324f;

			auto strFile = ini->getString(strThermal, L"PERFORMANCE_CURVE");
			compound->thermalPerformanceCurve.load(dataPath + strFile);
		}

		auto strFile = ini->getString(strCompound, L"WEAR_CURVE");
		compound->modelData.wearCurve.load(dataPath + strFile);
		compound->modelData.wearCurve.scale(0.01f);

		int iTpcCount = compound->thermalPerformanceCurve.getCount();
		if (iTpcCount > 0)
		{
			for (int n = 0; n < iTpcCount; ++n)
			{
				auto pair = compound->thermalPerformanceCurve.getPairAtIndex(n);
				if (pair.second >= 1.0f)
				{
					compound->data.grainThreshold = pair.first;
					break;
				}
			}

			for (int n = iTpcCount - 1; n > 0; --n)
			{
				auto pair = compound->thermalPerformanceCurve.getPairAtIndex(n);
				if (pair.second >= 1.0f)
				{
					compound->data.blisterThreshold = pair.first;
					compound->data.optimumTemp = pair.first;
					break;
				}
			}
		}

		compound->modelData.maxWearMult = 100.0f;
		int iWcCount = compound->modelData.wearCurve.getCount();
		for (int n = 0; n < iWcCount; ++n)
		{
			auto pair = compound->modelData.wearCurve.getPairAtIndex(n);
			if (pair.second < compound->modelData.maxWearMult)
			{
				compound->modelData.maxWearKM = pair.first;
				compound->modelData.maxWearMult = pair.second;
			}
		}

		compound->data.blisterGamma = ini->getFloat(strThermal, L"BLISTER_GAMMA");
		compound->data.blisterGain = ini->getFloat(strThermal, L"BLISTER_GAIN");
		compound->data.grainGamma = ini->getFloat(strThermal, L"GRAIN_GAMMA");
		compound->data.grainGain = ini->getFloat(strThermal, L"GRAIN_GAIN");

		float fSens = loadSensExpD(compound->modelData.lsExpY, compound->modelData.lsMultY, 3000.0f);
		compound->data.softnessIndex = tmax(0.0f, fSens - 1.0f);

		compound->slipProvider = std::move(bsp);
		compoundDefs.emplace_back(std::move(compound));
	}

	GUARD_FATAL(!compoundDefs.empty());
}

void Tyre::setCompound(int cindex) // TODO: THIS IS FKN MADNESS
{
	GUARD_FATAL(cindex >= 0 && cindex < compoundDefs.size());

	auto* compound = compoundDefs[cindex].get();

	data = compound->data;
	modelData = compound->modelData;
	thermalModel->patchData = compound->thermalPatchData;
	thermalModel->performanceCurve = compound->thermalPerformanceCurve;

	currentCompoundIndex = cindex;
	status.pressureStatic = compound->pressureStatic;
	status.pressureDynamic = compound->pressureStatic;

	thermalModel->reset();
	reset();

	// TODO: evOnTyreCompoundChanged

	auto* model = tyreModel.get();
	auto* brushModel = compound->slipProvider->brushModel.get();

	modelData.asy = compound->slipProvider->asy;
	model->asy = compound->slipProvider->asy;

	model->Fz0 = brushModel->data.Fz0;
	model->maxSlip0 = brushModel->data.maxSlip0;
	model->maxSlip1 = brushModel->data.maxSlip1;
	model->falloffSpeed = brushModel->data.falloffSpeed;

	model->lsExpX = modelData.lsExpX;
	model->lsExpY = modelData.lsExpY;
	model->lsMultX = modelData.lsMultX;
	model->lsMultY = modelData.lsMultY;
	model->speedSensitivity = modelData.speedSensitivity;
	model->camberGain = modelData.camberGain;
	model->dcamber0 = modelData.dcamber0;
	model->dcamber1 = modelData.dcamber1;
	model->cfXmult = modelData.cfXmult;
	model->pressureCfGain = modelData.pressureFlexGain;
	model->brakeDXMod = modelData.brakeDXMod;
	model->useSmoothDCamberCurve = modelData.useSmoothDCamberCurve;
	model->combinedFactor = modelData.combinedFactor;

	model->dyLoadCurve = modelData.dyLoadCurve;
	model->dxLoadCurve = modelData.dxLoadCurve;
	model->dCamberCurve = modelData.dCamberCurve;
}

void Tyre::reset()
{
	worldRotation = hub->getHubWorldMatrix();
	worldPosition = vec3f(&worldRotation.M41);

	status.slipAngleRAD = 0;
	status.slipRatio = 0;
	status.angularVelocity = 0;
	status.Fy = 0;
	status.Fx = 0;
	status.Mz = 0;
	status.isLocked = 1;
	status.inflation = 1;
	status.flatSpot = 0;
	status.lastGrain = (float)status.grain;
	status.lastBlister = (float)status.blister;
	status.grain = 0;
	status.blister = 0;

	rSlidingVelocityX = 0;
	rSlidingVelocityY = 0;

	if (status.virtualKM > 0.001f)
		thermalModel->getIMO(status.lastTempIMO);
	
	status.dirtyLevel = 0;
	status.feedbackTorque = 0;
	status.virtualKM = 0;
	absOverride = 1;
	tyreBlanketsOn = true;

	thermalModel->reset();
}

void Tyre::step(float dt)
{
	status.feedbackTorque = 0;
	status.Fx = 0;
	status.Mz = 0;
	status.slipFactor = 0;
	status.rollingResistence = 0;
	slidingVelocityY = 0;
	slidingVelocityX = 0;
	totalSlideVelocity = 0;
	totalHubVelocity = 0;
	surfaceDef = nullptr;

	mat44f mxWorld = hub->getHubWorldMatrix();
	vec3f vWorldM2(&mxWorld.M21);

	worldPosition.x = mxWorld.M41;
	worldPosition.y = mxWorld.M42;
	worldPosition.z = mxWorld.M43;

	worldRotation = mxWorld;
	worldRotation.M41 = 0;
	worldRotation.M42 = 0;
	worldRotation.M43 = 0;

	if (!isfinite(status.angularVelocity))
	{
		SHOULD_NOT_REACH_WARN;
		status.angularVelocity = 0;
	}

	if (!status.isLocked)
	{
		if (car->torqueModeEx == TorqueModeEX::reactionTorques)
		{
			float fTorq = inputs.electricTorque + inputs.brakeTorque + inputs.handBrakeTorque;
			hub->addTorque(vec3f(&mxWorld.M11) * fTorq);
		}
	}

	vec3f vWorldPos = worldPosition;
	vec3f vHitPos(0, 0, 0);
	vec3f vHitNorm(0, 0, 0);

	ICollisionObject* pCollisionObject = nullptr;
	Surface* pSurface = nullptr;
	bool bHasContact = false;

	auto pCollisionProvider = rayCastProvider;
	if (pCollisionProvider)
	{
		vec3f vRayPos(vWorldPos.x, vWorldPos.y + 2.0f, vWorldPos.z);
		vec3f vRayDir(0.0f, -1.0f, 0.0f);

		if (rayCaster)
		{
			RayCastHit hit = rayCaster->rayCast(vRayPos, vRayDir);
			bHasContact = hit.hasContact;

			if (bHasContact)
			{
				vHitPos = hit.pos;
				vHitNorm = hit.normal;
				pCollisionObject = (ICollisionObject*)hit.collisionObject;
				pSurface = (Surface*)pCollisionObject->getUserPointer();
			}
		}
		else
		{
			TrackRayCastHit hit;
			pCollisionProvider->rayCast(vRayPos, vRayDir, 2.0f, hit);
			bHasContact = hit.hasContact;

			if (bHasContact)
			{
				vHitPos = hit.pos;
				vHitNorm = hit.normal;
				pCollisionObject = (ICollisionObject*)hit.collisionObject;
				pSurface = (Surface*)hit.surface;
			}
		}
	}

	float fTest = 0;

	if (!bHasContact || mxWorld.M22 <= 0.35f)
	{
		status.ndSlip = 0;
		status.Fy = 0;
		goto LB_COMPUTE_TORQ;
	}

	surfaceDef = pSurface;
	unmodifiedContactPoint = vHitPos;

	fTest = vHitNorm * vWorldM2;
	if (fTest <= 0.96f)
	{
		float fTestAcos;
		if (fTest <= -1.0f || fTest >= 1.0f)
			fTestAcos = 0;
		else
			fTestAcos = acosf(fTest);

		float fAngle = fTestAcos - acosf(0.96f);

		// TODO: cross product?
		vec3f vAxis(
			(vWorldM2.z * vHitNorm.y) - (vWorldM2.y * vHitNorm.z),
			(vWorldM2.x * vHitNorm.z) - (vWorldM2.z * vHitNorm.x),
			(vWorldM2.y * vHitNorm.x) - (vWorldM2.x * vHitNorm.y)
		);

		mat44f mxHit = mat44f::createFromAxisAngle(vAxis.get_norm(), fAngle);

		vHitNorm = vec3f(
			(((mxHit.M11 * vHitNorm.x) + (mxHit.M21 * vHitNorm.y)) + (mxHit.M31 * vHitNorm.z)) + mxHit.M41,
			(((mxHit.M12 * vHitNorm.x) + (mxHit.M22 * vHitNorm.y)) + (mxHit.M32 * vHitNorm.z)) + mxHit.M42,
			(((mxHit.M13 * vHitNorm.x) + (mxHit.M23 * vHitNorm.y)) + (mxHit.M33 * vHitNorm.z)) + mxHit.M43
		);
	}
	else
	{
		vec3f vHitOff = vHitPos - worldPosition;
		float fDot = vHitNorm * vHitOff;
		vHitPos = (vHitNorm * fDot) + worldPosition;
	}

	contactPoint = vHitPos;
	contactNormal = vHitNorm;

	if (pSurface)
	{
		float fSinHeight = pSurface->sinHeight;
		if (fSinHeight != 0.0f)
		{
			float fSinLength = pSurface->sinLength;
			contactPoint.y -= (((sinf(fSinLength * contactPoint.x) * cosf(fSinLength * contactPoint.z)) + 1.0f) * fSinHeight);
		}

		if (pSurface->granularity != 0.0f)
		{
			float v1[3] = { 1.0f, 5.8f, 11.4f };
			float v2[3] = { 0.005f, 0.005f, 0.01f };

			float cx = contactPoint.x;
			float cy = contactPoint.y;
			float cz = contactPoint.z;

			for (int id = 0; id < 3; ++id)
			{
				float v = v1[id];
				cy = cy + ((((sinf(v * cx) * cosf(v * cz)) + 1.0f) * v2[id]) * -0.6f);
			}

			contactPoint.y = cy;
		}
	}

	addGroundContact(contactPoint, contactNormal);
	addTyreForcesV10(contactPoint, contactNormal, pSurface, dt);

	if (pSurface)
	{
		if (pSurface->damping > 0.0f)
		{
			auto vBodyVel = car->body->getVelocity();
			float fMass = car->body->getMass();

			vec3f vForce = vBodyVel * -(fMass * pSurface->damping);
			vec3f vPos(0, 0, 0);

			car->body->addForceAtLocalPos(vForce, vPos);
		}
	}

LB_COMPUTE_TORQ:

	float fHandBrakeTorque = inputs.handBrakeTorque;
	float fBrakeTorque = inputs.brakeTorque * absOverride;

	if (fBrakeTorque <= fHandBrakeTorque)
		fBrakeTorque = fHandBrakeTorque;

	float fAngularVelocitySign = signf(status.angularVelocity);
	float fTorq = status.rollingResistence - ((fAngularVelocitySign * fBrakeTorque) + localMX);

	if (!isfinite(fTorq))
	{
		SHOULD_NOT_REACH_WARN;
		fTorq = 0;
	}

	float fFeedbackTorque = fTorq + inputs.electricTorque;

	if (!isfinite(fFeedbackTorque))
	{
		SHOULD_NOT_REACH_WARN;
		fFeedbackTorque = 0;
	}

	status.feedbackTorque = fFeedbackTorque;

	if (driven)
	{
		updateLockedState(dt);

		float fS0 = signf(oldAngularVelocity);
		float fS1 = signf(status.angularVelocity);

		if (fS0 != fS1 && totalHubVelocity < 1.0f)
			status.isLocked = true;

		oldAngularVelocity = status.angularVelocity;
	}
	else
	{
		updateAngularSpeed(dt);
		stepRotationMatrix(dt);
	}

	if (totalHubVelocity < 10.0f)
		status.slipFactor = fabsf(totalHubVelocity * 0.1f) * status.slipFactor;

	stepThermalModel(dt);
	status.pressureDynamic = ((thermalModel->coreTemp - 26.0f) * pressureTemperatureGain) + status.pressureStatic;

	stepGrainBlister(dt, totalHubVelocity);
	stepFlatSpot(dt, totalHubVelocity);

	if (onStepCompleted)
		onStepCompleted();
}

void Tyre::addGroundContact(const vec3f& pos, const vec3f& normal)
{
	vec3f vOffset = worldPosition - pos;
	float fDistToGround = vOffset.len();
	status.distToGround = fDistToGround;

	float fRadius;
	if (data.radiusRaiseK == 0.0f)
		fRadius = data.radius;
	else
		fRadius = (fabsf(status.angularVelocity) * data.radiusRaiseK) + data.radius;

	if (status.inflation < 1.0f)
		fRadius = ((fRadius - data.rimRadius) * status.inflation) + data.rimRadius;

	status.liveRadius = fRadius;
	status.effectiveRadius = fRadius;

	if (fDistToGround > fRadius)
	{
		status.loadedRadius = fRadius;
		status.depth = 0;
		status.load = 0;
		status.Fy = 0;
		status.Fx = 0;
		status.Mz = 0;
		rSlidingVelocityX = 0;
		rSlidingVelocityY = 0;
		status.ndSlip = 0;
	}
	else
	{
		float fDepth = fRadius - fDistToGround;
		float fLoadedRadius = fRadius - fDepth;

		status.depth = fDepth;
		status.loadedRadius = fLoadedRadius;

		float fMaybePressure;
		if (fLoadedRadius <= data.rimRadius)
		{
			fMaybePressure = 200000.0f;
		}
		else
		{
			fMaybePressure = ((status.pressureDynamic - modelData.pressureRef) * modelData.pressureSpringGain) + data.k;
			if (fMaybePressure < 0.0f)
				fMaybePressure = 0;
		}

		vec3f vHubVel = hub->getPointVelocity(pos);
		float fLoad = -((vHubVel * normal) * data.d) + (fDepth * fMaybePressure);
		status.load = fLoad;

		hub->addForceAtPos(normal * fLoad, pos, driven, false);

		if (status.load < 0.0f)
			status.load = 0;
	}

	if (externalInputs.isActive)
		status.load = externalInputs.load;
}

void Tyre::updateLockedState(float dt)
{
	if (status.isLocked)
	{
		float fBrake = tmax(absOverride * inputs.brakeTorque, inputs.handBrakeTorque);

		status.isLocked =
			(fabsf(fBrake) >= fabsf(status.loadedRadius * status.Fx))
			&& (fabsf(status.angularVelocity) < 1.0f)
			&& (!driven);
	}
}

void Tyre::updateAngularSpeed(float dt)
{
	updateLockedState(dt);

	float fAngVel = status.angularVelocity + ((status.feedbackTorque / data.angularInertia) * dt);

	if (signf(fAngVel) != signf(oldAngularVelocity))
		status.isLocked = true;

	oldAngularVelocity = fAngVel;
	status.angularVelocity = status.isLocked ? 0.0f : fAngVel;

	if (fabsf(status.angularVelocity) < 1.0f)
		status.angularVelocity *= 0.9f;
}

void Tyre::stepRotationMatrix(float dt)
{
	if (!car->isSleeping() && fabsf(status.angularVelocity) > 0.1f)
	{
		mat44f m = mat44f::createFromAxisAngle(vec3f(1, 0, 0), status.angularVelocity * dt);
		localWheelRotation = mat44f::mult(m, localWheelRotation);
		localWheelRotation.M41 = 0;
		localWheelRotation.M42 = 0;
		localWheelRotation.M43 = 0;
	}
}

void Tyre::stepThermalModel(float dt)
{
	float fGripLevel = car->track->dynamicGripLevel;

	float fThermalInput = (
		sqrtf((slidingVelocityX * slidingVelocityX) + (slidingVelocityY * slidingVelocityY))
		* ((status.D * status.load) * data.thermalFrictionK))
		* fGripLevel;

	Surface* pSurface = surfaceDef;
	if (pSurface)
		fThermalInput *= pSurface->gripMod;

	status.thermalInput = fThermalInput;

	if (isfinite(fThermalInput))
	{
		float fPressureDynamic = status.pressureDynamic;
		float fIdealPressure = modelData.idealPressure;
		float fThermalRollingK = data.thermalRollingK;

		float fScale = (((fIdealPressure / fPressureDynamic) - 1.0f) * modelData.pressureRRGain) + 1.0f;
		if (fPressureDynamic >= 0.0)
			fThermalRollingK *= fScale;

		int iVer = modelData.version;
		if (iVer < 5)
			status.thermalInput += (((fThermalRollingK * status.angularVelocity) * status.load) * 0.001f);

		if (iVer >= 6)
			status.thermalInput += ((((fScale * data.thermalRollingSurfaceK) * status.angularVelocity) * status.load) * 0.001f);

		thermalModel->addThermalInput(status.camberRAD, (fPressureDynamic / fIdealPressure) - 1.0f, status.thermalInput);

		if (modelData.version >= 5)
			thermalModel->addThermalCoreInput(((fThermalRollingK * status.angularVelocity) * status.load) * 0.001f);

		thermalModel->step(dt, status.angularVelocity, status.camberRAD);
	}
	else
	{
		SHOULD_NOT_REACH_WARN;
	}

	if (car)
	{
		if (car->sim->allowTyreBlankets)
			stepTyreBlankets(dt);
	}
}

void Tyre::stepTyreBlankets(float dt)
{
	if (tyreBlanketsOn)
	{
		if (car->speed.kmh() > 10.0f)
			tyreBlanketsOn = false;

		float fTemp = tmin(blanketTemperature, data.optimumTemp);
		thermalModel->setTemperature(fTemp);
	}
}

void Tyre::stepGrainBlister(float dt, float hubVelocity)
{
	Car* pCar = car;

	if (pCar && car->sim->tyreConsumptionRate > 0.0f)
	{
		if (status.load > 0.0f)
		{
			float fGrainGain = data.grainGain;
			float fCoreTemp = thermalModel->coreTemp;
			float fNdSlip = tmin(status.ndSlip, 2.5f);

			if (fGrainGain > 0.0f)
			{
				float fGrainThreshold = data.grainThreshold;
				if (fCoreTemp < fGrainThreshold)
				{
					auto pSurface = surfaceDef;
					if (pSurface)
					{
						if (hubVelocity > 2.0f)
						{
							float fGripMod = pSurface->gripMod;
							if (fGripMod >= 0.95f)
							{
								float fTest = ((fGripMod * hubVelocity) * fGrainGain) * ((fGrainThreshold - fCoreTemp) * 0.0001f);
								if (isfinite(fTest))
								{
									status.grain += fTest * powf(fNdSlip, data.grainGamma) * car->sim->tyreConsumptionRate * dt;
								}
								else
								{
									SHOULD_NOT_REACH_WARN;
								}
							}
						}
					}
				}
			}

			auto pSurface = surfaceDef;
			if (pSurface)
			{
				float fTest = (pSurface->gripMod * hubVelocity) * fGrainGain * 0.00005f;
				if (isfinite(fTest))
				{
					if (fTest > 0.0f)
						status.grain -= fTest * powf(fNdSlip, data.grainGamma) * car->sim->tyreConsumptionRate * dt;
				}
				else
				{
					SHOULD_NOT_REACH_WARN;
				}
			}

			float fBlisterGain = data.blisterGain;
			if (fBlisterGain > 0.0f)
			{
				float fBlisterThreshold = data.blisterThreshold;
				if (fCoreTemp > fBlisterThreshold)
				{
					pSurface = surfaceDef;
					if (pSurface)
					{
						if (hubVelocity > 2.0f)
						{
							float fGripMod = pSurface->gripMod;
							if (fGripMod >= 0.95f)
							{
								float fTest = ((fGripMod * totalHubVelocity) * fBlisterGain) * ((fCoreTemp - fBlisterThreshold) * 0.0001f);
								if (isfinite(fTest))
								{
									status.blister += fTest * powf(fNdSlip, data.blisterGamma) * car->sim->tyreConsumptionRate * dt;
								}
								else
								{
									SHOULD_NOT_REACH_WARN;
								}
							}
						}
					}
				}
			}

			status.grain = tclamp<double>(status.grain, 0.0, 100.0);
			status.blister = tclamp<double>(status.blister, 0.0, 100.0);
		}
	}
	else
	{
		status.grain = 0;
		status.blister = 0;
	}
}

void Tyre::stepFlatSpot(float dt, float hubVelocity)
{
	if (fabsf(status.angularVelocity) <= 0.3f || status.slipRatio < -0.98f)
	{
		if (surfaceDef)
		{
			if (hubVelocity > 3.0f)
			{
				if (car)
				{
					float fDamage = car->sim->mechanicalDamageRate;
					if (fDamage != 0.0f)
					{
						float fGrip = surfaceDef->gripMod;
						if (fGrip >= 0.95f)
						{
							status.flatSpot += (((hubVelocity * flatSpotK) * status.load) * fGrip) * 0.00001f * dt * fDamage * data.softnessIndex;

							if (status.flatSpot > 1.0f)
								status.flatSpot = 1.0f;
						}
					}
				}
			}
		}
	}
}

float Tyre::getDX(float load)
{
	float fResult = 0;
	float fBlisterN = tclamp((float)status.blister * 0.01f, 0.0f, 1.0f);

	if (modelData.dxLoadCurve.getCount() <= 0)
	{
		float fExpX = modelData.lsExpX;
		if (fExpX == 0.0f)
		{
			fResult = ((load * 0.0005f * modelData.Dx1) + modelData.Dx0) / (fBlisterN * 0.2f + 1.0f);
		}
		else
		{
			float v8 = 0.0f;
			if (load != 0.0f)
				v8 = (powf(load, fExpX) * modelData.lsMultX) / load;
			fResult = v8 / (fBlisterN * 0.2f + 1.0f);
		}
	}
	else
	{
		fResult = modelData.dxLoadCurve.getCubicSplineValue(load) / (fBlisterN * 0.2f + 1.0f);
	}

	return fResult;
}

}
