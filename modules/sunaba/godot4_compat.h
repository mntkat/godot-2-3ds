/*************************************************************************/
/*  godot4_compat.h                                                      */
/*************************************************************************/
/*  Godot 4 view of Godot 2 objects for NativeObject/NativeReference.    */
/*                                                                       */
/*  libsunaba's Haxe code addresses engine objects with Godot 4 names:   */
/*  classes (Node3D, Camera3D, ...), properties (position, fov, ...) and */
/*  methods (move_and_slide, look_at_from_position, ...). This layer     */
/*  translates them: class aliases, property and method tables (with     */
/*  emulation where Godot 2 has no equivalent), then a fallback to the   */
/*  object's own property, then to get_x/is_x/set_x methods.             */
/*                                                                       */
/*  Properties Godot 2 cannot represent are kept in object metadata, so  */
/*  reads return what was last written.                                  */
/*************************************************************************/

#ifndef SUNABA_GODOT4_COMPAT_H
#define SUNABA_GODOT4_COMPAT_H

#include "object.h"

namespace sunaba {
namespace compat {

// Godot 4 class name -> Godot 2 class name (identity when unchanged).
String to_godot2_class(const String &p_class);
// Godot 4 name of an object's class.
String godot4_class(const Object *p_object);
bool is_class(const Object *p_object, const String &p_godot4_class);

Variant get(Object *p_object, const String &p_property);
void set(Object *p_object, const String &p_property, const Variant &p_value);
bool has_method(Object *p_object, const String &p_method);
// Returns false (and leaves r_ret nil) if the method exists in neither the
// compat tables nor the object.
bool call(Object *p_object, const String &p_method, const Array &p_args, Variant &r_ret);

void cleanup();

} // namespace compat
} // namespace sunaba

#endif // SUNABA_GODOT4_COMPAT_H
