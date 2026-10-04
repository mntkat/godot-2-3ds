# Tests ZipReader. Run from this directory:  godot_server -s test_zip.gd
extends SceneTree

func _init():
	var ok = true
	# Fixture built by the test runner script: files hello.txt, dir/b.bin
	var z = ZipReader.new()
	var err = z.open("res://fixture.zip")
	ok = ok and err == OK and z.is_open()
	var files = z.get_files()
	ok = ok and files.size() == 3 and z.file_exists("hello.txt") and z.file_exists("dir/b.bin")
	ok = ok and z.read_text("hello.txt") == "hello zip\n"
	var bin = z.read_file("dir/b.bin")
	ok = ok and bin.size() == 1000 and bin[0] == 0 and bin[999] == 81
	ok = ok and z.read_file("missing").size() == 0 and not z.file_exists("missing")
	ok = ok and z.read_text("unicode.txt") == "café ☃"

	# Same archive from a buffer
	var f = File.new()
	f.open("res://fixture.zip", File.READ)
	var data = f.get_buffer(f.get_len())
	f.close()
	var z2 = ZipReader.new()
	ok = ok and z2.open_buffer(data) == OK and z2.read_text("hello.txt") == "hello zip\n" and z2.get_files().size() == 3

	# Errors
	var z3 = ZipReader.new()
	ok = ok and z3.open("res://nope.zip") != OK and not z3.is_open()
	var junk = RawArray()
	junk.append(1)
	junk.append(2)
	ok = ok and z3.open_buffer(junk) != OK
	z.close()
	ok = ok and not z.is_open()

	print("ZIP TESTS PASSED" if ok else "ZIP TESTS FAILED")
	quit()
