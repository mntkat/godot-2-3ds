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

## Lua errors and C++

Lua raises errors with `longjmp`, which skips C++ destructors. Every binding
is declared with `SUNABA_LUA_FUNC` and keeps its C++ temporaries in an inner
scope. It reports failure through a `LuaError`, which is raised only after
that scope has unwound. Calls back into Lua always use `lua_pcall`.

## Tests

`tests/` contains a headless smoke test that covers every bound type. Build a
`server` (or desktop) binary with the module, then run it from the `tests`
directory:

    godot_server -s test_runtime.gd

It prints `SUNABA TESTS PASSED` on success.
