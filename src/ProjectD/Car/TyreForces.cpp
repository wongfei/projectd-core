#include "Car/Tyre.h"
#include "Car/Car.h"
#include "Car/ISuspension.h"
#include "Car/TyreModel.h"
#include "Car/TyreThermalModel.h"
#include "Sim/Simulator.h"
#include "Sim/Track.h"

#include "TyreUtils.inl"

namespace D {

void Tyre::addTyreForcesV10(const vec3f& pos, const vec3f& normal, Surface* pSurface, float dt) // TODO: cleanup
{
	vec3f vNegM3(&worldRotation.M31);
	vNegM3 *= -1.0f;
	roadHeading = vNegM3 - normal * (vNegM3 * normal);
	roadHeading.norm();

	vec3f vM1(&worldRotation.M11);
	roadRight = vM1 - normal * (vM1 * normal);
	roadRight.norm();

	vec3f hubAngVel = hub->getHubAngularVelocity();
	vec3f hubPointVel = hub->getPointVelocity(pos);

	slidingVelocityY = hubPointVel * roadRight;
	roadVelocityX = -(hubPointVel * roadHeading);
	float fSlipAngleTmp = calcSlipAngleRAD(slidingVelocityY, roadVelocityX);

	float fTmp = (hubAngVel * vM1) + status.angularVelocity;
	slidingVelocityX = (fTmp * status.effectiveRadius) - roadVelocityX;
	
	float fRoadVelocityXAbs = fabsf(roadVelocityX);
	float fSlipRatioTmp = ((fRoadVelocityXAbs == 0.0f) ? 0.0f : (slidingVelocityX / fRoadVelocityXAbs));

	if (externalInputs.isActive)
	{
		fSlipAngleTmp = externalInputs.slipAngle;
		fSlipRatioTmp = externalInputs.slipRatio;
	}

	status.camberRAD = calcCamberRAD(contactNormal, worldRotation);
	totalHubVelocity = sqrtf((roadVelocityX * roadVelocityX) + (slidingVelocityY * slidingVelocityY));

	float fNdSlip = tclamp(status.ndSlip, 0.0f, 1.0f);
	float fLoadDivFz0 = status.load / modelData.Fz0;
	float fRelaxLen = modelData.relaxationLength;
	float fRelax1 = (((fLoadDivFz0 * fRelaxLen) - fRelaxLen) * 0.3f) + fRelaxLen;
	float fRelax2 = ((fRelaxLen - (fRelax1 * 2.0f)) * fNdSlip) + (fRelax1 * 2.0f);

	if (totalHubVelocity < 1.0f)
	{
		fSlipRatioTmp = slidingVelocityX * 0.5f;
		fSlipRatioTmp = tclamp(fSlipRatioTmp, -1.0f, 1.0f);

		fSlipAngleTmp = slidingVelocityY * -5.5f;
		fSlipAngleTmp = tclamp(fSlipAngleTmp, -1.0f, 1.0f);
	}

	float fSlipRatio = status.slipRatio;
	float fSlipRatioDelta = fSlipRatioTmp - fSlipRatio;
	float fNewSlipRatio = fSlipRatioTmp;

	if (fRelax2 != 0.0f)
	{
		float fSlipRatioScale = (totalHubVelocity * dt) / fRelax2;
		if (fSlipRatioScale <= 1.0f)
		{
			if (fSlipRatioScale < 0.04f)
				fNewSlipRatio = (0.04f * fSlipRatioDelta) + fSlipRatio;
			else
				fNewSlipRatio = (fSlipRatioScale * fSlipRatioDelta) + fSlipRatio;
		}
	}

	float fSlipAngle = status.slipAngleRAD;
	float fSlipAngleDelta = fSlipAngleTmp - fSlipAngle;
	float fNewSlipAngle = fSlipAngleTmp;

	if (fRelax2 != 0.0f)
	{
		float fSlipAngleScale = (totalHubVelocity * dt) / fRelax2;
		if (fSlipAngleScale <= 1.0f)
		{
			if (fSlipAngleScale < 0.04f)
				fNewSlipAngle = (0.04f * fSlipAngleDelta) + fSlipAngle;
			else
				fNewSlipAngle = (fSlipAngleScale * fSlipAngleDelta) + fSlipAngle;
		}
	}

	status.slipAngleRAD = fNewSlipAngle;
	status.slipRatio = fNewSlipRatio;

	if (status.load <= 0.0f)
	{
		status.slipAngleRAD = 0;
		status.slipRatio = 0;
	}

	float fCorrectedD = getCorrectedD(1.0, &status.wearMult);
	float fDynamicGripLevel = car->track->dynamicGripLevel;

	TyreModelInput tmi;
	tmi.load = status.load;
	tmi.slipAngleRAD = status.slipAngleRAD;
	tmi.slipRatio = status.slipRatio;
	tmi.camberRAD = status.camberRAD;
	tmi.speed = totalHubVelocity;
	tmi.u = (fCorrectedD * pSurface->gripMod) * fDynamicGripLevel;
	tmi.tyreIndex = index;
	tmi.cpLength = calcContactPatchLength(status.liveRadius, status.depth);
	tmi.grain = (float)status.grain;
	tmi.blister = (float)status.blister;
	tmi.pressureRatio = (status.pressureDynamic / modelData.idealPressure) - 1.0f;
	tmi.useSimpleModel = 1.0f < aiMult;

	TyreModelOutput tmo = tyreModel->solve(tmi);
	
	status.Fy = tmo.Fy * aiMult;
	status.Fx = -tmo.Fx;
	status.Dy = tmo.Dy;
	status.Dx = tmo.Dx;

	float fHubSpeed = status.effectiveRadius * status.angularVelocity;

	stepDirtyLevel(dt, fabsf(fHubSpeed));
	stepPuncture(dt, totalHubVelocity);
	status.Mz = tmo.Mz;

	vec3f vForce = (roadHeading * status.Fx) + (roadRight * status.Fy);

	if (!(isfinite(vForce.x) && isfinite(vForce.y) && isfinite(vForce.z)))
	{
		SHOULD_NOT_REACH_WARN;
		vForce = vec3f(0, 0, 0);
	}

	if (car->torqueModeEx != TorqueModeEX::original)
	{
		addTyreForceToHub(pos, vForce);
	}
	else
	{
		hub->addForceAtPos(vForce, pos, driven, true);
		localMX = -(status.loadedRadius * status.Fx);
	}

	hub->addTorque(normal * tmo.Mz);

	float fAngularVelocityAbs = fabsf(status.angularVelocity);
	if (fAngularVelocityAbs > 1.0f)
	{
		fHubSpeed = status.effectiveRadius * status.angularVelocity;
		float fHubSpeedSign = signf(fHubSpeed);

		float fPressureDynamic = status.pressureDynamic;
		float fPressureUnk = (((modelData.idealPressure / fPressureDynamic) - 1.0f) * modelData.pressureRRGain) + 1.0f;

		if (fPressureDynamic <= 0.0f)
			fPressureUnk = 0;

		float fRrUnk = ((((fHubSpeed * fHubSpeed) * modelData.rr1) + modelData.rr0) * fHubSpeedSign) * fPressureUnk;

		if (fAngularVelocityAbs > 20.0f)
		{
			float fNdSlipNorm = tclamp(status.ndSlip, 0.0f, 1.0f);
			float fRrSlipUnk = fPressureUnk * modelData.rr_slip;
			float fSlipUnk = fNdSlipNorm * fRrSlipUnk;
			fRrUnk = fRrUnk * ((fSlipUnk * 0.001f) + 1.0f);
		}

		status.rollingResistence = -(((status.load * 0.001f) * fRrUnk) * status.effectiveRadius);
	}

	if (car)
	{
		float svx = slidingVelocityX;
		float svy = slidingVelocityY;
		float fSlidingVelocity = sqrtf(svx * svx + svy * svy);

		float fLoadVKM = 1.0f;
		if (useLoadForVKM)
			fLoadVKM = status.load / modelData.Fz0;

		status.virtualKM += (((fSlidingVelocity * dt) * car->sim->tyreConsumptionRate) * fLoadVKM) * 0.001f;
	}

	float fStaticDy = tyreModel->getStaticDY(status.load);

	status.ndSlip = tmo.ndSlip;
	status.D = fStaticDy;
}

float Tyre::getCorrectedD(float d, float* outWearMult)
{
	float fD = thermalModel->getCorrectedD(d, status.camberRAD) / ((fabsf(status.pressureDynamic - modelData.idealPressure) * modelData.pressureGainD) + 1.0f);

	if (modelData.wearCurve.getCount())
	{
		float fWearMult = modelData.wearCurve.getValue((float)status.virtualKM);
		fD *= fWearMult;

		if (outWearMult)
			*outWearMult = fWearMult;
	}

	return fD;
}

void Tyre::stepDirtyLevel(float dt, float hubSpeed)
{
	if (status.dirtyLevel < 5.0f)
		status.dirtyLevel += (((hubSpeed * surfaceDef->dirtAdditiveK) * 0.03f) * dt);

	if (surfaceDef->dirtAdditiveK == 0.0f)
	{
		if (status.dirtyLevel > 0.0f)
			status.dirtyLevel -= ((hubSpeed * 0.015f) * dt);

		if (status.dirtyLevel < 0.0f)
			status.dirtyLevel = 0;
	}

	if (aiMult == 1.0f)
	{
		float fM = tmax(0.8f, (1.0f - tclamp(status.dirtyLevel * 0.05f, 0.0f, 1.0f)));
		status.Fy *= fM;
		status.Fx *= fM;
		status.Mz *= fM;
	}
}

void Tyre::stepPuncture(float dt, float hubSpeed)
{
	if (car->sim->mechanicalDamageRate > 0.0f)
	{
		float imo[3];
		thermalModel->getIMO(imo);

		float fTemp = explosionTemperature;

		if (imo[0] > fTemp || imo[1] > fTemp || imo[2] > fTemp)
			status.inflation = 0;
	}
}

void Tyre::addTyreForceToHub(const vec3f& pos, const vec3f& force) // TODO: what car?
{
	TODO_NOT_IMPLEMENTED_FATAL;

	#if 0
	vec3f vWP = worldPosition;
	float fWorldX = worldPosition.x;
	float fWorldY = worldPosition.y;
	float fWorldZ = worldPosition.z;

	mat44f mxWR = worldRotation;
	float* M1 = &mxWR.M11;
	float* M2 = &mxWR.M21;
	float* M3 = &mxWR.M31;
	float* M4 = &mxWR.M41;

	M4[0] = fWorldX;
	M4[1] = fWorldY;
	M4[2] = fWorldZ;

	DirectX::XMVECTOR xmvDet;
	mat44f mxInv = xmstore(DirectX::XMMatrixInverse(&xmvDet, xmload(mxWR)));

	float fNewForceX = (force.y * mxInv.M21) + (force.x * mxInv.M11) + (force.z * mxInv.M31);
	float fNewForceY = (force.y * mxInv.M22) + (force.x * mxInv.M12) + (force.z * mxInv.M32);
	float fNewForceZ = (force.y * mxInv.M23) + (force.x * mxInv.M13) + (force.z * mxInv.M33);

	float fNewPosX = (pos.y * mxInv.M21) + (pos.x * mxInv.M11) + (pos.z * mxInv.M31) + mxInv.M41;
	float fNewPosY = (pos.y * mxInv.M22) + (pos.x * mxInv.M12) + (pos.z * mxInv.M32) + mxInv.M42;
	float fNewPosZ = (pos.y * mxInv.M23) + (pos.x * mxInv.M13) + (pos.z * mxInv.M33) + mxInv.M43;

	float fZY = (fNewForceZ * fNewPosY) - (fNewForceY * fNewPosZ);
	float fXZ = (fNewForceX * fNewPosZ) - (fNewForceZ * fNewPosX);
	float fYX = (fNewForceY * fNewPosX) - (fNewForceX * fNewPosY);

	vec3f vTorq(0, 0, 0);
	vec3f vDriveTorq(0, 0, 0);

	if (car->torqueModeEx == TorqueModeEX::reactionTorques) // (torqueModeEx - 1) == 0
	{
		vTorq.x = ((M2[0] * fXZ) + (M1[0] * 0.0f)) + (M3[0] * fYX);
		vTorq.y = ((M2[1] * fXZ) + (M1[1] * 0.0f)) + (M3[1] * fYX);
		vTorq.z = ((M2[2] * fXZ) + (M1[2] * 0.0f)) + (M3[2] * fYX);
	}
	else if (car->torqueModeEx == TorqueModeEX::driveTorques) // (torqueModeEx - 1) == 1
	{
		if (driven)
		{
			vDriveTorq.x = ((M1[0] * fZY) + (M2[0] * 0.0f)) + (M3[0] * 0.0f);
			vDriveTorq.y = ((M1[1] * fZY) + (M2[1] * 0.0f)) + (M3[1] * 0.0f);
			vDriveTorq.z = ((M1[2] * fZY) + (M2[2] * 0.0f)) + (M3[2] * 0.0f);

			vTorq.x = ((M2[0] * fXZ) + (M1[0] * 0.0f)) + (M3[0] * fYX);
			vTorq.y = ((M2[1] * fXZ) + (M1[1] * 0.0f)) + (M3[1] * fYX);
			vTorq.z = ((M2[2] * fXZ) + (M1[2] * 0.0f)) + (M3[2] * fYX);
		}
		else
		{
			vTorq.x = ((M2[0] * fXZ) + (M1[0] * fZY)) + (M3[0] * fYX);
			vTorq.y = ((M2[1] * fXZ) + (M1[1] * fZY)) + (M3[1] * fYX);
			vTorq.z = ((M2[2] * fXZ) + (M1[2] * fZY)) + (M3[2] * fYX);
		}
	}

	hub->addLocalForceAndTorque(force, vTorq, vDriveTorq);
	localMX = fabsf(fZY); // TODO: wtf?
	#endif
}

}
