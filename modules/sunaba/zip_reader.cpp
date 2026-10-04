/*************************************************************************/
/*  zip_reader.cpp                                                       */
/*************************************************************************/

#include "zip_reader.h"

#include "core/io/zip_io.h"
#include "os/file_access.h"

/* Memory I/O */

voidpf ZipReader::mem_open(voidpf opaque, const char *filename, int mode) {

	return opaque;
}

uLong ZipReader::mem_read(voidpf opaque, voidpf stream, void *buf, uLong size) {

	ZipReader *z = (ZipReader *)opaque;
	int available = z->buffer.size() - z->buffer_pos;
	int n = MIN((int)size, MAX(available, 0));
	if (n > 0) {
		ByteArray::Read r = z->buffer.read();
		copymem(buf, r.ptr() + z->buffer_pos, n);
		z->buffer_pos += n;
	}
	return n;
}

uLong ZipReader::mem_write(voidpf opaque, voidpf stream, const void *buf, uLong size) {

	return 0; // read-only
}

long ZipReader::mem_tell(voidpf opaque, voidpf stream) {

	return ((ZipReader *)opaque)->buffer_pos;
}

long ZipReader::mem_seek(voidpf opaque, voidpf stream, uLong offset, int origin) {

	ZipReader *z = (ZipReader *)opaque;
	long pos = (long)offset;
	switch (origin) {
		case ZLIB_FILEFUNC_SEEK_CUR: pos = z->buffer_pos + (long)offset; break;
		case ZLIB_FILEFUNC_SEEK_END: pos = z->buffer.size() + (long)offset; break;
		default: break;
	}
	if (pos < 0 || pos > z->buffer.size())
		return -1;
	z->buffer_pos = pos;
	return 0;
}

int ZipReader::mem_close(voidpf opaque, voidpf stream) {

	return 0;
}

int ZipReader::mem_error(voidpf opaque, voidpf stream) {

	return 0;
}

/* ZipReader */

Error ZipReader::_open(const zlib_filefunc_def &p_io, const String &p_name) {

	zlib_filefunc_def io = p_io;
	zfile = unzOpen2(p_name.utf8().get_data(), &io);
	if (!zfile)
		return ERR_FILE_CORRUPT;

	unz_global_info64 gi;
	if (unzGetGlobalInfo64(zfile, &gi) != UNZ_OK) {
		close();
		return ERR_FILE_CORRUPT;
	}

	for (unsigned int i = 0; i < gi.number_entry; i++) {
		char name[1024];
		unz_file_info64 info;
		if (unzGetCurrentFileInfo64(zfile, &info, name, sizeof(name), NULL, 0, NULL, 0) != UNZ_OK)
			break;

		String fname;
		fname.parse_utf8(name);
		fname = fname.replace("\\", "/");
		// Directory entries end with '/'.
		if (!fname.ends_with("/")) {
			unz_file_pos pos;
			unzGetFilePos(zfile, &pos);
			entries[fname] = pos;
			names.push_back(fname);
		}
		if (i + 1 < gi.number_entry && unzGoToNextFile(zfile) != UNZ_OK)
			break;
	}
	return OK;
}

Error ZipReader::open(const String &p_path) {

	close();
	from_buffer = false;
	zlib_filefunc_def io = zipio_create_io_from_file(&file);
	Error err = _open(io, p_path);
	if (err == OK)
		return OK;
	return FileAccess::exists(p_path) ? err : ERR_FILE_NOT_FOUND;
}

Error ZipReader::open_buffer(const ByteArray &p_data) {

	close();
	from_buffer = true;
	buffer = p_data;
	buffer_pos = 0;

	zlib_filefunc_def io;
	io.opaque = this;
	io.zopen_file = mem_open;
	io.zread_file = mem_read;
	io.zwrite_file = mem_write;
	io.ztell_file = mem_tell;
	io.zseek_file = mem_seek;
	io.zclose_file = mem_close;
	io.zerror_file = mem_error;
	io.alloc_mem = zipio_alloc;
	io.free_mem = zipio_free;
	return _open(io, "memory.zip");
}

void ZipReader::close() {

	if (zfile) {
		unzClose(zfile);
		zfile = NULL;
	}
	if (file) {
		file->close();
		memdelete(file);
		file = NULL;
	}
	buffer = ByteArray();
	buffer_pos = 0;
	entries.clear();
	names.clear();
}

bool ZipReader::is_open() const {

	return zfile != NULL;
}

StringArray ZipReader::get_files() const {

	StringArray out;
	for (int i = 0; i < names.size(); i++)
		out.push_back(names[i]);
	return out;
}

bool ZipReader::file_exists(const String &p_name) const {

	return entries.has(p_name.replace("\\", "/"));
}

ByteArray ZipReader::read_file(const String &p_name) {

	ByteArray out;
	const Map<String, unz_file_pos>::Element *E = entries.find(p_name.replace("\\", "/"));
	ERR_FAIL_COND_V(!zfile, out);
	if (!E)
		return out;

	unz_file_pos pos = E->get();
	if (unzGoToFilePos(zfile, &pos) != UNZ_OK)
		return out;
	unz_file_info64 info;
	if (unzGetCurrentFileInfo64(zfile, &info, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK)
		return out;
	if (unzOpenCurrentFile(zfile) != UNZ_OK)
		return out;

	out.resize((int)info.uncompressed_size);
	if (info.uncompressed_size > 0) {
		ByteArray::Write w = out.write();
		int read = unzReadCurrentFile(zfile, w.ptr(), (unsigned int)info.uncompressed_size);
		if (read != (int)info.uncompressed_size) {
			w = ByteArray::Write();
			out.resize(0);
		}
	}
	unzCloseCurrentFile(zfile);
	return out;
}

String ZipReader::read_text(const String &p_name) {

	ByteArray data = read_file(p_name);
	if (data.size() == 0)
		return String();
	ByteArray::Read r = data.read();
	String s;
	s.parse_utf8((const char *)r.ptr(), data.size());
	return s;
}

void ZipReader::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("open", "path"), &ZipReader::open);
	ObjectTypeDB::bind_method(_MD("open_buffer", "data"), &ZipReader::open_buffer);
	ObjectTypeDB::bind_method(_MD("close"), &ZipReader::close);
	ObjectTypeDB::bind_method(_MD("is_open"), &ZipReader::is_open);
	ObjectTypeDB::bind_method(_MD("get_files"), &ZipReader::get_files);
	ObjectTypeDB::bind_method(_MD("file_exists", "name"), &ZipReader::file_exists);
	ObjectTypeDB::bind_method(_MD("read_file", "name"), &ZipReader::read_file);
	ObjectTypeDB::bind_method(_MD("read_text", "name"), &ZipReader::read_text);
}

ZipReader::ZipReader() {

	zfile = NULL;
	file = NULL;
	buffer_pos = 0;
	from_buffer = false;
}

ZipReader::~ZipReader() {

	close();
}
