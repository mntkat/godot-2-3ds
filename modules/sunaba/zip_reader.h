/*************************************************************************/
/*  zip_reader.h                                                         */
/*************************************************************************/
/*  Reads zip archives (.snb apps, .slib libraries) from a file or a     */
/*  buffer. Godot 2 can only mount zips as resource packs; Sunaba keeps  */
/*  each archive separate and addresses it through virtual path urls.    */
/*************************************************************************/

#ifndef SUNABA_ZIP_READER_H
#define SUNABA_ZIP_READER_H

#include "map.h"
#include "reference.h"

#include "thirdparty/minizip/unzip.h"

class FileAccess;

class ZipReader : public Reference {
	OBJ_TYPE(ZipReader, Reference);

	unzFile zfile;
	FileAccess *file; // path mode: stream source
	ByteArray buffer; // buffer mode: archive bytes
	int buffer_pos;
	bool from_buffer;

	Map<String, unz_file_pos> entries;
	Vector<String> names;

	Error _open(const zlib_filefunc_def &p_io, const String &p_name);

	// Memory I/O callbacks for minizip.
	static voidpf mem_open(voidpf opaque, const char *filename, int mode);
	static uLong mem_read(voidpf opaque, voidpf stream, void *buf, uLong size);
	static uLong mem_write(voidpf opaque, voidpf stream, const void *buf, uLong size);
	static long mem_tell(voidpf opaque, voidpf stream);
	static long mem_seek(voidpf opaque, voidpf stream, uLong offset, int origin);
	static int mem_close(voidpf opaque, voidpf stream);
	static int mem_error(voidpf opaque, voidpf stream);

protected:
	static void _bind_methods();

public:
	Error open(const String &p_path);
	Error open_buffer(const ByteArray &p_data);
	void close();
	bool is_open() const;

	StringArray get_files() const;
	bool file_exists(const String &p_name) const;
	// Empty array if the entry is missing or unreadable.
	ByteArray read_file(const String &p_name);
	String read_text(const String &p_name);

	ZipReader();
	~ZipReader();
};

#endif // SUNABA_ZIP_READER_H
