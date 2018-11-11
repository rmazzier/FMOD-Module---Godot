//register_types.cpp

#include "register_types.h"
#include "core/class_db.h"
#include "fmod.h"

void register_fmod_types() {
	ClassDB::register_class<FMod>();
}

void unregister_fmod_types() {
	//nothing to do here
}
