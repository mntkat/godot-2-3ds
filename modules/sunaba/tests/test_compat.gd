# Tests the Godot 4 compatibility layer. Run from this directory:
#   godot_server -s test_compat.gd
extends SceneTree

class TestRuntime:
	extends Runtime

	var printed = []
	var errors = []

	func _print(msgarr):
		for m in msgarr:
			printed.append(m)

	func _errord(msg, title):
		errors.append(title + ": " + msg)

var rt
var frame = 0

func _init():
	rt = TestRuntime.new()
	get_root().add_child(rt)
	rt.init_state(false, [])
	var f = File.new()
	f.open("res://test_compat.lua", File.READ)
	rt.do_string(f.get_as_text())
	f.close()
	rt.do_string("physics_setup()")

func _iteration(delta):
	frame += 1
	if frame <= 90:
		rt.do_string("physics_step(" + str(delta) + ")")
		return false
	rt.do_string("physics_check()")
	var ok = rt.errors.size() == 0 and rt.printed.has("COMPAT OK") and rt.printed.has("PHYSICS OK")
	for e in rt.errors:
		print("LUA ERROR: " + e)
	print("COMPAT TESTS PASSED" if ok else "COMPAT TESTS FAILED")
	rt.free()
	quit()
	return true
