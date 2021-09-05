#include "Car/SuspensionDW.h"
#include "Car/AntirollBar.h"

namespace D {

SuspensionDW::SuspensionDW()
{
	k = 90000.0f;
	baseCFM = 0.0000001f;
	damageData.minVelocity = 15.0f;
	damageData.damageDirection = randDamageDirection();
}

SuspensionDW::~SuspensionDW()
{
}

void SuspensionDW::init(IPhysicsEnginePtr _core, IRigidBodyPtr _carBody, AntirollBar* arb1, AntirollBar* arb2, int _index, const std::wstring& dataPath)
{
	type = SuspensionType::DoubleWishbone;
	index = _index;

	core = _core;
	carBody = _carBody;
	arb[0] = arb1;
	arb[1] = arb2;

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

	dataRelToWheel.carTopWB_F = ini->getFloat3(strId, L"WBCAR_TOP_FRONT");
	dataRelToWheel.carTopWB_R = ini->getFloat3(strId, L"WBCAR_TOP_REAR");
	dataRelToWheel.carBottomWB_F = ini->getFloat3(strId, L"WBCAR_BOTTOM_FRONT");
	dataRelToWheel.carBottomWB_R = ini->getFloat3(strId, L"WBCAR_BOTTOM_REAR");
	dataRelToWheel.tyreTopWB = ini->getFloat3(strId, L"WBTYRE_TOP");
	dataRelToWheel.tyreBottomWB = ini->getFloat3(strId, L"WBTYRE_BOTTOM");
	dataRelToWheel.tyreSteer = ini->getFloat3(strId, L"WBTYRE_STEER");
	dataRelToWheel.carSteer = ini->getFloat3(strId, L"WBCAR_STEER");

	if (iVer >= 2)
	{
		float fRimOffset = -ini->getFloat(strId, L"RIM_OFFSET");
		if (fRimOffset != 0.0f)
		{
			dataRelToWheel.carTopWB_F.x += fRimOffset;
			dataRelToWheel.carTopWB_R.x += fRimOffset;
			dataRelToWheel.carBottomWB_F.x += fRimOffset;
			dataRelToWheel.carBottomWB_R.x += fRimOffset;
			dataRelToWheel.tyreTopWB.x += fRimOffset;
			dataRelToWheel.tyreBottomWB.x += fRimOffset;
			dataRelToWheel.tyreSteer.x += fRimOffset;
			dataRelToWheel.carSteer.x += fRimOffset;
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

	if (ini->hasKey(strId, L"BUMP_STOP_PROGRESSIVE"))
	{
		bumpStopProgressive = ini->getFloat(strId, L"BUMP_STOP_PROGRESSIVE");
	}

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
		dataRelToWheel.carTopWB_F.x *= -1.0f;
		dataRelToWheel.carTopWB_R.x *= -1.0f;
		dataRelToWheel.tyreBottomWB.x *= -1.0f;
		dataRelToWheel.tyreSteer.x *= -1.0f;
		dataRelToWheel.tyreTopWB.x *= -1.0f;
	}

	float fMass = dataRelToWheel.hubMass;
	if (fMass <= 0.0f)
		fMass = 20.0f;

	vec3f vInertia = dataRelToWheel.hubInertiaBox;
	if (vInertia.len() == 0.0f)
		vInertia = vec3f(0.2f, 0.6f, 0.6f); // TODO: check

	hub = core->createRigidBody();
	hub->setMassBox(fMass, vInertia.x, vInertia.y, vInertia.z);

	basePosition = dataRelToWheel.refPoint;
	attach();

	baseCarSteerPosition = dataRelToBody.carSteer;
	setSteerLengthOffset(0);

	for (auto& pJoint : joints)
	{
		pJoint->setERPCFM(0.3f, baseCFM);
	}
}

void SuspensionDW::attach()
{
	hub->setRotation(carBody->getWorldMatrix(0));
	hub->setPosition(carBody->localToWorld(basePosition));

	if (joints[0])
		return;

	#define CALC_REL_TO_BODY(name)\
		dataRelToBody.name = carBody->localToWorld(\
			dataRelToWheel.name + dataRelToWheel.refPoint)

	#define CREATE_JOINT(id, a, b)\
		joints[id] = core->createDistanceJoint(carBody, hub, \
			dataRelToBody.a, dataRelToBody.b)

	CALC_REL_TO_BODY(carBottomWB_F);
	CALC_REL_TO_BODY(carBottomWB_R);
	CALC_REL_TO_BODY(carTopWB_F);
	CALC_REL_TO_BODY(carTopWB_R);
	CALC_REL_TO_BODY(tyreBottomWB);
	CALC_REL_TO_BODY(tyreTopWB);
	CALC_REL_TO_BODY(carSteer);
	CALC_REL_TO_BODY(tyreSteer);

	CREATE_JOINT(0, carTopWB_R, tyreTopWB);
	CREATE_JOINT(1, carTopWB_F, tyreTopWB);
	CREATE_JOINT(2, carBottomWB_R, tyreBottomWB);
	CREATE_JOINT(3, carBottomWB_F, tyreBottomWB);
	CREATE_JOINT(4, carSteer, tyreSteer);

	#undef CALC_REL_TO_BODY
	#undef CREATE_JOINT

	steerLinkBaseLength = (dataRelToBody.carSteer - dataRelToBody.tyreSteer).len();

	bumpStopJoint = core->createBumpJoint(carBody, hub, 
		dataRelToWheel.refPoint, bumpStopUp, bumpStopDn);
}

void SuspensionDW::stop()
{
	hub->stop();
}

void SuspensionDW::step(float dt)
{
	steerTorque = 0;

	mat44f mxBodyWorld = carBody->getWorldMatrix(0.0f);
	vec3f vBodyM2(&mxBodyWorld.M21);

	vec3f vHubWorldPos = hub->getPosition(0.0f);
	vec3f vHubLocalPos = carBody->worldToLocal(vHubWorldPos);

	float fHubDeltaY = vHubLocalPos.y - dataRelToWheel.refPoint.y;
	float fTravel = fHubDeltaY + rodLength;
	status.travel = fTravel;

	if (useActiveActuator)
	{
		activeActuator.targetTravel = 0;
		float fActiveActuator = activeActuator.eval(dt, fHubDeltaY);

		arb[0]->k = 0;
		arb[1]->k = 0;

		vec3f vForce = vBodyM2 * fActiveActuator;
		addForceAtPos(vForce, vHubWorldPos, false, false);
		carBody->addForceAtLocalPos(vForce * -1.0f, dataRelToWheel.refPoint);

		status.travel = fHubDeltaY;
	}
	else
	{
		float fForce = ((fTravel * progressiveK) + k) * fTravel;
		if (packerRange != 0.0f && fTravel > packerRange && k != 0.0f)
		{
			fForce += (((fTravel - packerRange) * bumpStopProgressive) + bumpStopRate) * (fTravel - packerRange);
		}

		if (fForce > 0.0f)
		{
			addForceAtPos(vBodyM2 * -fForce, vHubWorldPos, false, false);
			carBody->addLocalForceAtLocalPos(vec3f(0, fForce, 0), dataRelToWheel.refPoint);
		}

		vec3f vHubVel = hub->getVelocity();
		vec3f vPointVel = carBody->getLocalPointVelocity(dataRelToWheel.refPoint);
		vec3f vDeltaVel = vHubVel - vPointVel;

		float fDamperSpeed = vDeltaVel * vBodyM2;
		status.damperSpeedMS = fDamperSpeed;

		float fDamperForce = damper.getForce(fDamperSpeed);
		vec3f vForce = vBodyM2 * fDamperForce;
		addForceAtPos(vForce, vHubWorldPos, false, false);
		carBody->addForceAtLocalPos(vForce * -1.0f, dataRelToWheel.refPoint);
	}

	if (bumpStopUp != 0.0f && fHubDeltaY > bumpStopUp && 0.0f != k)
	{
		float fForce = (((fHubDeltaY - bumpStopUp) * bumpStopProgressive) + bumpStopRate) * (fHubDeltaY - bumpStopUp);
		addForceAtPos(vBodyM2 * -fForce, vHubWorldPos, false, false);
		carBody->addLocalForceAtLocalPos(vec3f(0, fForce, 0), vHubLocalPos);
	}

	if (bumpStopDn != 0.0f && fHubDeltaY < bumpStopDn && 0.0f != k)
	{
		float fForce = (((fHubDeltaY - bumpStopDn) * bumpStopProgressive) + bumpStopRate) * (fHubDeltaY - bumpStopDn);
		addForceAtPos(vBodyM2 * -fForce, vHubWorldPos, false, false);
		carBody->addLocalForceAtLocalPos(vec3f(0, fForce, 0), vHubLocalPos);
	}
}

void SuspensionDW::addForceAtPos(const vec3f& force, const vec3f& pos, bool driven, bool addToSteerTorque)
{
	hub->addForceAtPos(force, pos);

	if (addToSteerTorque)
	{
		vec3f vCenter, vAxis;
		getSteerBasis(vCenter, vAxis);

		steerTorque += (pos - vCenter).cross(force) * vAxis;
	}
}

void SuspensionDW::addLocalForceAndTorque(const vec3f& force, const vec3f& torque, const vec3f& driveTorque)
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

void SuspensionDW::addTorque(const vec3f& torque)
{
	hub->addTorque(torque);

	vec3f vCenter, vAxis;
	getSteerBasis(vCenter, vAxis);

	steerTorque += torque * vAxis;
}

void SuspensionDW::setERPCFM(float erp, float cfm)
{
	for (auto& pJoint : joints)
	{
		pJoint->setERPCFM(erp, cfm);
	}
}

void SuspensionDW::setSteerLengthOffset(float offset)
{
	float sx = signf(dataRelToWheel.refPoint.x);
	float d = (damageData.damageDirection * damageData.damageAmount);
	float offx = d + offset + (sx * toeOUT_Linear);

	const auto& base = baseCarSteerPosition;
	dataRelToBody.carSteer = vec3f(base.x + offx, base.y, base.z);

	joints[4]->reseatDistanceJointLocal(dataRelToBody.carSteer, dataRelToWheel.tyreSteer);
}

void SuspensionDW::getSteerBasis(vec3f& centre, vec3f& axis)
{
	auto vTop = hub->localToWorld(dataRelToWheel.tyreTopWB);
	auto vBottom = hub->localToWorld(dataRelToWheel.tyreBottomWB);
	centre = (vTop + vBottom) * 0.5f;
	axis = (vTop - vBottom).get_norm();
}

mat44f SuspensionDW::getHubWorldMatrix()
{
	return mat44f::rotate(hub->getWorldMatrix(0), vec3f(0, 0, 1), staticCamber);
}

vec3f SuspensionDW::getBasePosition()
{
	return basePosition;
}

vec3f SuspensionDW::getVelocity()
{
	return hub->getVelocity();
}

vec3f SuspensionDW::getPointVelocity(const vec3f& p)
{
	return hub->getPointVelocity(p);
}

vec3f SuspensionDW::getHubAngularVelocity()
{
	return hub->getAngularVelocity();
}

float SuspensionDW::getMass()
{
	return hub->getMass();
}

float SuspensionDW::getSteerTorque()
{
	return steerTorque;
}

void SuspensionDW::getDebugState(CarDebug* state)
{
}

}
