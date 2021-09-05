#include "Car/SuspensionStrut.h"

namespace D {

SuspensionStrut::SuspensionStrut()
{
	k = 90000.0f;
	baseCFM = 0.0000001f;
	damageData.minVelocity = 15.0f;
	damageData.damageDirection = randDamageDirection();
}

SuspensionStrut::~SuspensionStrut()
{
}

void SuspensionStrut::init(IPhysicsEnginePtr _core, IRigidBodyPtr _carBody, int _index, const std::wstring& dataPath)
{
	type = SuspensionType::Strut;
	index = _index;

	core = _core;
	carBody = _carBody;

	auto ini(std::make_unique<INIReader>(dataPath + L"suspensions.ini"));
	GUARD_FATAL(ini->ready);

	int iVer = ini->getInt(L"HEADER", L"VERSION");

	std::wstring arrSelector[4] = {L"FRONT", L"FRONT", L"REAR", L"REAR"};
	auto strId = arrSelector[index];

	float fWheelBase = ini->getFloat(L"BASIC", L"WHEELBASE");
	float fCgLocation = ini->getFloat(L"BASIC", L"CG_LOCATION");
	float fFrontBaseY = ini->getFloat(L"FRONT", L"BASEY");
	float fFrontTrack = ini->getFloat(L"FRONT", L"TRACK") * 0.5f;
	float fRearBaseY = ini->getFloat(L"REAR", L"BASEY");
	float fRearTrack = ini->getFloat(L"REAR", L"TRACK") * 0.5f;

	vec3f vRefPoints[4];
	vRefPoints[0] = vec3f(fFrontTrack, fFrontBaseY, (1.0f - fCgLocation) * fWheelBase);
	vRefPoints[1] = vec3f(-fFrontTrack, fFrontBaseY, (1.0f - fCgLocation) * fWheelBase);
	vRefPoints[2] = vec3f(fRearTrack, fRearBaseY, -(fCgLocation * fWheelBase));
	vRefPoints[3] = vec3f(-fRearTrack, fRearBaseY, -(fCgLocation * fWheelBase));

	dataRelToWheel.refPoint = vRefPoints[index];

	dataRelToWheel.carStrut = ini->getFloat3(strId, L"STRUT_CAR");
	dataRelToWheel.tyreStrut = ini->getFloat3(strId, L"STRUT_TYRE");
	dataRelToWheel.carBottomWB_F = ini->getFloat3(strId, L"WBCAR_BOTTOM_FRONT");
	dataRelToWheel.carBottomWB_R = ini->getFloat3(strId, L"WBCAR_BOTTOM_REAR");
	dataRelToWheel.tyreBottomWB = ini->getFloat3(strId, L"WBTYRE_BOTTOM");
	dataRelToWheel.tyreSteer = ini->getFloat3(strId, L"WBTYRE_STEER");
	dataRelToWheel.carSteer = ini->getFloat3(strId, L"WBCAR_STEER");

	if (iVer >= 2)
	{
		float fRimOffset = -ini->getFloat(strId, L"RIM_OFFSET");
		if (fRimOffset != 0.0f)
		{
			dataRelToWheel.carStrut.x  += fRimOffset;
			dataRelToWheel.tyreStrut.x  += fRimOffset;
			dataRelToWheel.carBottomWB_F.x  += fRimOffset;
			dataRelToWheel.carBottomWB_R.x  += fRimOffset;
			dataRelToWheel.tyreBottomWB.x  += fRimOffset;
			dataRelToWheel.tyreSteer.x  += fRimOffset;
			dataRelToWheel.carSteer.x  += fRimOffset;
	  	}
	}

	dataRelToWheel.hubMass = ini->getFloat(strId, L"HUB_MASS");

	bumpStopUp = ini->getFloat(strId, L"BUMPSTOP_UP");
	bumpStopDn = -ini->getFloat(strId, L"BUMPSTOP_DN");
	rodLength = ini->getFloat(strId, L"ROD_LENGTH");
	toeOUT_Linear = ini->getFloat(strId, L"TOE_OUT");
	k = ini->getFloat(strId, L"SPRING_RATE");
	progressiveK = ini->getFloat(strId, L"PROGRESSIVE_SPRING_RATE");

	damper.bumpSlow = ini->getFloat(strId, L"DAMP_BUMP");
	damper.reboundSlow = ini->getFloat(strId, L"DAMP_REBOUND");
	damper.bumpFast = ini->getFloat(strId, L"DAMP_FAST_BUMP");
	damper.reboundFast = ini->getFloat(strId, L"DAMP_FAST_REBOUND");
	damper.fastThresholdBump = ini->getFloat(strId, L"DAMP_FAST_BUMPTHRESHOLD");
	damper.fastThresholdRebound = ini->getFloat(strId, L"DAMP_FAST_REBOUNDTHRESHOLD");

	if (damper.fastThresholdBump == 0.0f)
		damper.fastThresholdBump = 0.2f;
	if (damper.fastThresholdRebound == 0.0f)
		damper.fastThresholdRebound = 0.2f;
	if (damper.bumpFast == 0.0f)
		damper.bumpFast = damper.bumpSlow;
	if (damper.reboundFast == 0.0f)
		damper.reboundFast = damper.reboundSlow;

	bumpStopRate = ini->getFloat(strId, L"BUMP_STOP_RATE");
	if (bumpStopRate == 0.0f)
		bumpStopRate = 500000.0f;

	staticCamber = -ini->getFloat(strId, L"STATIC_CAMBER") * 0.017453f;
	if (index % 2)
		staticCamber *= -1.0f;

	packerRange = ini->getFloat(strId, L"PACKER_RANGE");

	if (ini->hasSection(L"DAMAGE"))
	{
		damageData.minVelocity = ini->getFloat(L"DAMAGE", L"MIN_VELOCITY");
		damageData.damageGain = ini->getFloat(L"DAMAGE", L"GAIN");
		damageData.maxDamage = ini->getFloat(L"DAMAGE", L"MAX_DAMAGE");
	}

	if (dataRelToWheel.refPoint.x > 0.0f)
	{
		dataRelToWheel.carBottomWB_F.x *= -1.0f;
		dataRelToWheel.carBottomWB_R.x *= -1.0f;
		dataRelToWheel.carSteer.x *= -1.0f;
		dataRelToWheel.carStrut.x *= -1.0f;
		dataRelToWheel.tyreBottomWB.x *= -1.0f;
		dataRelToWheel.tyreSteer.x *= -1.0f;
		dataRelToWheel.tyreStrut.x *= -1.0f;
	}

	float fMass = dataRelToWheel.hubMass;
	if (fMass <= 0.0f)
		fMass = 20.0f;

	vec3f vInertia = dataRelToWheel.hubInertiaBox;
	if (vInertia.len() == 0.0f)
		vInertia = vec3f(0.2f, 0.6f, 0.6f); // TODO: check

	hub = core->createRigidBody();
	hub->setMassBox(fMass * 0.8f, vInertia.x, vInertia.y, vInertia.z);

	strutBody = core->createRigidBody();
	strutBody->setMassBox(fMass * 0.2f, 0.05f, 0.5f, 0.2f); // TODO: check
	strutBodyLength = 0.2f;

	basePosition = dataRelToWheel.refPoint;
	attach();

	baseCarSteerPosition = dataRelToBody.carSteer;
	setSteerLengthOffset(0);
}

void SuspensionStrut::attach()
{
	setPositions();

	if (joints[0])
		return;

	#define CALC_REL_TO_BODY(name)\
		dataRelToBody.name = carBody->localToWorld(\
			dataRelToWheel.name + dataRelToWheel.refPoint)

	CALC_REL_TO_BODY(carBottomWB_F);
	CALC_REL_TO_BODY(carBottomWB_R);
	CALC_REL_TO_BODY(carStrut);
	CALC_REL_TO_BODY(tyreBottomWB);
	CALC_REL_TO_BODY(tyreStrut);
	CALC_REL_TO_BODY(carSteer);
	CALC_REL_TO_BODY(tyreSteer);

	setPositions();

	#define CREATE_DIST_JOINT(id, a, b)\
		joints[id] = core->createDistanceJoint(carBody, hub, \
			dataRelToBody.a, dataRelToBody.b)

	CREATE_DIST_JOINT(0, carBottomWB_R, tyreBottomWB);
	CREATE_DIST_JOINT(1, carBottomWB_F, tyreBottomWB);
	CREATE_DIST_JOINT(2, carSteer, tyreSteer);

	auto vCarStrut = carBody->localToWorld(dataRelToBody.carStrut);
	auto vTyreStrut = hub->localToWorld(dataRelToWheel.tyreStrut);

	joints[3] = core->createSliderJoint(strutBody, hub, (vTyreStrut - vCarStrut));
	joints[4] = core->createBallJoint(carBody, strutBody, vCarStrut);

	steerLinkBaseLength = (dataRelToBody.carSteer - dataRelToBody.tyreSteer).len();
	strutBaseLength = (vTyreStrut - vCarStrut).len();

	bumpStopJoint = core->createBumpJoint(carBody, hub, 
		dataRelToWheel.refPoint, bumpStopUp, bumpStopDn);
}

void SuspensionStrut::setPositions()
{
	mat44f mxBody(carBody->getWorldMatrix(0));
	auto vPos = carBody->localToWorld(basePosition);

	hub->setRotation(mxBody);
	hub->setPosition(vPos);

	auto vCarStrut = carBody->localToWorld(dataRelToBody.carStrut);
	auto vTyreStrut = hub->localToWorld(dataRelToWheel.tyreStrut);
	auto vNorm = (vTyreStrut - vCarStrut).get_norm();

	// TODO: WTF???
	auto vM3 = vec3f(&mxBody.M31) * -1.0f;
	auto vM3N = vM3.cross(vNorm);
	auto vM3NN = vM3N.cross(vNorm).norm();

	mat44f mxStrut;
	mxStrut.M11 = vM3NN.x;
	mxStrut.M12 = vM3NN.y;
	mxStrut.M13 = vM3NN.z;
	mxStrut.M14 = 0;
	mxStrut.M21 = -vM3N.x;
	mxStrut.M22 = -vM3N.y;
	mxStrut.M23 = -vM3N.z;
	mxStrut.M24 = 0;
	mxStrut.M31 = -vNorm.x;
	mxStrut.M32 = -vNorm.y;
	mxStrut.M33 = -vNorm.z;
	mxStrut.M34 = 0;

	strutBody->setRotation(mxStrut);

	auto vStrutPos = (vNorm * strutBodyLength) * 0.5f + vCarStrut;
	strutBody->setPosition(vStrutPos);
}

void SuspensionStrut::stop()
{
	hub->stop();
}

void SuspensionStrut::step(float dt)
{
	steerTorque = 0;

	mat44f mxBodyWorld = carBody->getWorldMatrix(0.0f);

	vec3f vCarStrut = carBody->localToWorld(dataRelToBody.carStrut);
	vec3f vTyreStrut = hub->localToWorld(dataRelToWheel.tyreStrut);

	vec3f vDelta = vTyreStrut - vCarStrut;
	float fDeltaLen = vDelta.len();
	vDelta.norm(fDeltaLen);

	float fDefaultLength = strutBaseLength + rodLength;
	float fTravel = fDefaultLength - fDeltaLen;
	status.travel = fTravel;

	float fForce = ((fTravel * progressiveK) + k) * fTravel;
	if (fForce < 0) 
		fForce = 0;

	if (packerRange != 0.0f && fTravel > packerRange)
		fForce += ((fTravel - packerRange) * bumpStopRate);

	if (fForce > 0)
	{
		vec3f vForce = vDelta * fForce;
		addForceAtPos(vForce, vTyreStrut, false, false);
		carBody->addForceAtPos(vForce * -1.0f, vCarStrut);
	}

	vec3f vHubWorld = hub->getPosition(0.0f);
	vec3f vHubLocal = carBody->worldToLocal(vHubWorld);
	float fHubDelta = vHubLocal.y - dataRelToWheel.refPoint.y;

	if (fHubDelta > bumpStopUp)
	{
		fForce = (fHubDelta - bumpStopUp) * 500000.0f;
		addForceAtPos(vec3f(&mxBodyWorld.M21) * -fForce, hub->getPosition(0.0f), false, false);
		carBody->addLocalForceAtLocalPos(vec3f(0, fForce, 0), vHubLocal);
	}

	if (fHubDelta < bumpStopDn)
	{
		fForce = (fHubDelta - bumpStopDn) * 500000.0f;
		addForceAtPos(vec3f(&mxBodyWorld.M21) * -fForce, hub->getPosition(0.0f), false, false);
		carBody->addLocalForceAtLocalPos(vec3f(0, fForce, 0), vHubLocal);
	}

	vec3f vTyreStrutVel = hub->getLocalPointVelocity(dataRelToWheel.tyreStrut);
	vec3f vCarStrutVel = carBody->getLocalPointVelocity(dataRelToBody.carStrut);
	vec3f vDamperDelta = vTyreStrutVel - vCarStrutVel;

	float fDamperSpeed = vDamperDelta * vDelta;
	status.damperSpeedMS = fDamperSpeed;

	float fDamperForce = damper.getForce(fDamperSpeed);
	vec3f vDamperForce = vDelta * fDamperForce;
	addForceAtPos(vDamperForce, vTyreStrut, false, false);
	carBody->addForceAtPos(vDamperForce * -1.0f, vCarStrut);
}

void SuspensionStrut::addForceAtPos(const vec3f& force, const vec3f& pos, bool driven, bool addToSteerTorque)
{
	hub->addForceAtPos(force, pos);

	if (addToSteerTorque)
	{
		vec3f vCenter, vAxis;
		getSteerBasis(vCenter, vAxis);

		steerTorque -= force.cross(pos - vCenter) * vAxis;
	}
}

void SuspensionStrut::addLocalForceAndTorque(const vec3f& force, const vec3f& torque, const vec3f& driveTorque)
{
	hub->addForceAtLocalPos(force, vec3f(0, 0, 0));
	hub->addTorque(torque);

	vec3f vCenter, vAxis;
	getSteerBasis(vCenter, vAxis);

	vec3f vHubPos = hub->getPosition(0);

	steerTorque += (vHubPos - vCenter).cross(force) * vAxis;
	steerTorque += torque * vAxis;

	if (driveTorque.x != 0.0f || driveTorque.y != 0.0f || driveTorque.z != 0.0f)
		carBody->addTorque(driveTorque);
}

void SuspensionStrut::addTorque(const vec3f& torque)
{
	hub->addTorque(torque);

	vec3f vCenter, vAxis;
	getSteerBasis(vCenter, vAxis);

	steerTorque += torque * vAxis;
}

void SuspensionStrut::setERPCFM(float erp, float cfm)
{
	for (auto& iter : joints)
	{
		iter->setERPCFM(erp, cfm);
	}
}

void SuspensionStrut::setSteerLengthOffset(float o)
{
	float sx = signf(dataRelToWheel.refPoint.x);
	float d = (damageData.damageDirection * damageData.damageAmount);
	float offx = d + o + (sx * toeOUT_Linear);

	const auto& base = baseCarSteerPosition;
	dataRelToBody.carSteer = vec3f(base.x + offx, base.y, base.z);

	joints[2]->reseatDistanceJointLocal(dataRelToBody.carSteer, dataRelToWheel.tyreSteer);
}

void SuspensionStrut::getSteerBasis(vec3f& centre, vec3f& axis)
{
	auto vTop = carBody->localToWorld(dataRelToBody.carStrut);
	auto vBottom = hub->localToWorld(dataRelToWheel.tyreStrut);
	centre = (vTop + vBottom) * 0.5f;
	axis = (vTop - vBottom).get_norm();
}

mat44f SuspensionStrut::getHubWorldMatrix() // TODO: check
{
	auto m0 = hub->getWorldMatrix(0);
	mat44f rotator = mat44f::createFromAxisAngle(vec3f(0, 0, 1), staticCamber);
	auto m1 = mat44f::mult(rotator, m0);
	return m1;
}

vec3f SuspensionStrut::getBasePosition()
{
	return basePosition;
}

vec3f SuspensionStrut::getVelocity()
{
	return hub->getVelocity();
}

vec3f SuspensionStrut::getPointVelocity(const vec3f& p)
{
	return hub->getPointVelocity(p);
}

vec3f SuspensionStrut::getHubAngularVelocity()
{
	return hub->getAngularVelocity();
}

float SuspensionStrut::getMass()
{
	return hub->getMass();
}

float SuspensionStrut::getSteerTorque()
{
	return steerTorque;
}

void SuspensionStrut::getDebugState(CarDebug* state)
{
	auto vCarStrut = carBody->localToWorld(dataRelToBody.carStrut);
	auto vTyreStrut = hub->localToWorld(dataRelToWheel.tyreStrut);

	state->points.emplace_back(ColorPoint{vCarStrut, rgb(255,69,0)});
	state->points.emplace_back(ColorPoint{vTyreStrut, rgb(255,255,255)});

	auto bodyM = carBody->getWorldMatrix(0);
	auto hubM = hub->getWorldMatrix(0);

	state->lines.emplace_back(ColorLine{
		dataRelToBody.carBottomWB_R * bodyM,
		dataRelToWheel.tyreBottomWB * hubM,
		vec3f(1,1,1)
	});

	state->lines.emplace_back(ColorLine{
		dataRelToBody.carBottomWB_F * bodyM,
		dataRelToWheel.tyreBottomWB * hubM,
		vec3f(1,1,1)
	});

	state->lines.emplace_back(ColorLine{
		dataRelToBody.carSteer * bodyM,
		dataRelToWheel.tyreSteer * hubM,
		vec3f(1,1,1)
	});

	state->lines.emplace_back(ColorLine{
		dataRelToBody.carStrut * bodyM,
		dataRelToWheel.tyreStrut * hubM,
		vec3f(1,1,1)
	});
}

}
