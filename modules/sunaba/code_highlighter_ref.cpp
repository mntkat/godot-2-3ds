/*************************************************************************/
/*  code_highlighter_ref.cpp                                             */
/*************************************************************************/

#include "code_highlighter_ref.h"

#include "scene/gui/text_edit.h"

static String region_key(const String &p_start, const String &p_end) {
	return p_start + " " + p_end;
}

static String region_start(const String &p_key) {
	int sp = p_key.find(" ");
	return sp < 0 ? p_key : p_key.substr(0, sp);
}

static String region_end(const String &p_key) {
	int sp = p_key.find(" ");
	return sp < 0 ? String() : p_key.substr(sp + 1, p_key.length());
}

Dictionary CodeHighlighterRef::copy_of(const Dictionary &p_dict) {
	Dictionary d;
	const Variant *k = NULL;
	while ((k = p_dict.next(k)))
		d[*k] = p_dict[*k];
	return d;
}

Color CodeHighlighterRef::get_keyword_color(const String &p_keyword) const {
	return keyword_colors.has(p_keyword) ? Color(keyword_colors[p_keyword]) : Color();
}

Color CodeHighlighterRef::get_member_keyword_color(const String &p_keyword) const {
	return member_keyword_colors.has(p_keyword) ? Color(member_keyword_colors[p_keyword]) : Color();
}

void CodeHighlighterRef::set_color_regions(const Dictionary &p_regions) {
	clear_color_regions();
	const Variant *k = NULL;
	while ((k = p_regions.next(k))) {
		String key = *k;
		String end = region_end(key);
		add_color_region(region_start(key), end, p_regions[*k], end == "");
	}
}

void CodeHighlighterRef::add_color_region(const String &p_start, const String &p_end, const Color &p_color, bool p_line_only) {
	remove_color_region(p_start);
	String key = region_key(p_start, p_end);
	color_regions[key] = p_color;
	region_line_only[key] = p_line_only || p_end == "";
}

void CodeHighlighterRef::remove_color_region(const String &p_start) {
	List<Variant> keys;
	color_regions.get_key_list(&keys);
	for (List<Variant>::Element *E = keys.front(); E; E = E->next()) {
		if (region_start(E->get()) == p_start) {
			color_regions.erase(E->get());
			region_line_only.erase(E->get());
		}
	}
}

bool CodeHighlighterRef::has_color_region(const String &p_start) const {
	const Variant *k = NULL;
	while ((k = color_regions.next(k))) {
		if (region_start(*k) == p_start)
			return true;
	}
	return false;
}

void CodeHighlighterRef::clear_color_regions() {
	color_regions.clear();
	region_line_only.clear();
}

void CodeHighlighterRef::apply_to(Object *p_text_edit) const {

	TextEdit *te = p_text_edit ? p_text_edit->cast_to<TextEdit>() : NULL;
	if (!te)
		return;
	te->clear_colors();
	const Variant *k = NULL;
	while ((k = keyword_colors.next(k)))
		te->add_keyword_color(*k, keyword_colors[*k]);
	// Godot 2 has no member keywords; color them like keywords.
	while ((k = member_keyword_colors.next(k)))
		te->add_keyword_color(*k, member_keyword_colors[*k]);
	while ((k = color_regions.next(k))) {
		String key = *k;
		te->add_color_region(region_start(key), region_end(key), color_regions[*k], region_line_only[*k]);
	}
	te->set_symbol_color(symbol_color);
	te->add_color_override("number_color", number_color);
	te->add_color_override("function_color", function_color);
	te->add_color_override("member_variable_color", member_variable_color);
	te->set_syntax_coloring(true);
}

uint32_t CodeHighlighterRef::get_signature() const {

	uint32_t h = hash_djb2_one_32(keyword_colors.size());
	const Variant *k = NULL;
	while ((k = keyword_colors.next(k)))
		h = hash_djb2_one_32(k->hash() ^ keyword_colors[*k].hash(), h);
	while ((k = member_keyword_colors.next(k)))
		h = hash_djb2_one_32(k->hash() ^ member_keyword_colors[*k].hash(), h);
	while ((k = color_regions.next(k)))
		h = hash_djb2_one_32(k->hash() ^ color_regions[*k].hash() ^ region_line_only[*k].hash(), h);
	h = hash_djb2_one_32(Variant(number_color).hash(), h);
	h = hash_djb2_one_32(Variant(symbol_color).hash(), h);
	h = hash_djb2_one_32(Variant(function_color).hash(), h);
	h = hash_djb2_one_32(Variant(member_variable_color).hash(), h);
	return h;
}

void CodeHighlighterRef::_bind_methods() {

	ObjectTypeDB::bind_method(_MD("set_number_color", "color"), &CodeHighlighterRef::set_number_color);
	ObjectTypeDB::bind_method(_MD("get_number_color"), &CodeHighlighterRef::get_number_color);
	ObjectTypeDB::bind_method(_MD("set_symbol_color", "color"), &CodeHighlighterRef::set_symbol_color);
	ObjectTypeDB::bind_method(_MD("get_symbol_color"), &CodeHighlighterRef::get_symbol_color);
	ObjectTypeDB::bind_method(_MD("set_function_color", "color"), &CodeHighlighterRef::set_function_color);
	ObjectTypeDB::bind_method(_MD("get_function_color"), &CodeHighlighterRef::get_function_color);
	ObjectTypeDB::bind_method(_MD("set_member_variable_color", "color"), &CodeHighlighterRef::set_member_variable_color);
	ObjectTypeDB::bind_method(_MD("get_member_variable_color"), &CodeHighlighterRef::get_member_variable_color);
	ObjectTypeDB::bind_method(_MD("set_keyword_colors", "keywords"), &CodeHighlighterRef::set_keyword_colors);
	ObjectTypeDB::bind_method(_MD("get_keyword_colors"), &CodeHighlighterRef::get_keyword_colors);
	ObjectTypeDB::bind_method(_MD("add_keyword_color", "keyword", "color"), &CodeHighlighterRef::add_keyword_color);
	ObjectTypeDB::bind_method(_MD("remove_keyword_color", "keyword"), &CodeHighlighterRef::remove_keyword_color);
	ObjectTypeDB::bind_method(_MD("has_keyword_color", "keyword"), &CodeHighlighterRef::has_keyword_color);
	ObjectTypeDB::bind_method(_MD("get_keyword_color", "keyword"), &CodeHighlighterRef::get_keyword_color);
	ObjectTypeDB::bind_method(_MD("clear_keyword_colors"), &CodeHighlighterRef::clear_keyword_colors);
	ObjectTypeDB::bind_method(_MD("set_member_keyword_colors", "member_keyword"), &CodeHighlighterRef::set_member_keyword_colors);
	ObjectTypeDB::bind_method(_MD("get_member_keyword_colors"), &CodeHighlighterRef::get_member_keyword_colors);
	ObjectTypeDB::bind_method(_MD("add_member_keyword_color", "member_keyword", "color"), &CodeHighlighterRef::add_member_keyword_color);
	ObjectTypeDB::bind_method(_MD("remove_member_keyword_color", "member_keyword"), &CodeHighlighterRef::remove_member_keyword_color);
	ObjectTypeDB::bind_method(_MD("has_member_keyword_color", "member_keyword"), &CodeHighlighterRef::has_member_keyword_color);
	ObjectTypeDB::bind_method(_MD("get_member_keyword_color", "member_keyword"), &CodeHighlighterRef::get_member_keyword_color);
	ObjectTypeDB::bind_method(_MD("clear_member_keyword_colors"), &CodeHighlighterRef::clear_member_keyword_colors);
	ObjectTypeDB::bind_method(_MD("set_color_regions", "color_regions"), &CodeHighlighterRef::set_color_regions);
	ObjectTypeDB::bind_method(_MD("get_color_regions"), &CodeHighlighterRef::get_color_regions);
	ObjectTypeDB::bind_method(_MD("add_color_region", "start_key", "end_key", "color", "line_only"), &CodeHighlighterRef::add_color_region, DEFVAL(false));
	ObjectTypeDB::bind_method(_MD("remove_color_region", "start_key"), &CodeHighlighterRef::remove_color_region);
	ObjectTypeDB::bind_method(_MD("has_color_region", "start_key"), &CodeHighlighterRef::has_color_region);
	ObjectTypeDB::bind_method(_MD("clear_color_regions"), &CodeHighlighterRef::clear_color_regions);

	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "number_color"), _SCS("set_number_color"), _SCS("get_number_color"));
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "symbol_color"), _SCS("set_symbol_color"), _SCS("get_symbol_color"));
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "function_color"), _SCS("set_function_color"), _SCS("get_function_color"));
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "member_variable_color"), _SCS("set_member_variable_color"), _SCS("get_member_variable_color"));
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "keyword_colors"), _SCS("set_keyword_colors"), _SCS("get_keyword_colors"));
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "member_keyword_colors"), _SCS("set_member_keyword_colors"), _SCS("get_member_keyword_colors"));
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "color_regions"), _SCS("set_color_regions"), _SCS("get_color_regions"));
}

CodeHighlighterRef::CodeHighlighterRef() {
	number_color = Color(1, 1, 1);
	symbol_color = Color(1, 1, 1);
	function_color = Color(1, 1, 1);
	member_variable_color = Color(1, 1, 1);
}
