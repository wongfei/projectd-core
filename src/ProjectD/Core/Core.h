#pragma once

#include <cstdint>
#include <memory>

#define STRINGIZE_W2(x) L ## x
#define STRINGIZE_W(x) STRINGIZE_W2(#x)

#define DECL_STRUCT_AND_PTR(name)\
	struct name;\
	using name##Ptr = std::shared_ptr<name>

#define DECL_SHARED_PTR(name)\
	using name##Ptr = std::shared_ptr<name>

namespace D {

struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(NonCopyable&&) = delete;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(NonCopyable&&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

struct IObject : public NonCopyable
{
	virtual ~IObject() {}
};

struct IAvatar : public virtual IObject
{
	virtual void draw() {}
};

template<typename T>
inline void memzero(T& obj) { memset(&obj, 0, sizeof(T)); }

template<typename C, typename V>
inline void eraseRemove(C& container, const V& value) { container.erase(std::remove(container.begin(), container.end(), value), container.end()); }

}
