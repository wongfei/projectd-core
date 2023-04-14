#pragma once

namespace D {

struct SharedMemory
{
	SharedMemory();
	~SharedMemory();
	
	void allocate(const wchar_t* name, size_t size);
	void open(const wchar_t* name, size_t size);
	void close();
	
	inline bool isValid() const { return dataPtr ? true : false; }
	inline void* data() { return dataPtr; }

	void* mappingHandle = nullptr;
	void* dataPtr = nullptr;
};
	
}
