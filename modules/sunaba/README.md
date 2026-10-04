# sunaba module (libsunaba for Godot 2)

A port of [libsunaba](https://github.com/sunabagg/libsunaba)'s Lua runtime to
Godot 2.1 as an engine module. Godot 2 has no GDExtension, so the runtime is
compiled into the engine. It builds with the 3DS toolchain flags
(`-std=gnu++11 -fno-rtti -fno-exceptions`).

The module is self-contained: it needs only `modules/sunaba` and
`thirdparty/lua` (Lua 5.4.8, unmodified), so it can be copied into other
Godot 2.1 trees. It has been built against this fork (2.1.7) and stock 2.1.5.

## Classes

| Class | Purpose |
| --- | --- |
| `Runtime` (Node) | Owns a Lua state. `init_state(sandboxed, classes)`, `do_string(code)`, `set_var(name, value)`, `bind_object(name, obj)`, `args`. |
| `ScriptObject` (Reference) | A Lua table seen from GDScript: `get_var`, `set_var`, `has_var`, `has_function`, `call_function(name, args)`. |
| `ScriptFunction` (Reference) | A Lua function: `call_func(args)`; `invoke(...)` is the varargs entry point used by signal connections. |
| `DisposableObject`, `RefObject` | Empty base classes, as in libsunaba. |
| `ZipReader` (Reference) | Reads `.snb`/`.slib` zip archives from a path or a buffer without mounting them: `open`, `open_buffer`, `get_files`, `file_exists`, `read_file`, `read_text`. Built on the engine's minizip. |
| `GodotFS` (Lua global) | Native file system for Lua: `exists`, `is_dir`, `is_file`, `size`, `mtime`, `list`, `mkdir` (recursive), `remove`, `rename`, `copy`, `read_all`, `write_all(path, data, append)`, `cwd`, `absolute`, and `open(path, mode)` returning a handle (`close`, `read`, `read_line`, `write`, `seek`, `tell`, `size`, `eof`, `flush`). Backs the Haxe `sys.*` classes, which need `luv` otherwise. Accepts `res://` and `user://` paths. |
| `ImageRef` (Resource) | Godot 4's `Image` (the alias `Image` constructs it): `load_png_from_buffer`/`jpg`/`webp`, `load`, `save_png`, `save_png_to_buffer`, `get_pixel`/`set_pixel`, `fill`, `resize`, `crop`, `flip_x`, `convert`, `blit_rect`, `get_region`, `create`, `set_data`, ... with Godot 4 format and interpolation values. Wraps Godot 2's builtin `Image`; Lua sees Image values as `ImageRef` and passes them back unwrapped. BMP, TGA, SVG, KTX, DDS and EXR have no Godot 2 loader and return `ERR_FILE_UNRECOGNIZED`. |
| `CodeHighlighterRef` (Resource) | Godot 4's `CodeHighlighter` (aliases `CodeHighlighter`, `SyntaxHighlighter`): keyword and member keyword colors, color regions, number/symbol/function/member colors. Assigning it to a `TextEdit`'s `syntax_highlighter` applies it with Godot 2's `add_keyword_color`/`add_color_region` and theme color overrides; reassigning an unchanged highlighter is skipped. |
| `Godot4Compat` (Reference) | `get_manifest()` returns what the compatibility layer emulates (see below), for libsunaba's bindings generator. |
| `InputEventRef` (Reference) | Wraps Godot 2's builtin `InputEvent` and presents it like Godot 4. `getClass`/`isClass` give `InputEventKey`, `InputEventMouseMotion`, ...; properties use Godot 4 names (`position`, `relative`, `keycode`, ...) and codes (keys, joypad buttons and axes); methods include `is_action_pressed`, `as_text`, `xformed_by`. |

Scripts extending `Runtime` can implement `_require(path) -> String`,
`_print(msgarr)`, `_errord/_warnd/_infod(msg, title)` and `_exit(code)`.

## Lua API

Same global names and calling conventions as libsunaba, so Haxe output
written against `sunaba.core` runs unchanged where the engine supports it:
`Variant`, `NativeObject`, `NativeReference`, `ArrayList`, `Dictionary`,
`Callable`, `Signal`, `Byte`, `ByteArray`/`BinaryData`, `Vector2`, `Vector3`,
`Rect2`, `Transform2D`, `Plane`, `Quaternion`, `AABB`, `Basis`,
`Transform3D`, `Color`, plus `Vector2i`, `Vector3i`, `Rect2i`, `Vector4`,
`Vector4i`.

Math types resolve members and methods through Godot 2's Variant reflection.
camelCase names map to snake_case, and a table covers Godot 4 → 2 renames
(`lerp` → `linear_interpolate`, `position` → `pos`, ...). Methods Godot 2
lacks, or whose semantics changed (`reflect`, `bounce`, `slide`), are
implemented in Lua in `lua_prelude.cpp`.

`InputEvent` values crossing into Lua are wrapped in an `InputEventRef`
automatically, and unwrapped when passed back to the engine, so libsunaba's
Haxe input classes work unchanged.

`bit32` is preloaded (`require("bit32")`), because Haxe's Lua output needs
it and Lua 5.4 no longer ships it. It uses the pure-Lua implementation from
sunaba desktop (MIT, Andras Horvath).

`Variant.getType()` returns Godot 4 type numbers, matching the Haxe
`VariantType` enum. `SUNABA_GODOT_MAJOR` is set to `2`.

## Godot 4 compatibility layer

libsunaba's Haxe components address engine objects with Godot 4 names
(`Node3D`, `Camera3D`, `position`, `move_and_slide`, ...). `NativeObject`
and `NativeReference` translate them through `godot4_compat.cpp`:

- **Classes:** about 90 aliases (`Node3D` → `Spatial`, `CharacterBody3D` →
  `KinematicBody`, `StandardMaterial3D` → `FixedMaterial`, ...). `getClass`
  and `isClass` answer with Godot 4 names.
- **Properties and methods:** tables map Godot 4 names to Godot 2 accessors
  or to emulation code, then fall back to the object's own property and to
  `get_x`/`is_x`/`set_x`. Properties Godot 2 cannot represent (e.g.
  `Light3D.light_temperature`) are stored in object metadata, so reads
  return the last written value.
- **Conventions:** Godot 4 Euler order (YXZ) and basis = rotation · scale
  for `Node3D`; `Node2D.rotation` and `Camera2D.zoom` have Godot 4's sign
  and scale; capsule heights include the caps, and a `CollisionShape` holding
  a capsule is turned so it stands along Y.
- **`CharacterBody3D`:** `move_and_slide`, `move_and_collide`, `test_move`,
  floor/wall/ceiling state, floor snap and `KinematicCollision3D`, built on
  `KinematicBody.move`. Constants such as `up_direction` and
  `floor_max_angle` live in the emulation state.
- **`RigidBody3D`:** `freeze`, `freeze_mode` and `lock_rotation` map onto
  Godot 2's body modes; forces become one-step impulses.
- **Shapes:** `CollisionShape3D` children register with their body when
  added or when `shape` is set (Godot 2 does this only in the editor).
  `shape_owner_*` is emulated on the body's flat shape list; use one
  mechanism or the other on a given body.
- **Classes added by the module:** `BoxMesh`, `SphereMesh`, `CapsuleMesh`,
  `CylinderMesh`, `PlaneMesh`, `QuadMesh` (Godot 4 property names and
  defaults, regenerated when a property changes, Godot's clockwise front
  faces), `PhysicsMaterial`, `CylinderShape` (a convex hull),
  `KinematicCollision3D`, `RemoteTransform3D` and `SpringArm3D` (a ray, not
  a shape cast).
- **Controls:** anchors are Godot 4 ratios and offsets are distances from
  the anchor point (`anchor_*`, `offset_*`, `get_anchor`, `set_anchor`,
  `set_offset`), mapped onto Godot 2's per-side anchor modes. Ratios 0, 0.5
  and 1 map exactly; any other ratio becomes a `RATIO` anchor with no pixel
  offset (the offset is only reported back). `set_anchors_preset`,
  `set_offsets_preset` and `set_anchors_and_offsets_preset` follow Godot 4.
  Also mapped: size flags (Godot 2 swaps `FILL` and `EXPAND`), mouse filter,
  focus neighbours, `custom_minimum_size`, the `add_theme_*_override`,
  `get_theme_*` and `has_theme_*` families (font sizes are not supported),
  and `get_property_list()` entries gain `class_name`.
- **GUI widgets:** Godot 4 names for `Button.icon`, `LineEdit.placeholder_text`,
  `Range.value`, `Label` alignment, `RichTextLabel.text` (BBCode when
  `bbcode_enabled`), `ScrollContainer` scroll modes, `TextEdit.editable` and
  caret methods, `PopupMenu.add_item`/`add_separator`/`set_item_id`,
  `OptionButton.get_popup`, `Window.title`, `AcceptDialog.dialog_text`, ...
  `CodeEdit` is a `TextEdit`; its code-editing properties are kept in
  metadata. `ImageTexture.set_image`/`update` and `Texture.get_image` take
  and return `Image` objects.
- **Trees and windows:** `TreeItem.get_first_child`/`get_children`/
  `get_child`/`get_child_count`/`get_index`; `Node.get_window()` returns the
  root `Viewport`, which carries Godot 4 `Window` properties such as
  `content_scale_factor` (kept in metadata).
- **Signals:** Godot 4 names are mapped when a `Signal` is created
  (`PopupMenu.id_pressed` → `item_pressed`, `LineEdit.text_submitted` →
  `text_entered`, `mouse_entered` → `mouse_enter`, `gui_input` →
  `input_event`, ...).
- **Manifest:** `Godot4Compat.new().get_manifest()` lists the emulated
  properties, methods and signals per Godot 2 class, plus the methods of
  `ImageRef` and `InputEventRef`. libsunaba's `tools/godot2/bindgen.py`
  reads a dump of it (`godot4_compat.json`), so update that dump when the
  tables change.
- **Not emulated:** per-surface material overrides (the instance's material
  override applies to every surface), continuous forces, angular axis locks,
  more than one linear axis lock, `Skeleton3D` modifiers and physical bones.

## Differences from libsunaba on Godot 4

- **No sol2.** sol2 needs C++17, and Godot 2's headers do not compile as
  C++17, so the bindings use the Lua C API directly. They are also much
  smaller, which matters on the 3DS.
- **Lua 5.4 only.** LuaJIT is not built (no 3DS support).
- **Integer vectors, Vector4, Projection:** Godot 2 has none of these.
  `Vector2i`/`Vector3i`/`Rect2i` produce truncated `Vector2`/`Vector3`/`Rect2`;
  `Vector4`/`Vector4i` are stored as `Quaternion`; `asProjection` returns nil.
- **Callable and Signal** are emulated with Godot 2's
  `connect(signal, object, method, binds)`. Lua functions become a
  `ScriptFunction`; the `Runtime` keeps connected ones alive. `unbind` is
  not supported.
- **`NativeObject.new(path, args, 2)` (C#)** is unsupported. Godot 2 has
  no Mono.
- **`callStatic`** only reaches engine singletons; Godot 2 has no static
  class methods.
- **`getService`** looks up Godot 2 singletons (`OS`, `Input`, `Globals`,
  ...). Godot 4 names such as `ProjectSettings` or `Engine` are not mapped.
- **Containers** are created as shared Godot 2 containers, so they keep
  Godot 4's reference semantics.
- The debugger (`dbg`) is only installed in unsandboxed runtimes, since it
  needs `io` and `os`.
- `Runtime` runs one incremental GC step per frame instead of a full
  collection.
- Fixed while porting: `Signal.emit` looped forever, `NativeObject.setMeta`
  did nothing, `ByteArray.new(table)` wrote past the end of its buffer,
  `toTable` returned a 0-based table, and `Vector2.gt/lt/gte/lte` were
  inverted.
- Engine fixes made for the port: `ResourceLoader::load` reported
  `ERR_CANT_OPEN` for cached resources (so every later `load()` of the same
  path printed an error), and the 3DS build now passes
  `-fno-delete-null-pointer-checks`. With `NO_SAFE_CAST`, `Object::cast_to`
  relies on `if (!this) return NULL`, which GCC and Clang otherwise remove;
  `CheckBox` crashed on that when drawn. Desktop builds with `NO_SAFE_CAST`
  need the same flag.

## Lua errors and C++

Lua raises errors with `longjmp`, which skips C++ destructors. Every binding
is declared with `SUNABA_LUA_FUNC` and keeps its C++ temporaries in an inner
scope. It reports failure through a `LuaError`, which is raised only after
that scope has unwound. Calls back into Lua always use `lua_pcall`.

## Tests

`tests/` contains headless tests: `test_runtime.gd` covers every bound type and
`test_compat.gd` the Godot 4 compatibility layer, including a stepped physics
scene. Build a
`server` (or desktop) binary with the module, then run it from the `tests`
directory:

    godot_server -s test_runtime.gd    # prints SUNABA TESTS PASSED
    godot_server -s test_compat.gd     # prints COMPAT TESTS PASSED
    godot_server -s test_zip.gd        # prints ZIP TESTS PASSED
    godot_server -s test_fs.gd         # prints FS TESTS PASSED
