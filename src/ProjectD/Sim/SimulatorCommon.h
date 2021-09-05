#pragma once

#include "Physics/IPhysicsEngine.h"
#include "Core/Speed.h"
#include "Core/INIReader.h"

#define C_CATEGORY_TRACK	0b00000001 // 1
#define C_CATEGORY_WALL		0b00000010 // 2
#define C_CATEGORY_CAR		0b00000100 // 4

#define C_MASK_SURFACE		0b00010100 // 20
#define C_MASK_CAR_BOX		0b00000001 // 1
#define C_MASK_CAR_MESH		0b00011110 // 30

namespace D {

DECL_STRUCT_AND_PTR(Simulator);
DECL_STRUCT_AND_PTR(Track);
DECL_STRUCT_AND_PTR(Surface);
DECL_STRUCT_AND_PTR(Car);

}
