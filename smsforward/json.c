
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

#include <stddef.h> // For NULL
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "json.h"

/** Add a character at the end of a string.
  * @param dest Pointer to the null character of the string
  * @param ch Value to be added.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the null character of the destination string. */
static char* chtoa( char* dest, char ch, size_t* remLen ) {
    if (*remLen != 0) {
        --*remLen;
        *dest   = ch;
        *++dest = '\0';
    }
    return dest;
}

/** Copy a null-terminated string.
  * @param dest Destination memory block.
  * @param src Source string.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the null character of the destination string. */
static char* atoa( char* dest, char const* src, size_t* remLen  ) {
    for( ; *src != '\0' && *remLen != 0; ++dest, ++src, --*remLen )
        *dest = *src;
    *dest = '\0';
    return dest;
}

/* Open a JSON object in a JSON string. */
char* json_objOpen( char* dest, char const* name, size_t* remLen  ) {
    if ( NULL == name )
        dest = chtoa( dest, '{', remLen );
    else {
        dest = chtoa( dest, '\"', remLen );
        dest = atoa( dest, name, remLen );
        dest = atoa( dest, "\":{", remLen );
    }
    return dest;
}

/* Close a JSON object in a JSON string. */
char* json_objClose( char* dest, size_t* remLen  ) {
    if ( dest[-1] == ',' )
    {
        --dest;
        ++*remLen;
    }
    return atoa( dest, "},", remLen );
}

/* Open an array in a JSON string. */
char* json_arrOpen( char* dest, char const* name, size_t* remLen  ) {
    if ( NULL == name )
        dest = chtoa( dest, '[', remLen );
    else {
        dest = chtoa( dest, '\"', remLen );
        dest = atoa( dest, name, remLen );
        dest = atoa( dest, "\":[", remLen );
    }
    return dest;
}

/* Close an array in a JSON string. */
char* json_arrClose( char* dest, size_t* remLen  ) {
    if ( dest[-1] == ',')
    {
        --dest;
        ++*remLen;
    }
    return atoa( dest, "],", remLen );
}

/** Add the name of a text property.
  * @param dest Destination memory.
  * @param name The name of the property.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the next char. */
static char* strname( char* dest, char const* name, size_t* remLen  ) {
    dest = chtoa( dest, '\"', remLen );
    if ( NULL != name ) {
        dest = atoa( dest, name, remLen );
        dest = atoa( dest, "\":\"", remLen );
    }
    return dest;
}

/** Get the hexadecimal digit of the least significant nibble of a integer. */
static int nibbletoch( int nibble ) {
    return "0123456789ABCDEF"[ nibble % 16u ];
}

/** Get the escape character of a non-printable.
  * @param ch Character source.
  * @return The escape character or null character if error. */
static int escape( int ch ) {
    int i;
    static struct { char code; char ch; } const pair[] = {
        { '\"', '\"' }, { '\\', '\\' }, { '/',  '/'  }, { 'b',  '\b' },
        { 'f',  '\f' }, { 'n',  '\n' }, { 'r',  '\r' }, { 't',  '\t' },
    };
    for( i = 0; i < sizeof pair / sizeof *pair; ++i )
        if ( ch == pair[i].ch )
            return pair[i].code;
    return '\0';
}

/** Copy a null-terminated string inserting escape characters if needed.
  * @param dest Destination memory block.
  * @param src Source string.
  * @param len Max length of source. < 0 for unlimit.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the null character of the destination string. */
static char* atoesc( char* dest, char const* src, int len, size_t* remLen  ) {
    int i;
    for( i = 0; src[i] != '\0' && ( i < len || 0 > len ) && *remLen != 0; ++dest, ++i, --*remLen ) {
        if ( src[i] >= ' ' && src[i] != '\"' && src[i] != '\\' && src[i] != '/' )
            *dest = src[i];
        else {
            if (*remLen != 0) {
                *dest++ = '\\';
                --*remLen;
                int const esc = escape( src[i] );
                if ( esc ) {
                    if (*remLen != 0)
                        *dest = esc;
                } else {
                    if (*remLen != 0) {
                        --*remLen;
                        *dest++ = 'u';
                    }
                    if (*remLen != 0) {
                        --*remLen;
                        *dest++ = '0';
                    }
                    if (*remLen != 0) {
                        --*remLen;
                        *dest++ = '0';
                    }
                    if (*remLen != 0) {
                        --*remLen;
                        *dest++ = nibbletoch( src[i] / 16 );
                    }
                    if (*remLen != 0) {
                        --*remLen;
                        *dest++ = nibbletoch( src[i] );
                    }
                }
            }
        }

        if (*remLen == 0)
            break;
    }
    *dest = '\0';
    return dest;
}

/* Add a text property in a JSON string. */
char* json_nstr( char* dest, char const* name, char const* value, int len, size_t* remLen  ) {
    dest = strname( dest, name, remLen );
    dest = atoesc( dest, value, len, remLen );
    dest = atoa( dest, "\",", remLen );
    return dest;
}

/** Add the name of a primitive property.
  * @param dest Destination memory.
  * @param name The name of the property.
  * @param remLen Pointer to remaining length of dest
  * @return Pointer to the next char. */
static char* primitivename( char* dest, char const* name, size_t* remLen  ) {
    if( NULL == name )
        return dest;
    dest = chtoa( dest, '\"', remLen );
    dest = atoa( dest, name, remLen );
    dest = atoa( dest, "\":", remLen );
    return dest;
}

/*  Add a boolean property in a JSON string. */
char* json_bool( char* dest, char const* name, int value, size_t* remLen  ) {
    dest = primitivename( dest, name, remLen );
    dest = atoa( dest, value ? "true," : "false,", remLen );
    return dest;
}

/* Add a null property in a JSON string. */
char* json_null( char* dest, char const* name, size_t* remLen  ) {
    dest = primitivename( dest, name, remLen );
    dest = atoa( dest, "null,", remLen );
    return dest;
}

/* Used to finish the root JSON object. After call json_objClose(). */
char* json_end( char* dest, size_t* remLen ) {
    if ( ',' == dest[-1] ) {
        dest[-1] = '\0';
        --dest;
        ++*remLen;
    }
    return dest;
}

// ---------------- Parser convenience wrappers ----------------
// 查找根对象下 key 的字符串值，返回malloc的字符串（需free）
char* json_find_string(const char* json_text, const char* key) {
    if (!json_text || !key) return NULL;
    struct json_token_t tokens[128];
    memset(tokens, 0, sizeof(tokens));
    size_t len = strlen(json_text);
    json_size_t num = json_parse(json_text, (json_size_t)len, tokens, (json_size_t)(sizeof(tokens)/sizeof(tokens[0])));
    if (num == 0) return NULL;
    for (json_size_t i = 1; i < num; ++i) {
        struct json_token_t* t = &tokens[i];
        if (t->id && t->id_length) {
            // 先将键名反转义再比较，支持包含引号/转义的键
            char keybuf[256];
            json_size_t klen = json_unescape(keybuf, sizeof(keybuf) - 1, json_text + t->id, t->id_length);
            keybuf[klen < sizeof(keybuf) - 1 ? klen : (sizeof(keybuf) - 1)] = '\0';
            if (!json_string_equal(keybuf, klen, key, strlen(key))) continue;
            if (t->type == JSON_STRING || t->type == JSON_PRIMITIVE) {
                char* out = (char*)malloc(t->value_length + 1);
                if (!out) return NULL;
                json_size_t n = json_unescape(out, t->value_length, json_text + t->value, t->value_length);
                out[n] = '\0';
                return out;
            }
        }
    }
    return NULL;
}

// 返回JSON中字符串值的原始字面量（不反转义）。返回值malloc，需调用方free
char* json_find_string_raw(const char* json_text, const char* key) {
    if (!json_text || !key) return NULL;
    struct json_token_t tokens[128];
    memset(tokens, 0, sizeof(tokens));
    size_t len = strlen(json_text);
    json_size_t num = json_parse(json_text, (json_size_t)len, tokens, (json_size_t)(sizeof(tokens)/sizeof(tokens[0])));
    if (num == 0) return NULL;
    for (json_size_t i = 1; i < num; ++i) {
        struct json_token_t* t = &tokens[i];
        if (t->id && t->id_length) {
            char keybuf[256];
            json_size_t klen = json_unescape(keybuf, sizeof(keybuf) - 1, json_text + t->id, t->id_length);
            keybuf[klen < sizeof(keybuf) - 1 ? klen : (sizeof(keybuf) - 1)] = '\0';
            if (!json_string_equal(keybuf, klen, key, strlen(key))) continue;
            if (t->type == JSON_STRING) {
                // 直接拷贝原始片段（含转义），不做反转义
                char* out = (char*)malloc(t->value_length + 1);
                if (!out) return NULL;
                memcpy(out, json_text + t->value, t->value_length);
                out[t->value_length] = '\0';
                return out;
            } else if (t->type == JSON_PRIMITIVE) {
                // 原样返回
                char* out = (char*)malloc(t->value_length + 1);
                if (!out) return NULL;
                memcpy(out, json_text + t->value, t->value_length);
                out[t->value_length] = '\0';
                return out;
            }
        }
    }
    return NULL;
}

// 查找根对象下 key 的整数值，找到返回1并写入*out，未找到返回0（不改*out）
int json_find_int(const char* json_text, const char* key, int defval, int* out) {
    if (!json_text || !key || !out) return 0;
    struct json_token_t tokens[128];
    memset(tokens, 0, sizeof(tokens));
    size_t len = strlen(json_text);
    json_size_t num = json_parse(json_text, (json_size_t)len, tokens, (json_size_t)(sizeof(tokens)/sizeof(tokens[0])));
    if (num == 0) { return 0; }
    for (json_size_t i = 1; i < num; ++i) {
        struct json_token_t* t = &tokens[i];
        if (t->id && t->id_length) {
            char keybuf[256];
            json_size_t klen = json_unescape(keybuf, sizeof(keybuf) - 1, json_text + t->id, t->id_length);
            keybuf[klen < sizeof(keybuf) - 1 ? klen : (sizeof(keybuf) - 1)] = '\0';
            if (!json_string_equal(keybuf, klen, key, strlen(key))) continue;
            if (t->type == JSON_PRIMITIVE) {
                char buf[64];
                size_t cpy = t->value_length < sizeof(buf)-1 ? t->value_length : sizeof(buf)-1;
                memcpy(buf, json_text + t->value, cpy);
                buf[cpy] = '\0';
                *out = atoi(buf);
                return 1;
            }
        }
    }
    *out = defval;
    return 0;
}

// 在父对象 parent_key 下查找字符串键
char* json_find_string_in(const char* json_text, const char* parent_key, const char* key) {
    if (!json_text || !parent_key || !key) return NULL;
    struct json_token_t tokens[256];
    memset(tokens, 0, sizeof(tokens));
    size_t len = strlen(json_text);
    json_size_t num = json_parse(json_text, (json_size_t)len, tokens, (json_size_t)(sizeof(tokens)/sizeof(tokens[0])));
    if (num == 0) return NULL;

    // 定位父对象
    for (json_size_t i = 1; i < num; ++i) {
        struct json_token_t* p = &tokens[i];
        if (p->id && p->id_length && p->type == JSON_OBJECT) {
            char keybuf[256];
            json_size_t klen = json_unescape(keybuf, sizeof(keybuf) - 1, json_text + p->id, p->id_length);
            keybuf[klen < sizeof(keybuf) - 1 ? klen : (sizeof(keybuf) - 1)] = '\0';
            if (!json_string_equal(keybuf, klen, parent_key, strlen(parent_key))) continue;

            // 遍历子键
            json_size_t child = p->child;
            while (child && child < num) {
                struct json_token_t* t = &tokens[child];
                if (t->id && t->id_length) {
                    char ckey[256];
                    json_size_t clen = json_unescape(ckey, sizeof(ckey) - 1, json_text + t->id, t->id_length);
                    ckey[clen < sizeof(ckey) - 1 ? clen : (sizeof(ckey) - 1)] = '\0';
                    if (json_string_equal(ckey, clen, key, strlen(key))) {
                        if (t->type == JSON_STRING || t->type == JSON_PRIMITIVE) {
                            char* out = (char*)malloc(t->value_length + 1);
                            if (!out) return NULL;
                            json_size_t n = json_unescape(out, t->value_length, json_text + t->value, t->value_length);
                            out[n] = '\0';
                            return out;
                        }
                    }
                }
                child = t->sibling;
            }
        }
    }
    return NULL;
}

// 在父对象 parent_key 下查找整数键
int json_find_int_in(const char* json_text, const char* parent_key, const char* key, int defval, int* out) {
    if (out) *out = defval;
    if (!json_text || !parent_key || !key || !out) return 0;
    struct json_token_t tokens[256];
    memset(tokens, 0, sizeof(tokens));
    size_t len = strlen(json_text);
    json_size_t num = json_parse(json_text, (json_size_t)len, tokens, (json_size_t)(sizeof(tokens)/sizeof(tokens[0])));
    if (num == 0) return 0;
    for (json_size_t i = 1; i < num; ++i) {
        struct json_token_t* p = &tokens[i];
        if (p->id && p->id_length && p->type == JSON_OBJECT) {
            char keybuf[256];
            json_size_t klen = json_unescape(keybuf, sizeof(keybuf) - 1, json_text + p->id, p->id_length);
            keybuf[klen < sizeof(keybuf) - 1 ? klen : (sizeof(keybuf) - 1)] = '\0';
            if (!json_string_equal(keybuf, klen, parent_key, strlen(parent_key))) continue;
            json_size_t child = p->child;
            while (child && child < num) {
                struct json_token_t* t = &tokens[child];
                if (t->id && t->id_length) {
                    char ckey[256];
                    json_size_t clen = json_unescape(ckey, sizeof(ckey) - 1, json_text + t->id, t->id_length);
                    ckey[clen < sizeof(ckey) - 1 ? clen : (sizeof(ckey) - 1)] = '\0';
                    if (json_string_equal(ckey, clen, key, strlen(key)) && t->type == JSON_PRIMITIVE) {
                        char buf[64];
                        size_t cpy = t->value_length < sizeof(buf)-1 ? t->value_length : sizeof(buf)-1;
                        memcpy(buf, json_text + t->value, cpy);
                        buf[cpy] = '\0';
                        *out = atoi(buf);
                        return 1;
                    }
                }
                child = t->sibling;
            }
        }
    }
    return 0;
}

#ifdef NO_SPRINTF

static char* format( char* dest, int len, int isnegative ) {
    if ( isnegative )
        dest[ len++ ] = '-';
    dest[ len ] = '\0';
    int head = 0;
    int tail = len - 1;
    while( head < tail ) {
        char tmp = dest[ head ];
        dest[ head ] = dest[ tail ];
        dest[ tail ] = tmp;
        ++head;
        --tail;
    }
    return dest + len;
}

#define numtoa( func, type, utype )         \
static char* func( char* dest, type val ) { \
    enum { base = 10 };                     \
    if ( 0 == val )                         \
        return chtoa( dest, '0' );          \
    int const isnegative = 0 > val;         \
    utype num = isnegative ? -val : val;    \
    int len = 0;                            \
    while( 0 != num ) {                     \
        int rem = num % base;               \
        dest[ len++ ] = rem + '0';          \
        num = num / base;                   \
    }                                       \
    return format( dest, len, isnegative ); \
}                                           \

#define json_num( func, func2, type )                       \
char* func( char* dest, char const* name, type value ) {    \
    dest = primitivename( dest, name );                     \
    dest = func2( dest, value );                            \
    dest = chtoa( dest, ',' );                              \
    return dest;                                            \
}                                                           \

#define ALL_TYPES \
    X( int,      int,          unsigned int        ) \
    X( long,     long,         unsigned long       ) \
    X( uint,     unsigned int, unsigned int        ) \
    X( ulong,    unsigned      long, unsigned long ) \
    X( verylong, long long,    unsigned long long  ) \

#define X( name, type, utype ) numtoa( name##toa, type, utype )
ALL_TYPES
#undef X

#define X( name, type, utype ) json_num( json_##name, name##toa, type )
ALL_TYPES
#undef X

char* json_double( char* dest, char const* name, double value ) {
    return json_verylong( dest, name, value );
}

#else

#include <stdio.h>

#define ALL_TYPES \
    X( json_int,      int,           "%d"   ) \
    X( json_long,     long,          "%ld"  ) \
    X( json_uint,     unsigned int,  "%u"   ) \
    X( json_ulong,    unsigned long, "%lu"  ) \
    X( json_verylong, long long,     "%lld" ) \
    X( json_double,   double,        "%g"   ) \


#define json_num( funcname, type, fmt )                         \
char* funcname( char* dest, char const* name, type value, size_t* remLen  ) {       \
    int digitLen;                                                                   \
    dest = primitivename( dest, name, remLen );                                     \
    digitLen = snprintf( dest, *remLen, fmt, value );                               \
    if(digitLen >= (int)*remLen+1){                                                 \
    	digitLen = (int)*remLen;}                                                     \
    *remLen -= (size_t)digitLen;                                                    \
    dest += digitLen;                                                               \
    dest = chtoa( dest, ',', remLen );                                              \
    return dest;                                                                    \
}

#define X( name, type, fmt ) json_num( name, type, fmt )
ALL_TYPES
#undef X


#endif