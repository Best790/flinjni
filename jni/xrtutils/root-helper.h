#ifndef __HEADER_ROOT_HELPER__
#define __HEADER_ROOT_HELPER__

// #include "idadefs.h"
// #define USE_SANTROPE_BULLSHIT

#define WORLD_MAX_X_POSITIVE 20000
#define WORLD_MAX_X_NEGATIVE -20000
#define WORLD_MAX_Y_POSITIVE 20000
#define WORLD_MAX_Y_NEGATIVE -20000
#define WORLD_MAX_Z_POSITIVE 20000
#define WORLD_MAX_Z_NEGATIVE -20000

#define WORLD_MAX_SPEED_X_POSITIVE 20000
#define WORLD_MAX_SPEED_X_NEGATIVE -20000
#define WORLD_MAX_SPEED_Y_POSITIVE 20000
#define WORLD_MAX_SPEED_Y_NEGATIVE -20000
#define WORLD_MAX_SPEED_Z_POSITIVE 20000
#define WORLD_MAX_SPEED_Z_NEGATIVE -20000

#define MAX_SKIN_MODELS 311
#define VEHICLE_MODELS_END_AT 611
#define VEHICLE_MODELS_BEGIN_AT 400
#define INVALID_SKIN_MODEL_ID 74

#define WORLD_BOUNDS_VALIDATE(x, y, z) ((x < WORLD_MAX_X_POSITIVE && x > WORLD_MAX_X_NEGATIVE) && (y < WORLD_MAX_Y_POSITIVE && y > WORLD_MAX_Y_NEGATIVE) && (z < WORLD_MAX_Z_POSITIVE && z > WORLD_MAX_Z_NEGATIVE))
#define WORLD_SPEED_VALIDATE(x, y, z) ((x < WORLD_MAX_SPEED_X_POSITIVE && x > WORLD_MAX_SPEED_X_NEGATIVE) && (y < WORLD_MAX_SPEED_Y_POSITIVE && y > WORLD_MAX_SPEED_Y_NEGATIVE) && (z < WORLD_MAX_SPEED_Z_POSITIVE && z > WORLD_MAX_SPEED_Z_NEGATIVE))
#define ANGLE_VALIDATE(f) (f >= 0.0f && f <= 360.0f)
#define DEST_VALIDATE(i, m) (i >= 0 && i < m)
#define STR_VALIDATE(str) (str && strlen(str))
#define SKIN_MODEL_VALIDATE(id) (id >= 0 && id <= MAX_SKIN_MODELS && id != INVALID_SKIN_MODEL_ID) ? true : false
#define VEHICLE_MODEL_VALIDATE(id) (id >= VEHICLE_MODELS_BEGIN_AT && id <= VEHICLE_MODELS_END_AT) ? true : false

#define ARRAY_SIZE(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#define ARRAY_VALIDATE(_Array, i)  (i >= 0 && i < ARRAY_SIZE(_Array))
#define ZERO_MEMORY_L(Destination, Length) memset((Destination),0,(Length))
#define ZERO_MEMORY(Destination) ZERO_MEMORY_L(Destination, ARRAY_SIZE(Destination))

#define SAFE_DELETE(p) { if (p) { delete (p); (p) = nullptr; } }
#define SAFE_RELEASE(p)	{ if (p) { (p)->Release(); (p) = nullptr; } }

void SAMP_DUMP(const char*, const char*, ...);

#endif
