/*************************************************************************/
/*  primitive_meshes.h                                                   */
/*************************************************************************/
/*  Godot 4's primitive mesh resources (BoxMesh, SphereMesh, ...), which */
/*  Godot 2 lacks, as Mesh subclasses with Godot 4 property names and    */
/*  defaults. The single surface is rebuilt whenever a property changes. */
/*************************************************************************/

#ifndef SUNABA_PRIMITIVE_MESHES_H
#define SUNABA_PRIMITIVE_MESHES_H

#include "scene/resources/mesh.h"

class PrimitiveMesh : public Mesh {
	OBJ_TYPE(PrimitiveMesh, Mesh);

	Ref<Material> material;
	bool flip_faces;
	bool add_uv2;
	float uv2_padding;
	Array mesh_arrays;

public:
	struct Builder {
		Vector<Vector3> vertices;
		Vector<Vector3> normals;
		Vector<Vector2> uvs;
		Vector<int> indices;

		int add(const Vector3 &p_pos, const Vector3 &p_normal, const Vector2 &p_uv);
		// Adds a triangle facing the outside of the surface (the side its
		// vertex normals point to); degenerate triangles are skipped.
		void tri(int a, int b, int c);
		void quad(int a, int b, int c, int d);
	};

protected:
	virtual void _build(Builder &r_builder) const = 0;
	void _update();
	static void _bind_methods();

public:
	void set_material(const Ref<Material> &p_material);
	Ref<Material> get_material() const;
	void set_flip_faces(bool p_flip);
	bool get_flip_faces() const;
	void set_add_uv2(bool p_add);
	bool get_add_uv2() const;
	void set_uv2_padding(float p_padding);
	float get_uv2_padding() const;
	// Godot 4 API: the generated arrays (Godot 2 Mesh::ARRAY_* layout).
	Array get_mesh_arrays() const;

	PrimitiveMesh();
};

class BoxMesh : public PrimitiveMesh {
	OBJ_TYPE(BoxMesh, PrimitiveMesh);

	Vector3 size;
	int subdivide_width, subdivide_height, subdivide_depth;

protected:
	virtual void _build(Builder &r_builder) const;
	static void _bind_methods();

public:
	void set_size(const Vector3 &p_size);
	Vector3 get_size() const;
	void set_subdivide_width(int p_divs);
	int get_subdivide_width() const;
	void set_subdivide_height(int p_divs);
	int get_subdivide_height() const;
	void set_subdivide_depth(int p_divs);
	int get_subdivide_depth() const;

	BoxMesh();
};

class SphereMesh : public PrimitiveMesh {
	OBJ_TYPE(SphereMesh, PrimitiveMesh);

	float radius, height;
	int radial_segments, rings;
	bool is_hemisphere;

protected:
	virtual void _build(Builder &r_builder) const;
	static void _bind_methods();

public:
	void set_radius(float p_radius);
	float get_radius() const;
	void set_height(float p_height);
	float get_height() const;
	void set_radial_segments(int p_segments);
	int get_radial_segments() const;
	void set_rings(int p_rings);
	int get_rings() const;
	void set_is_hemisphere(bool p_hemisphere);
	bool get_is_hemisphere() const;

	SphereMesh();
};

class CapsuleMesh : public PrimitiveMesh {
	OBJ_TYPE(CapsuleMesh, PrimitiveMesh);

	float radius, height;
	int radial_segments, rings;

protected:
	virtual void _build(Builder &r_builder) const;
	static void _bind_methods();

public:
	void set_radius(float p_radius);
	float get_radius() const;
	void set_height(float p_height);
	float get_height() const;
	void set_radial_segments(int p_segments);
	int get_radial_segments() const;
	void set_rings(int p_rings);
	int get_rings() const;

	CapsuleMesh();
};

class CylinderMesh : public PrimitiveMesh {
	OBJ_TYPE(CylinderMesh, PrimitiveMesh);

	float top_radius, bottom_radius, height;
	int radial_segments, rings;
	bool cap_top, cap_bottom;

protected:
	virtual void _build(Builder &r_builder) const;
	static void _bind_methods();

public:
	void set_top_radius(float p_radius);
	float get_top_radius() const;
	void set_bottom_radius(float p_radius);
	float get_bottom_radius() const;
	void set_height(float p_height);
	float get_height() const;
	void set_radial_segments(int p_segments);
	int get_radial_segments() const;
	void set_rings(int p_rings);
	int get_rings() const;
	void set_cap_top(bool p_cap);
	bool is_cap_top() const;
	void set_cap_bottom(bool p_cap);
	bool is_cap_bottom() const;

	CylinderMesh();
};

class PlaneMesh : public PrimitiveMesh {
	OBJ_TYPE(PlaneMesh, PrimitiveMesh);

public:
	enum Orientation {
		FACE_X,
		FACE_Y,
		FACE_Z,
	};

private:
	Vector2 size;
	int subdivide_width, subdivide_depth;
	Vector3 center_offset;
	Orientation orientation;

protected:
	virtual void _build(Builder &r_builder) const;
	static void _bind_methods();

public:
	void set_size(const Vector2 &p_size);
	Vector2 get_size() const;
	void set_subdivide_width(int p_divs);
	int get_subdivide_width() const;
	void set_subdivide_depth(int p_divs);
	int get_subdivide_depth() const;
	void set_center_offset(const Vector3 &p_offset);
	Vector3 get_center_offset() const;
	void set_orientation(int p_orientation);
	int get_orientation() const;

	PlaneMesh();
};

// Godot 4's QuadMesh: a 1x1 PlaneMesh facing +Z.
class QuadMesh : public PlaneMesh {
	OBJ_TYPE(QuadMesh, PlaneMesh);

protected:
	static void _bind_methods() {}

public:
	QuadMesh();
};

#endif // SUNABA_PRIMITIVE_MESHES_H
