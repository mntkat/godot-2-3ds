/*************************************************************************/
/*  godot4_classes.cpp                                                   */
/*************************************************************************/

#include "godot4_classes.h"

#include "godot4_compat.h"

#include "scene/3d/physics_body.h"
#include "scene/resources/world.h"
#include "servers/physics_server.h"

/* PhysicsMaterial */

void PhysicsMaterial::set_friction(float p_friction) {
	friction = p_friction;
}
float PhysicsMaterial::get_friction() const {
	return friction;
}
void PhysicsMaterial::set_bounce(float p_bounce) {
	bounce = p_bounce;
}
float PhysicsMaterial::get_bounce() const {
	return bounce;
}
void PhysicsMaterial::set_rough(bool p_rough) {
	rough = p_rough;
}
bool PhysicsMaterial::is_rough() const {
	return rough;
}
void PhysicsMaterial::set_absorbent(bool p_absorbent) {
	absorbent = p_absorbent;
}
bool PhysicsMaterial::is_absorbent() const {
	return absorbent;
}

void PhysicsMaterial::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("set_friction", "friction"), &PhysicsMaterial::set_friction);
	ObjectTypeDB::bind_method(_MD("get_friction"), &PhysicsMaterial::get_friction);
	ObjectTypeDB::bind_method(_MD("set_bounce", "bounce"), &PhysicsMaterial::set_bounce);
	ObjectTypeDB::bind_method(_MD("get_bounce"), &PhysicsMaterial::get_bounce);
	ObjectTypeDB::bind_method(_MD("set_rough", "rough"), &PhysicsMaterial::set_rough);
	ObjectTypeDB::bind_method(_MD("is_rough"), &PhysicsMaterial::is_rough);
	ObjectTypeDB::bind_method(_MD("set_absorbent", "absorbent"), &PhysicsMaterial::set_absorbent);
	ObjectTypeDB::bind_method(_MD("is_absorbent"), &PhysicsMaterial::is_absorbent);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "friction"), _SCS("set_friction"), _SCS("get_friction"));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "rough"), _SCS("set_rough"), _SCS("is_rough"));
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "bounce"), _SCS("set_bounce"), _SCS("get_bounce"));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "absorbent"), _SCS("set_absorbent"), _SCS("is_absorbent"));
}

PhysicsMaterial::PhysicsMaterial() {

	friction = 1.0;
	bounce = 0.0;
	rough = false;
	absorbent = false;
}

/* CylinderShape */

void CylinderShape::_update() {

	const int segments = 24;
	DVector<Vector3> points;
	points.resize(segments * 2);
	DVector<Vector3>::Write w = points.write();
	for (int i = 0; i < segments; i++) {
		float a = Math_PI * 2.0 * i / segments;
		Vector3 p(Math::sin(a) * radius, 0, Math::cos(a) * radius);
		w[i * 2] = p + Vector3(0, height * 0.5, 0);
		w[i * 2 + 1] = p - Vector3(0, height * 0.5, 0);
	}
	w = DVector<Vector3>::Write();
	set_points(points);
}

void CylinderShape::set_radius(float p_radius) {
	radius = p_radius;
	_update();
}
float CylinderShape::get_radius() const {
	return radius;
}
void CylinderShape::set_height(float p_height) {
	height = p_height;
	_update();
}
float CylinderShape::get_height() const {
	return height;
}

void CylinderShape::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("set_radius", "radius"), &CylinderShape::set_radius);
	ObjectTypeDB::bind_method(_MD("get_radius"), &CylinderShape::get_radius);
	ObjectTypeDB::bind_method(_MD("set_height", "height"), &CylinderShape::set_height);
	ObjectTypeDB::bind_method(_MD("get_height"), &CylinderShape::get_height);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "radius"), _SCS("set_radius"), _SCS("get_radius"));
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "height"), _SCS("set_height"), _SCS("get_height"));
}

CylinderShape::CylinderShape() {

	radius = 0.5;
	height = 2.0;
	_update();
}

/* KinematicCollision3D */

float KinematicCollision3D::get_angle(int p_index, const Vector3 &p_up) const {

	return Math::acos(CLAMP(normal.dot(p_up.normalized()), -1.0, 1.0));
}

Object *KinematicCollision3D::get_collider(int p_index) const {

	return collider_id ? ObjectDB::get_instance(collider_id) : NULL;
}

Object *KinematicCollision3D::get_collider_shape(int p_index) const {

	return collider_shape_id ? ObjectDB::get_instance(collider_shape_id) : NULL;
}

void KinematicCollision3D::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("get_position", "collision_index"), &KinematicCollision3D::get_position, DEFVAL(0));
	ObjectTypeDB::bind_method(_MD("get_normal", "collision_index"), &KinematicCollision3D::get_normal, DEFVAL(0));
	ObjectTypeDB::bind_method(_MD("get_travel"), &KinematicCollision3D::get_travel);
	ObjectTypeDB::bind_method(_MD("get_remainder"), &KinematicCollision3D::get_remainder);
	ObjectTypeDB::bind_method(_MD("get_angle", "collision_index", "up_direction"), &KinematicCollision3D::get_angle, DEFVAL(0), DEFVAL(Vector3(0, 1, 0)));
	ObjectTypeDB::bind_method(_MD("get_collider", "collision_index"), &KinematicCollision3D::get_collider, DEFVAL(0));
	ObjectTypeDB::bind_method(_MD("get_collider_id", "collision_index"), &KinematicCollision3D::get_collider_id, DEFVAL(0));
	ObjectTypeDB::bind_method(_MD("get_collider_shape", "collision_index"), &KinematicCollision3D::get_collider_shape, DEFVAL(0));
	ObjectTypeDB::bind_method(_MD("get_collider_shape_index", "collision_index"), &KinematicCollision3D::get_collider_shape_index, DEFVAL(0));
	ObjectTypeDB::bind_method(_MD("get_collider_velocity", "collision_index"), &KinematicCollision3D::get_collider_velocity, DEFVAL(0));
	ObjectTypeDB::bind_method(_MD("get_collision_count"), &KinematicCollision3D::get_collision_count);
	ObjectTypeDB::bind_method(_MD("get_depth"), &KinematicCollision3D::get_depth);
}

KinematicCollision3D::KinematicCollision3D() {

	collider_id = 0;
	collider_shape_id = 0;
	collider_shape_index = 0;
}

/* RemoteTransform3D */

void RemoteTransform3D::_update_remote() {

	if (!is_inside_tree() || remote_path.is_empty() || !has_node(remote_path))
		return;
	Spatial *target = get_node(remote_path)->cast_to<Spatial>();
	if (!target || target == this)
		return;

	Transform src = use_global_coordinates ? get_global_transform() : get_transform();
	Transform dst = use_global_coordinates ? target->get_global_transform() : target->get_transform();
	if (update_position && update_rotation && update_scale) {
		dst = src;
	} else {
		Vector3 scale = update_scale ? src.basis.get_scale() : dst.basis.get_scale();
		if (update_rotation)
			dst.basis = src.basis.orthonormalized();
		else
			dst.basis.orthonormalize();
		dst.basis.scale(scale);
		if (update_position)
			dst.origin = src.origin;
	}
	if (use_global_coordinates)
		target->set_global_transform(dst);
	else
		target->set_transform(dst);
}

void RemoteTransform3D::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_READY:
			set_process(true);
			break;
		case NOTIFICATION_PROCESS:
			_update_remote();
			break;
	}
}

void RemoteTransform3D::set_remote_node(const NodePath &p_path) {
	remote_path = p_path;
	_update_remote();
}
NodePath RemoteTransform3D::get_remote_node() const {
	return remote_path;
}
void RemoteTransform3D::set_use_global_coordinates(bool p_enable) {
	use_global_coordinates = p_enable;
}
bool RemoteTransform3D::get_use_global_coordinates() const {
	return use_global_coordinates;
}
void RemoteTransform3D::set_update_position(bool p_enable) {
	update_position = p_enable;
}
bool RemoteTransform3D::get_update_position() const {
	return update_position;
}
void RemoteTransform3D::set_update_rotation(bool p_enable) {
	update_rotation = p_enable;
}
bool RemoteTransform3D::get_update_rotation() const {
	return update_rotation;
}
void RemoteTransform3D::set_update_scale(bool p_enable) {
	update_scale = p_enable;
}
bool RemoteTransform3D::get_update_scale() const {
	return update_scale;
}

void RemoteTransform3D::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("set_remote_node", "path"), &RemoteTransform3D::set_remote_node);
	ObjectTypeDB::bind_method(_MD("get_remote_node"), &RemoteTransform3D::get_remote_node);
	ObjectTypeDB::bind_method(_MD("set_use_global_coordinates", "use_global_coordinates"), &RemoteTransform3D::set_use_global_coordinates);
	ObjectTypeDB::bind_method(_MD("get_use_global_coordinates"), &RemoteTransform3D::get_use_global_coordinates);
	ObjectTypeDB::bind_method(_MD("set_update_position", "update_remote_position"), &RemoteTransform3D::set_update_position);
	ObjectTypeDB::bind_method(_MD("get_update_position"), &RemoteTransform3D::get_update_position);
	ObjectTypeDB::bind_method(_MD("set_update_rotation", "update_remote_rotation"), &RemoteTransform3D::set_update_rotation);
	ObjectTypeDB::bind_method(_MD("get_update_rotation"), &RemoteTransform3D::get_update_rotation);
	ObjectTypeDB::bind_method(_MD("set_update_scale", "update_remote_scale"), &RemoteTransform3D::set_update_scale);
	ObjectTypeDB::bind_method(_MD("get_update_scale"), &RemoteTransform3D::get_update_scale);
	ObjectTypeDB::bind_method(_MD("force_update_cache"), &RemoteTransform3D::force_update_cache);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "remote_path"), _SCS("set_remote_node"), _SCS("get_remote_node"));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_global_coordinates"), _SCS("set_use_global_coordinates"), _SCS("get_use_global_coordinates"));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "update_position"), _SCS("set_update_position"), _SCS("get_update_position"));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "update_rotation"), _SCS("set_update_rotation"), _SCS("get_update_rotation"));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "update_scale"), _SCS("set_update_scale"), _SCS("get_update_scale"));
}

RemoteTransform3D::RemoteTransform3D() {

	use_global_coordinates = true;
	update_position = update_rotation = update_scale = true;
}

/* SpringArm3D */

void SpringArm3D::_process() {

	if (!is_inside_tree())
		return;
	Ref<World> world = get_world();
	PhysicsDirectSpaceState *space = world.is_valid() ? world->get_direct_space_state() : NULL;

	current_length = spring_length;
	if (space) {
		Transform gt = get_global_transform();
		Vector3 dir = gt.basis.get_axis(2).normalized();
		Vector3 to = gt.origin + dir * spring_length;
		Set<RID> exclude = excluded;
		CollisionObject *parent = get_parent() ? get_parent()->cast_to<CollisionObject>() : NULL;
		if (parent)
			exclude.insert(parent->get_rid());
		PhysicsDirectSpaceState::RayResult hit;
		if (space->intersect_ray(gt.origin, to, hit, exclude, collision_mask))
			current_length = MAX(gt.origin.distance_to(hit.position) - margin, 0);
	}

	for (int i = 0; i < get_child_count(); i++) {
		Spatial *child = get_child(i)->cast_to<Spatial>();
		if (!child)
			continue;
		Transform t = child->get_transform();
		t.origin = Vector3(0, 0, current_length);
		child->set_transform(t);
	}
}

void SpringArm3D::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_READY:
			set_fixed_process(true);
			break;
		case NOTIFICATION_FIXED_PROCESS:
			_process();
			break;
	}
}

void SpringArm3D::set_length(float p_length) {
	spring_length = p_length;
}
float SpringArm3D::get_length() const {
	return spring_length;
}
void SpringArm3D::set_margin(float p_margin) {
	margin = p_margin;
}
float SpringArm3D::get_margin() const {
	return margin;
}
void SpringArm3D::set_collision_mask(int p_mask) {
	collision_mask = p_mask;
}
int SpringArm3D::get_collision_mask() const {
	return collision_mask;
}
float SpringArm3D::get_hit_length() const {
	return current_length;
}
void SpringArm3D::add_excluded_object(const RID &p_rid) {
	excluded.insert(p_rid);
}
bool SpringArm3D::remove_excluded_object(const RID &p_rid) {
	bool had = excluded.has(p_rid);
	excluded.erase(p_rid);
	return had;
}
void SpringArm3D::clear_excluded_objects() {
	excluded.clear();
}

void SpringArm3D::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("set_length", "length"), &SpringArm3D::set_length);
	ObjectTypeDB::bind_method(_MD("get_length"), &SpringArm3D::get_length);
	ObjectTypeDB::bind_method(_MD("set_margin", "margin"), &SpringArm3D::set_margin);
	ObjectTypeDB::bind_method(_MD("get_margin"), &SpringArm3D::get_margin);
	ObjectTypeDB::bind_method(_MD("set_collision_mask", "mask"), &SpringArm3D::set_collision_mask);
	ObjectTypeDB::bind_method(_MD("get_collision_mask"), &SpringArm3D::get_collision_mask);
	ObjectTypeDB::bind_method(_MD("get_hit_length"), &SpringArm3D::get_hit_length);
	ObjectTypeDB::bind_method(_MD("add_excluded_object", "rid"), &SpringArm3D::add_excluded_object);
	ObjectTypeDB::bind_method(_MD("remove_excluded_object", "rid"), &SpringArm3D::remove_excluded_object);
	ObjectTypeDB::bind_method(_MD("clear_excluded_objects"), &SpringArm3D::clear_excluded_objects);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "spring_length"), _SCS("set_length"), _SCS("get_length"));
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "margin"), _SCS("set_margin"), _SCS("get_margin"));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_mask"), _SCS("set_collision_mask"), _SCS("get_collision_mask"));
}

SpringArm3D::SpringArm3D() {

	spring_length = 1.0;
	margin = 0.01;
	current_length = 0;
	collision_mask = 1;
}

/* Godot4Compat */

Dictionary Godot4Compat::get_manifest() const {

	return sunaba::compat::manifest();
}

void Godot4Compat::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("get_manifest"), &Godot4Compat::get_manifest);
}
