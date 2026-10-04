# Tests GodotFS. Run from this directory:  godot_server -s test_fs.gd
extends SceneTree

class R:
	extends Runtime
	var out = []
	var errs = []
	func _print(m):
		for x in m:
			out.append(x)
	func _errord(msg, title):
		errs.append(msg)

func _init():
	var rt = R.new()
	rt.init_state(false, [])
	var f = File.new()
	f.open("res://test_fs.lua", File.READ)
	rt.do_string(f.get_as_text())
	f.close()
	for e in rt.errs:
		print("LUA ERROR: " + e)
	print("FS TESTS PASSED" if rt.errs.size() == 0 and rt.out.has("FS OK") else "FS TESTS FAILED")
	rt.free()
	quit()
