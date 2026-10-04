/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/

#include "register_types.h"

#include "godot4_classes.h"
#include "godot4_compat.h"
#include "input_event_ref.h"
#include "lua_runtime.h"
#include "object_type_db.h"
#include "primitive_meshes.h"
#include "script_object.h"
#include "zip_reader.h"

void register_sunaba_types() {

	ObjectTypeDB::register_type<Runtime>();
	ObjectTypeDB::register_type<ScriptObject>();
	ObjectTypeDB::register_type<ScriptFunction>();
	ObjectTypeDB::register_type<DisposableObject>();
	ObjectTypeDB::register_type<RefObject>();
	ObjectTypeDB::register_type<InputEventRef>();
	ObjectTypeDB::register_type<ZipReader>();

	// Godot 4 classes missing from Godot 2.
	ObjectTypeDB::register_virtual_type<PrimitiveMesh>();
	ObjectTypeDB::register_type<BoxMesh>();
	ObjectTypeDB::register_type<SphereMesh>();
	ObjectTypeDB::register_type<CapsuleMesh>();
	ObjectTypeDB::register_type<CylinderMesh>();
	ObjectTypeDB::register_type<PlaneMesh>();
	ObjectTypeDB::register_type<QuadMesh>();
	ObjectTypeDB::register_type<PhysicsMaterial>();
	ObjectTypeDB::register_type<CylinderShape>();
	ObjectTypeDB::register_type<KinematicCollision3D>();
	ObjectTypeDB::register_type<RemoteTransform3D>();
	ObjectTypeDB::register_type<SpringArm3D>();
}

void unregister_sunaba_types() {

	sunaba::compat::cleanup();
}
