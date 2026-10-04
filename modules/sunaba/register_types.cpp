/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/

#include "register_types.h"

#include "input_event_ref.h"
#include "lua_runtime.h"
#include "object_type_db.h"
#include "script_object.h"

void register_sunaba_types() {

	ObjectTypeDB::register_type<Runtime>();
	ObjectTypeDB::register_type<ScriptObject>();
	ObjectTypeDB::register_type<ScriptFunction>();
	ObjectTypeDB::register_type<DisposableObject>();
	ObjectTypeDB::register_type<RefObject>();
	ObjectTypeDB::register_type<InputEventRef>();
}

void unregister_sunaba_types() {
}
