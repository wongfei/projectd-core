#pragma once

namespace D {

struct TyreStatus // orig
{
	float depth;
	float load;
	float camberRAD;
	float slipAngleRAD;
	float slipRatio;
	float angularVelocity;
	float Fy;
	float Fx;
	float Mz;
	bool isLocked;
	float slipFactor;
	float ndSlip;
	float distToGround;
	float Dy;
	float Dx;
	float D;
	float dirtyLevel;
	float rollingResistence;
	float thermalInput;
	float feedbackTorque;
	float loadedRadius;
	float effectiveRadius;
	float liveRadius;
	float pressureStatic;
	float pressureDynamic;
	double virtualKM;
	float lastTempIMO[3];
	float peakSA;
	double grain;
	double blister;
	float inflation;
	double flatSpot;
	float lastGrain;
	float lastBlister;
	float normalizedSlideX;
	float normalizedSlideY;
	float finalDY;
	float wearMult;
};

}
