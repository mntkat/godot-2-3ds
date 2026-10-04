/*************************************************************************/
/*  primitive_meshes.cpp                                                 */
/*************************************************************************/

#include "primitive_meshes.h"

#include "math_funcs.h"

/* Builder */

int PrimitiveMesh::Builder::add(const Vector3 &p_pos, const Vector3 &p_normal, const Vector2 &p_uv) {

	vertices.push_back(p_pos);
	normals.push_back(p_normal);
	uvs.push_back(p_uv);
	return vertices.size() - 1;
}

// Godot draws clockwise faces as front faces: seen from the outside,
// cross(b - a, c - a) must point into the surface. Orienting each
// triangle against its vertex normals keeps the generators simple.
void PrimitiveMesh::Builder::tri(int a, int b, int c) {

	Vector3 n = (vertices[b] - vertices[a]).cross(vertices[c] - vertices[a]);
	if (n.length_squared() < CMP_EPSILON * CMP_EPSILON)
		return; // degenerate (e.g. at a pole)
	Vector3 outward = normals[a] + normals[b] + normals[c];
	if (n.dot(outward) > 0)
		SWAP(b, c);
	indices.push_back(a);
	indices.push_back(b);
	indices.push_back(c);
}

void PrimitiveMesh::Builder::quad(int a, int b, int c, int d) {

	tri(a, b, c);
	tri(a, c, d);
}

/* PrimitiveMesh */

void PrimitiveMesh::_update() {

	Builder b;
	_build(b);

	while (get_surface_count() > 0)
		surface_remove(0);
	mesh_arrays = Array();
	if (b.indices.size() == 0)
		return;

	int count = b.vertices.size();
	DVector<Vector3> vertices, normals;
	DVector<real_t> tangents;
	DVector<Vector2> uvs, uv2s;
	DVector<int> indices;
	vertices.resize(count);
	normals.resize(count);
	tangents.resize(count * 4);
	uvs.resize(count);
	{
		DVector<Vector3>::Write vw = vertices.write();
		DVector<Vector3>::Write nw = normals.write();
		DVector<real_t>::Write tw = tangents.write();
		DVector<Vector2>::Write uw = uvs.write();
		for (int i = 0; i < count; i++) {
			Vector3 n = flip_faces ? -b.normals[i] : b.normals[i];
			vw[i] = b.vertices[i];
			nw[i] = n;
			uw[i] = b.uvs[i];
			Vector3 t = Vector3(0, 1, 0).cross(n);
			if (t.length_squared() < CMP_EPSILON)
				t = Vector3(1, 0, 0);
			t.normalize();
			tw[i * 4 + 0] = t.x;
			tw[i * 4 + 1] = t.y;
			tw[i * 4 + 2] = t.z;
			tw[i * 4 + 3] = 1.0;
		}
	}
	indices.resize(b.indices.size());
	{
		DVector<int>::Write iw = indices.write();
		for (int i = 0; i < b.indices.size(); i += 3) {
			iw[i] = b.indices[i];
			// flip_faces reverses the winding so the inside is visible.
			iw[i + 1] = flip_faces ? b.indices[i + 2] : b.indices[i + 1];
			iw[i + 2] = flip_faces ? b.indices[i + 1] : b.indices[i + 2];
		}
	}

	Array arrays;
	arrays.resize(ARRAY_MAX);
	arrays[ARRAY_VERTEX] = vertices;
	arrays[ARRAY_NORMAL] = normals;
	arrays[ARRAY_TANGENT] = tangents;
	arrays[ARRAY_TEX_UV] = uvs;
	if (add_uv2)
		arrays[ARRAY_TEX_UV2] = uvs;
	arrays[ARRAY_INDEX] = indices;
	mesh_arrays = arrays;
	add_surface(PRIMITIVE_TRIANGLES, arrays);
	if (material.is_valid())
		surface_set_material(0, material);
}

Array PrimitiveMesh::get_mesh_arrays() const {

	return mesh_arrays;
}

void PrimitiveMesh::set_material(const Ref<Material> &p_material) {

	material = p_material;
	if (get_surface_count() > 0)
		surface_set_material(0, material);
}

Ref<Material> PrimitiveMesh::get_material() const {

	return material;
}

void PrimitiveMesh::set_flip_faces(bool p_flip) {

	flip_faces = p_flip;
	_update();
}

bool PrimitiveMesh::get_flip_faces() const {

	return flip_faces;
}

void PrimitiveMesh::set_add_uv2(bool p_add) {

	add_uv2 = p_add;
	_update();
}

bool PrimitiveMesh::get_add_uv2() const {

	return add_uv2;
}

void PrimitiveMesh::set_uv2_padding(float p_padding) {

	uv2_padding = p_padding;
}

float PrimitiveMesh::get_uv2_padding() const {

	return uv2_padding;
}

void PrimitiveMesh::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("set_material", "material:Material"), &PrimitiveMesh::set_material);
	ObjectTypeDB::bind_method(_MD("get_material:Material"), &PrimitiveMesh::get_material);
	ObjectTypeDB::bind_method(_MD("set_flip_faces", "flip_faces"), &PrimitiveMesh::set_flip_faces);
	ObjectTypeDB::bind_method(_MD("get_flip_faces"), &PrimitiveMesh::get_flip_faces);
	ObjectTypeDB::bind_method(_MD("set_add_uv2", "add_uv2"), &PrimitiveMesh::set_add_uv2);
	ObjectTypeDB::bind_method(_MD("get_add_uv2"), &PrimitiveMesh::get_add_uv2);
	ObjectTypeDB::bind_method(_MD("set_uv2_padding", "uv2_padding"), &PrimitiveMesh::set_uv2_padding);
	ObjectTypeDB::bind_method(_MD("get_uv2_padding"), &PrimitiveMesh::get_uv2_padding);
	ObjectTypeDB::bind_method(_MD("get_mesh_arrays"), &PrimitiveMesh::get_mesh_arrays);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), _SCS("set_material"), _SCS("get_material"));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_faces"), _SCS("set_flip_faces"), _SCS("get_flip_faces"));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "add_uv2"), _SCS("set_add_uv2"), _SCS("get_add_uv2"));
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "uv2_padding"), _SCS("set_uv2_padding"), _SCS("get_uv2_padding"));
}

PrimitiveMesh::PrimitiveMesh() {

	flip_faces = false;
	add_uv2 = false;
	uv2_padding = 2.0;
}

#define PRIMITIVE_PROPERTY_T(m_class, m_set_type, m_get_type, m_name) \
	void m_class::set_##m_name(m_set_type p_value) {                    \
		m_name = p_value;                                               \
		_update();                                                      \
	}                                                                   \
	m_get_type m_class::get_##m_name() const { return m_name; }
#define PRIMITIVE_PROPERTY(m_class, m_type, m_name, m_vtype) PRIMITIVE_PROPERTY_T(m_class, m_type, m_type, m_name)

#define BIND_PRIMITIVE_PROPERTY(m_class, m_name, m_vtype)                                          \
	ObjectTypeDB::bind_method(_MD("set_" #m_name, #m_name), &m_class::set_##m_name);               \
	ObjectTypeDB::bind_method(_MD("get_" #m_name), &m_class::get_##m_name);                        \
	ADD_PROPERTY(PropertyInfo(m_vtype, #m_name), _SCS("set_" #m_name), _SCS("get_" #m_name));

/* BoxMesh */

void BoxMesh::_build(Builder &b) const {

	Vector3 h = size * 0.5;
	// Each face: normal and two in-plane axes. Subdivisions are a grid.
	static const Vector3 normals[6] = { Vector3(1, 0, 0), Vector3(-1, 0, 0), Vector3(0, 1, 0), Vector3(0, -1, 0), Vector3(0, 0, 1), Vector3(0, 0, -1) };
	static const Vector3 us[6] = { Vector3(0, 0, -1), Vector3(0, 0, 1), Vector3(1, 0, 0), Vector3(1, 0, 0), Vector3(1, 0, 0), Vector3(-1, 0, 0) };
	static const Vector3 vs[6] = { Vector3(0, -1, 0), Vector3(0, -1, 0), Vector3(0, 0, 1), Vector3(0, 0, -1), Vector3(0, -1, 0), Vector3(0, -1, 0) };
	for (int f = 0; f < 6; f++) {
		Vector3 n = normals[f], u = us[f], v = vs[f];
		int du = (f < 2 ? subdivide_depth : subdivide_width) + 1;
		int dv = (f == 2 || f == 3 ? subdivide_depth : subdivide_height) + 1;
		Vector3 hn = n * h.dot(Vector3(Math::abs(n.x), Math::abs(n.y), Math::abs(n.z)));
		float su = h.dot(Vector3(Math::abs(u.x), Math::abs(u.y), Math::abs(u.z)));
		float sv = h.dot(Vector3(Math::abs(v.x), Math::abs(v.y), Math::abs(v.z)));
		int base = b.vertices.size();
		for (int j = 0; j <= dv; j++) {
			for (int i = 0; i <= du; i++) {
				float fu = (float)i / du, fv = (float)j / dv;
				Vector3 p = hn + u * (fu * 2 - 1) * su + v * (fv * 2 - 1) * sv;
				b.add(p, n, Vector2(fu, fv));
			}
		}
		for (int j = 0; j < dv; j++) {
			for (int i = 0; i < du; i++) {
				int a = base + j * (du + 1) + i;
				b.quad(a, a + 1, a + du + 2, a + du + 1);
			}
		}
	}
}

PRIMITIVE_PROPERTY_T(BoxMesh, const Vector3 &, Vector3, size)
PRIMITIVE_PROPERTY(BoxMesh, int, subdivide_width, Variant::INT)
PRIMITIVE_PROPERTY(BoxMesh, int, subdivide_height, Variant::INT)
PRIMITIVE_PROPERTY(BoxMesh, int, subdivide_depth, Variant::INT)

void BoxMesh::_bind_methods() {

	BIND_PRIMITIVE_PROPERTY(BoxMesh, size, Variant::VECTOR3)
	BIND_PRIMITIVE_PROPERTY(BoxMesh, subdivide_width, Variant::INT)
	BIND_PRIMITIVE_PROPERTY(BoxMesh, subdivide_height, Variant::INT)
	BIND_PRIMITIVE_PROPERTY(BoxMesh, subdivide_depth, Variant::INT)
}

BoxMesh::BoxMesh() {

	size = Vector3(1, 1, 1);
	subdivide_width = subdivide_height = subdivide_depth = 0;
	_update();
}

/* Shared: rings of a surface of revolution around Y */

// Adds one ring of `segments` + 1 vertices (the seam is duplicated for UVs).
static int add_ring(PrimitiveMesh::Builder &b, int segments, float y, float radius, float ny_scale, float v) {

	int base = b.vertices.size();
	for (int i = 0; i <= segments; i++) {
		float u = (float)i / segments;
		float phi = u * Math_PI * 2.0;
		Vector3 dir(Math::sin(phi), 0, Math::cos(phi));
		Vector3 n = Vector3(dir.x, ny_scale, dir.z).normalized();
		b.add(Vector3(dir.x * radius, y, dir.z * radius), n, Vector2(u, v));
	}
	return base;
}

static void connect_rings(PrimitiveMesh::Builder &b, int ring_a, int ring_b, int segments) {

	for (int i = 0; i < segments; i++)
		b.quad(ring_a + i, ring_a + i + 1, ring_b + i + 1, ring_b + i);
}

/* SphereMesh */

void SphereMesh::_build(Builder &b) const {

	int segments = MAX(radial_segments, 3);
	int bands = MAX(rings, 1) + 1;
	float half = height * 0.5;
	float end = is_hemisphere ? Math_PI * 0.5 : Math_PI;
	int prev = -1;
	for (int j = 0; j <= bands; j++) {
		float v = (float)j / bands;
		float theta = v * end;
		float ring_radius = Math::sin(theta) * radius;
		float y = Math::cos(theta) * half;
		// Ellipsoid normal: scale the Y component by radius / half-height.
		float ny = Math::cos(theta) * (half > 0 ? radius / half : 0);
		float nr = Math::sin(theta);
		int ring = add_ring(b, segments, y, ring_radius, nr > CMP_EPSILON ? ny / nr : (ny >= 0 ? 1e6 : -1e6), v);
		if (prev >= 0)
			connect_rings(b, prev, ring, segments);
		prev = ring;
	}
	if (is_hemisphere) {
		int center = b.add(Vector3(), Vector3(0, -1, 0), Vector2(0.5, 0.5));
		int rim = add_ring(b, segments, 0, radius, -1e6, 1.0);
		for (int i = 0; i < segments; i++)
			b.tri(center, rim + i, rim + i + 1);
	}
}

PRIMITIVE_PROPERTY(SphereMesh, float, radius, Variant::REAL)
PRIMITIVE_PROPERTY(SphereMesh, float, height, Variant::REAL)
PRIMITIVE_PROPERTY(SphereMesh, int, radial_segments, Variant::INT)
PRIMITIVE_PROPERTY(SphereMesh, int, rings, Variant::INT)
PRIMITIVE_PROPERTY(SphereMesh, bool, is_hemisphere, Variant::BOOL)

void SphereMesh::_bind_methods() {

	BIND_PRIMITIVE_PROPERTY(SphereMesh, radius, Variant::REAL)
	BIND_PRIMITIVE_PROPERTY(SphereMesh, height, Variant::REAL)
	BIND_PRIMITIVE_PROPERTY(SphereMesh, radial_segments, Variant::INT)
	BIND_PRIMITIVE_PROPERTY(SphereMesh, rings, Variant::INT)
	BIND_PRIMITIVE_PROPERTY(SphereMesh, is_hemisphere, Variant::BOOL)
}

SphereMesh::SphereMesh() {

	radius = 0.5;
	height = 1.0;
	radial_segments = 64;
	rings = 32;
	is_hemisphere = false;
	_update();
}

/* CapsuleMesh */

void CapsuleMesh::_build(Builder &b) const {

	int segments = MAX(radial_segments, 3);
	int cap_bands = MAX(rings, 1) + 1;
	float mid = MAX(height - radius * 2, 0) * 0.5;
	int prev = -1;
	// Top hemisphere, then bottom; the two equator rings form the side.
	for (int half = 0; half < 2; half++) {
		for (int j = 0; j <= cap_bands; j++) {
			float t = (float)j / cap_bands;
			float theta = half == 0 ? t * Math_PI * 0.5 : Math_PI * 0.5 + t * Math_PI * 0.5;
			float y = Math::cos(theta) * radius + (half == 0 ? mid : -mid);
			float nr = Math::sin(theta);
			float ny = Math::cos(theta);
			float v = (y + height * 0.5) / MAX(height, CMP_EPSILON);
			int ring = add_ring(b, segments, y, nr * radius, nr > CMP_EPSILON ? ny / nr : (ny >= 0 ? 1e6 : -1e6), 1.0 - v);
			if (prev >= 0)
				connect_rings(b, prev, ring, segments);
			prev = ring;
		}
	}
}

PRIMITIVE_PROPERTY(CapsuleMesh, float, radius, Variant::REAL)
PRIMITIVE_PROPERTY(CapsuleMesh, float, height, Variant::REAL)
PRIMITIVE_PROPERTY(CapsuleMesh, int, radial_segments, Variant::INT)
PRIMITIVE_PROPERTY(CapsuleMesh, int, rings, Variant::INT)

void CapsuleMesh::_bind_methods() {

	BIND_PRIMITIVE_PROPERTY(CapsuleMesh, radius, Variant::REAL)
	BIND_PRIMITIVE_PROPERTY(CapsuleMesh, height, Variant::REAL)
	BIND_PRIMITIVE_PROPERTY(CapsuleMesh, radial_segments, Variant::INT)
	BIND_PRIMITIVE_PROPERTY(CapsuleMesh, rings, Variant::INT)
}

CapsuleMesh::CapsuleMesh() {

	radius = 0.5;
	height = 2.0;
	radial_segments = 64;
	rings = 8;
	_update();
}

/* CylinderMesh */

void CylinderMesh::_build(Builder &b) const {

	int segments = MAX(radial_segments, 3);
	int bands = MAX(rings, 0) + 1;
	float half = height * 0.5;
	// Side normals tilt with the slope between the two radii.
	float slope = height > CMP_EPSILON ? (bottom_radius - top_radius) / height : 0;
	int prev = -1;
	for (int j = 0; j <= bands; j++) {
		float t = (float)j / bands;
		float r = top_radius + (bottom_radius - top_radius) * t;
		int ring = add_ring(b, segments, half - height * t, r, slope, t);
		if (prev >= 0)
			connect_rings(b, prev, ring, segments);
		prev = ring;
	}
	for (int cap = 0; cap < 2; cap++) {
		if ((cap == 0 && (!cap_top || top_radius <= 0)) || (cap == 1 && (!cap_bottom || bottom_radius <= 0)))
			continue;
		float y = cap == 0 ? half : -half;
		float r = cap == 0 ? top_radius : bottom_radius;
		float ny = cap == 0 ? 1e6 : -1e6;
		int center = b.add(Vector3(0, y, 0), Vector3(0, cap == 0 ? 1 : -1, 0), Vector2(0.5, 0.5));
		int rim = add_ring(b, segments, y, r, ny, cap);
		for (int i = 0; i < segments; i++)
			b.tri(center, rim + i, rim + i + 1);
	}
}

PRIMITIVE_PROPERTY(CylinderMesh, float, top_radius, Variant::REAL)
PRIMITIVE_PROPERTY(CylinderMesh, float, bottom_radius, Variant::REAL)
PRIMITIVE_PROPERTY(CylinderMesh, float, height, Variant::REAL)
PRIMITIVE_PROPERTY(CylinderMesh, int, radial_segments, Variant::INT)
PRIMITIVE_PROPERTY(CylinderMesh, int, rings, Variant::INT)

void CylinderMesh::set_cap_top(bool p_cap) {

	cap_top = p_cap;
	_update();
}

bool CylinderMesh::is_cap_top() const {

	return cap_top;
}

void CylinderMesh::set_cap_bottom(bool p_cap) {

	cap_bottom = p_cap;
	_update();
}

bool CylinderMesh::is_cap_bottom() const {

	return cap_bottom;
}

void CylinderMesh::_bind_methods() {

	BIND_PRIMITIVE_PROPERTY(CylinderMesh, top_radius, Variant::REAL)
	BIND_PRIMITIVE_PROPERTY(CylinderMesh, bottom_radius, Variant::REAL)
	BIND_PRIMITIVE_PROPERTY(CylinderMesh, height, Variant::REAL)
	BIND_PRIMITIVE_PROPERTY(CylinderMesh, radial_segments, Variant::INT)
	BIND_PRIMITIVE_PROPERTY(CylinderMesh, rings, Variant::INT)
	ObjectTypeDB::bind_method(_MD("set_cap_top", "cap_top"), &CylinderMesh::set_cap_top);
	ObjectTypeDB::bind_method(_MD("is_cap_top"), &CylinderMesh::is_cap_top);
	ObjectTypeDB::bind_method(_MD("set_cap_bottom", "cap_bottom"), &CylinderMesh::set_cap_bottom);
	ObjectTypeDB::bind_method(_MD("is_cap_bottom"), &CylinderMesh::is_cap_bottom);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cap_top"), _SCS("set_cap_top"), _SCS("is_cap_top"));
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cap_bottom"), _SCS("set_cap_bottom"), _SCS("is_cap_bottom"));
}

CylinderMesh::CylinderMesh() {

	top_radius = 0.5;
	bottom_radius = 0.5;
	height = 2.0;
	radial_segments = 64;
	rings = 4;
	cap_top = true;
	cap_bottom = true;
	_update();
}

/* PlaneMesh */

void PlaneMesh::_build(Builder &b) const {

	Vector3 n, u, v;
	switch (orientation) {
		case FACE_X:
			n = Vector3(1, 0, 0);
			u = Vector3(0, 0, -1);
			v = Vector3(0, -1, 0);
			break;
		case FACE_Y:
			n = Vector3(0, 1, 0);
			u = Vector3(1, 0, 0);
			v = Vector3(0, 0, 1);
			break;
		default:
			n = Vector3(0, 0, 1);
			u = Vector3(1, 0, 0);
			v = Vector3(0, -1, 0);
			break;
	}
	int du = subdivide_width + 1, dv = subdivide_depth + 1;
	for (int j = 0; j <= dv; j++) {
		for (int i = 0; i <= du; i++) {
			float fu = (float)i / du, fv = (float)j / dv;
			Vector3 p = center_offset + u * (fu - 0.5) * size.x + v * (fv - 0.5) * size.y;
			b.add(p, n, Vector2(fu, fv));
		}
	}
	for (int j = 0; j < dv; j++) {
		for (int i = 0; i < du; i++) {
			int a = j * (du + 1) + i;
			b.quad(a, a + 1, a + du + 2, a + du + 1);
		}
	}
}

PRIMITIVE_PROPERTY_T(PlaneMesh, const Vector2 &, Vector2, size)
PRIMITIVE_PROPERTY(PlaneMesh, int, subdivide_width, Variant::INT)
PRIMITIVE_PROPERTY(PlaneMesh, int, subdivide_depth, Variant::INT)
PRIMITIVE_PROPERTY_T(PlaneMesh, const Vector3 &, Vector3, center_offset)

void PlaneMesh::set_orientation(int p_orientation) {

	orientation = (Orientation)CLAMP(p_orientation, 0, 2);
	_update();
}

int PlaneMesh::get_orientation() const {

	return orientation;
}

void PlaneMesh::_bind_methods() {

	BIND_PRIMITIVE_PROPERTY(PlaneMesh, size, Variant::VECTOR2)
	BIND_PRIMITIVE_PROPERTY(PlaneMesh, subdivide_width, Variant::INT)
	BIND_PRIMITIVE_PROPERTY(PlaneMesh, subdivide_depth, Variant::INT)
	BIND_PRIMITIVE_PROPERTY(PlaneMesh, center_offset, Variant::VECTOR3)
	ObjectTypeDB::bind_method(_MD("set_orientation", "orientation"), &PlaneMesh::set_orientation);
	ObjectTypeDB::bind_method(_MD("get_orientation"), &PlaneMesh::get_orientation);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "orientation", PROPERTY_HINT_ENUM, "Face X,Face Y,Face Z"), _SCS("set_orientation"), _SCS("get_orientation"));

	BIND_CONSTANT(FACE_X);
	BIND_CONSTANT(FACE_Y);
	BIND_CONSTANT(FACE_Z);
}

PlaneMesh::PlaneMesh() {

	size = Vector2(2, 2);
	subdivide_width = subdivide_depth = 0;
	orientation = FACE_Y;
	_update();
}

QuadMesh::QuadMesh() {

	set_size(Vector2(1, 1));
	set_orientation(FACE_Z);
}
