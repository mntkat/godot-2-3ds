/*************************************************************************/
/*  godot4_classes.h                                                     */
/*************************************************************************/
/*  Godot 4 classes with no Godot 2 counterpart that libsunaba's         */
/*  components rely on, implemented on top of Godot 2.                   */
/*************************************************************************/

#ifndef SUNABA_GODOT4_CLASSES_H
#define SUNABA_GODOT4_CLASSES_H

#include "scene/3d/spatial.h"
#include "scene/resources/convex_polygon_shape.h"

// Physics material; applied as friction/bounce on Godot 2 bodies.
class PhysicsMaterial : public Resource {
	OBJ_TYPE(PhysicsMaterial, Resource);

	float friction;
	float bounce;
	bool rough;
	bool absorbent;

protected:
	static void _bind_methods();

public:
	void set_friction(float p_friction);
	float get_friction() const;
	void set_bounce(float p_bounce);
	float get_bounce() const;
	void set_rough(bool p_rough);
	bool is_rough() const;
	void set_absorbent(bool p_absorbent);
	bool is_absorbent() const;

	PhysicsMaterial();
};

// Godot 2 has no cylinder shape: a convex hull rebuilt from radius/height.
class CylinderShape : public ConvexPolygonShape {
	OBJ_TYPE(CylinderShape, ConvexPolygonShape);

	float radius;
	float height;

	void _update();

protected:
	static void _bind_methods();

public:
	void set_radius(float p_radius);
	float get_radius() const;
	void set_height(float p_height);
	float get_height() const;

	CylinderShape();
};

// Result of move_and_collide / slide collisions (Godot 4 API).
class KinematicCollision3D : public Reference {
	OBJ_TYPE(KinematicCollision3D, Reference);

protected:
	static void _bind_methods();

public:
	Vector3 position;
	Vector3 normal;
	Vector3 travel;
	Vector3 remainder;
	Vector3 collider_velocity;
	ObjectID collider_id;
	ObjectID collider_shape_id;
	int collider_shape_index;

	Vector3 get_position(int p_index = 0) const { return position; }
	Vector3 get_normal(int p_index = 0) const { return normal; }
	Vector3 get_travel() const { return travel; }
	Vector3 get_remainder() const { return remainder; }
	float get_angle(int p_index = 0, const Vector3 &p_up = Vector3(0, 1, 0)) const;
	Object *get_collider(int p_index = 0) const;
	int get_collider_id(int p_index = 0) const { return collider_id; }
	Object *get_collider_shape(int p_index = 0) const;
	int get_collider_shape_index(int p_index = 0) const { return collider_shape_index; }
	Vector3 get_collider_velocity(int p_index = 0) const { return collider_velocity; }
	int get_collision_count() const { return 1; }
	float get_depth() const { return 0; }

	KinematicCollision3D();
};

// Copies its global transform to another node every frame.
class RemoteTransform3D : public Spatial {
	OBJ_TYPE(RemoteTransform3D, Spatial);

	NodePath remote_path;
	bool use_global_coordinates;
	bool update_position, update_rotation, update_scale;

	void _update_remote();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_remote_node(const NodePath &p_path);
	NodePath get_remote_node() const;
	void set_use_global_coordinates(bool p_enable);
	bool get_use_global_coordinates() const;
	void set_update_position(bool p_enable);
	bool get_update_position() const;
	void set_update_rotation(bool p_enable);
	bool get_update_rotation() const;
	void set_update_scale(bool p_enable);
	bool get_update_scale() const;
	void force_update_cache() {}

	RemoteTransform3D();
};

// Casts a ray along its +Z axis and moves its children to the hit point
// (Godot 4 casts a shape; Godot 2 offers rays only).
class SpringArm3D : public Spatial {
	OBJ_TYPE(SpringArm3D, Spatial);

	float spring_length;
	float margin;
	float current_length;
	uint32_t collision_mask;
	Set<RID> excluded;

	void _process();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_length(float p_length);
	float get_length() const;
	void set_margin(float p_margin);
	float get_margin() const;
	void set_collision_mask(int p_mask);
	int get_collision_mask() const;
	float get_hit_length() const;
	void add_excluded_object(const RID &p_rid);
	bool remove_excluded_object(const RID &p_rid);
	void clear_excluded_objects();

	SpringArm3D();
};

// Exposes the compatibility layer's manifest (see godot4_compat.h) so tools
// can dump it: Godot4Compat.new().get_manifest().
class Godot4Compat : public Reference {
	OBJ_TYPE(Godot4Compat, Reference);

protected:
	static void _bind_methods();

public:
	Dictionary get_manifest() const;
};

#endif // SUNABA_GODOT4_CLASSES_H
