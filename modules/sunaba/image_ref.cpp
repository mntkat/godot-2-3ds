/*************************************************************************/
/*  image_ref.cpp                                                        */
/*************************************************************************/

#include "image_ref.h"

#include "io/file_access_memory.h"
#include "io/image_loader.h"
#include "os/dir_access.h"
#include "os/file_access.h"
#include "os/os.h"

Ref<ImageRef> ImageRef::wrap(const Image &p_image) {

	Ref<ImageRef> r;
	r.instance();
	r->image = p_image;
	return r;
}

Image ImageRef::_to_image(const Variant &p_image) {

	if (p_image.get_type() == Variant::IMAGE)
		return p_image;
	Object *o = p_image;
	ImageRef *r = o ? o->cast_to<ImageRef>() : NULL;
	return r ? r->image : Image();
}

// Godot 4: L8 0, LA8 1, R8 2, RG8 3, RGB8 4, RGBA8 5, ..., DXT1 17, DXT3 18,
// DXT5 19, RGTC_R 20, RGTC_RG 21.
int ImageRef::format_to_godot4(Image::Format p_format) {

	switch (p_format) {
		case Image::FORMAT_GRAYSCALE: return 0;
		case Image::FORMAT_INTENSITY: return 2;
		case Image::FORMAT_GRAYSCALE_ALPHA: return 1;
		case Image::FORMAT_RGB: return 4;
		case Image::FORMAT_BC1: return 17;
		case Image::FORMAT_BC2: return 18;
		case Image::FORMAT_BC3: return 19;
		case Image::FORMAT_BC4: return 20;
		case Image::FORMAT_BC5: return 21;
		default: return 5;
	}
}

Image::Format ImageRef::format_from_godot4(int p_format) {

	switch (p_format) {
		case 0: return Image::FORMAT_GRAYSCALE;
		case 1: return Image::FORMAT_GRAYSCALE_ALPHA;
		case 2: return Image::FORMAT_INTENSITY;
		case 3: // RG8: no two-channel format
		case 4: return Image::FORMAT_RGB;
		case 17: return Image::FORMAT_BC1;
		case 18: return Image::FORMAT_BC2;
		case 19: return Image::FORMAT_BC3;
		case 20: return Image::FORMAT_BC4;
		case 21: return Image::FORMAT_BC5;
		default: return Image::FORMAT_RGBA;
	}
}

static Image::Interpolation interpolation_from_godot4(int p_interp) {

	// Godot 4: NEAREST 0, BILINEAR 1, CUBIC 2, TRILINEAR 3, LANCZOS 4
	switch (p_interp) {
		case 0: return Image::INTERPOLATE_NEAREST;
		case 2:
		case 4: return Image::INTERPOLATE_CUBIC;
		default: return Image::INTERPOLATE_BILINEAR;
	}
}

/* Loading and saving */

Error ImageRef::_load_from_buffer(const ByteArray &p_data, const String &p_extension) {

	if (p_data.size() == 0)
		return ERR_INVALID_PARAMETER;
	FileAccessMemory *f = memnew(FileAccessMemory);
	ByteArray::Read r = p_data.read();
	Error err = f->open_custom(r.ptr(), p_data.size());
	if (err == OK) {
		Image loaded;
		// The loaders are picked by the file name's extension.
		err = ImageLoader::load_image("buffer." + p_extension, &loaded, f);
		if (err == OK)
			image = loaded;
	}
	memdelete(f);
	return err;
}

Error ImageRef::load(const String &p_path) {

	Image loaded;
	Error err = ImageLoader::load_image(p_path, &loaded);
	if (err == OK)
		image = loaded;
	return err;
}

Error ImageRef::load_png_from_buffer(const ByteArray &p_data) { return _load_from_buffer(p_data, "png"); }
Error ImageRef::load_jpg_from_buffer(const ByteArray &p_data) { return _load_from_buffer(p_data, "jpg"); }
Error ImageRef::load_webp_from_buffer(const ByteArray &p_data) { return _load_from_buffer(p_data, "webp"); }
// BMP, TGA, SVG, KTX, DDS and EXR have no Godot 2 loader.
Error ImageRef::load_unsupported_from_buffer(const ByteArray &p_data) { return ERR_FILE_UNRECOGNIZED; }

Error ImageRef::save_png(const String &p_path) const { return image.save_png(p_path); }

ByteArray ImageRef::save_png_to_buffer() const {

	// Image only saves to files.
	String path = OS::get_singleton()->get_data_dir().plus_file("_sunaba_png_buffer.png");
	ByteArray out;
	if (image.save_png(path) != OK)
		return out;
	FileAccess *f = FileAccess::open(path, FileAccess::READ);
	if (f) {
		out.resize(f->get_len());
		if (out.size()) {
			ByteArray::Write w = out.write();
			f->get_buffer(w.ptr(), out.size());
		}
		memdelete(f);
	}
	DirAccess *d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	d->remove(path);
	memdelete(d);
	return out;
}

/* Properties */

int ImageRef::get_width() const { return image.get_width(); }
int ImageRef::get_height() const { return image.get_height(); }
Vector2 ImageRef::get_size() const { return Vector2(image.get_width(), image.get_height()); }
int ImageRef::get_format() const { return format_to_godot4(image.get_format()); }
bool ImageRef::is_empty() const { return image.empty(); }
bool ImageRef::has_mipmaps() const { return image.get_mipmaps() > 0; }
int ImageRef::get_mipmap_count() const { return image.get_mipmaps(); }
Error ImageRef::generate_mipmaps(bool p_renormalize) { return image.generate_mipmaps(); }
void ImageRef::clear_mipmaps() { image.clear_mipmaps(); }
ByteArray ImageRef::get_data() const { return image.get_data(); }
int ImageRef::get_data_size() const { return image.get_data().size(); }

/* Pixels */

Color ImageRef::get_pixel(int p_x, int p_y) const {
	ERR_FAIL_INDEX_V(p_x, image.get_width(), Color());
	ERR_FAIL_INDEX_V(p_y, image.get_height(), Color());
	return image.get_pixel(p_x, p_y);
}
Color ImageRef::get_pixelv(const Vector2 &p_pos) const { return get_pixel(p_pos.x, p_pos.y); }
void ImageRef::set_pixel(int p_x, int p_y, const Color &p_color) {
	ERR_FAIL_INDEX(p_x, image.get_width());
	ERR_FAIL_INDEX(p_y, image.get_height());
	image.put_pixel(p_x, p_y, p_color);
}
void ImageRef::set_pixelv(const Vector2 &p_pos, const Color &p_color) { set_pixel(p_pos.x, p_pos.y, p_color); }
void ImageRef::fill(const Color &p_color) { image.fill(p_color); }

void ImageRef::fill_rect(const Rect2 &p_rect, const Color &p_color) {
	Rect2 r = p_rect.clip(Rect2(0, 0, image.get_width(), image.get_height()));
	for (int y = r.pos.y; y < r.pos.y + r.size.y; y++)
		for (int x = r.pos.x; x < r.pos.x + r.size.x; x++)
			image.put_pixel(x, y, p_color);
}

/* Creation and editing */

void ImageRef::create(int p_width, int p_height, bool p_mipmaps, int p_format) {
	image.create(p_width, p_height, p_mipmaps, format_from_godot4(p_format));
}

void ImageRef::create_from_data(int p_width, int p_height, bool p_mipmaps, int p_format, const ByteArray &p_data) {
	set_data(p_width, p_height, p_mipmaps, p_format, p_data);
}

void ImageRef::set_data(int p_width, int p_height, bool p_mipmaps, int p_format, const ByteArray &p_data) {
	Image::Format f = format_from_godot4(p_format);
	int mipmaps = p_mipmaps ? Image::get_image_required_mipmaps(p_width, p_height, f) : 0;
	image.create(p_width, p_height, mipmaps, f, p_data);
}

void ImageRef::copy_from(const Variant &p_src) { image = _to_image(p_src); }

Ref<ImageRef> ImageRef::get_region(const Rect2 &p_rect) const {
	Image region;
	region.create(p_rect.size.x, p_rect.size.y, false, image.get_format());
	region.blit_rect(image, p_rect, Point2());
	return wrap(region);
}

Rect2 ImageRef::get_used_rect() const { return image.get_used_rect(); }

void ImageRef::blit_rect(const Variant &p_src, const Rect2 &p_src_rect, const Vector2 &p_dst) {
	image.blit_rect(_to_image(p_src), p_src_rect, p_dst);
}

void ImageRef::blend_rect(const Variant &p_src, const Rect2 &p_src_rect, const Vector2 &p_dst) {
	image.blend_rect(_to_image(p_src), p_src_rect, p_dst);
}

void ImageRef::resize(int p_width, int p_height, int p_interpolation) {
	image.resize(p_width, p_height, interpolation_from_godot4(p_interpolation));
}
void ImageRef::crop(int p_width, int p_height) { image.crop(p_width, p_height); }
void ImageRef::flip_x() { image.flip_x(); }
void ImageRef::flip_y() { image.flip_y(); }
void ImageRef::convert(int p_format) { image.convert(format_from_godot4(p_format)); }
Error ImageRef::decompress() { return image.decompress(); }
bool ImageRef::is_compressed() const { return image.is_compressed(); }
int ImageRef::detect_alpha() const { return image.detect_alpha(); } // NONE/BIT/BLEND match Godot 4
bool ImageRef::is_invisible() const { return image.is_invisible(); }
void ImageRef::fix_alpha_edges() { image.fix_alpha_edges(); }
void ImageRef::premultiply_alpha() { image.premultiply_alpha(); }
void ImageRef::srgb_to_linear() { image.srgb_to_linear(); }

void ImageRef::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("load", "path"), &ImageRef::load);
	ObjectTypeDB::bind_method(_MD("load_png_from_buffer", "buffer"), &ImageRef::load_png_from_buffer);
	ObjectTypeDB::bind_method(_MD("load_jpg_from_buffer", "buffer"), &ImageRef::load_jpg_from_buffer);
	ObjectTypeDB::bind_method(_MD("load_webp_from_buffer", "buffer"), &ImageRef::load_webp_from_buffer);
	ObjectTypeDB::bind_method(_MD("load_bmp_from_buffer", "buffer"), &ImageRef::load_unsupported_from_buffer);
	ObjectTypeDB::bind_method(_MD("load_tga_from_buffer", "buffer"), &ImageRef::load_unsupported_from_buffer);
	ObjectTypeDB::bind_method(_MD("load_svg_from_buffer", "buffer"), &ImageRef::load_unsupported_from_buffer);
	ObjectTypeDB::bind_method(_MD("load_ktx_from_buffer", "buffer"), &ImageRef::load_unsupported_from_buffer);
	ObjectTypeDB::bind_method(_MD("load_dds_from_buffer", "buffer"), &ImageRef::load_unsupported_from_buffer);
	ObjectTypeDB::bind_method(_MD("load_exr_from_buffer", "buffer"), &ImageRef::load_unsupported_from_buffer);
	ObjectTypeDB::bind_method(_MD("save_png", "path"), &ImageRef::save_png);
	ObjectTypeDB::bind_method(_MD("save_png_to_buffer"), &ImageRef::save_png_to_buffer);

	ObjectTypeDB::bind_method(_MD("get_width"), &ImageRef::get_width);
	ObjectTypeDB::bind_method(_MD("get_height"), &ImageRef::get_height);
	ObjectTypeDB::bind_method(_MD("get_size"), &ImageRef::get_size);
	ObjectTypeDB::bind_method(_MD("get_format"), &ImageRef::get_format);
	ObjectTypeDB::bind_method(_MD("is_empty"), &ImageRef::is_empty);
	ObjectTypeDB::bind_method(_MD("has_mipmaps"), &ImageRef::has_mipmaps);
	ObjectTypeDB::bind_method(_MD("get_mipmap_count"), &ImageRef::get_mipmap_count);
	ObjectTypeDB::bind_method(_MD("generate_mipmaps", "renormalize"), &ImageRef::generate_mipmaps, DEFVAL(false));
	ObjectTypeDB::bind_method(_MD("clear_mipmaps"), &ImageRef::clear_mipmaps);
	ObjectTypeDB::bind_method(_MD("get_data"), &ImageRef::get_data);
	ObjectTypeDB::bind_method(_MD("get_data_size"), &ImageRef::get_data_size);

	ObjectTypeDB::bind_method(_MD("get_pixel", "x", "y"), &ImageRef::get_pixel);
	ObjectTypeDB::bind_method(_MD("get_pixelv", "point"), &ImageRef::get_pixelv);
	ObjectTypeDB::bind_method(_MD("set_pixel", "x", "y", "color"), &ImageRef::set_pixel);
	ObjectTypeDB::bind_method(_MD("set_pixelv", "point", "color"), &ImageRef::set_pixelv);
	ObjectTypeDB::bind_method(_MD("fill", "color"), &ImageRef::fill);
	ObjectTypeDB::bind_method(_MD("fill_rect", "rect", "color"), &ImageRef::fill_rect);

	ObjectTypeDB::bind_method(_MD("create", "width", "height", "use_mipmaps", "format"), &ImageRef::create);
	ObjectTypeDB::bind_method(_MD("create_empty", "width", "height", "use_mipmaps", "format"), &ImageRef::create);
	ObjectTypeDB::bind_method(_MD("create_from_data", "width", "height", "use_mipmaps", "format", "data"), &ImageRef::create_from_data);
	ObjectTypeDB::bind_method(_MD("set_data", "width", "height", "use_mipmaps", "format", "data"), &ImageRef::set_data);
	ObjectTypeDB::bind_method(_MD("copy_from", "src"), &ImageRef::copy_from);
	ObjectTypeDB::bind_method(_MD("get_region", "region"), &ImageRef::get_region);
	ObjectTypeDB::bind_method(_MD("get_used_rect"), &ImageRef::get_used_rect);
	ObjectTypeDB::bind_method(_MD("blit_rect", "src", "src_rect", "dst"), &ImageRef::blit_rect);
	ObjectTypeDB::bind_method(_MD("blend_rect", "src", "src_rect", "dst"), &ImageRef::blend_rect);

	ObjectTypeDB::bind_method(_MD("resize", "width", "height", "interpolation"), &ImageRef::resize, DEFVAL(1));
	ObjectTypeDB::bind_method(_MD("crop", "width", "height"), &ImageRef::crop);
	ObjectTypeDB::bind_method(_MD("flip_x"), &ImageRef::flip_x);
	ObjectTypeDB::bind_method(_MD("flip_y"), &ImageRef::flip_y);
	ObjectTypeDB::bind_method(_MD("convert", "format"), &ImageRef::convert);
	ObjectTypeDB::bind_method(_MD("decompress"), &ImageRef::decompress);
	ObjectTypeDB::bind_method(_MD("is_compressed"), &ImageRef::is_compressed);
	ObjectTypeDB::bind_method(_MD("detect_alpha"), &ImageRef::detect_alpha);
	ObjectTypeDB::bind_method(_MD("is_invisible"), &ImageRef::is_invisible);
	ObjectTypeDB::bind_method(_MD("fix_alpha_edges"), &ImageRef::fix_alpha_edges);
	ObjectTypeDB::bind_method(_MD("premultiply_alpha"), &ImageRef::premultiply_alpha);
	ObjectTypeDB::bind_method(_MD("srgb_to_linear"), &ImageRef::srgb_to_linear);
}
