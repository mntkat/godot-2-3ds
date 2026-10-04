/*************************************************************************/
/*  image_ref.h                                                          */
/*************************************************************************/
/*  Godot 2's Image is a builtin value type; Godot 4's is a Resource     */
/*  class. ImageRef wraps a Godot 2 Image and implements Godot 4's       */
/*  Image API (load_png_from_buffer, get_pixel, resize, ...), with       */
/*  Godot 4 format and interpolation values. "Image" is an alias for it  */
/*  in the compatibility layer.                                          */
/*                                                                       */
/*  The Lua runtime wraps Image values crossing into Lua automatically,  */
/*  and unwraps them when they are passed back to Godot 2 APIs.          */
/*************************************************************************/

#ifndef SUNABA_IMAGE_REF_H
#define SUNABA_IMAGE_REF_H

#include "image.h"
#include "resource.h"

class ImageRef : public Resource {
	OBJ_TYPE(ImageRef, Resource);

	Image image;

	Error _load_from_buffer(const ByteArray &p_data, const String &p_extension);
	static Image _to_image(const Variant &p_image);

protected:
	static void _bind_methods();

public:
	static Ref<ImageRef> wrap(const Image &p_image);
	const Image &get_image() const { return image; }
	void set_image(const Image &p_image) { image = p_image; }

	// Godot 4 Image.Format <-> Godot 2 Image::Format
	static int format_to_godot4(Image::Format p_format);
	static Image::Format format_from_godot4(int p_format);

	Error load(const String &p_path);
	Error load_png_from_buffer(const ByteArray &p_data);
	Error load_jpg_from_buffer(const ByteArray &p_data);
	Error load_webp_from_buffer(const ByteArray &p_data);
	Error load_unsupported_from_buffer(const ByteArray &p_data);
	Error save_png(const String &p_path) const;
	ByteArray save_png_to_buffer() const;

	int get_width() const;
	int get_height() const;
	Vector2 get_size() const;
	int get_format() const;
	bool is_empty() const;
	bool has_mipmaps() const;
	int get_mipmap_count() const;
	Error generate_mipmaps(bool p_renormalize = false);
	void clear_mipmaps();
	ByteArray get_data() const;
	int get_data_size() const;

	Color get_pixel(int p_x, int p_y) const;
	Color get_pixelv(const Vector2 &p_pos) const;
	void set_pixel(int p_x, int p_y, const Color &p_color);
	void set_pixelv(const Vector2 &p_pos, const Color &p_color);
	void fill(const Color &p_color);
	void fill_rect(const Rect2 &p_rect, const Color &p_color);

	void create(int p_width, int p_height, bool p_mipmaps, int p_format);
	void create_from_data(int p_width, int p_height, bool p_mipmaps, int p_format, const ByteArray &p_data);
	void set_data(int p_width, int p_height, bool p_mipmaps, int p_format, const ByteArray &p_data);
	void copy_from(const Variant &p_src);
	Ref<ImageRef> get_region(const Rect2 &p_rect) const;
	Rect2 get_used_rect() const;
	void blit_rect(const Variant &p_src, const Rect2 &p_src_rect, const Vector2 &p_dst);
	void blend_rect(const Variant &p_src, const Rect2 &p_src_rect, const Vector2 &p_dst);

	void resize(int p_width, int p_height, int p_interpolation = 1);
	void crop(int p_width, int p_height);
	void flip_x();
	void flip_y();
	void convert(int p_format);
	Error decompress();
	bool is_compressed() const;
	int detect_alpha() const;
	bool is_invisible() const;
	void fix_alpha_edges();
	void premultiply_alpha();
	void srgb_to_linear();

	ImageRef() {}
};

#endif // SUNABA_IMAGE_REF_H
