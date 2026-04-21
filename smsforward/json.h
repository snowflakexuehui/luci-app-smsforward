
/*
<https://github.com/rafagafe/tiny-json>

  Licensed under the MIT License <http://opensource.org/licenses/MIT>.
  SPDX-License-Identifier: MIT
  Copyright (c) 2018 Rafa Garcia <rafagarcia77@gmail.com>.
  Permission is hereby  granted, free of charge, to any  person obtaining a copy
  of this software and associated  documentation files (the "Software"), to deal
  in the Software  without restriction, including without  limitation the rights
  to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
  copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
  IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
  FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
  AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
  LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifndef MAKE_JSON_H
#define	MAKE_JSON_H

#ifdef	__cplusplus
extern "C" {
#endif

/** @defgroup makejoson Make JSON.
  * @{ */

/** Open a JSON object in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_objOpen( char* dest, char const* name, size_t* remLen );

/** Close a JSON object in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_objClose( char* dest, size_t* remLen );

/** Used to finish the root JSON object. After call json_objClose().
  * @param dest Pointer to the end of JSON under construction.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_end( char* dest, size_t* remLen );

/** Open an array in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_arrOpen( char* dest, char const* name, size_t* remLen );

/** Close an array in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_arrClose( char* dest, size_t* remLen );
/** Add a text property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value A valid null-terminated string with the value.
  *              Backslash escapes will be added for special characters.
  * @param len Max length of value. < 0 for unlimit.  
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */  
char* json_nstr( char* dest, char const* name, char const* value, int len, size_t* remLen );

/** Add a text property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value A valid null-terminated string with the value.
  *              Backslash escapes will be added for special characters.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
static inline char* json_str( char* dest, char const* name, char const* value, size_t* remLen ) {
    return json_nstr( dest, name, value, -1, remLen );  
}

/** Add a boolean property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Zero for false. Non zero for true.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_bool( char* dest, char const* name, int value, size_t* remLen );

/** Add a null property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_null( char* dest, char const* name, size_t* remLen );

/** Add an integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_int( char* dest, char const* name, int value, size_t* remLen );

/** Add an unsigned integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_uint( char* dest, char const* name, unsigned int value, size_t* remLen );

/** Add a long integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_long( char* dest, char const* name, long int value, size_t* remLen );

/** Add an unsigned long integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_ulong( char* dest, char const* name, unsigned long int value, size_t* remLen );

/** Add a long long integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_verylong( char* dest, char const* name, long long int value, size_t* remLen );

/** Add a double precision number property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* json_double( char* dest, char const* name, double value, size_t* remLen );

/** @ } */

// Parsing convenience API (implemented in src/json.c using public-domain parser)
// 返回malloc出来的字符串（调用方负责free）。找不到返回NULL。
char* json_find_string(const char* json_text, const char* key);
// 保留JSON中的转义字符，直接返回原始字面量内容（不反转义）
char* json_find_string_raw(const char* json_text, const char* key);
// 找到返回1并写入*out，未找到返回0（并将*out设为defval）。
int json_find_int(const char* json_text, const char* key, int defval, int* out);
// 在 parent_key 对象内查找
char* json_find_string_in(const char* json_text, const char* parent_key, const char* key);
int json_find_int_in(const char* json_text, const char* parent_key, const char* key, int defval, int* out);

/* ---------------- Embedded JSON/SJSON parser (public domain, Mattias Jansson) ---------------- */
/* 为了简化项目结构，将解析器实现内联在本头文件中（static函数不会产生重复定义）。 */

typedef size_t json_size_t;
enum json_type_t { JSON_UNDEFINED = 0, JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_PRIMITIVE };
struct json_token_t {
    json_size_t id; json_size_t id_length; json_size_t value; json_size_t value_length;
    json_size_t child; json_size_t sibling; enum json_type_t type;
};

#define JSON_STRING_CONST(s) (s), (sizeof((s))-1)
#define JSON_INVALID_POS ((json_size_t)-1)

static struct json_token_t* json_get_token(struct json_token_t* tokens, json_size_t capacity, json_size_t index) { return index < capacity ? tokens + index : 0; }
static bool json_is_valid_token(struct json_token_t* tokens, json_size_t capacity, json_size_t index) { struct json_token_t* token = json_get_token(tokens, capacity, index); return token ? (token->type != JSON_UNDEFINED) : true; }
static void json_set_token_primitive(struct json_token_t* tokens, json_size_t capacity, json_size_t current, enum json_type_t type, json_size_t value, json_size_t value_length) { struct json_token_t* token = json_get_token(tokens, capacity, current); if (token) { token->type = type; token->child = 0; token->sibling = 0; token->value = value; token->value_length = value_length; } }
static struct json_token_t* json_set_token_complex(struct json_token_t* tokens, json_size_t capacity, json_size_t current, enum json_type_t type, json_size_t pos) { struct json_token_t* token = json_get_token(tokens, capacity, current); if (token) { token->type = type; token->child = current + 1; token->sibling = 0; token->value = pos; token->value_length = 0; } return token; }
static void json_set_token_id(struct json_token_t* tokens, json_size_t capacity, json_size_t current, json_size_t id, json_size_t id_length) { struct json_token_t* token = json_get_token(tokens, capacity, current); if (token) { token->id = id; token->id_length = id_length; } }
static bool json_is_whitespace(char c) { return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'); }
static bool json_is_token_delimiter(char c) { return json_is_whitespace(c) || (c == ']') || (c == '}') || (c == ','); }
static json_size_t json_skip_whitespace(const char* buffer, json_size_t length, json_size_t pos) { while (pos < length) { if (!json_is_whitespace(buffer[pos])) return pos; ++pos; } return pos; }
static char json_hex_char(unsigned char val) { if (val < 10) return '0' + (char)val; else if (val < 16) return 'a' + (char)(val - 10); return '0'; }
static json_size_t json_parse_string(const char* buffer, json_size_t length, json_size_t pos, bool key, bool simple) { json_size_t start = pos; json_size_t esc; while (pos < length) { char c = buffer[pos]; if (simple && (json_is_token_delimiter(c) || (key && ((c == '=') || (c == ':'))))) return pos - start; if (c == '"') return pos - start; ++pos; if (c == '\\' && (pos < length)) { switch (buffer[pos]) { case '"': case '/': case '\\': case 'b': case 'f': case 'r': case 'n': case 't': break; case 'u': for (esc = 0; esc < 4 && pos < length; ++esc) { ++pos; if (!((buffer[pos] >= 48 && buffer[pos] <= 57) || (buffer[pos] >= 65 && buffer[pos] <= 70) || (buffer[pos] >= 97 && buffer[pos] <= 102))) return JSON_INVALID_POS; } break; default: return JSON_INVALID_POS; } ++pos; } } return simple ? pos - start : JSON_INVALID_POS; }
static json_size_t json_parse_number(const char* buffer, json_size_t length, json_size_t pos) { json_size_t start = pos; bool has_dot = false; bool has_digit = false; bool has_exp = false; while (pos < length) { char c = buffer[pos]; if (json_is_token_delimiter(c)) break; if (c == '-') { if (start != pos) return JSON_INVALID_POS; } else if (c == '.') { if (has_dot || has_exp) return JSON_INVALID_POS; has_dot = true; } else if ((c == 'e') || (c == 'E')) { if (!has_digit || has_exp) return JSON_INVALID_POS; has_exp = true; if ((pos + 1) < length) { if ((buffer[pos + 1] == '+') || (buffer[pos + 1] == '-')) ++pos; } } else if ((c < '0') || (c > '9')) return JSON_INVALID_POS; else has_digit = true; ++pos; } return has_digit ? (pos - start) : JSON_INVALID_POS; }
static json_size_t json_parse_object(const char* buffer, json_size_t length, json_size_t pos, struct json_token_t* tokens, json_size_t capacity, json_size_t* current, bool simple);
static json_size_t json_parse_value(const char* buffer, json_size_t length, json_size_t pos, struct json_token_t* tokens, json_size_t capacity, json_size_t* current, bool simple);
static json_size_t json_parse_array(const char* buffer, json_size_t length, json_size_t pos, struct json_token_t* tokens, json_size_t capacity, json_size_t owner, json_size_t* current, bool simple);
static json_size_t json_parse_object(const char* buffer, json_size_t length, json_size_t pos, struct json_token_t* tokens, json_size_t capacity, json_size_t* current, bool simple) { struct json_token_t* token; json_size_t string; bool simple_string; json_size_t last = 0; pos = json_skip_whitespace(buffer, length, pos); while (pos < length) { char c = buffer[pos++]; switch (c) { case '}': if (last && !json_is_valid_token(tokens, capacity, last)) return JSON_INVALID_POS; return pos; case ',': if (!last || !json_is_valid_token(tokens, capacity, last)) return JSON_INVALID_POS; token = json_get_token(tokens, capacity, last); if (token) token->sibling = *current; last = 0; pos = json_skip_whitespace(buffer, length, pos); break; case '"': default: if (last) return JSON_INVALID_POS; if (c != '"') { if (!simple) return JSON_INVALID_POS; simple_string = true; --pos; } else { simple_string = false; } string = json_parse_string(buffer, length, pos, true, simple_string); if (string == JSON_INVALID_POS) return JSON_INVALID_POS; last = *current; json_set_token_id(tokens, capacity, *current, pos, string); if (!simple || ((pos + string < length) && (buffer[pos + string] == '"'))) ++string; pos += string; pos = json_skip_whitespace(buffer, length, pos); if ((buffer[pos] != ':') && (!simple || (buffer[pos] != '='))) return JSON_INVALID_POS; pos = json_parse_value(buffer, length, pos + 1, tokens, capacity, current, simple); pos = json_skip_whitespace(buffer, length, pos); if (simple_string && ((pos < length) && (buffer[pos] != ',') && (buffer[pos] != '}'))) { token = json_get_token(tokens, capacity, last); if (token) token->sibling = *current; last = 0; } break; } } return simple ? pos : JSON_INVALID_POS; }
static json_size_t json_parse_array(const char* buffer, json_size_t length, json_size_t pos, struct json_token_t* tokens, json_size_t capacity, json_size_t owner, json_size_t* current, bool simple) { struct json_token_t* parent = json_get_token(tokens, capacity, owner); struct json_token_t* token; json_size_t now; json_size_t last = 0; pos = json_skip_whitespace(buffer, length, pos); if (buffer[pos] == ']') { if (parent) parent->child = 0; return json_skip_whitespace(buffer, length, ++pos); } while (pos < length) { now = *current; json_set_token_id(tokens, capacity, now, 0, 0); pos = json_parse_value(buffer, length, pos, tokens, capacity, current, simple); if (pos == JSON_INVALID_POS) return JSON_INVALID_POS; if (parent) parent->value_length++; if (last) { token = json_get_token(tokens, capacity, last); if (token) token->sibling = now; } last = now; pos = json_skip_whitespace(buffer, length, pos); if (buffer[pos] == ',') ++pos; else if (buffer[pos] == ']') return ++pos; else if (!simple || buffer[pos] == '}') return JSON_INVALID_POS; } return JSON_INVALID_POS; }
static json_size_t json_parse_value(const char* buffer, json_size_t length, json_size_t pos, struct json_token_t* tokens, json_size_t capacity, json_size_t* current, bool simple) { struct json_token_t* subtoken; json_size_t string, owner; bool simple_string; pos = json_skip_whitespace(buffer, length, pos); while (pos < length) { char c = buffer[pos++]; switch (c) { case '{': subtoken = json_set_token_complex(tokens, capacity, *current, JSON_OBJECT, pos - 1); ++(*current); pos = json_parse_object(buffer, length, pos, tokens, capacity, current, simple); if (subtoken) { if (pos != JSON_INVALID_POS) subtoken->value_length = (pos - subtoken->value); if (subtoken->child == *current) subtoken->child = 0; } return pos; case '[': owner = *current; json_set_token_complex(tokens, capacity, *current, JSON_ARRAY, 0); ++(*current); pos = json_parse_array(buffer, length, pos, tokens, capacity, owner, current, simple); return pos; case '-': case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '.': string = json_parse_number(buffer, length, pos - 1); if (string == JSON_INVALID_POS) return JSON_INVALID_POS; json_set_token_primitive(tokens, capacity, *current, JSON_PRIMITIVE, pos - 1, string); ++(*current); return pos + string - 1; case 't': case 'f': case 'n': if ((c == 't') && (length - pos >= 4) && (buffer[pos] == 'r') && (buffer[pos + 1] == 'u') && (buffer[pos + 2] == 'e') && json_is_token_delimiter(buffer[pos + 3])) { json_set_token_primitive(tokens, capacity, *current, JSON_PRIMITIVE, pos - 1, 4); ++(*current); return pos + 3; } if ((c == 'f') && (length - pos >= 5) && (buffer[pos] == 'a') && (buffer[pos + 1] == 'l') && (buffer[pos + 2] == 's') && (buffer[pos + 3] == 'e') && json_is_token_delimiter(buffer[pos + 4])) { json_set_token_primitive(tokens, capacity, *current, JSON_PRIMITIVE, pos - 1, 5); ++(*current); return pos + 4; } if ((c == 'n') && (length - pos >= 4) && (buffer[pos] == 'u') && (buffer[pos + 1] == 'l') && (buffer[pos + 2] == 'l') && json_is_token_delimiter(buffer[pos + 3])) { json_set_token_primitive(tokens, capacity, *current, JSON_PRIMITIVE, pos - 1, 4); ++(*current); return pos + 3; } if (!simple) return JSON_INVALID_POS; /* fallthrough */ case '"': default: if (c != '"') { if (!simple) return JSON_INVALID_POS; simple_string = true; --pos; } else { simple_string = false; } string = json_parse_string(buffer, length, pos, false, simple_string); if (string == JSON_INVALID_POS) return JSON_INVALID_POS; json_set_token_primitive(tokens, capacity, *current, JSON_STRING, pos, string); ++(*current); if (!simple_string || ((pos + string < length) && (buffer[pos + string] == '"'))) ++string; return pos + string; } } return JSON_INVALID_POS; }
static json_size_t json_parse(const char* buffer, json_size_t size, struct json_token_t* tokens, json_size_t capacity) { json_size_t current = 0; json_set_token_id(tokens, capacity, current, 0, 0); json_set_token_primitive(tokens, capacity, current, JSON_UNDEFINED, 0, 0); if (json_parse_value(buffer, size, 0, tokens, capacity, &current, false) == JSON_INVALID_POS) return 0; return current; }
static json_size_t sjson_parse(const char* buffer, json_size_t size, struct json_token_t* tokens, json_size_t capacity) { json_size_t current = 0; json_size_t pos = json_skip_whitespace(buffer, size, 0); if ((pos < size) && (buffer[pos] != '{')) { json_set_token_id(tokens, capacity, current, 0, 0); json_set_token_complex(tokens, capacity, current, JSON_OBJECT, pos); ++current; if (json_parse_object(buffer, size, pos, tokens, capacity, &current, true) == JSON_INVALID_POS) return 0; if (capacity) tokens[0].value_length = size - tokens[0].value; return current; } if (json_parse_value(buffer, size, pos, tokens, capacity, &current, true) == JSON_INVALID_POS) return 0; return current; }
static unsigned int json_get_num_bytes_as_utf8(unsigned int val) { if (val >= 0x04000000) return 6; else if (val >= 0x00200000) return 5; else if (val >= 0x00010000) return 4; else if (val >= 0x00000800) return 3; else if (val >= 0x00000080) return 2; return 1; }
static json_size_t json_encode_utf8(char* str, unsigned int val) { unsigned int num, j; if (val < 0x80) { *str = (char)val; return 1; } num = json_get_num_bytes_as_utf8(val) - 1; *str++ = (char)((0x80U | (((1U << (num)) - 1) << (7U - num))) | ((val >> (6U * num)) & ((1U << (6U - num)) - 1))); for (j = 1; j <= num; ++j) *str++ = (char)(0x80U | ((val >> (6U * (num - j))) & 0x3F)); return num + 1; }
static json_size_t json_unescape(char* buffer, json_size_t capacity, const char* string, json_size_t length) { json_size_t i, j; json_size_t outlength = 0; unsigned int hexval, numbytes; for (i = 0; (i < length) && (outlength < capacity); ++i) { char c = string[i]; if ((c == '\\') && (i + 1 < length)) { c = string[++i]; switch (c) { case '"': case '/': case '\\': buffer[outlength++] = c; break; case 'b': buffer[outlength++] = '\b'; break; case 'f': buffer[outlength++] = '\f'; break; case 'r': buffer[outlength++] = '\r'; break; case 'n': buffer[outlength++] = '\n'; break; case 't': buffer[outlength++] = '\t'; break; case 'u': if (i + 4 < length) { unsigned int uival = 0; hexval = 0; for (j = 0; j < 4; ++j) { char val = string[++i]; if ((val >= 'a') && (val <= 'f')) uival = 10 + (val - 'a'); else if ((val >= 'A') && (val <= 'F')) uival = 10 + (val - 'A'); else if ((val >= '0') && (val <= '9')) uival = val - '0'; hexval |= uival << (3 - j); } numbytes = json_get_num_bytes_as_utf8(hexval); if ((outlength + numbytes) < capacity) outlength += json_encode_utf8(buffer + outlength, hexval); } break; default: break; } } else { buffer[outlength++] = c; } } return outlength; }
static bool json_string_equal(const char* rhs, size_t rhs_length, const char* lhs, size_t lhs_length) { if (rhs_length && (lhs_length == rhs_length)) { return (memcmp(rhs, lhs, rhs_length) == 0); } return (!rhs_length && !lhs_length); }

#ifdef	__cplusplus
}
#endif

#endif	/* MAKE_JSON_H */