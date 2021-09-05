#include "Car/AntirollBar.h"
#include "Car/ISuspension.h"

namespace D {

AntirollBar::AntirollBar()
{}

AntirollBar::~AntirollBar()
{}

void AntirollBar::init(IRigidBody* _carBody, ISuspension* s0, ISuspension* s1)
{
	carBody = _carBody;
	hubs[0] = s0;
	hubs[1] = s1;
}

void AntirollBar::step(float dt)
{
	if (ctrl.ready)
		k = ctrl.eval();

	if (k > 0.0f)
	{
		mat44f mxBodyWorld = carBody->getWorldMatrix(0);
		vec3f vBodyM2(&mxBodyWorld.M21);

		mat44f mxHub0 = hubs[0]->getHubWorldMatrix();
		vec3f vHubWorld0(&mxHub0.M41);
		vec3f vHubLoc0 = carBody->worldToLocal(vHubWorld0);

		mat44f mxHub1 = hubs[1]->getHubWorldMatrix();
		vec3f vHubWorld1(&mxHub1.M41);
		vec3f vHubLoc1 = carBody->worldToLocal(vHubWorld1);

		float fDelta = vHubLoc1.y - vHubLoc0.y;
		float fDeltaK = fDelta * k;
		vec3f vForce = vBodyM2.get_norm() * fDeltaK;

		hubs[0]->addForceAtPos(vForce, vHubWorld0, false, false);
		hubs[1]->addForceAtPos(vForce * -1.0f, vHubWorld1, false, false);

		carBody->addLocalForceAtLocalPos(vec3f(0, -fDeltaK, 0), vHubLoc0);
		carBody->addLocalForceAtLocalPos(vec3f(0, fDeltaK, 0), vHubLoc1);
	}
}

}
