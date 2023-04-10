#include "Audio/FmodAudioRenderer.h"
#include "Audio/FmodContext.h"
#include "Car/CarState.h"
#include "Car/Car.h"
#include "Car/Drivetrain.h"
#include "Car/Tyre.h"
#include "Sim/Surface.h"

namespace D {

inline FMOD::Studio::EventInstance* castEvent(void* ptr) { return (FMOD::Studio::EventInstance*)ptr; }
inline void stopEvent(void* ptr) { auto ev = castEvent(ptr); if (ev) { ev->stop(FMOD_STUDIO_STOP_IMMEDIATE); } }
inline void releaseEvent(void*& ptr) { auto ev = castEvent(ptr); if (ev) { ev->release(); ptr = nullptr; } }

FmodAudioRenderer::FmodAudioRenderer(Car* _car, const std::wstring& basePath, const std::wstring& carModel)
{
	TRACE_CTOR(FmodAudioRenderer);

	car = _car;

	context.reset(new FmodContext());
	context->init();

	const std::string basePathA(stra(basePath));
	context->loadGUIDs(basePathA + "content/sfx/GUIDs.txt");
	context->loadBank(basePathA + "content/sfx/common.bank");
	//context->loadBank(basePathA + "content/sfx/common.strings.bank");

	context->loadGUIDs(basePathA + "content/cars/" + stra(carModel) + "/sfx/" + "GUIDs.txt");
	context->loadBank(basePathA + "content/cars/" + stra(carModel) + "/sfx/" + stra(carModel) + ".bank");

	context->enumerate();

	// CarAudioFMOD::renderAudio
	// Car::getPhysicsState

	// TODO: fix this ugly hardcoded mess

	{
		auto name = std::string("event:/cars/") + stra(carModel) + "/engine_int";
		auto ev = context->getUniqInstance(name.c_str());
		evEngineInt = ev;

		if (ev)
		{
			//ev->setParameterValueByIndex(0, 0.0f);
			//ev->setParameterValueByIndex(1, 0.0f);
			ev->setParameterValue("throttle", 0.0f);
			ev->setParameterValue("rpms", 0.0f);
			ev->start();
		}
	}

	#if 0
	{
		auto name = std::string("event:/cars/") + stra(carModel) + "/engine_ext";
		auto ev = context->getUniqInstance(name.c_str());
		evEngineExt = ev;

		if (ev)
		{
			ev->setParameterValue("throttle", 0.0f);
			ev->setParameterValue("rpms", 0.0f);
			ev->start();
		}
	}
	#endif

	#if 0
	{
		auto name = std::string("event:/cars/") + stra(carModel) + "/turbo";
		auto ev = context->getUniqInstance(name.c_str());
		evTurbo = ev;

		if (ev)
		{
			ev->setParameterValue("bov", 0.0f); // this->car->physicsState.turboBov
			ev->setParameterValue("bov_decay", 0.0f); // this->turboBovDecay
			ev->setParameterValue("boost", 0.0f); // this->car->physicsState.turboBoost / this->maxTurboBoost
			ev->setParameterValue("Direction", 0.0f);
			ev->start();
		}
	}
	#endif

	{
		auto name = std::string("event:/cars/") + stra(carModel) + "/transmission";
		auto ev = context->getUniqInstance(name.c_str());
		evTransmission = ev;

		if (ev)
		{
			ev->setParameterValue("throttle", 0.0f);
			ev->setParameterValue("drivetrain_speed", 0.0f);
			ev->start();
		}
	}

	{
		auto name = std::string("event:/cars/") + stra(carModel) + "/skid_int";
		auto ev = context->getUniqInstance(name.c_str());
		evSkidInt = ev;

		if (ev)
		{
			ev->setVolume(0.0f);
			ev->start();
		}
	}
}

FmodAudioRenderer::~FmodAudioRenderer()
{
	TRACE_DTOR(FmodAudioRenderer);

	releaseEvent(evEngineInt);
	releaseEvent(evEngineExt);
	releaseEvent(evTurbo);
	releaseEvent(evTransmission);
	releaseEvent(evSkidInt);
}

void FmodAudioRenderer::update(float dt)
{
	if (evEngineInt)
	{
		auto ev = castEvent(evEngineInt);
		//ev->setParameterValueByIndex(0, car->controls.gas);
		//ev->setParameterValueByIndex(1, car->getEngineRpm());
		ev->setParameterValue("throttle", car->controls.gas);
		ev->setParameterValue("rpms", car->getEngineRpm());
	}

	if (evEngineExt)
	{
		auto ev = castEvent(evEngineExt);
		ev->setParameterValue("throttle", car->controls.gas);
		ev->setParameterValue("rpms", car->getEngineRpm());
	}

	if (evTransmission)
	{
		auto ev = castEvent(evTransmission);
		ev->setParameterValue("throttle", car->controls.gas);
		ev->setParameterValue("drivetrain_speed", car->drivetrain->getDrivetrainSpeed());
	}

	//CarAudioFMOD::updateSkids
	if (evSkidInt)
	{
		// TODO: read from config?
		float fSkidEntry = 0.5f;
		float fSkidPitchBase = 0.75f;
		float fSkidPitchGain = 0.8f;
		float fSkidVolumeGain = 2.5f;
		float fSkidSmoothAlhpa = 60.0f;

		float speed = car->speed.value;
		float fSpeedKmh = car->speed.kmh();

		float fLoadAvg = (car->tyres[0]->status.load + car->tyres[1]->status.load + car->tyres[2]->status.load + car->tyres[3]->status.load) * 0.125f;
		if (fLoadAvg == 0.0f)
			fLoadAvg = 0.01f;

		float fVolumeMax = 0;
		float fPitchMax = 0;

		for (int i = 0; i < 4; ++i)
		{
			float fVolume = 0;
			float fPitch = 0;

			auto tyre = car->tyres[i].get();
			float wheelAngularSpeed = tyre->status.angularVelocity;

			if (tyre->surfaceDef && tyre->surfaceDef->gripMod >= 0.9f)
			{
				if (speed <= 1.0 && fabsf(wheelAngularSpeed) <= 8.0f || fabsf(tyre->status.slipRatio) <= 0.8f)
				{
					if (speed > 1.0f)
					{
						if (tyre->status.ndSlip >= fSkidEntry)
						{
							float fSlipEntry = (tyre->status.ndSlip - fSkidEntry) * 0.16666667f;
							fSlipEntry = tclamp(fSlipEntry, 0.0f, 1.0f);

							float fVolumeGain = fSlipEntry * fSkidVolumeGain;

							float fLoadRatio = car->tyres[i]->status.load / fLoadAvg;
							fLoadRatio = tmax(fLoadRatio, 0.2f);

							fVolume = fVolumeGain * fLoadRatio;
							fVolume = tclamp(fVolume, 0.0f, 1.0f);

							fPitch = (fSlipEntry * fSkidPitchGain) + fSkidPitchBase;
							fPitch = tclamp(fPitch, fSkidPitchBase, 1.2f);
						}
					}
				}
				else
				{
					float fSpeedFactor = fSpeedKmh;
					float fTest = fSpeedKmh;
					if (_fdtest(&fTest) > 0)
						fSpeedFactor = 0;

					fVolume = tclamp(fSpeedFactor * 0.05f, 0.0f, 1.0f);
					fPitch = 1.2f;
				}
			}

			if (fPitch <= 0.1f)
				fPitch = 0.1f;
			
			float fTest = fPitch;
			if (_fdtest(&fTest) <= 0)
			{
				float fAlpha = skidPitches[i].alpha;
				float fValue = skidPitches[i].value;

				if (fAlpha != 0.0f && (fAlpha * dt) < 1.0f)
					fPitch = (((fPitch - fValue) * fAlpha) * dt) + fValue;

				skidPitches[i].value = fPitch;
			}

			fTest = fVolume;
			if (_fdtest(&fTest) <= 0)
			{
				float fAlpha = skidVolumes[i].alpha;
				float fValue = skidVolumes[i].value;

				if (fAlpha != 0.0f && (fAlpha * dt) < 1.0f)
					fVolume = (((fVolume - fValue) * fAlpha) * dt) + fValue;

				skidVolumes[i].value = fVolume;
			}

			// TODO: this is hack
			fVolumeMax = tmax(fVolumeMax, fVolume);
			fPitchMax = tmax(fPitchMax, fPitch);
		}

		// TODO: implement separate events for each tyre
		auto ev = castEvent(evSkidInt);
		ev->setVolume(fVolumeMax);
		ev->setPitch(fPitchMax);
	}

	if (context)
	{
		context->update();
	}
}

}
