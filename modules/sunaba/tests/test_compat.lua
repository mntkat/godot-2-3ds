-- Godot 4 API compatibility layer (godot4_compat.cpp, primitive meshes,
-- Godot 4 classes), exercised the way libsunaba's components use it:
-- NativeObject/NativeReference with Godot 4 class/property/method names.

local function check(cond, what)
  if not cond then error("check failed: " .. what, 2) end
end
local function near(a, b, eps) return math.abs(a - b) < (eps or 1e-3) end
local function vnear(v, x, y, z, eps)
  return near(v.x, x, eps) and near(v.y, y, eps) and (z == nil or near(v.z, z, eps))
end
local function args(...)
  local a = ArrayList.new()
  for _, v in ipairs({...}) do a:append(v) end
  return a
end
local function get(o, p) return o:get(p) end

-- Node3D: class alias, transforms in Godot 4 conventions
local n = NativeObject.new("Node3D")
check(n:isClass("Node3D") and n:getClass() == "Node3D", "Node3D alias")
n:set("position", Vector3.new(1, 2, 3))
check(vnear(get(n, "position"):asVector3(), 1, 2, 3), "Node3D.position")
n:set("rotation_degrees", Vector3.new(0, 90, 0))
check(vnear(get(n, "rotation_degrees"):asVector3(), 0, 90, 0), "Node3D.rotation_degrees")
check(vnear(get(n, "basis"):asBasis().x, 0, 0, -1), "Y rotation maps +X to -Z (Godot 4)")
n:set("scale", Vector3.new(2, 2, 2))
check(vnear(get(n, "scale"):asVector3(), 2, 2, 2) and vnear(get(n, "rotation_degrees"):asVector3(), 0, 90, 0), "scale keeps rotation")
check(vnear(n:call("to_global", args(Vector3.new(1, 0, 0))):asVector3(), 1, 2, 1), "Node3D.to_global")
check(vnear(n:call("to_local", args(Vector3.new(1, 2, 1))):asVector3(), 1, 0, 0), "Node3D.to_local")
n:set("rotation", Vector3.new(0.3, 0.5, 0.2))
check(vnear(get(n, "rotation"):asVector3(), 0.3, 0.5, 0.2), "YXZ Euler round trip")
n:set("visible", false)
check(get(n, "visible"):asBool() == false and n:call("is_hidden", ArrayList.new()):asBool(), "Node3D.visible")
n:set("visible", true)
check(get(n, "rotation_order"):asInt() == 2, "rotation_order is YXZ")
n:call("look_at_from_position", args(Vector3.new(0, 0, 0), Vector3.new(0, 0, -5)))
check(vnear(get(n, "global_position"):asVector3(), 0, 0, 0), "look_at_from_position (default up)")
check(n:hasMethod("rotate_object_local") and n:hasMethod("add_child"), "hasMethod sees compat and native methods")
n:free()

-- Camera3D
local cam = NativeObject.new("Camera3D")
cam:set("fov", 60.0)
cam:set("near", 0.5)
cam:set("far", 200.0)
check(near(get(cam, "fov"):asFloat(), 60) and near(get(cam, "near"):asFloat(), 0.5) and near(get(cam, "far"):asFloat(), 200), "Camera3D fov/near/far")
cam:set("size", 10.0)
cam:set("projection", 1)
check(get(cam, "projection"):asInt() == 1 and near(get(cam, "size"):asFloat(), 10), "Camera3D orthogonal")
cam:call("set_cull_mask_value", args(2, false))
check(not cam:call("get_cull_mask_value", args(2)):asBool() and cam:call("get_cull_mask_value", args(1)):asBool(), "Camera3D cull mask values")
check(get(cam, "doppler_tracking"):asInt() == 0, "unsupported property default")
cam:free()

-- Lights
local omni = NativeObject.new("OmniLight3D")
omni:set("omni_range", 7.0)
omni:set("light_energy", 2.0)
omni:set("light_color", Color.new(1, 0, 0))
omni:set("shadow_enabled", true)
check(near(get(omni, "omni_range"):asFloat(), 7) and near(get(omni, "light_energy"):asFloat(), 2), "OmniLight3D range/energy")
check(get(omni, "light_color"):asColor().g == 0 and get(omni, "shadow_enabled"):asBool(), "Light color/shadows")
check(near(get(omni, "light_temperature"):asFloat(), 6500), "metadata default")
omni:set("light_temperature", 3000.0)
check(near(get(omni, "light_temperature"):asFloat(), 3000), "metadata-backed property")
omni:call("set_param", args(4, 9.0)) -- Light3D.PARAM_RANGE
check(near(get(omni, "omni_range"):asFloat(), 9), "Light3D.set_param maps to Godot 2 parameters")
omni:free()
local spot = NativeObject.new("SpotLight3D")
spot:set("spot_angle", 30.0)
check(near(get(spot, "spot_angle"):asFloat(), 30), "SpotLight3D.spot_angle")
spot:free()

-- Primitive meshes: Godot 4 properties, and Godot's clockwise front faces
local function check_winding(mesh, what)
  local a = mesh:call("get_mesh_arrays", ArrayList.new()):asArrayList()
  local verts = a[0]:asVector3Array()
  local normals = a[1]:asVector3Array()
  local idx = a[8]:asIntArray()
  check(#idx > 0 and #idx % 3 == 0, what .. " has triangles")
  for i = 1, #idx, 3 do
    local ia, ib, ic = idx[i] + 1, idx[i + 1] + 1, idx[i + 2] + 1
    local va, vb, vc = verts[ia], verts[ib], verts[ic]
    local face = (vb - va):cross(vc - va)
    local out = normals[ia] + normals[ib] + normals[ic]
    if face:dot(out) >= 0 then error(what .. ": triangle " .. ((i + 2) // 3) .. " faces inward") end
  end
end
local box = NativeReference.new("BoxMesh")
box:set("size", Vector3.new(2, 4, 6))
check(vnear(get(box, "size"):asVector3(), 2, 4, 6) and box:call("get_surface_count", ArrayList.new()):asInt() == 1, "BoxMesh")
check_winding(box, "BoxMesh")
box:set("subdivide_width", 2)
check_winding(box, "BoxMesh subdivided")
for _, cls in ipairs({ "SphereMesh", "CapsuleMesh", "CylinderMesh", "PlaneMesh", "QuadMesh" }) do
  local m = NativeReference.new(cls)
  check(m:isValid() and m:isClass("PrimitiveMesh") and m:isClass("Mesh"), cls)
  check_winding(m, cls)
end
local hemi = NativeReference.new("SphereMesh")
hemi:set("is_hemisphere", true)
check_winding(hemi, "SphereMesh hemisphere")
local flipped = NativeReference.new("BoxMesh")
flipped:set("flip_faces", true)
check_winding(flipped, "BoxMesh flip_faces")
local cyl = NativeReference.new("CylinderMesh")
cyl:set("top_radius", 0.0)
check_winding(cyl, "CylinderMesh cone")

-- MeshInstance3D
local mi = NativeObject.new("MeshInstance3D")
mi:set("mesh", box)
check(mi:call("get_surface_override_count", ArrayList.new()):asInt() == 1, "MeshInstance3D surface count")
local mat = NativeReference.new("StandardMaterial3D")
mi:call("set_surface_override_material", args(0, mat))
check(not mi:call("get_surface_override_material", args(0)):asReference():isNull(), "surface override material")
mi:set("cast_shadow", 0)
check(get(mi, "cast_shadow"):asInt() == 0, "GeometryInstance3D.cast_shadow")
mi:free()

-- ArrayMesh with Godot 4 array layout
local am = NativeReference.new("ArrayMesh")
local arrays = ArrayList.new()
arrays:resize(13)
arrays:set(0, Variant.fromVector3Array({ Vector3.new(0, 0, 0), Vector3.new(1, 0, 0), Vector3.new(0, 1, 0) }))
am:call("add_surface_from_arrays", args(3, arrays)) -- PRIMITIVE_TRIANGLES
check(am:call("get_surface_count", ArrayList.new()):asInt() == 1 and am:isClass("ArrayMesh"), "ArrayMesh.add_surface_from_arrays")
check(am:call("surface_get_primitive_type", args(0)):asInt() == 4, "Godot 4 TRIANGLES -> Godot 2 TRIANGLES")

-- Shapes
local bs = NativeReference.new("BoxShape3D")
bs:set("size", Vector3.new(2, 4, 6))
check(vnear(get(bs, "size"):asVector3(), 2, 4, 6) and vnear(get(bs, "extents"):asVector3(), 1, 2, 3), "BoxShape3D size <-> extents")
local caps = NativeReference.new("CapsuleShape3D")
caps:set("radius", 0.5)
caps:set("height", 2.0)
check(near(get(caps, "height"):asFloat(), 2) and near(caps:call("get_height", ArrayList.new()):asFloat(), 1), "CapsuleShape3D height includes caps")
local cs = NativeReference.new("CylinderShape3D")
cs:set("radius", 1.0)
check(near(get(cs, "radius"):asFloat(), 1) and cs:isClass("CylinderShape3D"), "CylinderShape3D")

-- CollisionShape3D: Godot 4 transform hides the capsule fix-up
local col = NativeObject.new("CollisionShape3D")
col:set("position", Vector3.new(0, 1, 0))
col:set("shape", caps)
check(vnear(get(col, "rotation_degrees"):asVector3(), 0, 0, 0) and vnear(get(col, "position"):asVector3(), 0, 1, 0), "capsule collision shape keeps Godot 4 transform")
local raw = col:call("get_transform", ArrayList.new()):asTransform3D()
check(vnear(raw.basis.z, 0, 1, 0, 1e-3) or vnear(raw.basis.z, 0, -1, 0, 1e-3), "Godot 2 capsule rotated to stand along Y")
col:free()

-- RigidBody3D
local rb = NativeObject.new("RigidBody3D")
rb:set("freeze", true)
check(get(rb, "freeze"):asBool() and rb:call("get_mode", ArrayList.new()):asInt() == 1, "RigidBody3D.freeze (static)")
rb:set("freeze_mode", 1)
check(rb:call("get_mode", ArrayList.new()):asInt() == 3, "RigidBody3D.freeze_mode kinematic")
rb:set("freeze", false)
rb:set("lock_rotation", true)
check(rb:call("get_mode", ArrayList.new()):asInt() == 2, "RigidBody3D.lock_rotation")
local pm = NativeReference.new("PhysicsMaterial")
pm:set("friction", 0.3)
pm:set("bounce", 0.6)
rb:set("physics_material_override", pm)
check(near(rb:call("get_friction", ArrayList.new()):asFloat(), 0.3) and near(rb:call("get_bounce", ArrayList.new()):asFloat(), 0.6), "physics_material_override")
rb:set("collision_layer", 5)
check(rb:call("get_collision_layer_value", args(3)):asBool() and not rb:call("get_collision_layer_value", args(2)):asBool(), "collision layer values")
local owner_id = rb:call("create_shape_owner", args(rb)):asInt()
rb:call("shape_owner_add_shape", args(owner_id, bs))
check(rb:call("shape_owner_get_shape_count", args(owner_id)):asInt() == 1 and rb:call("get_shape_count", ArrayList.new()):asInt() == 1, "shape owners")
rb:call("remove_shape_owner", args(owner_id))
check(rb:call("get_shape_count", ArrayList.new()):asInt() == 0, "remove_shape_owner")
rb:free()

-- CharacterBody3D defaults
local cb = NativeObject.new("CharacterBody3D")
check(cb:isClass("CharacterBody3D") and cb:getClass() == "CharacterBody3D", "CharacterBody3D alias")
check(vnear(get(cb, "up_direction"):asVector3(), 0, 1, 0) and near(get(cb, "floor_max_angle"):asFloat(), math.rad(45)), "CharacterBody3D defaults")
cb:set("velocity", Vector3.new(1, 2, 3))
check(vnear(get(cb, "velocity"):asVector3(), 1, 2, 3), "CharacterBody3D.velocity")
cb:free()

-- RayCast3D
local ray = NativeObject.new("RayCast3D")
ray:set("target_position", Vector3.new(0, -5, 0))
ray:set("collide_with_areas", true)
check(vnear(get(ray, "target_position"):asVector3(), 0, -5, 0) and get(ray, "collide_with_areas"):asBool(), "RayCast3D")
ray:free()

-- 2D: rotation sign, Camera2D zoom, Sprite2D frame_coords
local n2 = NativeObject.new("Node2D")
n2:set("rotation", 0.5)
check(near(get(n2, "rotation"):asFloat(), 0.5) and near(n2:call("get_rot", ArrayList.new()):asFloat(), -0.5), "Node2D rotation is negated")
n2:set("position", Vector2.new(3, 4))
check(vnear(get(n2, "position"):asVector2(), 3, 4), "Node2D.position")
n2:set("rotation", 0.0)
check(near(n2:call("get_angle_to", args(Vector2.new(3, 5))):asFloat(), math.pi / 2), "Node2D.get_angle_to (y down, clockwise)")
n2:free()
local c2 = NativeObject.new("Camera2D")
c2:set("zoom", Vector2.new(2, 2))
check(vnear(get(c2, "zoom"):asVector2(), 2, 2) and vnear(c2:call("get_zoom", ArrayList.new()):asVector2(), 0.5, 0.5), "Camera2D zoom is inverted")
c2:set("limit_left", -100)
check(get(c2, "limit_left"):asInt() == -100, "Camera2D.limit_left")
c2:free()
local sp = NativeObject.new("Sprite2D")
sp:set("hframes", 4)
sp:set("vframes", 4)
sp:set("frame_coords", Vector2.new(1, 2))
check(get(sp, "frame"):asInt() == 9 and vnear(get(sp, "frame_coords"):asVector2(), 1, 2), "Sprite2D.frame_coords")
sp:free()

-- Godot 4 classes implemented by the module
local kc = NativeReference.new("KinematicCollision3D")
check(kc:isValid() and kc:call("get_collision_count", ArrayList.new()):asInt() == 1, "KinematicCollision3D")

print("COMPAT OK")

-- Physics scene, stepped by test_compat.gd
local root = __rootNode
local floor, body, arm, follower, remote
local frames = 0

function physics_setup()
  floor = NativeObject.new("StaticBody3D")
  local fs = NativeObject.new("CollisionShape3D")
  local fshape = NativeReference.new("BoxShape3D")
  fshape:set("size", Vector3.new(20, 1, 20))
  fs:set("shape", fshape)
  floor:call("add_child", args(fs))
  root:call("add_child", args(floor))

  body = NativeObject.new("CharacterBody3D")
  local bs = NativeObject.new("CollisionShape3D")
  local cap = NativeReference.new("CapsuleShape3D")
  cap:set("radius", 0.5)
  cap:set("height", 2.0)
  bs:set("shape", cap)
  body:call("add_child", args(bs))
  root:call("add_child", args(body))
  body:set("global_position", Vector3.new(0, 3, 0))

  arm = NativeObject.new("SpringArm3D")
  arm:set("spring_length", 10.0)
  root:call("add_child", args(arm))
  arm:set("global_position", Vector3.new(4, 5, 0))
  arm:set("rotation_degrees", Vector3.new(90, 0, 0)) -- +Z points down
  arm:call("add_child", args(NativeObject.new("Node3D")))

  follower = NativeObject.new("Node3D")
  follower:set("name", "Follower")
  root:call("add_child", args(follower))
  remote = NativeObject.new("RemoteTransform3D")
  root:call("add_child", args(remote))
  remote:set("remote_path", follower:call("get_path", ArrayList.new()))
  remote:set("global_position", Vector3.new(7, 8, 9))
end

function physics_step(delta)
  frames = frames + 1
  local v = get(body, "velocity"):asVector3()
  v.y = v.y - 9.8 * delta
  body:set("velocity", v)
  body:call("move_and_slide", ArrayList.new())
end

function physics_check()
  local pos = get(body, "global_position"):asVector3()
  check(body:call("is_on_floor", ArrayList.new()):asBool(), "CharacterBody3D lands (is_on_floor)")
  check(near(pos.y, 1.5, 0.15), "CharacterBody3D rests on the floor (y=" .. pos.y .. ")")
  check(body:call("get_slide_collision_count", ArrayList.new()):asInt() >= 1, "slide collisions recorded")
  local c = body:call("get_last_slide_collision", ArrayList.new()):asReference()
  check(vnear(c:call("get_normal", ArrayList.new()):asVector3(), 0, 1, 0, 0.05), "floor collision normal")
  check(near(arm:call("get_hit_length", ArrayList.new()):asFloat(), 4.5, 0.1), "SpringArm3D hit length")
  check(vnear(get(follower, "global_position"):asVector3(), 7, 8, 9), "RemoteTransform3D")
  print("PHYSICS OK")
end
