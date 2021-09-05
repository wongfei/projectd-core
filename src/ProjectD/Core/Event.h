#pragma once

#include "Core/Core.h"
#include <functional>
#include <vector>

namespace D {

template<typename T>
struct Event
{
	typedef std::function<void (const T&)> Callback;
	typedef std::pair<void*, Callback> Entry;
	typedef std::vector<Entry> Vector;

	inline void add(void* key, Callback value)
	{
		handlers.push_back({key, value});
	}

	inline void fire(const T& arg)
	{
		for (auto& h : handlers)
		{
			if (h.second)
				(h.second)(arg);
		}
	}

	Vector handlers;
};

struct IEventTrigger
{
	virtual void update(float) = 0;
};

struct OnValueChanged
{
	float currentValue;
	float oldValue;
};

template<typename T>
struct EventTriggerOnChange : IEventTrigger, Event<OnValueChanged>
{
	virtual void update(float) {}

	T* attached;
	T oldValue;
};

}
