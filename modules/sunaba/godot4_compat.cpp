/*************************************************************************/
/*  godot4_compat.cpp                                                    */
/*************************************************************************/

#include "godot4_compat.h"

#include "code_highlighter_ref.h"
#include "godot4_classes.h"
#include "input_event_ref.h"
#include "hash_map.h"
#include "math_funcs.h"
#include "object_type_db.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/control.h"
#include "scene/main/scene_main_loop.h"
#include "scene/main/viewport.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/text_edit.h"
#include "scene/3d/body_shape.h"
#include "scene/3d/physics_body.h"
#include "scene/resources/capsule_shape.h"
#include "scene/resources/mesh.h"

namespace sunaba {
namespace compat {

/* Class names */

static const char *class_aliases[][2] = {
	{ "Node3D", "Spatial" },
	{ "VisualInstance3D", "VisualInstance" },
	{ "GeometryInstance3D", "GeometryInstance" },
	{ "MeshInstance3D", "MeshInstance" },
	{ "MultiMeshInstance3D", "MultiMeshInstance" },
	{ "Camera3D", "Camera" },
	{ "Light3D", "Light" },
	{ "DirectionalLight3D", "DirectionalLight" },
	{ "OmniLight3D", "OmniLight" },
	{ "SpotLight3D", "SpotLight" },
	{ "CollisionObject3D", "CollisionObject" },
	{ "PhysicsBody3D", "PhysicsBody" },
	{ "StaticBody3D", "StaticBody" },
	{ "RigidBody3D", "RigidBody" },
	{ "CharacterBody3D", "KinematicBody" },
	{ "AnimatableBody3D", "KinematicBody" },
	{ "Area3D", "Area" },
	{ "CollisionShape3D", "CollisionShape" },
	{ "CollisionPolygon3D", "CollisionPolygon" },
	{ "Shape3D", "Shape" },
	{ "BoxShape3D", "BoxShape" },
	{ "SphereShape3D", "SphereShape" },
	{ "CapsuleShape3D", "CapsuleShape" },
	{ "CylinderShape3D", "CylinderShape" },
	{ "ConvexPolygonShape3D", "ConvexPolygonShape" },
	{ "ConcavePolygonShape3D", "ConcavePolygonShape" },
	{ "WorldBoundaryShape3D", "PlaneShape" },
	{ "SeparationRayShape3D", "RayShape" },
	{ "RayCast3D", "RayCast" },
	{ "Skeleton3D", "Skeleton" },
	{ "BoneAttachment3D", "BoneAttachment" },
	{ "Path3D", "Path" },
	{ "PathFollow3D", "PathFollow" },
	{ "Marker3D", "Position3D" },
	{ "VisibleOnScreenNotifier3D", "VisibilityNotifier" },
	{ "VisibleOnScreenEnabler3D", "VisibilityEnabler" },
	{ "VehicleBody3D", "VehicleBody" },
	{ "VehicleWheel3D", "VehicleWheel" },
	{ "Joint3D", "Joint" },
	{ "PinJoint3D", "PinJoint" },
	{ "HingeJoint3D", "HingeJoint" },
	{ "SliderJoint3D", "SliderJoint" },
	{ "ConeTwistJoint3D", "ConeTwistJoint" },
	{ "Generic6DOFJoint3D", "Generic6DOFJoint" },
	{ "NavigationRegion3D", "NavigationMeshInstance" },
	{ "GPUParticles3D", "Particles" },
	{ "CPUParticles3D", "Particles" },
	{ "AudioListener3D", "Listener" },
	{ "AudioStreamPlayer3D", "SpatialStreamPlayer" },
	{ "World3D", "World" },
	{ "ArrayMesh", "Mesh" },
	{ "StandardMaterial3D", "FixedMaterial" },
	{ "ORMMaterial3D", "FixedMaterial" },
	{ "BaseMaterial3D", "FixedMaterial" },
	{ "Texture2D", "Texture" },
	{ "Sprite2D", "Sprite" },
	{ "AnimatedSprite2D", "AnimatedSprite" },
	{ "CharacterBody2D", "KinematicBody2D" },
	{ "Marker2D", "Position2D" },
	{ "PointLight2D", "Light2D" },
	{ "VisibleOnScreenNotifier2D", "VisibilityNotifier2D" },
	{ "VisibleOnScreenEnabler2D", "VisibilityEnabler2D" },
	{ "NavigationRegion2D", "NavigationPolygonInstance" },
	{ "GPUParticles2D", "Particles2D" },
	{ "CPUParticles2D", "Particles2D" },
	{ "WorldBoundaryShape2D", "LineShape2D" },
	{ "SeparationRayShape2D", "RayShape2D" },
	{ "AudioStreamPlayer", "StreamPlayer" },
	{ "ColorRect", "ColorFrame" },
	{ "TextureRect", "TextureFrame" },
	{ "NinePatchRect", "Patch9Frame" },
	{ "TextureProgressBar", "TextureProgress" },
	{ "TabBar", "Tabs" },
	{ "ReferenceRect", "ReferenceFrame" },
	{ "SubViewport", "Viewport" },
	{ "Window", "WindowDialog" },
	{ "RefCounted", "Reference" },
	{ "AnimationTree", "AnimationTreePlayer" },
	{ "VideoStreamPlayer", "VideoPlayer" },
	{ "Gradient", "ColorRamp" },
	{ "Shortcut", "ShortCut" },
	{ "Cubemap", "CubeMap" },
	{ "FontFile", "DynamicFontData" },
	{ "ViewportTexture", "RenderTargetTexture" },
	{ "Image", "ImageRef" },
	{ "CodeHighlighter", "CodeHighlighterRef" },
	{ "SyntaxHighlighter", "CodeHighlighterRef" },
	// Last, so a plain TextEdit still reports itself as TextEdit.
	{ "CodeEdit", "TextEdit" },
	{ NULL, NULL }
};

String to_godot2_class(const String &p_class) {

	for (int i = 0; class_aliases[i][0]; i++) {
		if (p_class == class_aliases[i][0])
			return class_aliases[i][1];
	}
	return p_class;
}

String godot4_class(const Object *p_object) {

	String type = p_object->get_type();
	for (int i = 0; class_aliases[i][0]; i++) {
		if (type == class_aliases[i][1])
			return class_aliases[i][0]; // first alias wins (CharacterBody3D, ...)
	}
	return type;
}

bool is_class(const Object *p_object, const String &p_godot4_class) {

	return p_object->is_type(to_godot2_class(p_godot4_class));
}

/* Helpers */

static Variant call0(Object *o, const char *m) {

	Variant::CallError ce;
	return o->call(m, NULL, 0, ce);
}

static Variant call1(Object *o, const char *m, const Variant &a) {

	const Variant *args[1] = { &a };
	Variant::CallError ce;
	return o->call(m, args, 1, ce);
}

static Variant call2(Object *o, const char *m, const Variant &a, const Variant &b) {

	const Variant *args[2] = { &a, &b };
	Variant::CallError ce;
	return o->call(m, args, 2, ce);
}

static Variant call3(Object *o, const char *m, const Variant &a, const Variant &b, const Variant &c) {

	const Variant *args[3] = { &a, &b, &c };
	Variant::CallError ce;
	return o->call(m, args, 3, ce);
}

static Variant arg(const Array &p_args, int i, const Variant &p_default = Variant()) {

	return i < p_args.size() ? p_args[i] : p_default;
}

static const float DEG = 180.0 / Math_PI;

// Godot 2's Variant -> container/Transform constructors are ambiguous.
static Array to_array(const Variant &v) {
	return v.operator Array();
}
static Dictionary to_dict(const Variant &v) {
	return v.operator Dictionary();
}
static Transform to_xform(const Variant &v) {
	return v.operator Transform();
}

/* Godot 4 basis conventions: YXZ Euler order, basis = rotation * scale */

static Vector3 euler_yxz(const Matrix3 &b) {

	Vector3 e;
	real_t m12 = b.elements[1][2];
	if (m12 < (1 - CMP_EPSILON)) {
		if (m12 > -(1 - CMP_EPSILON)) {
			if (b.elements[1][0] == 0 && b.elements[0][1] == 0 && b.elements[0][2] == 0 && b.elements[2][0] == 0 && b.elements[0][0] == 1) {
				e.x = Math::atan2(-m12, b.elements[1][1]);
			} else {
				e.x = Math::asin(-m12);
				e.y = Math::atan2(b.elements[0][2], b.elements[2][2]);
				e.z = Math::atan2(b.elements[1][0], b.elements[1][1]);
			}
		} else {
			e.x = Math_PI * 0.5;
			e.y = Math::atan2(b.elements[0][1], b.elements[0][0]);
		}
	} else {
		e.x = -Math_PI * 0.5;
		e.y = -Math::atan2(b.elements[0][1], b.elements[0][0]);
	}
	return e;
}

static Matrix3 from_euler_yxz(const Vector3 &e) {

	float c, s;
	c = Math::cos(e.x);
	s = Math::sin(e.x);
	Matrix3 xm(1, 0, 0, 0, c, -s, 0, s, c);
	c = Math::cos(e.y);
	s = Math::sin(e.y);
	Matrix3 ym(c, 0, s, 0, 1, 0, -s, 0, c);
	c = Math::cos(e.z);
	s = Math::sin(e.z);
	Matrix3 zm(c, -s, 0, s, c, 0, 0, 0, 1);
	return ym * xm * zm;
}

static Matrix3 scaled_local(const Matrix3 &b, const Vector3 &s) {

	Matrix3 m = b;
	for (int i = 0; i < 3; i++)
		m.set_axis(i, m.get_axis(i) * s[i]);
	return m;
}

static Matrix3 with_rotation(const Matrix3 &b, const Matrix3 &rotation) {

	return scaled_local(rotation, b.get_scale());
}

/* Spatial transforms (with the capsule fix-up for collision shapes) */

// Godot 2 capsules lie along Z; Godot 4's stand along Y. A CollisionShape
// holding a capsule is rotated by -90 degrees around X so it matches; its
// Godot 4 transform excludes that rotation.
static const Matrix3 &capsule_fix() {

	static Matrix3 m(Vector3(1, 0, 0), -Math_PI * 0.5);
	return m;
}

static bool has_capsule_fix(Object *o) {

	CollisionShape *cs = o->cast_to<CollisionShape>();
	if (!cs)
		return false;
	Ref<Shape> shape = cs->get_shape();
	return shape.is_valid() && shape->cast_to<CapsuleShape>();
}

static Transform local_transform(Object *o) {

	Transform t = call0(o, "get_transform");
	if (has_capsule_fix(o))
		t.basis = t.basis * capsule_fix().inverse();
	return t;
}

static void set_local_transform(Object *o, Transform t) {

	if (has_capsule_fix(o))
		t.basis = t.basis * capsule_fix();
	call1(o, "set_transform", t);
}

static bool outside_tree(Object *o) {

	Node *n = o->cast_to<Node>();
	return n && !n->is_inside_tree();
}

// Godot 2 refuses global transforms outside the tree (returning identity);
// Godot 4 treats a parentless node's global transform as its local one.
static Transform global_transform(Object *o) {

	if (outside_tree(o))
		return local_transform(o);
	Transform t = call0(o, "get_global_transform");
	if (has_capsule_fix(o))
		t.basis = t.basis * capsule_fix().inverse();
	return t;
}

static void set_global_transform(Object *o, Transform t) {

	if (outside_tree(o)) {
		set_local_transform(o, t);
		return;
	}
	if (has_capsule_fix(o))
		t.basis = t.basis * capsule_fix();
	call1(o, "set_global_transform", t);
}

// Godot 4 registers CollisionShape children with their body automatically;
// Godot 2 does that only in the editor. Mirrors the engine's private
// CollisionObject::_update_shapes_from_children through public API.
// (Shapes added with shape_owner_add_shape live in the same list, so a body
// should use one mechanism or the other.)
static void sync_collision_shapes(Object *p_body) {

	CollisionObject *co = p_body ? p_body->cast_to<CollisionObject>() : NULL;
	if (!co)
		return;
	co->clear_shapes();
	for (int i = 0; i < co->get_child_count(); i++) {
		Node *child = co->get_child(i);
		if (child->cast_to<CollisionShape>())
			child->call("_add_to_collision_object", co);
	}
}

static void sync_parent_shapes(Object *p_shape_node) {

	Node *n = p_shape_node->cast_to<Node>();
	if (n && n->cast_to<CollisionShape>() && n->get_parent())
		sync_collision_shapes(n->get_parent());
}

#define G4_GET(m_name) static Variant m_name(Object *o)
#define G4_SET(m_name) static void m_name(Object *o, const Variant &v)
#define G4_CALL(m_name) static Variant m_name(Object *o, const Array &args)

G4_GET(spatial_get_transform) { return local_transform(o); }
G4_SET(spatial_set_transform) { set_local_transform(o, v); }
G4_GET(spatial_get_global_transform) { return global_transform(o); }
G4_SET(spatial_set_global_transform) { set_global_transform(o, v); }
G4_GET(spatial_get_basis) { return local_transform(o).basis; }
G4_SET(spatial_set_basis) {
	Transform t = local_transform(o);
	t.basis = v;
	set_local_transform(o, t);
}
G4_GET(spatial_get_global_basis) { return global_transform(o).basis; }
G4_SET(spatial_set_global_basis) {
	Transform t = global_transform(o);
	t.basis = v;
	set_global_transform(o, t);
}
G4_GET(spatial_get_position) { return local_transform(o).origin; }
G4_SET(spatial_set_position) {
	Transform t = local_transform(o);
	t.origin = v;
	set_local_transform(o, t);
}
G4_GET(spatial_get_global_position) { return global_transform(o).origin; }
G4_SET(spatial_set_global_position) {
	Transform t = global_transform(o);
	t.origin = v;
	set_global_transform(o, t);
}
G4_GET(spatial_get_rotation) { return euler_yxz(local_transform(o).basis.orthonormalized()); }
G4_SET(spatial_set_rotation) {
	Transform t = local_transform(o);
	t.basis = with_rotation(t.basis, from_euler_yxz(v));
	set_local_transform(o, t);
}
G4_GET(spatial_get_rotation_degrees) { return Vector3(spatial_get_rotation(o)) * DEG; }
G4_SET(spatial_set_rotation_degrees) { spatial_set_rotation(o, Vector3(v) / DEG); }
G4_GET(spatial_get_global_rotation) { return euler_yxz(global_transform(o).basis.orthonormalized()); }
G4_SET(spatial_set_global_rotation) {
	Transform t = global_transform(o);
	t.basis = with_rotation(t.basis, from_euler_yxz(v));
	set_global_transform(o, t);
}
G4_GET(spatial_get_global_rotation_degrees) { return Vector3(spatial_get_global_rotation(o)) * DEG; }
G4_SET(spatial_set_global_rotation_degrees) { spatial_set_global_rotation(o, Vector3(v) / DEG); }
G4_GET(spatial_get_scale) { return local_transform(o).basis.get_scale(); }
G4_SET(spatial_set_scale) {
	Transform t = local_transform(o);
	t.basis = scaled_local(t.basis.orthonormalized(), v);
	set_local_transform(o, t);
}
G4_GET(spatial_get_quaternion) { return Quat(local_transform(o).basis.orthonormalized()); }
G4_SET(spatial_set_quaternion) {
	Transform t = local_transform(o);
	t.basis = with_rotation(t.basis, Matrix3(Quat(v)));
	set_local_transform(o, t);
}
G4_GET(spatial_get_rotation_order) { return 2; } // EULER_ORDER_YXZ, the only one supported
G4_SET(ignore_set) {}
G4_GET(spatial_get_visible) { return !(bool)call0(o, "is_hidden"); }
G4_SET(spatial_set_visible) { call1(o, "set_hidden", !(bool)v); }

G4_CALL(spatial_look_at) {
	return call2(o, "look_at", arg(args, 0), arg(args, 1, Vector3(0, 1, 0)));
}
G4_CALL(spatial_look_at_from_position) {
	return call3(o, "look_at_from_pos", arg(args, 0), arg(args, 1), arg(args, 2, Vector3(0, 1, 0)));
}
G4_CALL(noop) { return Variant(); }
G4_CALL(return_false) { return false; }
G4_CALL(spatial_global_scale) {
	Transform t = global_transform(o);
	t.basis.scale(arg(args, 0));
	set_global_transform(o, t);
	return Variant();
}
G4_CALL(spatial_rotate_object_local) {
	Transform t = local_transform(o);
	t.basis = t.basis * Matrix3(Vector3(arg(args, 0)).normalized(), (float)arg(args, 1));
	set_local_transform(o, t);
	return Variant();
}
G4_CALL(spatial_scale_object_local) {
	Transform t = local_transform(o);
	t.basis = scaled_local(t.basis, arg(args, 0));
	set_local_transform(o, t);
	return Variant();
}
G4_CALL(spatial_to_global) { return global_transform(o).xform(Vector3(arg(args, 0))); }
G4_CALL(spatial_to_local) { return global_transform(o).affine_inverse().xform(Vector3(arg(args, 0))); }
G4_CALL(spatial_is_visible_in_tree) { return call0(o, "is_visible"); }
G4_CALL(spatial_is_top_level) { return call0(o, "is_set_as_toplevel"); }

/* Node2D: Godot 4 rotation is the negative of Godot 2's */

G4_GET(n2d_get_rotation) { return -(float)call0(o, "get_rot"); }
G4_SET(n2d_set_rotation) { call1(o, "set_rot", -(float)v); }
G4_GET(n2d_get_rotation_degrees) { return -(float)call0(o, "get_rotd"); }
G4_SET(n2d_set_rotation_degrees) { call1(o, "set_rotd", -(float)v); }
G4_GET(n2d_get_global_rotation) { return -(float)call0(o, "get_global_rot"); }
G4_SET(n2d_set_global_rotation) { call1(o, "set_global_rot", -(float)v); }
G4_GET(n2d_get_global_rotation_degrees) { return -(float)call0(o, "get_global_rotd"); }
G4_SET(n2d_set_global_rotation_degrees) { call1(o, "set_global_rotd", -(float)v); }
G4_CALL(n2d_rotate) { return call1(o, "rotate", -(float)arg(args, 0)); }
G4_CALL(n2d_apply_scale) {
	Vector2 s = call0(o, "get_scale");
	Vector2 r = arg(args, 0);
	return call1(o, "set_scale", Vector2(s.x * r.x, s.y * r.y));
}
G4_CALL(n2d_to_global) {
	Matrix32 t = call0(o, "get_global_transform");
	return t.xform(Vector2(arg(args, 0)));
}
G4_CALL(n2d_to_local) {
	Matrix32 t = call0(o, "get_global_transform");
	return t.affine_inverse().xform(Vector2(arg(args, 0)));
}
G4_CALL(n2d_get_angle_to) {
	Matrix32 t = call0(o, "get_global_transform");
	Vector2 p = t.affine_inverse().xform(Vector2(arg(args, 0)));
	return Math::atan2(p.y, p.x);
}
G4_CALL(n2d_look_at) {
	Vector2 pos = call0(o, "get_global_pos");
	Vector2 d = Vector2(arg(args, 0)) - pos;
	n2d_set_global_rotation(o, Math::atan2(d.y, d.x));
	return Variant();
}

/* Sprite */

G4_GET(sprite_get_frame_coords) {
	int frame = call0(o, "get_frame");
	int h = MAX((int)call0(o, "get_hframes"), 1);
	return Vector2(frame % h, frame / h);
}
G4_SET(sprite_set_frame_coords) {
	Vector2 c = v;
	int h = MAX((int)call0(o, "get_hframes"), 1);
	call1(o, "set_frame", (int)c.y * h + (int)c.x);
}
G4_CALL(sprite_get_rect) {
	Ref<Texture> tex = call0(o, "get_texture");
	if (tex.is_null())
		return Rect2();
	Vector2 size = tex->get_size();
	if ((bool)call0(o, "is_region"))
		size = Rect2(call0(o, "get_region_rect")).size;
	size.x /= MAX((int)call0(o, "get_hframes"), 1);
	size.y /= MAX((int)call0(o, "get_vframes"), 1);
	Vector2 ofs = call0(o, "get_offset");
	if ((bool)call0(o, "is_centered"))
		ofs -= size / 2;
	return Rect2(ofs, size);
}

/* Camera2D: Godot 4 zoom is the reciprocal of Godot 2's */

G4_GET(cam2d_get_zoom) {
	Vector2 z = call0(o, "get_zoom");
	return Vector2(z.x != 0 ? 1.0 / z.x : 0, z.y != 0 ? 1.0 / z.y : 0);
}
G4_SET(cam2d_set_zoom) {
	Vector2 z = v;
	call1(o, "set_zoom", Vector2(z.x != 0 ? 1.0 / z.x : 0, z.y != 0 ? 1.0 / z.y : 0));
}
G4_GET(cam2d_get_ignore_rotation) { return !(bool)call0(o, "is_rotating"); }
G4_SET(cam2d_set_ignore_rotation) { call1(o, "set_rotating", !(bool)v); }
#define CAM2D_LIMIT(m_name, m_margin)                                 \
	G4_GET(cam2d_get_##m_name) { return call1(o, "get_limit", m_margin); } \
	G4_SET(cam2d_set_##m_name) { call2(o, "set_limit", m_margin, v); }
CAM2D_LIMIT(limit_left, MARGIN_LEFT)
CAM2D_LIMIT(limit_top, MARGIN_TOP)
CAM2D_LIMIT(limit_right, MARGIN_RIGHT)
CAM2D_LIMIT(limit_bottom, MARGIN_BOTTOM)
#define CAM2D_DRAG(m_name, m_margin)                                        \
	G4_GET(cam2d_get_##m_name) { return call1(o, "get_drag_margin", m_margin); } \
	G4_SET(cam2d_set_##m_name) { call2(o, "set_drag_margin", m_margin, v); }
CAM2D_DRAG(drag_left_margin, MARGIN_LEFT)
CAM2D_DRAG(drag_top_margin, MARGIN_TOP)
CAM2D_DRAG(drag_right_margin, MARGIN_RIGHT)
CAM2D_DRAG(drag_bottom_margin, MARGIN_BOTTOM)

/* Camera */

static bool cam_is_ortho(Object *o) {
	return (int)call0(o, "get_projection") == 1;
}
static float cam_size(Object *o) {
	return o->has_meta("_g4_size") ? (float)o->get_meta("_g4_size") : (float)call0(o, "get_size");
}
static void cam_apply(Object *o, int projection, float fov, float size, float near, float far) {
	if (projection == 1)
		call3(o, "set_orthogonal", size, near, far);
	else
		call3(o, "set_perspective", fov, near, far);
}
G4_SET(cam_set_fov) {
	cam_apply(o, cam_is_ortho(o) ? 1 : 0, v, cam_size(o), call0(o, "get_znear"), call0(o, "get_zfar"));
}
G4_SET(cam_set_near) {
	cam_apply(o, cam_is_ortho(o) ? 1 : 0, call0(o, "get_fov"), cam_size(o), v, call0(o, "get_zfar"));
}
G4_SET(cam_set_far) {
	cam_apply(o, cam_is_ortho(o) ? 1 : 0, call0(o, "get_fov"), cam_size(o), call0(o, "get_znear"), v);
}
G4_GET(cam_get_size) { return cam_size(o); }
G4_SET(cam_set_size) {
	o->set_meta("_g4_size", v);
	if (cam_is_ortho(o))
		cam_apply(o, 1, call0(o, "get_fov"), v, call0(o, "get_znear"), call0(o, "get_zfar"));
}
G4_SET(cam_set_projection) {
	cam_apply(o, (int)v == 1 ? 1 : 0, call0(o, "get_fov"), cam_size(o), call0(o, "get_znear"), call0(o, "get_zfar"));
}
G4_SET(cam_set_current) {
	if ((bool)v)
		call0(o, "make_current");
	else
		call0(o, "clear_current");
}
G4_CALL(cam_get_cull_mask_value) {
	int layer = arg(args, 0);
	return (((int)call0(o, "get_visible_layers") >> (layer - 1)) & 1) != 0;
}
G4_CALL(cam_set_cull_mask_value) {
	int layer = arg(args, 0);
	int mask = call0(o, "get_visible_layers");
	if ((bool)arg(args, 1))
		mask |= 1 << (layer - 1);
	else
		mask &= ~(1 << (layer - 1));
	return call1(o, "set_visible_layers", mask);
}
G4_CALL(cam_set_orthogonal) {
	o->set_meta("_g4_size", arg(args, 0));
	return call3(o, "set_orthogonal", arg(args, 0), arg(args, 1), arg(args, 2));
}

/* Lights */

enum { G2_PARAM_SPOT_ATTENUATION = 0, G2_PARAM_SPOT_ANGLE = 1, G2_PARAM_RADIUS = 2, G2_PARAM_ENERGY = 3, G2_PARAM_ATTENUATION = 4, G2_PARAM_SHADOW_DARKENING = 5, G2_PARAM_SHADOW_Z_OFFSET = 6 };

#define LIGHT_PARAM(m_name, m_param)                                         \
	G4_GET(light_get_##m_name) { return call1(o, "get_parameter", m_param); } \
	G4_SET(light_set_##m_name) { call2(o, "set_parameter", m_param, v); }
LIGHT_PARAM(energy, G2_PARAM_ENERGY)
LIGHT_PARAM(range, G2_PARAM_RADIUS)
LIGHT_PARAM(attenuation, G2_PARAM_ATTENUATION)
LIGHT_PARAM(spot_angle, G2_PARAM_SPOT_ANGLE)
LIGHT_PARAM(spot_attenuation, G2_PARAM_SPOT_ATTENUATION)
LIGHT_PARAM(shadow_bias, G2_PARAM_SHADOW_Z_OFFSET)
LIGHT_PARAM(shadow_opacity, G2_PARAM_SHADOW_DARKENING)

G4_GET(light_get_color) { return call1(o, "get_color", 0); }
G4_SET(light_set_color) { call2(o, "set_color", 0, v); }
G4_GET(light_get_specular) { return Color(call1(o, "get_color", 1)).r; }
G4_SET(light_set_specular) {
	float s = v;
	call2(o, "set_color", 1, Color(s, s, s));
}
G4_GET(light_get_negative) { return (int)call0(o, "get_operator") == 1; }
G4_SET(light_set_negative) { call1(o, "set_operator", (bool)v ? 1 : 0); }
// Godot 4 BAKE_DISABLED/STATIC/DYNAMIC vs Godot 2 DISABLED/INDIRECT/.../FULL
G4_GET(light_get_bake_mode) {
	int m = call0(o, "get_bake_mode");
	return m == 0 ? 0 : (m == 3 ? 1 : 2);
}
G4_SET(light_set_bake_mode) {
	int m = v;
	call1(o, "set_bake_mode", m == 0 ? 0 : (m == 1 ? 3 : 1));
}

// Godot 4 Light3D.Param -> Godot 2 parameter (-1: kept in metadata)
static int light_param_to_godot2(int p) {
	switch (p) {
		case 0: return G2_PARAM_ENERGY;
		case 4: return G2_PARAM_RADIUS;
		case 6: return G2_PARAM_ATTENUATION;
		case 7: return G2_PARAM_SPOT_ANGLE;
		case 8: return G2_PARAM_SPOT_ATTENUATION;
		case 15: return G2_PARAM_SHADOW_Z_OFFSET;
		case 17: return G2_PARAM_SHADOW_DARKENING;
		default: return -1;
	}
}
G4_CALL(light_get_param) {
	int p = arg(args, 0);
	int g2 = light_param_to_godot2(p);
	if (g2 >= 0)
		return call1(o, "get_parameter", g2);
	String key = "_g4_param_" + itos(p);
	return o->has_meta(key) ? o->get_meta(key) : Variant(0.0);
}
G4_CALL(light_set_param) {
	int p = arg(args, 0);
	int g2 = light_param_to_godot2(p);
	if (g2 >= 0)
		return call2(o, "set_parameter", g2, arg(args, 1));
	o->set_meta("_g4_param_" + itos(p), arg(args, 1));
	return Variant();
}
G4_CALL(light_get_correlated_color) { return Color(1, 1, 1); }

// Godot 4 ORTHOGONAL/PARALLEL_2/PARALLEL_4 vs Godot 2 ORTHOGONAL/PERSPECTIVE/2/4
G4_GET(dlight_get_shadow_mode) {
	int m = call0(o, "get_shadow_mode");
	return m == 2 ? 1 : (m == 3 ? 2 : 0);
}
G4_SET(dlight_set_shadow_mode) {
	int m = v;
	call1(o, "set_shadow_mode", m == 1 ? 2 : (m == 2 ? 3 : 0));
}
G4_GET(dlight_get_max_distance) { return call1(o, "get_shadow_param", 0); }
G4_SET(dlight_set_max_distance) { call2(o, "set_shadow_param", 0, v); }

/* MeshInstance / Mesh */

static Ref<Mesh> instance_mesh(Object *o) {
	return Ref<Mesh>(call0(o, "get_mesh"));
}
G4_CALL(mi_get_surface_override_count) {
	Ref<Mesh> mesh = instance_mesh(o);
	return mesh.is_valid() ? mesh->get_surface_count() : 0;
}
// Godot 2 has no per-surface overrides on the instance; the material
// override applies to every surface.
G4_CALL(mi_get_surface_override_material) { return call0(o, "get_material_override"); }
G4_CALL(mi_set_surface_override_material) { return call1(o, "set_material_override", arg(args, 1)); }
G4_CALL(mi_get_blend_shape_count) {
	Ref<Mesh> mesh = instance_mesh(o);
	return mesh.is_valid() ? mesh->get_morph_target_count() : 0;
}
G4_CALL(mi_find_blend_shape_by_name) {
	Ref<Mesh> mesh = instance_mesh(o);
	if (mesh.is_null())
		return -1;
	String name = arg(args, 0);
	for (int i = 0; i < mesh->get_morph_target_count(); i++) {
		if (String(mesh->get_morph_target_name(i)) == name)
			return i;
	}
	return -1;
}
G4_CALL(mi_get_blend_shape_value) {
	Ref<Mesh> mesh = instance_mesh(o);
	int i = arg(args, 0);
	if (mesh.is_null() || i < 0 || i >= mesh->get_morph_target_count())
		return 0.0;
	return o->get("morph/" + String(mesh->get_morph_target_name(i)));
}
G4_CALL(mi_set_blend_shape_value) {
	Ref<Mesh> mesh = instance_mesh(o);
	int i = arg(args, 0);
	if (mesh.is_valid() && i >= 0 && i < mesh->get_morph_target_count())
		o->set("morph/" + String(mesh->get_morph_target_name(i)), arg(args, 1));
	return Variant();
}
G4_GET(mi_get_skeleton) { return call0(o, "get_skeleton_path"); }
G4_SET(mi_set_skeleton) { call1(o, "set_skeleton_path", NodePath(String(v))); }

// Godot 4 primitive types and array slots -> Godot 2
static int primitive_to_godot2(int p) {
	return p >= 3 ? p + 1 : p; // TRIANGLES 3->4, TRIANGLE_STRIP 4->5
}
static Array arrays_to_godot2(const Array &p_arrays) {
	// Godot 4: VERTEX, NORMAL, TANGENT, COLOR, TEX_UV, TEX_UV2, CUSTOM0-3,
	// BONES, WEIGHTS, INDEX. Godot 2: same up to TEX_UV2, then BONES,
	// WEIGHTS, INDEX.
	static const int map[13] = { 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, 6, 7, 8 };
	Array out;
	out.resize(Mesh::ARRAY_MAX);
	for (int i = 0; i < p_arrays.size() && i < 13; i++) {
		if (map[i] >= 0)
			out[map[i]] = p_arrays[i];
	}
	return out;
}
G4_CALL(mesh_add_surface_from_arrays) {
	Array blend = arg(args, 2, Array());
	Array blend2;
	for (int i = 0; i < blend.size(); i++)
		blend2.push_back(arrays_to_godot2(blend[i]));
	return call3(o, "add_surface", primitive_to_godot2(arg(args, 0)), arrays_to_godot2(arg(args, 1)), blend2);
}
G4_CALL(mesh_add_blend_shape) { return call1(o, "add_morph_target", arg(args, 0)); }

/* Shapes */

G4_GET(box_get_size) { return Vector3(call0(o, "get_extents")) * 2.0; }
G4_SET(box_set_size) { call1(o, "set_extents", Vector3(v) * 0.5); }
// Godot 4 capsule height includes the caps; Godot 2's is between centers.
G4_GET(capsule_get_height) { return (float)call0(o, "get_height") + 2.0 * (float)call0(o, "get_radius"); }
G4_SET(capsule_set_height) { call1(o, "set_height", MAX((float)v - 2.0 * (float)call0(o, "get_radius"), 0.0)); }
G4_GET(capsule_get_mid_height) { return call0(o, "get_height"); }
G4_SET(capsule_set_mid_height) { call1(o, "set_height", v); }
G4_SET(cshape_set_shape) {
	// Keep the Godot 4 transform when the capsule fix-up changes.
	Transform t = local_transform(o);
	call1(o, "set_shape", v);
	set_local_transform(o, t);
	sync_parent_shapes(o);
}
// Children added to / removed from a body (CollisionShape registration).
G4_CALL(node_add_child) {
	Variant ret = o->callv("add_child", args);
	Object *child = arg(args, 0);
	if (child && child->cast_to<CollisionShape>())
		sync_collision_shapes(o);
	return ret;
}
G4_CALL(node_remove_child) {
	Object *child = arg(args, 0);
	bool was_shape = child && child->cast_to<CollisionShape>();
	Variant ret = o->callv("remove_child", args);
	if (was_shape)
		sync_collision_shapes(o);
	return ret;
}

/* Collision objects */

static bool is_area(Object *o) {
	return o->is_type("Area");
}
G4_CALL(co_get_layer_value) { return call1(o, "get_layer_mask_bit", (int)arg(args, 0) - 1); }
G4_CALL(co_set_layer_value) { return call2(o, "set_layer_mask_bit", (int)arg(args, 0) - 1, arg(args, 1)); }
G4_CALL(co_get_mask_value) { return call1(o, "get_collision_mask_bit", (int)arg(args, 0) - 1); }
G4_CALL(co_set_mask_value) { return call2(o, "set_collision_mask_bit", (int)arg(args, 0) - 1, arg(args, 1)); }

// Shape owners: Godot 2 has a flat shape list. Owners are kept in metadata
// as {id: {"shapes": [indices], "transform": Transform, "disabled": bool}}.
static const char *OWNERS = "_g4_shape_owners";
static Dictionary owners(Object *o) {
	if (!o->has_meta(OWNERS))
		o->set_meta(OWNERS, Dictionary(true));
	return o->get_meta(OWNERS);
}
static Dictionary owner(Object *o, int id) {
	Dictionary d = owners(o);
	if (!d.has(id)) {
		Dictionary rec(true);
		rec["shapes"] = Array(true);
		rec["transform"] = Transform();
		rec["disabled"] = false;
		d[id] = rec;
	}
	return d[id];
}
G4_CALL(co_create_shape_owner) {
	Dictionary d = owners(o);
	int id = 1;
	while (d.has(id))
		id++;
	owner(o, id);
	return id;
}
G4_CALL(co_get_shape_owners) {
	return owners(o).keys();
}
static void owners_shift(Object *o, int removed_index) {
	Array keys = owners(o).keys();
	for (int k = 0; k < keys.size(); k++) {
		Array shapes = to_dict(owners(o)[keys[k]])["shapes"];
		for (int i = 0; i < shapes.size(); i++) {
			if ((int)shapes[i] > removed_index)
				shapes[i] = (int)shapes[i] - 1;
		}
	}
}
G4_CALL(co_shape_owner_add_shape) {
	Dictionary rec = owner(o, arg(args, 0));
	Array shapes = rec["shapes"];
	shapes.push_back(call0(o, "get_shape_count"));
	call2(o, "add_shape", arg(args, 1), rec["transform"]);
	return Variant();
}
G4_CALL(co_shape_owner_get_shape_count) { return to_array(owner(o, arg(args, 0))["shapes"]).size(); }
G4_CALL(co_shape_owner_get_shape_index) { return to_array(owner(o, arg(args, 0))["shapes"])[(int)arg(args, 1)]; }
G4_CALL(co_shape_owner_get_shape) {
	return call1(o, "get_shape", to_array(owner(o, arg(args, 0))["shapes"])[(int)arg(args, 1)]);
}
G4_CALL(co_shape_owner_get_transform) { return owner(o, arg(args, 0))["transform"]; }
G4_CALL(co_shape_owner_set_transform) {
	Dictionary rec = owner(o, arg(args, 0));
	rec["transform"] = arg(args, 1);
	Array shapes = rec["shapes"];
	for (int i = 0; i < shapes.size(); i++)
		call2(o, "set_shape_transform", shapes[i], arg(args, 1));
	return Variant();
}
G4_CALL(co_shape_owner_remove_shape) {
	Array shapes = owner(o, arg(args, 0))["shapes"];
	int i = arg(args, 1);
	if (i < 0 || i >= shapes.size())
		return Variant();
	int index = shapes[i];
	shapes.remove(i);
	call1(o, "remove_shape", index);
	owners_shift(o, index);
	return Variant();
}
G4_CALL(co_shape_owner_clear_shapes) {
	Array shapes = owner(o, arg(args, 0))["shapes"];
	while (shapes.size()) {
		Array a;
		a.push_back(arg(args, 0));
		a.push_back(shapes.size() - 1);
		co_shape_owner_remove_shape(o, a);
	}
	return Variant();
}
G4_CALL(co_remove_shape_owner) {
	co_shape_owner_clear_shapes(o, args);
	owners(o).erase(arg(args, 0));
	return Variant();
}
G4_CALL(co_shape_find_owner) {
	int index = arg(args, 0);
	Array keys = owners(o).keys();
	for (int k = 0; k < keys.size(); k++) {
		if (to_array(to_dict(owners(o)[keys[k]])["shapes"]).has(index))
			return keys[k];
	}
	return 0;
}
// Godot 2 cannot disable shapes; disabled owners become triggers.
G4_CALL(co_shape_owner_set_disabled) {
	Dictionary rec = owner(o, arg(args, 0));
	rec["disabled"] = arg(args, 1);
	Array shapes = rec["shapes"];
	for (int i = 0; i < shapes.size(); i++)
		call2(o, "set_shape_as_trigger", shapes[i], arg(args, 1));
	return Variant();
}
G4_CALL(co_is_shape_owner_disabled) { return owner(o, arg(args, 0))["disabled"]; }

/* Physics bodies */

// Godot 2 locks at most one linear axis (set_axis_lock: 1 X, 2 Y, 3 Z).
#define AXIS_LOCK(m_name, m_axis)                                        \
	G4_GET(pb_get_##m_name) { return (int)call0(o, "get_axis_lock") == m_axis; } \
	G4_SET(pb_set_##m_name) {                                            \
		if ((bool)v)                                                     \
			call1(o, "set_axis_lock", m_axis);                           \
		else if ((int)call0(o, "get_axis_lock") == m_axis)               \
			call1(o, "set_axis_lock", 0);                                \
	}
AXIS_LOCK(axis_lock_linear_x, 1)
AXIS_LOCK(axis_lock_linear_y, 2)
AXIS_LOCK(axis_lock_linear_z, 3)

G4_CALL(pb_get_collision_exceptions) { return Array(); }
G4_CALL(pb_get_gravity) {
	float scale = o->is_type("RigidBody") ? (float)call0(o, "get_gravity_scale") : 1.0;
	return Vector3(0, -9.8, 0) * scale;
}

G4_SET(body_set_physics_material) {
	o->set_meta("_g4_physics_material", v);
	Object *mat = v;
	if (mat && mat->cast_to<PhysicsMaterial>()) {
		PhysicsMaterial *pm = mat->cast_to<PhysicsMaterial>();
		call1(o, "set_friction", pm->get_friction());
		call1(o, "set_bounce", pm->get_bounce());
	}
}
G4_GET(body_get_physics_material) {
	return o->has_meta("_g4_physics_material") ? o->get_meta("_g4_physics_material") : Variant();
}

/* RigidBody */

enum { G2_MODE_RIGID = 0, G2_MODE_STATIC = 1, G2_MODE_CHARACTER = 2, G2_MODE_KINEMATIC = 3 };
static int rb_freeze_mode(Object *o) {
	return o->has_meta("_g4_freeze_mode") ? (int)o->get_meta("_g4_freeze_mode") : 0;
}
G4_GET(rb_get_freeze) {
	int m = call0(o, "get_mode");
	return m == G2_MODE_STATIC || m == G2_MODE_KINEMATIC;
}
G4_SET(rb_set_freeze) {
	if ((bool)v)
		call1(o, "set_mode", rb_freeze_mode(o) == 1 ? G2_MODE_KINEMATIC : G2_MODE_STATIC);
	else
		call1(o, "set_mode", o->has_meta("_g4_lock_rotation") && (bool)o->get_meta("_g4_lock_rotation") ? G2_MODE_CHARACTER : G2_MODE_RIGID);
}
G4_GET(rb_get_freeze_mode) { return rb_freeze_mode(o); }
G4_SET(rb_set_freeze_mode) {
	o->set_meta("_g4_freeze_mode", v);
	if ((bool)rb_get_freeze(o))
		rb_set_freeze(o, true);
}
G4_GET(rb_get_lock_rotation) { return (int)call0(o, "get_mode") == G2_MODE_CHARACTER; }
G4_SET(rb_set_lock_rotation) {
	o->set_meta("_g4_lock_rotation", v);
	if (!(bool)rb_get_freeze(o))
		call1(o, "set_mode", (bool)v ? G2_MODE_CHARACTER : G2_MODE_RIGID);
}
// Godot 4: apply_impulse(impulse, position); Godot 2: apply_impulse(pos, impulse)
G4_CALL(rb_apply_impulse) { return call2(o, "apply_impulse", arg(args, 1, Vector3()), arg(args, 0)); }
G4_CALL(rb_apply_central_impulse) { return call2(o, "apply_impulse", Vector3(), arg(args, 0)); }
// Godot 2 has no continuous forces: a force acts as one physics step's
// impulse.
static float physics_step(Object *o) {
	Node *n = o->cast_to<Node>();
	float d = n ? n->get_fixed_process_delta_time() : 0;
	return d > 0 ? d : 1.0 / 60.0;
}
G4_CALL(rb_apply_force) {
	return call2(o, "apply_impulse", arg(args, 1, Vector3()), Vector3(arg(args, 0)) * physics_step(o));
}
G4_CALL(rb_apply_central_force) { return call2(o, "apply_impulse", Vector3(), Vector3(arg(args, 0)) * physics_step(o)); }
G4_CALL(rb_apply_torque_impulse) {
	float mass = MAX((float)call0(o, "get_mass"), CMP_EPSILON);
	return call1(o, "set_angular_velocity", Vector3(call0(o, "get_angular_velocity")) + Vector3(arg(args, 0)) / mass);
}
G4_CALL(rb_apply_torque) {
	float mass = MAX((float)call0(o, "get_mass"), CMP_EPSILON);
	return call1(o, "set_angular_velocity", Vector3(call0(o, "get_angular_velocity")) + Vector3(arg(args, 0)) * physics_step(o) / mass);
}
G4_CALL(rb_get_contact_count) { return to_array(call0(o, "get_colliding_bodies")).size(); }
G4_CALL(rb_get_inverse_inertia_tensor) {
	float mass = MAX((float)call0(o, "get_mass"), CMP_EPSILON);
	return Matrix3().scaled(Vector3(1, 1, 1) / mass);
}

/* RayCast */

enum { TYPE_MASK_BODIES = 15, TYPE_MASK_AREA = 16 };
G4_GET(ray_get_collide_with_areas) { return ((int)call0(o, "get_type_mask") & TYPE_MASK_AREA) != 0; }
G4_SET(ray_set_collide_with_areas) {
	int m = call0(o, "get_type_mask");
	call1(o, "set_type_mask", (bool)v ? (m | TYPE_MASK_AREA) : (m & ~TYPE_MASK_AREA));
}
G4_GET(ray_get_collide_with_bodies) { return ((int)call0(o, "get_type_mask") & TYPE_MASK_BODIES) != 0; }
G4_SET(ray_set_collide_with_bodies) {
	int m = call0(o, "get_type_mask");
	call1(o, "set_type_mask", (bool)v ? (m | TYPE_MASK_BODIES) : (m & ~TYPE_MASK_BODIES));
}
G4_CALL(ray_get_mask_value) { return (((int)call0(o, "get_layer_mask") >> ((int)arg(args, 0) - 1)) & 1) != 0; }
G4_CALL(ray_set_mask_value) {
	int bit = 1 << ((int)arg(args, 0) - 1);
	int m = call0(o, "get_layer_mask");
	return call1(o, "set_layer_mask", (bool)arg(args, 1) ? (m | bit) : (m & ~bit));
}
G4_CALL(return_minus_one) { return -1; }

/* Skeleton */

G4_CALL(skel_get_bone_pose_position) { return to_xform(call1(o, "get_bone_pose", arg(args, 0))).origin; }
G4_CALL(skel_get_bone_pose_rotation) { return Quat(to_xform(call1(o, "get_bone_pose", arg(args, 0))).basis.orthonormalized()); }
G4_CALL(skel_get_bone_pose_scale) { return to_xform(call1(o, "get_bone_pose", arg(args, 0))).basis.get_scale(); }
G4_CALL(skel_set_bone_pose_position) {
	Transform t = call1(o, "get_bone_pose", arg(args, 0));
	t.origin = arg(args, 1);
	return call2(o, "set_bone_pose", arg(args, 0), t);
}
G4_CALL(skel_set_bone_pose_rotation) {
	Transform t = call1(o, "get_bone_pose", arg(args, 0));
	t.basis = with_rotation(t.basis, Matrix3(Quat(arg(args, 1))));
	return call2(o, "set_bone_pose", arg(args, 0), t);
}
G4_CALL(skel_set_bone_pose_scale) {
	Transform t = call1(o, "get_bone_pose", arg(args, 0));
	t.basis = scaled_local(t.basis.orthonormalized(), arg(args, 1));
	return call2(o, "set_bone_pose", arg(args, 0), t);
}
G4_CALL(skel_reset_bone_pose) { return call2(o, "set_bone_pose", arg(args, 0), Transform()); }
G4_CALL(skel_reset_bone_poses) {
	int n = call0(o, "get_bone_count");
	for (int i = 0; i < n; i++)
		call2(o, "set_bone_pose", i, Transform());
	return Variant();
}
G4_CALL(skel_get_bone_children) {
	Array out;
	int n = call0(o, "get_bone_count");
	for (int i = 0; i < n; i++) {
		if ((int)call1(o, "get_bone_parent", i) == (int)arg(args, 0))
			out.push_back(i);
	}
	return out;
}
G4_CALL(skel_get_parentless_bones) {
	Array out;
	int n = call0(o, "get_bone_count");
	for (int i = 0; i < n; i++) {
		if ((int)call1(o, "get_bone_parent", i) < 0)
			out.push_back(i);
	}
	return out;
}
G4_CALL(skel_get_bone_global_rest) {
	Transform t;
	for (int b = arg(args, 0); b >= 0; b = call1(o, "get_bone_parent", b))
		t = to_xform(call1(o, "get_bone_rest", b)) * t;
	return t;
}
G4_CALL(skel_get_concatenated_bone_names) {
	String names;
	int n = call0(o, "get_bone_count");
	for (int i = 0; i < n; i++)
		names += (i ? "," : "") + String(call1(o, "get_bone_name", i));
	return names;
}
G4_CALL(skel_set_bone_global_pose_override) { return call2(o, "set_bone_global_pose", arg(args, 0), arg(args, 1)); }
G4_CALL(return_zero) { return 0; }
G4_CALL(return_true) { return true; }
G4_CALL(return_array) { return Array(); }

G4_GET(bone_get_idx) {
	Node *n = o->cast_to<Node>();
	Object *skel = n ? n->get_parent() : NULL;
	return skel && skel->is_type("Skeleton") ? call1(skel, "find_bone", call0(o, "get_bone_name")) : Variant(-1);
}
G4_SET(bone_set_idx) {
	Node *n = o->cast_to<Node>();
	Object *skel = n ? n->get_parent() : NULL;
	if (skel && skel->is_type("Skeleton"))
		call1(o, "set_bone_name", call1(skel, "get_bone_name", v));
}

/* CharacterBody3D (move_and_slide on top of Godot 2's KinematicBody) */

class CharacterBodyState : public Reference {
	OBJ_TYPE(CharacterBodyState, Reference);

public:
	Vector3 velocity;
	Vector3 up_direction;
	float floor_max_angle;
	float floor_snap_length;
	float wall_min_slide_angle;
	int max_slides;
	bool slide_on_ceiling;
	bool floor_stop_on_slope;
	bool floor_block_on_wall;
	bool floor_constant_speed;
	int motion_mode;
	int platform_on_leave;
	int platform_floor_layers;
	int platform_wall_layers;

	bool on_floor, on_wall, on_ceiling;
	Vector3 floor_normal, wall_normal, platform_velocity, real_velocity, last_motion;
	Vector<Ref<KinematicCollision3D> > collisions;

	CharacterBodyState() {
		up_direction = Vector3(0, 1, 0);
		floor_max_angle = Math::deg2rad(45.0);
		floor_snap_length = 0.1;
		wall_min_slide_angle = Math::deg2rad(15.0);
		max_slides = 6;
		slide_on_ceiling = true;
		floor_stop_on_slope = true;
		floor_block_on_wall = true;
		floor_constant_speed = false;
		motion_mode = 0;
		platform_on_leave = 0;
		platform_floor_layers = 0xFFFFFFFF;
		platform_wall_layers = 0;
		on_floor = on_wall = on_ceiling = false;
	}
};

static CharacterBodyState *cb_state(Object *o) {
	static const char *KEY = "_g4_character_body";
	if (!o->has_meta(KEY)) {
		Ref<CharacterBodyState> s;
		s.instance();
		o->set_meta(KEY, s);
	}
	Object *s = o->get_meta(KEY);
	return static_cast<CharacterBodyState *>(s);
}

static Ref<KinematicCollision3D> cb_record(KinematicBody *kb, const Vector3 &motion, const Vector3 &remainder) {
	Ref<KinematicCollision3D> c;
	c.instance();
	c->position = kb->get_collision_pos();
	c->normal = kb->get_collision_normal();
	c->travel = motion - remainder;
	c->remainder = remainder;
	c->collider_velocity = kb->get_collider_velocity();
	c->collider_id = kb->get_collider();
	c->collider_shape_index = kb->get_collider_shape();
	return c;
}

static Variant cb_move_and_slide(Object *o, const Array &args) {
	KinematicBody *kb = o->cast_to<KinematicBody>();
	if (!kb)
		return false;
	CharacterBodyState *s = cb_state(o);
	float delta = physics_step(o);
	Vector3 up = s->up_direction.normalized();
	bool was_on_floor = s->on_floor;
	Vector3 start = kb->get_global_transform().origin;

	s->on_floor = s->on_wall = s->on_ceiling = false;
	s->floor_normal = s->wall_normal = Vector3();
	s->collisions.clear();

	// Moving platforms carry the body along.
	Vector3 motion = (s->velocity + (was_on_floor ? s->platform_velocity : Vector3())) * delta;
	s->platform_velocity = Vector3();
	for (int i = 0; i < MAX(s->max_slides, 1) && motion.length_squared() > CMP_EPSILON * CMP_EPSILON; i++) {
		Vector3 remainder = kb->move(motion);
		if (!kb->is_colliding())
			break;
		Ref<KinematicCollision3D> c = cb_record(kb, motion, remainder);
		s->collisions.push_back(c);
		Vector3 n = c->normal;
		float angle = Math::acos(CLAMP(n.dot(up), -1.0, 1.0));
		if (s->motion_mode == 0 && angle <= s->floor_max_angle + 0.01) {
			s->on_floor = true;
			s->floor_normal = n;
			s->platform_velocity = c->collider_velocity;
			if (s->floor_stop_on_slope && s->velocity.normalized().dot(-up) > 0.99 && remainder.length() < CMP_EPSILON)
				break;
		} else if (s->motion_mode == 0 && angle >= Math_PI - s->floor_max_angle - 0.01) {
			s->on_ceiling = true;
			if (!s->slide_on_ceiling && s->velocity.dot(up) > 0)
				s->velocity -= up * s->velocity.dot(up);
		} else {
			s->on_wall = true;
			s->wall_normal = n;
		}
		motion = remainder - n * remainder.dot(n);
		if (s->velocity.dot(n) < 0)
			s->velocity -= n * s->velocity.dot(n);
	}

	// Floor snap: stay glued to the floor when walking down slopes.
	if (was_on_floor && !s->on_floor && s->motion_mode == 0 && s->floor_snap_length > 0 && s->velocity.dot(up) <= 0) {
		Transform before = kb->get_global_transform();
		Vector3 snap = -up * s->floor_snap_length;
		Vector3 remainder = kb->move(snap);
		bool floor = kb->is_colliding() && Math::acos(CLAMP(kb->get_collision_normal().dot(up), -1.0, 1.0)) <= s->floor_max_angle + 0.01;
		if (floor) {
			s->on_floor = true;
			s->floor_normal = kb->get_collision_normal();
			s->collisions.push_back(cb_record(kb, snap, remainder));
		} else {
			kb->set_global_transform(before);
		}
	}

	Vector3 end = kb->get_global_transform().origin;
	s->last_motion = end - start;
	s->real_velocity = s->last_motion / delta;
	return s->collisions.size() > 0;
}

static Variant cb_move_and_collide(Object *o, const Array &args) {
	KinematicBody *kb = o->cast_to<KinematicBody>();
	if (!kb)
		return Variant();
	Vector3 motion = arg(args, 0);
	bool test_only = arg(args, 1, false);
	Transform before = kb->get_global_transform();
	Vector3 remainder = kb->move(motion);
	Ref<KinematicCollision3D> c;
	if (kb->is_colliding())
		c = cb_record(kb, motion, remainder);
	if (test_only)
		kb->set_global_transform(before);
	return c;
}

static Variant cb_test_move(Object *o, const Array &args) {
	KinematicBody *kb = o->cast_to<KinematicBody>();
	if (!kb)
		return false;
	Transform before = kb->get_global_transform();
	kb->set_global_transform(arg(args, 0, before));
	kb->move(arg(args, 1));
	bool hit = kb->is_colliding();
	kb->set_global_transform(before);
	return hit;
}

#define CB_FIELD(m_name, m_type)                                     \
	G4_GET(cb_get_##m_name) { return cb_state(o)->m_name; }          \
	G4_SET(cb_set_##m_name) { cb_state(o)->m_name = (m_type)v; }
CB_FIELD(velocity, Vector3)
CB_FIELD(up_direction, Vector3)
CB_FIELD(floor_max_angle, float)
CB_FIELD(floor_snap_length, float)
CB_FIELD(wall_min_slide_angle, float)
CB_FIELD(max_slides, int)
CB_FIELD(slide_on_ceiling, bool)
CB_FIELD(floor_stop_on_slope, bool)
CB_FIELD(floor_block_on_wall, bool)
CB_FIELD(floor_constant_speed, bool)
CB_FIELD(motion_mode, int)
CB_FIELD(platform_on_leave, int)
CB_FIELD(platform_floor_layers, int)
CB_FIELD(platform_wall_layers, int)

G4_CALL(cb_is_on_floor) { return cb_state(o)->on_floor; }
G4_CALL(cb_is_on_floor_only) {
	CharacterBodyState *s = cb_state(o);
	return s->on_floor && !s->on_wall && !s->on_ceiling;
}
G4_CALL(cb_is_on_wall) { return cb_state(o)->on_wall; }
G4_CALL(cb_is_on_wall_only) {
	CharacterBodyState *s = cb_state(o);
	return s->on_wall && !s->on_floor && !s->on_ceiling;
}
G4_CALL(cb_is_on_ceiling) { return cb_state(o)->on_ceiling; }
G4_CALL(cb_is_on_ceiling_only) {
	CharacterBodyState *s = cb_state(o);
	return s->on_ceiling && !s->on_floor && !s->on_wall;
}
G4_CALL(cb_get_floor_normal) { return cb_state(o)->floor_normal; }
G4_CALL(cb_get_wall_normal) { return cb_state(o)->wall_normal; }
G4_CALL(cb_get_floor_angle) {
	CharacterBodyState *s = cb_state(o);
	Vector3 up = arg(args, 0, s->up_direction);
	return Math::acos(CLAMP(s->floor_normal.dot(up.normalized()), -1.0, 1.0));
}
G4_CALL(cb_get_last_motion) { return cb_state(o)->last_motion; }
G4_CALL(cb_get_real_velocity) { return cb_state(o)->real_velocity; }
G4_CALL(cb_get_platform_velocity) { return cb_state(o)->platform_velocity; }
G4_CALL(cb_get_slide_collision_count) { return cb_state(o)->collisions.size(); }
G4_CALL(cb_get_slide_collision) {
	CharacterBodyState *s = cb_state(o);
	int i = arg(args, 0);
	return i >= 0 && i < s->collisions.size() ? Variant(s->collisions[i]) : Variant();
}
G4_CALL(cb_get_last_slide_collision) {
	CharacterBodyState *s = cb_state(o);
	return s->collisions.size() ? Variant(s->collisions[s->collisions.size() - 1]) : Variant();
}
G4_CALL(cb_apply_floor_snap) {
	CharacterBodyState *s = cb_state(o);
	KinematicBody *kb = o->cast_to<KinematicBody>();
	if (!kb || s->on_floor)
		return Variant();
	Transform before = kb->get_global_transform();
	kb->move(-s->up_direction.normalized() * s->floor_snap_length);
	if (kb->is_colliding() && Math::acos(CLAMP(kb->get_collision_normal().dot(s->up_direction.normalized()), -1.0, 1.0)) <= s->floor_max_angle + 0.01) {
		s->on_floor = true;
		s->floor_normal = kb->get_collision_normal();
	} else {
		kb->set_global_transform(before);
	}
	return Variant();
}


/* Control: Godot 4 anchors and offsets */

// Godot 4 places each side at anchor * parent_size + offset, with anchors as
// ratios. Godot 2 has an anchor mode per side and a margin whose meaning
// depends on it: BEGIN (pos = m), END (pos = size - m), RATIO
// (pos = m * size) and CENTER (pos = size / 2 - m). Ratios 0, 1 and 0.5 map
// exactly; other ratios become RATIO anchors, which have no pixel offset, so
// their offset is kept in metadata and only reported back.

static float ctl_parent_range(Control *c, int p_side) {
	if (!c->is_inside_tree())
		return 0;
	Size2 s = c->get_parent_area_size();
	return (p_side & 1) ? s.y : s.x;
}

static String ratio_offset_key(int p_side) {
	return "_g4_ratio_offset_" + itos(p_side);
}

static void ctl_get_side(Control *c, int p_side, float &r_anchor, float &r_offset) {
	Margin m = (Margin)p_side;
	float v = c->get_margin(m);
	switch (c->get_anchor(m)) {
		case Control::ANCHOR_BEGIN: r_anchor = 0; r_offset = v; break;
		case Control::ANCHOR_END: r_anchor = 1; r_offset = -v; break;
		case Control::ANCHOR_CENTER: r_anchor = 0.5; r_offset = -v; break;
		case Control::ANCHOR_RATIO: {
			r_anchor = v;
			String key = ratio_offset_key(p_side);
			r_offset = c->has_meta(key) ? (float)c->get_meta(key) : 0.0;
		} break;
	}
}

static void ctl_set_side(Control *c, int p_side, float p_anchor, float p_offset) {
	Margin m = (Margin)p_side;
	Control::AnchorType type;
	float margin;
	if (p_anchor == 0) {
		type = Control::ANCHOR_BEGIN;
		margin = p_offset;
	} else if (p_anchor == 1) {
		type = Control::ANCHOR_END;
		margin = -p_offset;
	} else if (p_anchor == 0.5) {
		type = Control::ANCHOR_CENTER;
		margin = -p_offset;
	} else {
		type = Control::ANCHOR_RATIO;
		margin = p_anchor;
	}
	String key = ratio_offset_key(p_side);
	if (type == Control::ANCHOR_RATIO && p_offset != 0)
		c->set_meta(key, p_offset);
	else if (c->has_meta(key))
		c->set_meta(key, Variant());
	c->set_anchor(m, type, true);
	c->set_margin(m, margin);
}

// Godot 4 Control.set_anchor: without keep_offset the side stays in place.
static void ctl_set_anchor(Control *c, int p_side, float p_anchor, bool p_keep_offset, bool p_push_opposite) {
	float range = ctl_parent_range(c, p_side);
	int opp = (p_side + 2) % 4;
	float a, o, oa, oo;
	ctl_get_side(c, p_side, a, o);
	ctl_get_side(c, opp, oa, oo);
	float prev_pos = o + a * range;
	float prev_opp_pos = oo + oa * range;
	a = p_anchor;
	bool opp_changed = false;
	if (p_push_opposite && ((p_side < 2 && a > oa) || (p_side >= 2 && a < oa))) {
		oa = a;
		opp_changed = true;
	}
	if (!p_keep_offset) {
		o = prev_pos - a * range;
		if (opp_changed)
			oo = prev_opp_pos - oa * range;
	}
	ctl_set_side(c, p_side, a, o);
	if (opp_changed)
		ctl_set_side(c, opp, oa, oo);
}

#define CTL_SIDE(m_side, m_name)                                           \
	G4_GET(ctl_get_anchor_##m_name) {                                      \
		Control *c = o->cast_to<Control>();                                \
		float a, off;                                                      \
		ctl_get_side(c, m_side, a, off);                                   \
		return a;                                                          \
	}                                                                      \
	G4_SET(ctl_set_anchor_##m_name) {                                      \
		ctl_set_anchor(o->cast_to<Control>(), m_side, v, false, true);     \
	}                                                                      \
	G4_GET(ctl_get_offset_##m_name) {                                      \
		Control *c = o->cast_to<Control>();                                \
		float a, off;                                                      \
		ctl_get_side(c, m_side, a, off);                                   \
		return off;                                                        \
	}                                                                      \
	G4_SET(ctl_set_offset_##m_name) {                                      \
		Control *c = o->cast_to<Control>();                                \
		float a, off;                                                      \
		ctl_get_side(c, m_side, a, off);                                   \
		ctl_set_side(c, m_side, a, v);                                     \
	}

CTL_SIDE(MARGIN_LEFT, left)
CTL_SIDE(MARGIN_TOP, top)
CTL_SIDE(MARGIN_RIGHT, right)
CTL_SIDE(MARGIN_BOTTOM, bottom)

G4_CALL(ctl_get_anchor) {
	float a, off;
	ctl_get_side(o->cast_to<Control>(), CLAMP((int)arg(args, 0), 0, 3), a, off);
	return a;
}
G4_CALL(ctl_set_anchor) {
	ctl_set_anchor(o->cast_to<Control>(), CLAMP((int)arg(args, 0), 0, 3), arg(args, 1), arg(args, 2, false), arg(args, 3, true));
	return Variant();
}
G4_CALL(ctl_get_offset) {
	float a, off;
	ctl_get_side(o->cast_to<Control>(), CLAMP((int)arg(args, 0), 0, 3), a, off);
	return off;
}
G4_CALL(ctl_set_offset) {
	Control *c = o->cast_to<Control>();
	int side = CLAMP((int)arg(args, 0), 0, 3);
	float a, off;
	ctl_get_side(c, side, a, off);
	ctl_set_side(c, side, a, arg(args, 1));
	return Variant();
}
G4_CALL(ctl_set_anchor_and_offset) {
	Control *c = o->cast_to<Control>();
	int side = CLAMP((int)arg(args, 0), 0, 3);
	ctl_set_anchor(c, side, arg(args, 1), false, arg(args, 3, false));
	float a, off;
	ctl_get_side(c, side, a, off);
	ctl_set_side(c, side, a, arg(args, 2));
	return Variant();
}

// Godot 4 LayoutPreset
enum {
	PRESET_TOP_LEFT,
	PRESET_TOP_RIGHT,
	PRESET_BOTTOM_LEFT,
	PRESET_BOTTOM_RIGHT,
	PRESET_CENTER_LEFT,
	PRESET_CENTER_TOP,
	PRESET_CENTER_RIGHT,
	PRESET_CENTER_BOTTOM,
	PRESET_CENTER,
	PRESET_LEFT_WIDE,
	PRESET_TOP_WIDE,
	PRESET_RIGHT_WIDE,
	PRESET_BOTTOM_WIDE,
	PRESET_VCENTER_WIDE,
	PRESET_HCENTER_WIDE,
	PRESET_FULL_RECT,
};

// Anchor of each side (left, top, right, bottom) for a preset, as in Godot 4.
static float preset_anchor(int p_preset, int p_side) {
	switch (p_side) {
		case 0:
			switch (p_preset) {
				case PRESET_CENTER_TOP: case PRESET_CENTER_BOTTOM: case PRESET_CENTER: case PRESET_VCENTER_WIDE: return 0.5;
				case PRESET_TOP_RIGHT: case PRESET_BOTTOM_RIGHT: case PRESET_CENTER_RIGHT: case PRESET_RIGHT_WIDE: return 1;
				default: return 0;
			}
		case 1:
			switch (p_preset) {
				case PRESET_CENTER_LEFT: case PRESET_CENTER_RIGHT: case PRESET_CENTER: case PRESET_HCENTER_WIDE: return 0.5;
				case PRESET_BOTTOM_LEFT: case PRESET_BOTTOM_RIGHT: case PRESET_CENTER_BOTTOM: case PRESET_BOTTOM_WIDE: return 1;
				default: return 0;
			}
		case 2:
			switch (p_preset) {
				case PRESET_TOP_LEFT: case PRESET_BOTTOM_LEFT: case PRESET_CENTER_LEFT: case PRESET_LEFT_WIDE: return 0;
				case PRESET_CENTER_TOP: case PRESET_CENTER_BOTTOM: case PRESET_CENTER: case PRESET_VCENTER_WIDE: return 0.5;
				default: return 1;
			}
		default:
			switch (p_preset) {
				case PRESET_TOP_LEFT: case PRESET_TOP_RIGHT: case PRESET_CENTER_TOP: case PRESET_TOP_WIDE: return 0;
				case PRESET_CENTER_LEFT: case PRESET_CENTER_RIGHT: case PRESET_CENTER: case PRESET_HCENTER_WIDE: return 0.5;
				default: return 1;
			}
	}
}

static void ctl_set_anchors_preset(Control *c, int p_preset, bool p_keep_offsets) {
	for (int side = 0; side < 4; side++)
		ctl_set_anchor(c, side, preset_anchor(p_preset, side), p_keep_offsets, false);
}

// Godot 4 LayoutPresetMode
enum { PRESET_MODE_MINSIZE, PRESET_MODE_KEEP_WIDTH, PRESET_MODE_KEEP_HEIGHT, PRESET_MODE_KEEP_SIZE };

static void ctl_set_offsets_preset(Control *c, int p_preset, int p_mode, float p_margin) {
	Size2 size = c->get_size();
	Size2 min_size = c->get_combined_minimum_size();
	if (p_mode == PRESET_MODE_MINSIZE || p_mode == PRESET_MODE_KEEP_HEIGHT)
		size.x = min_size.x;
	if (p_mode == PRESET_MODE_MINSIZE || p_mode == PRESET_MODE_KEEP_WIDTH)
		size.y = min_size.y;
	Size2 parent = c->is_inside_tree() ? c->get_parent_area_size() : Size2();
	float anchor[4], offset[4];
	for (int side = 0; side < 4; side++)
		ctl_get_side(c, side, anchor[side], offset[side]);
	for (int side = 0; side < 4; side++) {
		float range = (side & 1) ? parent.y : parent.x;
		float extent = (side & 1) ? size.y : size.x;
		float target = preset_anchor(p_preset, side);
		// Where the preset puts the side, relative to its anchor point.
		float rel;
		if (side < 2)
			rel = target == 0 ? p_margin : (target == 1 ? -extent - p_margin : -extent / 2);
		else
			rel = target == 0 ? extent + p_margin : (target == 1 ? -p_margin : extent / 2);
		offset[side] = range * (target - anchor[side]) + rel;
		ctl_set_side(c, side, anchor[side], offset[side]);
	}
}

G4_CALL(ctl_set_anchors_preset) {
	ctl_set_anchors_preset(o->cast_to<Control>(), arg(args, 0), arg(args, 1, false));
	return Variant();
}
G4_CALL(ctl_set_offsets_preset) {
	ctl_set_offsets_preset(o->cast_to<Control>(), arg(args, 0), arg(args, 1, PRESET_MODE_MINSIZE), arg(args, 2, 0));
	return Variant();
}
G4_CALL(ctl_set_anchors_and_offsets_preset) {
	Control *c = o->cast_to<Control>();
	ctl_set_anchors_preset(c, arg(args, 0), false);
	ctl_set_offsets_preset(c, arg(args, 0), arg(args, 1, PRESET_MODE_MINSIZE), arg(args, 2, 0));
	return Variant();
}
G4_CALL(ctl_get_screen_position) { return call0(o, "get_global_pos"); }
G4_CALL(ctl_get_theme_font_size) { return 16; }
// Godot 2 removes a resource override when it is set to null; constant and
// color overrides cannot be removed.
G4_CALL(ctl_remove_theme_icon_override) { return call2(o, "add_icon_override", arg(args, 0), Variant()); }
G4_CALL(ctl_remove_theme_stylebox_override) { return call2(o, "add_style_override", arg(args, 0), Variant()); }
G4_CALL(ctl_remove_theme_font_override) { return call2(o, "add_font_override", arg(args, 0), Variant()); }
G4_CALL(ctl_update_minimum_size) { return call0(o, "minimum_size_changed"); }

// Godot 4 SizeFlags: FILL 1, EXPAND 2, SHRINK_CENTER 4, SHRINK_END 8.
// Godot 2: EXPAND 1, FILL 2, no shrink flags.
static int size_flags_to_godot2(int f) { return ((f & 1) ? 2 : 0) | ((f & 2) ? 1 : 0); }
static int size_flags_from_godot2(int f) { return ((f & 2) ? 1 : 0) | ((f & 1) ? 2 : 0); }
G4_GET(ctl_get_size_flags_horizontal) { return size_flags_from_godot2(call0(o, "get_h_size_flags")); }
G4_SET(ctl_set_size_flags_horizontal) { call1(o, "set_h_size_flags", size_flags_to_godot2(v)); }
G4_GET(ctl_get_size_flags_vertical) { return size_flags_from_godot2(call0(o, "get_v_size_flags")); }
G4_SET(ctl_set_size_flags_vertical) { call1(o, "set_v_size_flags", size_flags_to_godot2(v)); }

G4_GET(ctl_get_mouse_filter) {
	// Godot 4 MOUSE_FILTER_STOP 0, PASS 1, IGNORE 2
	if ((bool)call0(o, "is_ignoring_mouse"))
		return 2;
	return (bool)call0(o, "is_stopping_mouse") ? 0 : 1;
}
G4_SET(ctl_set_mouse_filter) {
	int f = v;
	call1(o, "set_ignore_mouse", f == 2);
	call1(o, "set_stop_mouse", f == 0);
}

#define CTL_FOCUS_NEIGHBOR(m_name, m_margin)                                    \
	G4_GET(ctl_get_##m_name) { return call1(o, "get_focus_neighbour", m_margin); } \
	G4_SET(ctl_set_##m_name) { call2(o, "set_focus_neighbour", m_margin, NodePath(String(v))); }
CTL_FOCUS_NEIGHBOR(focus_neighbor_left, MARGIN_LEFT)
CTL_FOCUS_NEIGHBOR(focus_neighbor_top, MARGIN_TOP)
CTL_FOCUS_NEIGHBOR(focus_neighbor_right, MARGIN_RIGHT)
CTL_FOCUS_NEIGHBOR(focus_neighbor_bottom, MARGIN_BOTTOM)

/* Object */

// Godot 4 property dictionaries carry the class of object properties in
// "class_name"; Godot 2 has it only as a resource type hint.
G4_CALL(obj_get_property_list) {
	List<PropertyInfo> plist;
	o->get_property_list(&plist);
	Array out;
	for (List<PropertyInfo>::Element *E = plist.front(); E; E = E->next()) {
		const PropertyInfo &pi = E->get();
		Dictionary d;
		d["name"] = pi.name;
		d["type"] = pi.type;
		d["hint"] = pi.hint;
		d["hint_string"] = pi.hint_string;
		d["usage"] = pi.usage;
		String cls;
		if (pi.type == Variant::OBJECT && pi.hint == PROPERTY_HINT_RESOURCE_TYPE)
			cls = pi.hint_string;
		d["class_name"] = cls;
		out.push_back(d);
	}
	return out;
}

/* Node */

// Godot 4's root Window is Godot 2's root Viewport.
G4_CALL(node_get_window) {
	Node *n = o->cast_to<Node>();
	return (n && n->is_inside_tree()) ? Variant(n->get_tree()->get_root()) : Variant();
}
G4_CALL(viewport_get_size_with_decorations) { return call0(o, "get_visible_rect").operator Rect2().size; }

/* Input (Godot 4 key, joypad and mouse codes) */

G4_CALL(input_is_key_pressed) {
	return call1(o, "is_key_pressed", (int)InputEventRef::key_from_godot4(arg(args, 0)));
}
G4_CALL(input_is_joy_button_pressed) {
	return call2(o, "is_joy_button_pressed", arg(args, 0), InputEventRef::joy_button_from_godot4(arg(args, 1)));
}
G4_CALL(input_get_joy_axis) {
	return call2(o, "get_joy_axis", arg(args, 0), InputEventRef::joy_axis_from_godot4(arg(args, 1)));
}
// Godot 2 actions are digital.
static float action_strength(Object *o, const Variant &p_action) {
	return (bool)call1(o, "is_action_pressed", p_action) ? 1.0 : 0.0;
}
G4_CALL(input_get_action_strength) { return action_strength(o, arg(args, 0)); }
G4_CALL(input_get_vector) {
	// get_vector(negative_x, positive_x, negative_y, positive_y)
	Vector2 v;
	v.x = action_strength(o, arg(args, 1)) - action_strength(o, arg(args, 0));
	v.y = action_strength(o, arg(args, 3)) - action_strength(o, arg(args, 2));
	return v.length() > 1 ? v.normalized() : v;
}
G4_CALL(input_get_axis) {
	return action_strength(o, arg(args, 1)) - action_strength(o, arg(args, 0));
}

/* GUI widgets */

// Godot 4's text is the BBCode source when bbcode_enabled is set.
G4_GET(rtl_get_text) {
	if ((bool)call0(o, "is_using_bbcode"))
		return call0(o, "get_bbcode");
	return o->has_meta("_g4_text") ? o->get_meta("_g4_text") : call0(o, "get_text");
}
G4_SET(rtl_set_text) {
	String text = v;
	if ((bool)call0(o, "is_using_bbcode")) {
		call1(o, "set_bbcode", text);
	} else {
		call0(o, "clear");
		call1(o, "add_text", text);
		o->set_meta("_g4_text", text);
	}
}
G4_SET(rtl_set_bbcode_enabled) {
	String text = rtl_get_text(o);
	call1(o, "set_use_bbcode", v);
	rtl_set_text(o, text);
}
G4_CALL(rtl_append_text) {
	if ((bool)call0(o, "is_using_bbcode"))
		return call1(o, "append_bbcode", arg(args, 0));
	return call1(o, "add_text", arg(args, 0));
}
G4_CALL(rtl_get_parsed_text) { return call0(o, "get_text"); }

G4_CALL(popup_add_separator) { return call0(o, "add_separator"); }
G4_CALL(popup_add_item) {
	// Godot 4: add_item(label, id = -1, accel = 0)
	return call3(o, "add_item", arg(args, 0), arg(args, 1, -1), arg(args, 2, 0));
}
G4_CALL(popup_add_icon_item) {
	PopupMenu *pm = o->cast_to<PopupMenu>();
	pm->add_icon_item(arg(args, 0), arg(args, 1), arg(args, 2, -1), arg(args, 3, 0));
	return Variant();
}
G4_CALL(popup_add_check_item) {
	return call3(o, "add_check_item", arg(args, 0), arg(args, 1, -1), arg(args, 2, 0));
}
G4_CALL(popup_set_item_checkable) { return call2(o, "set_item_as_checkable", arg(args, 0), arg(args, 1)); }
G4_CALL(popup_set_item_id) { return call2(o, "set_item_ID", arg(args, 0), arg(args, 1)); }
G4_CALL(popup_get_item_id) { return call1(o, "get_item_ID", arg(args, 0)); }
G4_CALL(popup_set_item_as_separator) { return call2(o, "set_item_as_separator", arg(args, 0), arg(args, 1)); }

// Godot 2's OptionButton keeps its PopupMenu private; it is its only
// PopupMenu child.
G4_CALL(option_get_popup) {
	Node *n = o->cast_to<Node>();
	for (int i = 0; n && i < n->get_child_count(); i++) {
		if (n->get_child(i)->cast_to<PopupMenu>())
			return n->get_child(i);
	}
	return Variant();
}

G4_GET(dialog_get_ok_text) {
	Object *ok = call0(o, "get_ok");
	return ok ? ok->call("get_text") : Variant();
}
G4_SET(dialog_set_ok_text) {
	Object *ok = call0(o, "get_ok");
	if (ok)
		ok->call("set_text", v);
}

// Godot 2 TreeItems link to their first child (get_children) and siblings.
static Array treeitem_children(Object *o) {
	Array out;
	Object *c = call0(o, "get_children");
	while (c) {
		out.push_back(c);
		c = c->call("get_next");
	}
	return out;
}
G4_CALL(treeitem_get_children) { return treeitem_children(o); }
G4_CALL(treeitem_get_child_count) { return treeitem_children(o).size(); }
G4_CALL(treeitem_get_child) {
	Array c = treeitem_children(o);
	int idx = arg(args, 0);
	if (idx < 0)
		idx += c.size();
	return (idx >= 0 && idx < c.size()) ? c[idx] : Variant();
}
G4_CALL(treeitem_get_index) {
	Object *parent = call0(o, "get_parent");
	if (!parent)
		return 0;
	Array c = treeitem_children(parent);
	for (int i = 0; i < c.size(); i++) {
		if ((Object *)c[i] == o)
			return i;
	}
	return -1;
}

G4_GET(scroll_get_h_mode) { return (bool)call0(o, "is_h_scroll_enabled") ? 1 : 0; } // AUTO : DISABLED
G4_SET(scroll_set_h_mode) { call1(o, "set_enable_h_scroll", (int)v != 0); }
G4_GET(scroll_get_v_mode) { return (bool)call0(o, "is_v_scroll_enabled") ? 1 : 0; }
G4_SET(scroll_set_v_mode) { call1(o, "set_enable_v_scroll", (int)v != 0); }

G4_GET(textedit_get_editable) { return !(o->has_meta("_g4_readonly") && (bool)o->get_meta("_g4_readonly")); }
G4_SET(textedit_set_editable) {
	o->set_meta("_g4_readonly", !(bool)v);
	call1(o, "set_readonly", !(bool)v);
}
// Godot 2 TextEdits color syntax themselves; a CodeHighlighterRef is applied
// to them. Re-applying is skipped while its colors are unchanged, since code
// may assign a fresh highlighter every frame.
G4_GET(textedit_get_syntax_highlighter) {
	return o->has_meta("_g4_syntax_highlighter") ? o->get_meta("_g4_syntax_highlighter") : Variant();
}
G4_SET(textedit_set_syntax_highlighter) {
	Object *obj = v;
	CodeHighlighterRef *h = obj ? obj->cast_to<CodeHighlighterRef>() : NULL;
	o->set_meta("_g4_syntax_highlighter", h ? v : Variant());
	if (!h) {
		if (o->has_meta("_g4_highlighter_sig")) {
			o->set_meta("_g4_highlighter_sig", Variant());
			call0(o, "clear_colors");
			call1(o, "set_syntax_coloring", false);
		}
		return;
	}
	int sig = (int)h->get_signature();
	if (o->has_meta("_g4_highlighter_sig") && (int)o->get_meta("_g4_highlighter_sig") == sig)
		return;
	o->set_meta("_g4_highlighter_sig", sig);
	h->apply_to(o);
}

// Godot 4's TextEdit.text setter does not emit text_changed. Godot 2's
// set_text means not to either, but clear() resets its guard, so the signal
// is queued anyway (code marking a file modified on text_changed then fires
// for loading it). TextEdit::text_changed_dirty is private; this reaches it
// through explicit template instantiation, which ignores access checks.
template <typename Tag, typename Tag::type M>
struct PrivateMember {
	friend typename Tag::type get(Tag) { return M; }
};
struct TextChangedDirty {
	typedef bool TextEdit::*type;
	friend type get(TextChangedDirty);
};
template struct PrivateMember<TextChangedDirty, &TextEdit::text_changed_dirty>;

G4_GET(textedit_get_text) { return call0(o, "get_text"); }
G4_SET(textedit_set_text) {
	TextEdit *te = o->cast_to<TextEdit>();
	if (!te)
		return;
	bool &dirty = te->*get(TextChangedDirty());
	bool was_dirty = dirty;
	dirty = true; // already "dirty": set_text queues no emission
	te->set_text(v);
	dirty = was_dirty;
}

G4_CALL(textedit_get_caret_line) { return call0(o, "cursor_get_line"); }
G4_CALL(textedit_get_caret_column) { return call0(o, "cursor_get_column"); }
G4_CALL(textedit_set_caret_line) { return call2(o, "cursor_set_line", arg(args, 0), arg(args, 1, true)); }
G4_CALL(textedit_set_caret_column) { return call2(o, "cursor_set_column", arg(args, 0), arg(args, 1, true)); }
G4_CALL(textedit_get_selected_text) { return call0(o, "get_selection_text"); }
G4_CALL(textedit_has_selection) { return call0(o, "is_selection_active"); }

/* Tables */

typedef Variant (*Getter)(Object *);
typedef void (*Setter)(Object *, const Variant &);
typedef Variant (*Caller)(Object *, const Array &);

struct PropEntry {
	const char *cls; // Godot 2 class
	const char *name; // Godot 4 property
	const char *getter; // Godot 2 methods, or NULL to use the functions
	const char *setter;
	Getter get;
	Setter set;
};

struct CallEntry {
	const char *cls;
	const char *name; // Godot 4 method
	const char *target; // Godot 2 method with the same arguments, or NULL
	Caller fn;
};

// Properties Godot 2 cannot represent: kept in metadata, with the Godot 4
// default. kind: f float, i int, b bool, n nil, v Vector2, w Vector3,
// c Color (white).
struct MetaEntry {
	const char *cls;
	const char *name;
	char kind;
	double value;
};

// Ordered most-derived first: the first entry whose class matches wins.
static const PropEntry props[] = {
	// CharacterBody3D
	{ "KinematicBody", "velocity", NULL, NULL, cb_get_velocity, cb_set_velocity },
	{ "KinematicBody", "up_direction", NULL, NULL, cb_get_up_direction, cb_set_up_direction },
	{ "KinematicBody", "floor_max_angle", NULL, NULL, cb_get_floor_max_angle, cb_set_floor_max_angle },
	{ "KinematicBody", "floor_snap_length", NULL, NULL, cb_get_floor_snap_length, cb_set_floor_snap_length },
	{ "KinematicBody", "wall_min_slide_angle", NULL, NULL, cb_get_wall_min_slide_angle, cb_set_wall_min_slide_angle },
	{ "KinematicBody", "max_slides", NULL, NULL, cb_get_max_slides, cb_set_max_slides },
	{ "KinematicBody", "slide_on_ceiling", NULL, NULL, cb_get_slide_on_ceiling, cb_set_slide_on_ceiling },
	{ "KinematicBody", "floor_stop_on_slope", NULL, NULL, cb_get_floor_stop_on_slope, cb_set_floor_stop_on_slope },
	{ "KinematicBody", "floor_block_on_wall", NULL, NULL, cb_get_floor_block_on_wall, cb_set_floor_block_on_wall },
	{ "KinematicBody", "floor_constant_speed", NULL, NULL, cb_get_floor_constant_speed, cb_set_floor_constant_speed },
	{ "KinematicBody", "motion_mode", NULL, NULL, cb_get_motion_mode, cb_set_motion_mode },
	{ "KinematicBody", "platform_on_leave", NULL, NULL, cb_get_platform_on_leave, cb_set_platform_on_leave },
	{ "KinematicBody", "platform_floor_layers", NULL, NULL, cb_get_platform_floor_layers, cb_set_platform_floor_layers },
	{ "KinematicBody", "platform_wall_layers", NULL, NULL, cb_get_platform_wall_layers, cb_set_platform_wall_layers },
	{ "KinematicBody", "safe_margin", "get_collision_margin", "set_collision_margin", NULL, NULL },
	// RigidBody3D
	{ "RigidBody", "can_sleep", "is_able_to_sleep", "set_can_sleep", NULL, NULL },
	{ "RigidBody", "sleeping", "is_sleeping", "set_sleeping", NULL, NULL },
	{ "RigidBody", "contact_monitor", "is_contact_monitor_enabled", "set_contact_monitor", NULL, NULL },
	{ "RigidBody", "continuous_cd", "is_using_continuous_collision_detection", "set_use_continuous_collision_detection", NULL, NULL },
	{ "RigidBody", "custom_integrator", "is_using_custom_integrator", "set_use_custom_integrator", NULL, NULL },
	{ "RigidBody", "freeze", NULL, NULL, rb_get_freeze, rb_set_freeze },
	{ "RigidBody", "freeze_mode", NULL, NULL, rb_get_freeze_mode, rb_set_freeze_mode },
	{ "RigidBody", "lock_rotation", NULL, NULL, rb_get_lock_rotation, rb_set_lock_rotation },
	{ "RigidBody", "physics_material_override", NULL, NULL, body_get_physics_material, body_set_physics_material },
	{ "StaticBody", "physics_material_override", NULL, NULL, body_get_physics_material, body_set_physics_material },
	// PhysicsBody3D
	{ "PhysicsBody", "axis_lock_linear_x", NULL, NULL, pb_get_axis_lock_linear_x, pb_set_axis_lock_linear_x },
	{ "PhysicsBody", "axis_lock_linear_y", NULL, NULL, pb_get_axis_lock_linear_y, pb_set_axis_lock_linear_y },
	{ "PhysicsBody", "axis_lock_linear_z", NULL, NULL, pb_get_axis_lock_linear_z, pb_set_axis_lock_linear_z },
	{ "PhysicsBody", "collision_layer", "get_layer_mask", "set_layer_mask", NULL, NULL },
	{ "Area", "collision_layer", "get_layer_mask", "set_layer_mask", NULL, NULL },
	{ "Area", "monitoring", "is_monitoring_enabled", "set_enable_monitoring", NULL, NULL },
	// CollisionObject3D
	{ "CollisionObject", "input_ray_pickable", "is_ray_pickable", "set_ray_pickable", NULL, NULL },
	{ "CollisionObject", "input_capture_on_drag", "get_capture_input_on_drag", "set_capture_input_on_drag", NULL, NULL },
	// CollisionShape3D, shapes
	{ "CollisionShape", "shape", "get_shape", NULL, NULL, cshape_set_shape },
	{ "BoxShape", "size", NULL, NULL, box_get_size, box_set_size },
	{ "CapsuleShape", "height", NULL, NULL, capsule_get_height, capsule_set_height },
	{ "CapsuleShape", "mid_height", NULL, NULL, capsule_get_mid_height, capsule_set_mid_height },
	// RayCast3D
	{ "RayCast", "target_position", "get_cast_to", "set_cast_to", NULL, NULL },
	{ "RayCast", "collision_mask", "get_layer_mask", "set_layer_mask", NULL, NULL },
	{ "RayCast", "collide_with_areas", NULL, NULL, ray_get_collide_with_areas, ray_set_collide_with_areas },
	{ "RayCast", "collide_with_bodies", NULL, NULL, ray_get_collide_with_bodies, ray_set_collide_with_bodies },
	// Camera3D
	{ "Camera", "fov", "get_fov", NULL, NULL, cam_set_fov },
	{ "Camera", "near", "get_znear", NULL, NULL, cam_set_near },
	{ "Camera", "far", "get_zfar", NULL, NULL, cam_set_far },
	{ "Camera", "size", NULL, NULL, cam_get_size, cam_set_size },
	{ "Camera", "projection", "get_projection", NULL, NULL, cam_set_projection },
	{ "Camera", "current", "is_current", NULL, NULL, cam_set_current },
	{ "Camera", "cull_mask", "get_visible_layers", "set_visible_layers", NULL, NULL },
	{ "Camera", "keep_aspect", "get_keep_aspect_mode", "set_keep_aspect_mode", NULL, NULL },
	// Lights
	{ "DirectionalLight", "directional_shadow_mode", NULL, NULL, dlight_get_shadow_mode, dlight_set_shadow_mode },
	{ "DirectionalLight", "directional_shadow_max_distance", NULL, NULL, dlight_get_max_distance, dlight_set_max_distance },
	{ "OmniLight", "omni_range", NULL, NULL, light_get_range, light_set_range },
	{ "OmniLight", "omni_attenuation", NULL, NULL, light_get_attenuation, light_set_attenuation },
	{ "SpotLight", "spot_range", NULL, NULL, light_get_range, light_set_range },
	{ "SpotLight", "spot_attenuation", NULL, NULL, light_get_attenuation, light_set_attenuation },
	{ "SpotLight", "spot_angle", NULL, NULL, light_get_spot_angle, light_set_spot_angle },
	{ "SpotLight", "spot_angle_attenuation", NULL, NULL, light_get_spot_attenuation, light_set_spot_attenuation },
	{ "Light", "light_color", NULL, NULL, light_get_color, light_set_color },
	{ "Light", "light_energy", NULL, NULL, light_get_energy, light_set_energy },
	{ "Light", "light_specular", NULL, NULL, light_get_specular, light_set_specular },
	{ "Light", "light_negative", NULL, NULL, light_get_negative, light_set_negative },
	{ "Light", "light_projector", "get_projector", "set_projector", NULL, NULL },
	{ "Light", "light_bake_mode", NULL, NULL, light_get_bake_mode, light_set_bake_mode },
	{ "Light", "shadow_enabled", "has_project_shadows", "set_project_shadows", NULL, NULL },
	{ "Light", "shadow_bias", NULL, NULL, light_get_shadow_bias, light_set_shadow_bias },
	{ "Light", "shadow_opacity", NULL, NULL, light_get_shadow_opacity, light_set_shadow_opacity },
	// MeshInstance3D / GeometryInstance3D
	{ "MeshInstance", "skeleton", NULL, NULL, mi_get_skeleton, mi_set_skeleton },
	{ "GeometryInstance", "cast_shadow", "get_cast_shadows_setting", "set_cast_shadows_setting", NULL, NULL },
	{ "GeometryInstance", "visibility_range_begin", "get_draw_range_begin", "set_draw_range_begin", NULL, NULL },
	{ "GeometryInstance", "visibility_range_end", "get_draw_range_end", "set_draw_range_end", NULL, NULL },
	{ "VisualInstance", "layers", "get_layer_mask", "set_layer_mask", NULL, NULL },
	// BoneAttachment3D
	{ "BoneAttachment", "bone_idx", NULL, NULL, bone_get_idx, bone_set_idx },
	// Node3D
	{ "Spatial", "transform", NULL, NULL, spatial_get_transform, spatial_set_transform },
	{ "Spatial", "global_transform", NULL, NULL, spatial_get_global_transform, spatial_set_global_transform },
	{ "Spatial", "basis", NULL, NULL, spatial_get_basis, spatial_set_basis },
	{ "Spatial", "global_basis", NULL, NULL, spatial_get_global_basis, spatial_set_global_basis },
	{ "Spatial", "position", NULL, NULL, spatial_get_position, spatial_set_position },
	{ "Spatial", "global_position", NULL, NULL, spatial_get_global_position, spatial_set_global_position },
	{ "Spatial", "rotation", NULL, NULL, spatial_get_rotation, spatial_set_rotation },
	{ "Spatial", "rotation_degrees", NULL, NULL, spatial_get_rotation_degrees, spatial_set_rotation_degrees },
	{ "Spatial", "global_rotation", NULL, NULL, spatial_get_global_rotation, spatial_set_global_rotation },
	{ "Spatial", "global_rotation_degrees", NULL, NULL, spatial_get_global_rotation_degrees, spatial_set_global_rotation_degrees },
	{ "Spatial", "scale", NULL, NULL, spatial_get_scale, spatial_set_scale },
	{ "Spatial", "quaternion", NULL, NULL, spatial_get_quaternion, spatial_set_quaternion },
	{ "Spatial", "rotation_order", NULL, NULL, spatial_get_rotation_order, ignore_set },
	{ "Spatial", "visible", NULL, NULL, spatial_get_visible, spatial_set_visible },
	{ "Spatial", "top_level", "is_set_as_toplevel", "set_as_toplevel", NULL, NULL },
	// Sprite2D
	{ "Sprite", "flip_h", "is_flipped_h", "set_flip_h", NULL, NULL },
	{ "Sprite", "flip_v", "is_flipped_v", "set_flip_v", NULL, NULL },
	{ "Sprite", "region_enabled", "is_region", "set_region", NULL, NULL },
	{ "Sprite", "frame_coords", NULL, NULL, sprite_get_frame_coords, sprite_set_frame_coords },
	// Camera2D
	{ "Camera2D", "zoom", NULL, NULL, cam2d_get_zoom, cam2d_set_zoom },
	{ "Camera2D", "ignore_rotation", NULL, NULL, cam2d_get_ignore_rotation, cam2d_set_ignore_rotation },
	{ "Camera2D", "limit_left", NULL, NULL, cam2d_get_limit_left, cam2d_set_limit_left },
	{ "Camera2D", "limit_top", NULL, NULL, cam2d_get_limit_top, cam2d_set_limit_top },
	{ "Camera2D", "limit_right", NULL, NULL, cam2d_get_limit_right, cam2d_set_limit_right },
	{ "Camera2D", "limit_bottom", NULL, NULL, cam2d_get_limit_bottom, cam2d_set_limit_bottom },
	{ "Camera2D", "drag_left_margin", NULL, NULL, cam2d_get_drag_left_margin, cam2d_set_drag_left_margin },
	{ "Camera2D", "drag_top_margin", NULL, NULL, cam2d_get_drag_top_margin, cam2d_set_drag_top_margin },
	{ "Camera2D", "drag_right_margin", NULL, NULL, cam2d_get_drag_right_margin, cam2d_set_drag_right_margin },
	{ "Camera2D", "drag_bottom_margin", NULL, NULL, cam2d_get_drag_bottom_margin, cam2d_set_drag_bottom_margin },
	{ "Camera2D", "limit_smoothed", "is_limit_smoothing_enabled", "set_limit_smoothing_enabled", NULL, NULL },
	{ "Camera2D", "drag_horizontal_enabled", "is_h_drag_enabled", "set_h_drag_enabled", NULL, NULL },
	{ "Camera2D", "drag_vertical_enabled", "is_v_drag_enabled", "set_v_drag_enabled", NULL, NULL },
	{ "Camera2D", "drag_horizontal_offset", "get_h_offset", "set_h_offset", NULL, NULL },
	{ "Camera2D", "drag_vertical_offset", "get_v_offset", "set_v_offset", NULL, NULL },
	{ "Camera2D", "position_smoothing_enabled", "is_follow_smoothing_enabled", "set_enable_follow_smoothing", NULL, NULL },
	{ "Camera2D", "position_smoothing_speed", "get_follow_smoothing", "set_follow_smoothing", NULL, NULL },
	// Node2D
	{ "Node2D", "position", "get_pos", "set_pos", NULL, NULL },
	{ "Node2D", "global_position", "get_global_pos", "set_global_pos", NULL, NULL },
	{ "Node2D", "rotation", NULL, NULL, n2d_get_rotation, n2d_set_rotation },
	{ "Node2D", "rotation_degrees", NULL, NULL, n2d_get_rotation_degrees, n2d_set_rotation_degrees },
	{ "Node2D", "global_rotation", NULL, NULL, n2d_get_global_rotation, n2d_set_global_rotation },
	{ "Node2D", "global_rotation_degrees", NULL, NULL, n2d_get_global_rotation_degrees, n2d_set_global_rotation_degrees },
	{ "Node2D", "z_index", "get_z", "set_z", NULL, NULL },
	{ "Node2D", "z_as_relative", "is_z_relative", "set_z_as_relative", NULL, NULL },
	// RichTextLabel, ScrollContainer, TextEdit
	{ "RichTextLabel", "text", NULL, NULL, rtl_get_text, rtl_set_text },
	{ "RichTextLabel", "bbcode_enabled", "is_using_bbcode", NULL, NULL, rtl_set_bbcode_enabled },
	{ "RichTextLabel", "scroll_following", "is_scroll_following", "set_scroll_follow", NULL, NULL },
	{ "RichTextLabel", "scroll_active", "is_scroll_active", "set_scroll_active", NULL, NULL },
	{ "RichTextLabel", "selection_enabled", "is_selection_enabled", "set_selection_enabled", NULL, NULL },
	{ "RichTextLabel", "meta_underlined", "is_meta_underlined", "set_meta_underline", NULL, NULL },
	{ "RichTextLabel", "tab_size", "get_tab_size", "set_tab_size", NULL, NULL },
	{ "RichTextLabel", "visible_characters", "get_visible_characters", "set_visible_characters", NULL, NULL },
	{ "ScrollContainer", "horizontal_scroll_mode", NULL, NULL, scroll_get_h_mode, scroll_set_h_mode },
	{ "ScrollContainer", "vertical_scroll_mode", NULL, NULL, scroll_get_v_mode, scroll_set_v_mode },
	{ "ScrollContainer", "scroll_horizontal", "get_h_scroll", "set_h_scroll", NULL, NULL },
	{ "ScrollContainer", "scroll_vertical", "get_v_scroll", "set_v_scroll", NULL, NULL },
	{ "ScrollContainer", "scroll_deadzone", "get_deadzone", "set_deadzone", NULL, NULL },
	{ "TextEdit", "editable", NULL, NULL, textedit_get_editable, textedit_set_editable },
	{ "TextEdit", "text", NULL, NULL, textedit_get_text, textedit_set_text },
	{ "TextEdit", "syntax_highlighter", NULL, NULL, textedit_get_syntax_highlighter, textedit_set_syntax_highlighter },
	{ "TextEdit", "highlight_all_occurrences", "is_highlight_all_occurrences_enabled", "set_highlight_all_occurrences", NULL, NULL },
	{ "TextEdit", "caret_blink", "cursor_get_blink_enabled", "cursor_set_blink_enabled", NULL, NULL },
	{ "TextEdit", "caret_blink_interval", "cursor_get_blink_speed", "cursor_set_blink_speed", NULL, NULL },
	{ "TextEdit", "gutters_draw_line_numbers", "is_show_line_numbers_enabled", "set_show_line_numbers", NULL, NULL },
	// Line edits, buttons, labels, ranges
	{ "LineEdit", "placeholder_text", "get_placeholder", "set_placeholder", NULL, NULL },
	{ "LineEdit", "caret_column", "get_cursor_pos", "set_cursor_pos", NULL, NULL },
	{ "LineEdit", "secret", "is_secret", "set_secret", NULL, NULL },
	{ "LineEdit", "alignment", "get_align", "set_align", NULL, NULL },
	{ "LineEdit", "caret_blink", "cursor_get_blink_enabled", "cursor_set_blink_enabled", NULL, NULL },
	{ "Button", "icon", "get_button_icon", "set_button_icon", NULL, NULL },
	{ "Button", "alignment", "get_text_align", "set_text_align", NULL, NULL },
	{ "Button", "clip_text", "get_clip_text", "set_clip_text", NULL, NULL },
	{ "BaseButton", "button_pressed", "is_pressed", "set_pressed", NULL, NULL },
	{ "BaseButton", "action_mode", NULL, NULL, NULL, ignore_set },
	{ "Label", "horizontal_alignment", "get_align", "set_align", NULL, NULL },
	{ "Label", "vertical_alignment", "get_valign", "set_valign", NULL, NULL },
	{ "Label", "clip_text", "is_clipping_text", "set_clip_text", NULL, NULL },
	{ "Label", "max_lines_visible", "get_max_lines_visible", "set_max_lines_visible", NULL, NULL },
	{ "Label", "lines_skipped", "get_lines_skipped", "set_lines_skipped", NULL, NULL },
	{ "Label", "uppercase", "is_uppercase", "set_uppercase", NULL, NULL },
	{ "ProgressBar", "show_percentage", "is_percent_visible", "set_percent_visible", NULL, NULL },
	{ "Range", "value", "get_val", "set_val", NULL, NULL },
	{ "Range", "min_value", "get_min", "set_min", NULL, NULL },
	{ "Range", "max_value", "get_max", "set_max", NULL, NULL },
	{ "Range", "rounded", "is_rounded_values", "set_rounded_values", NULL, NULL },
	{ "Range", "exp_edit", "is_unit_value_exp", "set_exp_unit_value", NULL, NULL },
	{ "Range", "page", "get_page", "set_page", NULL, NULL },
	{ "SplitContainer", "collapsed", "is_collapsed", "set_collapsed", NULL, NULL },
	{ "SplitContainer", "dragger_visibility", "get_dragger_visibility", "set_dragger_visibility", NULL, NULL },
	{ "TabContainer", "tabs_visible", "are_tabs_visible", "set_tabs_visible", NULL, NULL },
	{ "TabContainer", "tab_alignment", "get_tab_align", "set_tab_align", NULL, NULL },
	{ "Tree", "hide_root", NULL, "set_hide_root", NULL, NULL },
	{ "Tree", "hide_folding", "is_folding_hidden", "set_hide_folding", NULL, NULL },
	{ "Tree", "column_titles_visible", "are_column_titles_visible", "set_column_titles_visible", NULL, NULL },
	{ "Tree", "allow_rmb_select", "get_allow_rmb_select", "set_allow_rmb_select", NULL, NULL },
	{ "ItemList", "same_column_width", "is_same_column_width", "set_same_column_width", NULL, NULL },
	{ "ItemList", "allow_rmb_select", "get_allow_rmb_select", "set_allow_rmb_select", NULL, NULL },
	{ "PopupMenu", "item_count", "get_item_count", NULL, NULL, ignore_set },
	{ "PopupMenu", "hide_on_item_selection", "is_hide_on_item_selection", "set_hide_on_item_selection", NULL, NULL },
	{ "OptionButton", "item_count", "get_item_count", NULL, NULL, ignore_set },
	{ "OptionButton", "selected", "get_selected", "select", NULL, NULL },
	{ "TextureFrame", "expand", "has_expand", "set_expand", NULL, NULL },
	{ "Popup", "exclusive", "is_exclusive", "set_exclusive", NULL, NULL },
	{ "Popup", "min_size", "get_custom_minimum_size", "set_custom_minimum_size", NULL, NULL },
	{ "AcceptDialog", "ok_button_text", NULL, NULL, dialog_get_ok_text, dialog_set_ok_text },
	{ "WindowDialog", "title", "get_title", "set_title", NULL, NULL },
	{ "AcceptDialog", "dialog_text", "get_text", "set_text", NULL, NULL },
	{ "AcceptDialog", "dialog_hide_on_ok", "get_hide_on_ok", "set_hide_on_ok", NULL, NULL },
	// Control
	{ "Control", "anchor_left", NULL, NULL, ctl_get_anchor_left, ctl_set_anchor_left },
	{ "Control", "anchor_top", NULL, NULL, ctl_get_anchor_top, ctl_set_anchor_top },
	{ "Control", "anchor_right", NULL, NULL, ctl_get_anchor_right, ctl_set_anchor_right },
	{ "Control", "anchor_bottom", NULL, NULL, ctl_get_anchor_bottom, ctl_set_anchor_bottom },
	{ "Control", "offset_left", NULL, NULL, ctl_get_offset_left, ctl_set_offset_left },
	{ "Control", "offset_top", NULL, NULL, ctl_get_offset_top, ctl_set_offset_top },
	{ "Control", "offset_right", NULL, NULL, ctl_get_offset_right, ctl_set_offset_right },
	{ "Control", "offset_bottom", NULL, NULL, ctl_get_offset_bottom, ctl_set_offset_bottom },
	{ "Control", "custom_minimum_size", "get_custom_minimum_size", "set_custom_minimum_size", NULL, NULL },
	{ "Control", "position", "get_pos", "set_pos", NULL, NULL },
	{ "Control", "global_position", "get_global_pos", "set_global_pos", NULL, NULL },
	{ "Control", "size", "get_size", "set_size", NULL, NULL },
	{ "Control", "rotation", "get_rotation", "set_rotation", NULL, NULL },
	{ "Control", "rotation_degrees", "get_rotation_deg", "set_rotation_deg", NULL, NULL },
	{ "Control", "scale", "get_scale", "set_scale", NULL, NULL },
	{ "Control", "tooltip_text", "get_tooltip", "set_tooltip", NULL, NULL },
	{ "Control", "mouse_filter", NULL, NULL, ctl_get_mouse_filter, ctl_set_mouse_filter },
	{ "Control", "mouse_default_cursor_shape", "get_default_cursor_shape", "set_default_cursor_shape", NULL, NULL },
	{ "Control", "size_flags_horizontal", NULL, NULL, ctl_get_size_flags_horizontal, ctl_set_size_flags_horizontal },
	{ "Control", "size_flags_vertical", NULL, NULL, ctl_get_size_flags_vertical, ctl_set_size_flags_vertical },
	{ "Control", "size_flags_stretch_ratio", "get_stretch_ratio", "set_stretch_ratio", NULL, NULL },
	{ "Control", "focus_neighbor_left", NULL, NULL, ctl_get_focus_neighbor_left, ctl_set_focus_neighbor_left },
	{ "Control", "focus_neighbor_top", NULL, NULL, ctl_get_focus_neighbor_top, ctl_set_focus_neighbor_top },
	{ "Control", "focus_neighbor_right", NULL, NULL, ctl_get_focus_neighbor_right, ctl_set_focus_neighbor_right },
	{ "Control", "focus_neighbor_bottom", NULL, NULL, ctl_get_focus_neighbor_bottom, ctl_set_focus_neighbor_bottom },
	// CanvasItem
	{ "CanvasItem", "visible", NULL, NULL, spatial_get_visible, spatial_set_visible },
	{ "CanvasItem", "top_level", "is_set_as_toplevel", "set_as_toplevel", NULL, NULL },
	{ "CanvasItem", "show_behind_parent", "is_draw_behind_parent_enabled", "set_draw_behind_parent", NULL, NULL },
	{ "CanvasItem", "light_mask", "get_light_mask", "set_light_mask", NULL, NULL },
	{ "CanvasItem", "use_parent_material", "get_use_parent_material", "set_use_parent_material", NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL, NULL }
};

static const MetaEntry meta_props[] = {
	{ "KinematicBody", "sync_to_physics", 'b', 1 },
	{ "RigidBody", "angular_damp_mode", 'i', 0 },
	{ "RigidBody", "linear_damp_mode", 'i', 0 },
	{ "RigidBody", "center_of_mass", 'w', 0 },
	{ "RigidBody", "center_of_mass_mode", 'i', 0 },
	{ "RigidBody", "inertia", 'w', 0 },
	{ "RigidBody", "constant_force", 'w', 0 },
	{ "RigidBody", "constant_torque", 'w', 0 },
	{ "PhysicsBody", "axis_lock_angular_x", 'b', 0 },
	{ "PhysicsBody", "axis_lock_angular_y", 'b', 0 },
	{ "PhysicsBody", "axis_lock_angular_z", 'b', 0 },
	{ "CollisionObject", "collision_priority", 'f', 1 },
	{ "CollisionObject", "disable_mode", 'i', 0 },
	{ "CollisionShape", "margin", 'f', 0.04 },
	{ "CollisionShape", "custom_solver_bias", 'f', 0 },
	{ "RayCast", "exclude_parent", 'b', 1 },
	{ "RayCast", "hit_from_inside", 'b', 0 },
	{ "RayCast", "hit_back_faces", 'b', 1 },
	{ "RayCast", "debug_shape_custom_color", 'n', 0 },
	{ "RayCast", "debug_shape_thickness", 'i', 2 },
	{ "Camera", "doppler_tracking", 'i', 0 },
	{ "Camera", "frustum_offset", 'v', 0 },
	{ "DirectionalLight", "directional_shadow_split_1", 'f', 0.1 },
	{ "DirectionalLight", "directional_shadow_split_2", 'f', 0.2 },
	{ "DirectionalLight", "directional_shadow_split_3", 'f', 0.5 },
	{ "DirectionalLight", "directional_shadow_blend_splits", 'b', 0 },
	{ "DirectionalLight", "directional_shadow_fade_start", 'f', 0.8 },
	{ "DirectionalLight", "directional_shadow_pancake_size", 'f', 20 },
	{ "DirectionalLight", "sky_mode", 'i', 0 },
	{ "OmniLight", "omni_shadow_mode", 'i', 1 },
	{ "Light", "light_indirect_energy", 'f', 1 },
	{ "Light", "light_volumetric_fog_energy", 'f', 1 },
	{ "Light", "light_size", 'f', 0 },
	{ "Light", "light_angular_distance", 'f', 0 },
	{ "Light", "light_temperature", 'f', 6500 },
	{ "Light", "light_intensity_lumens", 'f', 1000 },
	{ "Light", "light_intensity_lux", 'f', 100000 },
	{ "Light", "light_cull_mask", 'i', 4294967295.0 },
	{ "Light", "distance_fade_enabled", 'b', 0 },
	{ "Light", "distance_fade_begin", 'f', 40 },
	{ "Light", "distance_fade_length", 'f', 10 },
	{ "Light", "distance_fade_shadow", 'f', 50 },
	{ "Light", "shadow_normal_bias", 'f', 2 },
	{ "Light", "shadow_blur", 'f', 1 },
	{ "Light", "shadow_caster_mask", 'i', 4294967295.0 },
	{ "Light", "shadow_reverse_cull_face", 'b', 0 },
	{ "Light", "shadow_transmittance_bias", 'f', 0.05 },
	{ "MeshInstance", "skin", 'n', 0 },
	{ "BoneAttachment", "override_pose", 'b', 0 },
	{ "BoneAttachment", "use_external_skeleton", 'b', 0 },
	{ "BoneAttachment", "external_skeleton", 'n', 0 },
	{ "Skeleton", "show_rest_only", 'b', 0 },
	{ "Skeleton", "motion_scale", 'f', 1 },
	{ "Skeleton", "animate_physical_bones", 'b', 1 },
	{ "Skeleton", "modifier_callback_mode_process", 'i', 1 },
	{ "Sprite", "region_filter_clip_enabled", 'b', 0 },
	{ "Camera2D", "enabled", 'b', 1 },
	{ "Camera2D", "limit_enabled", 'b', 1 },
	{ "Camera2D", "rotation_smoothing_enabled", 'b', 0 },
	{ "Camera2D", "rotation_smoothing_speed", 'f', 5 },
	{ "Node2D", "skew", 'f', 0 },
	{ "Node2D", "global_skew", 'f', 0 },
	{ "Spatial", "process_priority", 'i', 0 },
	{ "Control", "clip_contents", 'b', 0 },
	{ "Control", "grow_horizontal", 'i', 1 },
	{ "Control", "grow_vertical", 'i', 1 },
	{ "Control", "pivot_offset", 'v', 0 },
	{ "Control", "layout_direction", 'i', 0 },
	{ "Control", "auto_translate", 'b', 1 },
	{ "Control", "localize_numeral_system", 'b', 1 },
	{ "Control", "theme_type_variation", 'n', 0 },
	{ "Control", "focus_next", 'n', 0 },
	{ "Control", "focus_previous", 'n', 0 },
	{ "Control", "custom_maximum_size", 'v', 0 },
	{ "Control", "shortcut_context", 'n', 0 },
	{ "Button", "icon_alignment", 'i', 0 },
	{ "Button", "vertical_icon_alignment", 'i', 1 },
	{ "Button", "expand_icon", 'b', 0 },
	{ "Button", "text_overrun_behavior", 'i', 0 },
	{ "Label", "text_overrun_behavior", 'i', 0 },
	{ "Label", "justification_flags", 'i', 163 },
	{ "LineEdit", "clear_button_enabled", 'b', 0 },
	{ "LineEdit", "select_all_on_focus", 'b', 0 },
	{ "LineEdit", "context_menu_enabled", 'b', 1 },
	{ "LineEdit", "expand_to_text_length", 'b', 0 },
	{ "LineEdit", "flat", 'b', 0 },
	{ "TextEdit", "placeholder_text", 'n', 0 },
	{ "TextEdit", "wrap_mode", 'i', 0 },
	{ "TextEdit", "context_menu_enabled", 'b', 1 },
	{ "TextEdit", "draw_tabs", 'b', 0 },
	{ "TextEdit", "draw_spaces", 'b', 0 },
	{ "TextEdit", "draw_control_chars", 'b', 0 },
	{ "TextEdit", "scroll_smooth", 'b', 0 },
	{ "TextEdit", "minimap_draw", 'b', 0 },
	// CodeEdit (a TextEdit on Godot 2)
	{ "TextEdit", "line_folding", 'b', 0 },
	{ "TextEdit", "gutters_draw_fold_gutter", 'b', 0 },
	{ "TextEdit", "gutters_draw_bookmarks", 'b', 0 },
	{ "TextEdit", "gutters_draw_breakpoints_gutter", 'b', 0 },
	{ "TextEdit", "gutters_draw_executing_lines", 'b', 0 },
	{ "TextEdit", "code_completion_enabled", 'b', 0 },
	{ "TextEdit", "indent_automatic", 'b', 0 },
	{ "TextEdit", "indent_size", 'i', 4 },
	{ "TextEdit", "indent_use_spaces", 'b', 0 },
	{ "TextEdit", "auto_brace_completion_enabled", 'b', 0 },
	{ "TextEdit", "auto_brace_completion_highlight_matching", 'b', 0 },
	{ "RichTextLabel", "fit_content", 'b', 0 },
	{ "RichTextLabel", "autowrap_mode", 'i', 3 },
	{ "RichTextLabel", "context_menu_enabled", 'b', 0 },
	{ "RichTextLabel", "threaded", 'b', 0 },
	{ "Tree", "enable_recursive_folding", 'b', 1 },
	{ "Tree", "scroll_horizontal_enabled", 'b', 1 },
	{ "Tree", "scroll_vertical_enabled", 'b', 1 },
	{ "Tree", "auto_tooltip", 'b', 1 },
	{ "ItemList", "auto_height", 'b', 0 },
	{ "ItemList", "text_overrun_behavior", 'i', 3 },
	{ "TabContainer", "drag_to_rearrange_enabled", 'b', 0 },
	{ "TabContainer", "clip_tabs", 'b', 1 },
	{ "SplitContainer", "drag_area_margin_begin", 'i', 0 },
	{ "SplitContainer", "drag_area_margin_end", 'i', 0 },
	{ "ScrollContainer", "follow_focus", 'b', 0 },
	{ "TextureFrame", "flip_h", 'b', 0 },
	{ "TextureFrame", "flip_v", 'b', 0 },
	{ "ProgressBar", "fill_mode", 'i', 0 },
	{ "ProgressBar", "indeterminate", 'b', 0 },
	{ "WindowDialog", "transient", 'b', 0 },
	{ "WindowDialog", "borderless", 'b', 0 },
	{ "WindowDialog", "unresizable", 'b', 0 },
	{ "WindowDialog", "always_on_top", 'b', 0 },
	{ "WindowDialog", "popup_window", 'b', 0 },
	{ "WindowDialog", "wrap_controls", 'b', 0 },
	{ "AcceptDialog", "dialog_close_on_escape", 'b', 1 },
	{ "AcceptDialog", "dialog_autowrap", 'b', 0 },
	{ "TreeItem", "visible", 'b', 1 },
	{ "TreeItem", "disable_folding", 'b', 0 },
	{ "Viewport", "content_scale_factor", 'f', 1 },
	{ "Viewport", "content_scale_mode", 'i', 0 },
	{ "Viewport", "content_scale_aspect", 'i', 0 },
	{ "Viewport", "borderless", 'b', 0 },
	{ "Viewport", "gui_embed_subwindows", 'b', 1 },
	{ "Popup", "content_scale_factor", 'f', 1 },
	{ "CanvasItem", "z_index", 'i', 0 },
	{ "CanvasItem", "self_modulate", 'c', 0 },
	{ "CanvasItem", "clip_children", 'i', 0 },
	{ "CanvasItem", "texture_filter", 'i', 0 },
	{ "CanvasItem", "texture_repeat", 'i', 0 },
	{ NULL, NULL, 0, 0 }
};

static const CallEntry calls[] = {
	// CharacterBody3D
	{ "KinematicBody", "move_and_slide", NULL, cb_move_and_slide },
	{ "KinematicBody", "move_and_collide", NULL, cb_move_and_collide },
	{ "KinematicBody", "test_move", NULL, cb_test_move },
	{ "KinematicBody", "is_on_floor", NULL, cb_is_on_floor },
	{ "KinematicBody", "is_on_floor_only", NULL, cb_is_on_floor_only },
	{ "KinematicBody", "is_on_wall", NULL, cb_is_on_wall },
	{ "KinematicBody", "is_on_wall_only", NULL, cb_is_on_wall_only },
	{ "KinematicBody", "is_on_ceiling", NULL, cb_is_on_ceiling },
	{ "KinematicBody", "is_on_ceiling_only", NULL, cb_is_on_ceiling_only },
	{ "KinematicBody", "get_floor_normal", NULL, cb_get_floor_normal },
	{ "KinematicBody", "get_wall_normal", NULL, cb_get_wall_normal },
	{ "KinematicBody", "get_floor_angle", NULL, cb_get_floor_angle },
	{ "KinematicBody", "get_last_motion", NULL, cb_get_last_motion },
	{ "KinematicBody", "get_real_velocity", NULL, cb_get_real_velocity },
	{ "KinematicBody", "get_platform_velocity", NULL, cb_get_platform_velocity },
	{ "KinematicBody", "get_platform_angular_velocity", NULL, return_zero },
	{ "KinematicBody", "get_slide_collision_count", NULL, cb_get_slide_collision_count },
	{ "KinematicBody", "get_slide_collision", NULL, cb_get_slide_collision },
	{ "KinematicBody", "get_last_slide_collision", NULL, cb_get_last_slide_collision },
	{ "KinematicBody", "apply_floor_snap", NULL, cb_apply_floor_snap },
	// RigidBody3D
	{ "RigidBody", "apply_impulse", NULL, rb_apply_impulse },
	{ "RigidBody", "apply_central_impulse", NULL, rb_apply_central_impulse },
	{ "RigidBody", "apply_force", NULL, rb_apply_force },
	{ "RigidBody", "apply_central_force", NULL, rb_apply_central_force },
	{ "RigidBody", "apply_torque", NULL, rb_apply_torque },
	{ "RigidBody", "apply_torque_impulse", NULL, rb_apply_torque_impulse },
	{ "RigidBody", "get_contact_count", NULL, rb_get_contact_count },
	{ "RigidBody", "get_inverse_inertia_tensor", NULL, rb_get_inverse_inertia_tensor },
	// PhysicsBody3D
	{ "PhysicsBody", "get_collision_exceptions", NULL, pb_get_collision_exceptions },
	{ "PhysicsBody", "get_gravity", NULL, pb_get_gravity },
	// CollisionObject3D
	{ "CollisionObject", "get_collision_layer_value", NULL, co_get_layer_value },
	{ "CollisionObject", "set_collision_layer_value", NULL, co_set_layer_value },
	{ "CollisionObject", "get_collision_mask_value", NULL, co_get_mask_value },
	{ "CollisionObject", "set_collision_mask_value", NULL, co_set_mask_value },
	{ "CollisionObject", "create_shape_owner", NULL, co_create_shape_owner },
	{ "CollisionObject", "get_shape_owners", NULL, co_get_shape_owners },
	{ "CollisionObject", "remove_shape_owner", NULL, co_remove_shape_owner },
	{ "CollisionObject", "shape_find_owner", NULL, co_shape_find_owner },
	{ "CollisionObject", "shape_owner_add_shape", NULL, co_shape_owner_add_shape },
	{ "CollisionObject", "shape_owner_clear_shapes", NULL, co_shape_owner_clear_shapes },
	{ "CollisionObject", "shape_owner_get_shape", NULL, co_shape_owner_get_shape },
	{ "CollisionObject", "shape_owner_get_shape_count", NULL, co_shape_owner_get_shape_count },
	{ "CollisionObject", "shape_owner_get_shape_index", NULL, co_shape_owner_get_shape_index },
	{ "CollisionObject", "shape_owner_get_transform", NULL, co_shape_owner_get_transform },
	{ "CollisionObject", "shape_owner_remove_shape", NULL, co_shape_owner_remove_shape },
	{ "CollisionObject", "shape_owner_set_disabled", NULL, co_shape_owner_set_disabled },
	{ "CollisionObject", "shape_owner_set_transform", NULL, co_shape_owner_set_transform },
	{ "CollisionObject", "is_shape_owner_disabled", NULL, co_is_shape_owner_disabled },
	// RayCast3D
	{ "RayCast", "get_collision_mask_value", NULL, ray_get_mask_value },
	{ "RayCast", "set_collision_mask_value", NULL, ray_set_mask_value },
	{ "RayCast", "get_collision_face_index", NULL, return_minus_one },
	// Camera3D
	{ "Camera", "get_cull_mask_value", NULL, cam_get_cull_mask_value },
	{ "Camera", "set_cull_mask_value", NULL, cam_set_cull_mask_value },
	{ "Camera", "set_orthogonal", NULL, cam_set_orthogonal },
	{ "Camera", "get_frustum", NULL, return_array },
	// Lights
	{ "Light", "get_param", NULL, light_get_param },
	{ "Light", "set_param", NULL, light_set_param },
	{ "Light", "get_correlated_color", NULL, light_get_correlated_color },
	// MeshInstance3D, ArrayMesh
	{ "MeshInstance", "get_surface_override_count", NULL, mi_get_surface_override_count },
	{ "MeshInstance", "get_surface_override_material", NULL, mi_get_surface_override_material },
	{ "MeshInstance", "set_surface_override_material", NULL, mi_set_surface_override_material },
	{ "MeshInstance", "get_blend_shape_count", NULL, mi_get_blend_shape_count },
	{ "MeshInstance", "find_blend_shape_by_name", NULL, mi_find_blend_shape_by_name },
	{ "MeshInstance", "get_blend_shape_value", NULL, mi_get_blend_shape_value },
	{ "MeshInstance", "set_blend_shape_value", NULL, mi_set_blend_shape_value },
	{ "MeshInstance", "create_debug_tangents", NULL, noop },
	{ "Mesh", "add_surface_from_arrays", NULL, mesh_add_surface_from_arrays },
	{ "Mesh", "add_blend_shape", NULL, mesh_add_blend_shape },
	{ "Mesh", "get_blend_shape_count", "get_morph_target_count", NULL },
	{ "Mesh", "get_blend_shape_name", "get_morph_target_name", NULL },
	{ "Mesh", "clear_blend_shapes", "clear_morph_targets", NULL },
	// Skeleton3D
	{ "Skeleton", "get_bone_pose_position", NULL, skel_get_bone_pose_position },
	{ "Skeleton", "get_bone_pose_rotation", NULL, skel_get_bone_pose_rotation },
	{ "Skeleton", "get_bone_pose_scale", NULL, skel_get_bone_pose_scale },
	{ "Skeleton", "set_bone_pose_position", NULL, skel_set_bone_pose_position },
	{ "Skeleton", "set_bone_pose_rotation", NULL, skel_set_bone_pose_rotation },
	{ "Skeleton", "set_bone_pose_scale", NULL, skel_set_bone_pose_scale },
	{ "Skeleton", "reset_bone_pose", NULL, skel_reset_bone_pose },
	{ "Skeleton", "reset_bone_poses", NULL, skel_reset_bone_poses },
	{ "Skeleton", "get_bone_children", NULL, skel_get_bone_children },
	{ "Skeleton", "get_parentless_bones", NULL, skel_get_parentless_bones },
	{ "Skeleton", "get_bone_global_rest", NULL, skel_get_bone_global_rest },
	{ "Skeleton", "get_bone_global_pose_no_override", "get_bone_global_pose", NULL },
	{ "Skeleton", "get_bone_global_pose_override", "get_bone_global_pose", NULL },
	{ "Skeleton", "set_bone_global_pose_override", NULL, skel_set_bone_global_pose_override },
	{ "Skeleton", "get_concatenated_bone_names", NULL, skel_get_concatenated_bone_names },
	{ "Skeleton", "is_bone_enabled", NULL, return_true },
	{ "Skeleton", "set_bone_enabled", NULL, noop },
	{ "Skeleton", "get_version", NULL, return_zero },
	{ "Skeleton", "force_update_all_bone_transforms", NULL, noop },
	{ "Skeleton", "force_update_bone_child_transform", NULL, noop },
	{ "Skeleton", "clear_bones_global_pose_override", NULL, noop },
	{ "Skeleton", "localize_rests", NULL, noop },
	{ "Skeleton", "advance", NULL, noop },
	// Node3D
	{ "Spatial", "look_at", NULL, spatial_look_at },
	{ "Spatial", "look_at_from_position", NULL, spatial_look_at_from_position },
	{ "Spatial", "force_update_transform", NULL, noop },
	{ "Spatial", "global_scale", NULL, spatial_global_scale },
	{ "Spatial", "is_scale_disabled", NULL, return_false },
	{ "Spatial", "set_disable_scale", NULL, noop },
	{ "Spatial", "is_top_level", NULL, spatial_is_top_level },
	{ "Spatial", "set_as_top_level", "set_as_toplevel", NULL },
	{ "Spatial", "is_visible_in_tree", NULL, spatial_is_visible_in_tree },
	{ "Spatial", "rotate_object_local", NULL, spatial_rotate_object_local },
	{ "Spatial", "scale_object_local", NULL, spatial_scale_object_local },
	{ "Spatial", "to_global", NULL, spatial_to_global },
	{ "Spatial", "to_local", NULL, spatial_to_local },
	// Node (CollisionShape registration)
	{ "CollisionObject", "add_child", NULL, node_add_child },
	{ "CollisionObject", "remove_child", NULL, node_remove_child },
	// Sprite2D
	{ "Sprite", "get_rect", NULL, sprite_get_rect },
	{ "Sprite", "is_pixel_opaque", NULL, return_true },
	// Camera2D
	{ "Camera2D", "get_screen_center_position", "get_camera_screen_center", NULL },
	{ "Camera2D", "get_target_position", "get_camera_pos", NULL },
	{ "Camera2D", "get_screen_rotation", NULL, return_zero },
	// Node2D
	{ "Node2D", "rotate", NULL, n2d_rotate },
	{ "Node2D", "apply_scale", NULL, n2d_apply_scale },
	{ "Node2D", "to_global", NULL, n2d_to_global },
	{ "Node2D", "to_local", NULL, n2d_to_local },
	{ "Node2D", "get_angle_to", NULL, n2d_get_angle_to },
	{ "Node2D", "look_at", NULL, n2d_look_at },
	// Control
	{ "Control", "get_anchor", NULL, ctl_get_anchor },
	{ "Control", "set_anchor", NULL, ctl_set_anchor },
	{ "Control", "get_offset", NULL, ctl_get_offset },
	{ "Control", "set_offset", NULL, ctl_set_offset },
	{ "Control", "set_anchor_and_offset", NULL, ctl_set_anchor_and_offset },
	{ "Control", "set_anchors_preset", NULL, ctl_set_anchors_preset },
	{ "Control", "set_offsets_preset", NULL, ctl_set_offsets_preset },
	{ "Control", "set_anchors_and_offsets_preset", NULL, ctl_set_anchors_and_offsets_preset },
	{ "Control", "set_position", "set_pos", NULL },
	{ "Control", "get_position", "get_pos", NULL },
	{ "Control", "set_global_position", "set_global_pos", NULL },
	{ "Control", "get_global_position", "get_global_pos", NULL },
	{ "Control", "get_screen_position", NULL, ctl_get_screen_position },
	{ "Control", "update_minimum_size", NULL, ctl_update_minimum_size },
	{ "Control", "add_theme_color_override", "add_color_override", NULL },
	{ "Control", "add_theme_constant_override", "add_constant_override", NULL },
	{ "Control", "add_theme_font_override", "add_font_override", NULL },
	{ "Control", "add_theme_icon_override", "add_icon_override", NULL },
	{ "Control", "add_theme_stylebox_override", "add_style_override", NULL },
	{ "Control", "add_theme_font_size_override", NULL, noop },
	{ "Control", "remove_theme_icon_override", NULL, ctl_remove_theme_icon_override },
	{ "Control", "remove_theme_stylebox_override", NULL, ctl_remove_theme_stylebox_override },
	{ "Control", "remove_theme_font_override", NULL, ctl_remove_theme_font_override },
	{ "Control", "remove_theme_color_override", NULL, noop },
	{ "Control", "remove_theme_constant_override", NULL, noop },
	{ "Control", "remove_theme_font_size_override", NULL, noop },
	{ "Control", "get_theme_color", "get_color", NULL },
	{ "Control", "get_theme_constant", "get_constant", NULL },
	{ "Control", "get_theme_font", "get_font", NULL },
	{ "Control", "get_theme_icon", "get_icon", NULL },
	{ "Control", "get_theme_stylebox", "get_stylebox", NULL },
	{ "Control", "get_theme_font_size", NULL, ctl_get_theme_font_size },
	{ "Control", "get_theme_default_font_size", NULL, ctl_get_theme_font_size },
	{ "Control", "has_theme_color", "has_color", NULL },
	{ "Control", "has_theme_constant", "has_constant", NULL },
	{ "Control", "has_theme_font", "has_font", NULL },
	{ "Control", "has_theme_icon", "has_icon", NULL },
	{ "Control", "has_theme_stylebox", "has_stylebox", NULL },
	{ "Control", "has_theme_font_size", NULL, return_false },
	{ "Control", "has_theme_color_override", "has_color_override", NULL },
	{ "Control", "has_theme_constant_override", "has_constant_override", NULL },
	{ "Control", "has_theme_font_override", "has_font_override", NULL },
	{ "Control", "has_theme_icon_override", "has_icon_override", NULL },
	{ "Control", "has_theme_stylebox_override", "has_stylebox_override", NULL },
	{ "Control", "has_theme_font_size_override", NULL, return_false },
	{ "Control", "begin_bulk_theme_override", NULL, noop },
	{ "Control", "end_bulk_theme_override", NULL, noop },
	{ "Control", "reset_size", NULL, noop },
	// CanvasItem
	{ "CanvasItem", "queue_redraw", "update", NULL },
	{ "CanvasItem", "is_visible_in_tree", "is_visible", NULL },
	{ "CanvasItem", "get_global_mouse_position", "get_global_mouse_pos", NULL },
	{ "CanvasItem", "get_local_mouse_position", "get_local_mouse_pos", NULL },
	// GUI widgets
	{ "RichTextLabel", "append_text", NULL, rtl_append_text },
	{ "RichTextLabel", "get_parsed_text", NULL, rtl_get_parsed_text },
	{ "PopupMenu", "add_item", NULL, popup_add_item },
	{ "PopupMenu", "add_icon_item", NULL, popup_add_icon_item },
	{ "PopupMenu", "add_check_item", NULL, popup_add_check_item },
	{ "PopupMenu", "add_separator", NULL, popup_add_separator },
	{ "PopupMenu", "set_item_as_checkable", NULL, popup_set_item_checkable },
	{ "PopupMenu", "set_item_id", NULL, popup_set_item_id },
	{ "PopupMenu", "get_item_id", NULL, popup_get_item_id },
	{ "PopupMenu", "set_item_as_separator", NULL, popup_set_item_as_separator },
	{ "OptionButton", "add_separator", NULL, popup_add_separator },
	{ "OptionButton", "get_popup", NULL, option_get_popup },
	{ "OptionButton", "set_item_id", NULL, popup_set_item_id },
	{ "OptionButton", "get_item_id", NULL, popup_get_item_id },
	{ "OptionButton", "get_selected_id", "get_selected_ID", NULL },
	{ "TextEdit", "get_caret_line", NULL, textedit_get_caret_line },
	{ "TextEdit", "get_caret_column", NULL, textedit_get_caret_column },
	{ "TextEdit", "set_caret_line", NULL, textedit_set_caret_line },
	{ "TextEdit", "set_caret_column", NULL, textedit_set_caret_column },
	{ "TextEdit", "get_selected_text", NULL, textedit_get_selected_text },
	{ "TextEdit", "has_selection", NULL, textedit_has_selection },
	{ "TextEdit", "insert_text_at_caret", "insert_text_at_cursor", NULL },
	{ "TextEdit", "get_word_under_caret", "get_word_under_cursor", NULL },
	{ "LineEdit", "insert_text_at_caret", "append_at_cursor", NULL },
	{ "ScrollContainer", "ensure_control_visible", NULL, noop },
	{ "TreeItem", "get_first_child", "get_children", NULL },
	{ "TreeItem", "get_children", NULL, treeitem_get_children },
	{ "TreeItem", "get_child_count", NULL, treeitem_get_child_count },
	{ "TreeItem", "get_child", NULL, treeitem_get_child },
	{ "TreeItem", "get_index", NULL, treeitem_get_index },
	{ "TreeItem", "get_next_in_tree", "get_next_visible", NULL },
	{ "TreeItem", "get_prev_in_tree", "get_prev_visible", NULL },
	// Input
	{ "Input", "is_key_pressed", NULL, input_is_key_pressed },
	{ "Input", "is_key_label_pressed", NULL, input_is_key_pressed },
	{ "Input", "is_physical_key_pressed", NULL, input_is_key_pressed },
	{ "Input", "is_joy_button_pressed", NULL, input_is_joy_button_pressed },
	{ "Input", "get_joy_axis", NULL, input_get_joy_axis },
	{ "Input", "get_action_strength", NULL, input_get_action_strength },
	{ "Input", "get_action_raw_strength", NULL, input_get_action_strength },
	{ "Input", "get_vector", NULL, input_get_vector },
	{ "Input", "get_axis", NULL, input_get_axis },
	{ "Input", "is_anything_pressed", NULL, return_false },
	// Object, Node, Viewport (as Godot 4's root Window)
	{ "Object", "get_property_list", NULL, obj_get_property_list },
	{ "Node", "get_window", NULL, node_get_window },
	{ "Node", "get_last_exclusive_window", NULL, node_get_window },
	{ "Viewport", "get_size_with_decorations", NULL, viewport_get_size_with_decorations },
	// Textures (Image values are wrapped as ImageRef in Lua)
	{ "ImageTexture", "set_image", "set_data", NULL },
	{ "ImageTexture", "update", "set_data", NULL },
	{ "Texture", "get_image", "get_data", NULL },
	{ NULL, NULL, NULL, NULL }
};

// (Godot 2 class, Godot 4 signal) -> Godot 2 signal
static const char *signal_aliases[][3] = {
	{ "PopupMenu", "id_pressed", "item_pressed" },
	{ "LineEdit", "text_submitted", "text_entered" },
	{ "Control", "focus_entered", "focus_enter" },
	{ "Control", "focus_exited", "focus_exit" },
	{ "Control", "mouse_entered", "mouse_enter" },
	{ "Control", "mouse_exited", "mouse_exit" },
	{ "Control", "gui_input", "input_event" },
	{ "TextEdit", "caret_changed", "cursor_changed" },
	{ "Popup", "popup_hide", "popup_hide" },
	{ "WindowDialog", "close_requested", "popup_hide" },
	{ "AcceptDialog", "canceled", "popup_hide" },
	{ "Tree", "item_mouse_selected", "cell_selected" },
	{ NULL, NULL, NULL }
};

/* Lookup (cached per class) */

static HashMap<String, int> *prop_cache = NULL;
static HashMap<String, int> *meta_cache = NULL;
static HashMap<String, int> *call_cache = NULL;

template <class T>
static int find_entry(HashMap<String, int> *&r_cache, const T *p_table, const Object *p_object, const String &p_name) {

	if (!r_cache)
		r_cache = memnew((HashMap<String, int>));
	String type = p_object->get_type();
	String key = type + ":" + p_name;
	const int *cached = r_cache->getptr(key);
	if (cached)
		return *cached;
	int found = -1;
	for (int i = 0; p_table[i].cls; i++) {
		if (p_name == p_table[i].name && ObjectTypeDB::is_type(type, p_table[i].cls)) {
			found = i;
			break;
		}
	}
	r_cache->set(key, found);
	return found;
}

static Variant meta_default(const MetaEntry &e) {
	switch (e.kind) {
		case 'f': return e.value;
		case 'i': return (int64_t)e.value;
		case 'b': return e.value != 0;
		case 'v': return Vector2();
		case 'w': return Vector3();
		case 'c': return Color(1, 1, 1);
		default: return Variant();
	}
}

static String meta_key(const String &p_name) {
	return "_g4_" + p_name;
}

Variant get(Object *p_object, const String &p_property) {

	int i = find_entry(prop_cache, props, p_object, p_property);
	if (i >= 0) {
		const PropEntry &e = props[i];
		if (e.get)
			return e.get(p_object);
		return call0(p_object, e.getter);
	}
	i = find_entry(meta_cache, meta_props, p_object, p_property);
	if (i >= 0) {
		String key = meta_key(p_property);
		return p_object->has_meta(key) ? p_object->get_meta(key) : meta_default(meta_props[i]);
	}

	bool valid = false;
	Variant v = p_object->get(p_property, &valid);
	if (valid)
		return v;
	if (p_object->has_method("get_" + p_property))
		return p_object->call("get_" + p_property);
	if (p_object->has_method("is_" + p_property))
		return p_object->call("is_" + p_property);
	return Variant();
}

void set(Object *p_object, const String &p_property, const Variant &p_value) {

	int i = find_entry(prop_cache, props, p_object, p_property);
	if (i >= 0) {
		const PropEntry &e = props[i];
		if (e.set)
			e.set(p_object, p_value);
		else if (e.setter)
			call1(p_object, e.setter, p_value);
		return;
	}
	i = find_entry(meta_cache, meta_props, p_object, p_property);
	if (i >= 0) {
		p_object->set_meta(meta_key(p_property), p_value);
		return;
	}

	bool valid = false;
	p_object->set(p_property, p_value, &valid);
	if (valid)
		return;
	if (p_object->has_method("set_" + p_property))
		call1(p_object, String("set_" + p_property).utf8().get_data(), p_value);
}

bool has_method(Object *p_object, const String &p_method) {

	return find_entry(call_cache, calls, p_object, p_method) >= 0 || p_object->has_method(p_method);
}

bool call(Object *p_object, const String &p_method, const Array &p_args, Variant &r_ret) {

	int i = find_entry(call_cache, calls, p_object, p_method);
	if (i >= 0) {
		const CallEntry &e = calls[i];
		r_ret = e.fn ? e.fn(p_object, p_args) : p_object->callv(e.target, p_args);
		return true;
	}
	if (!p_object->has_method(p_method))
		return false;
	r_ret = p_object->callv(p_method, p_args);
	return true;
}

String signal_name(const Object *p_object, const String &p_signal) {

	for (int i = 0; signal_aliases[i][0]; i++) {
		if (p_signal == signal_aliases[i][1] && p_object->is_type(signal_aliases[i][0]))
			return signal_aliases[i][2];
	}
	return p_signal;
}

static void manifest_add(Dictionary &r_out, const String &p_cls, const String &p_kind, const String &p_name) {
	if (!r_out.has(p_cls))
		r_out[p_cls] = Dictionary();
	// Godot 2 dictionaries are copy-on-write: modify, then store back.
	Dictionary c = to_dict(r_out[p_cls]);
	Array a = c.has(p_kind) ? to_array(c[p_kind]) : Array();
	a.push_back(p_name);
	c[p_kind] = a;
	r_out[p_cls] = c;
}

Dictionary manifest() {

	Dictionary out;
	for (int i = 0; props[i].cls; i++)
		manifest_add(out, props[i].cls, "properties", props[i].name);
	for (int i = 0; meta_props[i].cls; i++)
		manifest_add(out, meta_props[i].cls, "properties", meta_props[i].name);
	for (int i = 0; calls[i].cls; i++)
		manifest_add(out, calls[i].cls, "methods", calls[i].name);
	for (int i = 0; signal_aliases[i][0]; i++)
		manifest_add(out, signal_aliases[i][0], "signals", signal_aliases[i][1]);
	// Module classes standing in for Godot 4 classes implement Godot 4
	// methods under their own names.
	const char *backing[] = { "InputEventRef", "ImageRef", "CodeHighlighterRef", NULL };
	for (int i = 0; backing[i]; i++) {
		List<MethodInfo> methods;
		ObjectTypeDB::get_method_list(backing[i], &methods, true);
		for (List<MethodInfo>::Element *E = methods.front(); E; E = E->next())
			manifest_add(out, backing[i], "methods", E->get().name);
	}
	return out;
}

void cleanup() {

	if (prop_cache)
		memdelete(prop_cache);
	if (meta_cache)
		memdelete(meta_cache);
	if (call_cache)
		memdelete(call_cache);
	prop_cache = meta_cache = call_cache = NULL;
}

} // namespace compat
} // namespace sunaba
