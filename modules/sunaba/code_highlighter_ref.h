/*************************************************************************/
/*  code_highlighter_ref.h                                               */
/*************************************************************************/
/*  Godot 4's CodeHighlighter (keyword and member colors, color regions, */
/*  number/symbol/function/member colors). Godot 2 has no highlighter    */
/*  resources; its TextEdit colors syntax itself. Setting one as a       */
/*  TextEdit's syntax_highlighter (compatibility layer) applies it with  */
/*  clear_colors, add_keyword_color, add_color_region and theme color    */
/*  overrides. "CodeHighlighter" and "SyntaxHighlighter" are aliases.    */
/*************************************************************************/

#ifndef SUNABA_CODE_HIGHLIGHTER_REF_H
#define SUNABA_CODE_HIGHLIGHTER_REF_H

#include "resource.h"

class CodeHighlighterRef : public Resource {
	OBJ_TYPE(CodeHighlighterRef, Resource);

	Color number_color;
	Color symbol_color;
	Color function_color;
	Color member_variable_color;
	Dictionary keyword_colors;
	Dictionary member_keyword_colors;
	// "start end" -> Color, as in Godot 4; line_only regions end with "".
	Dictionary color_regions;
	Dictionary region_line_only;

	static Dictionary copy_of(const Dictionary &p_dict);

protected:
	static void _bind_methods();

public:
	void set_number_color(const Color &p_color) { number_color = p_color; }
	Color get_number_color() const { return number_color; }
	void set_symbol_color(const Color &p_color) { symbol_color = p_color; }
	Color get_symbol_color() const { return symbol_color; }
	void set_function_color(const Color &p_color) { function_color = p_color; }
	Color get_function_color() const { return function_color; }
	void set_member_variable_color(const Color &p_color) { member_variable_color = p_color; }
	Color get_member_variable_color() const { return member_variable_color; }

	void set_keyword_colors(const Dictionary &p_colors) { keyword_colors = copy_of(p_colors); }
	Dictionary get_keyword_colors() const { return keyword_colors; }
	void add_keyword_color(const String &p_keyword, const Color &p_color) { keyword_colors[p_keyword] = p_color; }
	void remove_keyword_color(const String &p_keyword) { keyword_colors.erase(p_keyword); }
	bool has_keyword_color(const String &p_keyword) const { return keyword_colors.has(p_keyword); }
	Color get_keyword_color(const String &p_keyword) const;
	void clear_keyword_colors() { keyword_colors.clear(); }

	void set_member_keyword_colors(const Dictionary &p_colors) { member_keyword_colors = copy_of(p_colors); }
	Dictionary get_member_keyword_colors() const { return member_keyword_colors; }
	void add_member_keyword_color(const String &p_keyword, const Color &p_color) { member_keyword_colors[p_keyword] = p_color; }
	void remove_member_keyword_color(const String &p_keyword) { member_keyword_colors.erase(p_keyword); }
	bool has_member_keyword_color(const String &p_keyword) const { return member_keyword_colors.has(p_keyword); }
	Color get_member_keyword_color(const String &p_keyword) const;
	void clear_member_keyword_colors() { member_keyword_colors.clear(); }

	void set_color_regions(const Dictionary &p_regions);
	Dictionary get_color_regions() const { return color_regions; }
	void add_color_region(const String &p_start, const String &p_end, const Color &p_color, bool p_line_only = false);
	void remove_color_region(const String &p_start);
	bool has_color_region(const String &p_start) const;
	void clear_color_regions();

	// Applies the colors to a Godot 2 TextEdit.
	void apply_to(Object *p_text_edit) const;
	// Changes whenever the colors do; lets repeated sets be skipped.
	uint32_t get_signature() const;

	CodeHighlighterRef();
};

#endif // SUNABA_CODE_HIGHLIGHTER_REF_H
