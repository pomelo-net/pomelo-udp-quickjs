#include <assert.h>
#include <inttypes.h>
#include "logger/logger.h"
#include "core/core.h"
#include "console.h"


// Buffer size for console
#define CONSOLE_MESSAGE_BUFFER_SIZE 4096
#define CONSOLE_MESSAGE_MAX_LENGTH (CONSOLE_MESSAGE_BUFFER_SIZE - 1)
#define CONSOLE_MESSAGE_MAX_DEPTH 4
#define CONSOLE_MAX_SEEN_OBJECTS 64
#define CONSOLE_MAX_DISPLAYED_ARRAY_ELEMENTS 16
#define CONSOLE_MAX_DISPLAYED_OBJECT_PROPERTIES 32


/// @brief String buffer
typedef struct {
    char buffer[CONSOLE_MESSAGE_BUFFER_SIZE]; /// > Buffer
    size_t position;                          /// > Position
} string_buffer_t;


/// @brief Initialize string buffer
/// @param buffer The buffer
static void string_buffer_init(string_buffer_t * buffer) {
    assert(buffer != NULL);
    buffer->position = 0;
    buffer->buffer[0] = '\0';
}


/// @brief Append string to buffer
/// @param buffer The buffer
/// @param str The string to append
/// @return Whether the string is appended
static bool string_buffer_append(string_buffer_t * buffer, const char * str) {
    assert(buffer != NULL);
    assert(str != NULL);

    // Reserve space for null terminator
    if (buffer->position == CONSOLE_MESSAGE_MAX_LENGTH) {
        return false; // Nothing to do
    }
    
    // Copy as much as will fit, ensuring space for null terminator
    size_t to_copy = strnlen(
        str,
        CONSOLE_MESSAGE_MAX_LENGTH - buffer->position
    );
    if (to_copy > 0) {
        memcpy(buffer->buffer + buffer->position, str, to_copy);
        buffer->position += to_copy;
        buffer->buffer[buffer->position] = '\0';
    }

    return to_copy > 0;
}


#define SEEN_VALUE_SET_CAPACITY 128  // Choose a prime or power of 2 > expected items * 2

typedef struct {
    void* keys[SEEN_VALUE_SET_CAPACITY];
    uint8_t occupied[SEEN_VALUE_SET_CAPACITY];  // 0 = empty, 1 = occupied, 2 = tombstone
    size_t size;
    size_t max_elements;
} seen_value_set_t;


static inline uint64_t ptr_hash(void * ptr) {
    uintptr_t x = (uintptr_t) ptr;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}


static void seen_value_set_init(seen_value_set_t * set) {
    assert(set != NULL);
    memset(set, 0, sizeof(seen_value_set_t));
}


static bool seen_value_set_add(seen_value_set_t * set, void * key) {
    assert(set != NULL);
    assert(key != NULL);

    // Check the capacity
    if (set->size >= SEEN_VALUE_SET_CAPACITY * 3 / 4) return false;

    // Check the seen objects
    if (set->max_elements > 0 && set->size >= set->max_elements) return false;

    uint64_t hash = ptr_hash(key);
    size_t index = hash % SEEN_VALUE_SET_CAPACITY;

    size_t first_tombstone = SEEN_VALUE_SET_CAPACITY;

    for (size_t i = 0; i < SEEN_VALUE_SET_CAPACITY; ++i) {
        size_t pos = (index + i) % SEEN_VALUE_SET_CAPACITY;

        if (set->occupied[pos] == 0) {
            if (first_tombstone != SEEN_VALUE_SET_CAPACITY) pos = first_tombstone;
            set->keys[pos] = key;
            set->occupied[pos] = 1;
            set->size++;
            return true;
        } else if (set->occupied[pos] == 2) {
            if (first_tombstone == SEEN_VALUE_SET_CAPACITY) first_tombstone = pos;
        } else if (set->keys[pos] == key) {
            return false;  // Already present
        }
    }

    return false;  // Table full
}


static bool seen_value_set_find(const seen_value_set_t * set, void * key) {
    assert(set != NULL);
    assert(key != NULL);

    uint64_t hash = ptr_hash(key);
    size_t index = hash % SEEN_VALUE_SET_CAPACITY;

    for (size_t i = 0; i < SEEN_VALUE_SET_CAPACITY; ++i) {
        size_t pos = (index + i) % SEEN_VALUE_SET_CAPACITY;

        if (set->occupied[pos] == 0) return false;
        if (set->occupied[pos] == 1 && set->keys[pos] == key) return true;
    }
    return false;
}


static bool seen_value_set_remove(seen_value_set_t * set, void * key) {
    assert(set != NULL);
    assert(key != NULL);

    uint64_t hash = ptr_hash(key);
    size_t index = hash % SEEN_VALUE_SET_CAPACITY;

    for (size_t i = 0; i < SEEN_VALUE_SET_CAPACITY; ++i) {
        size_t pos = (index + i) % SEEN_VALUE_SET_CAPACITY;

        if (set->occupied[pos] == 0) return false;
        if (set->occupied[pos] == 1 && set->keys[pos] == key) {
            set->occupied[pos] = 2;  // Tombstone
            set->keys[pos] = NULL;
            set->size--;
            return true;
        }
    }
    return false;
}


/// @brief Convert JSValue to string
static bool js_value_to_string(
    string_buffer_t * buffer,
    JSContext * ctx,
    JSValue value,
    size_t depth,
    seen_value_set_t * seen
);


/// @brief Convert basic value to string
static bool js_basic_to_string(
    string_buffer_t * buffer,
    JSContext * ctx,
    JSValue value
) {
    const char * str = JS_ToCString(ctx, value);
    bool result = string_buffer_append(buffer, str);
    JS_FreeCString(ctx, str);
    if (!result) return result;

    if (JS_IsBigInt(value)) {
        // Add `n` at the end of BigInt
        result = string_buffer_append(buffer, "n");
    }

    return result;
}


/// @brief Convert function to string
static bool js_func_to_string(
    string_buffer_t * buffer,
    JSContext * ctx,
    JSValue value
) {
    // Function
    JSValue fn_str = JS_ToString(ctx, value);
    const char * str = JS_ToCString(ctx, fn_str);
    JS_FreeValue(ctx, fn_str);
    bool result = string_buffer_append(buffer, str);
    JS_FreeCString(ctx, str);
    return result;
}


/// @brief Convert array to string
static bool js_array_to_string2(
    string_buffer_t * buffer,
    JSContext * ctx,
    JSValue value,
    size_t depth,
    seen_value_set_t * seen
) {
    // Append open tag
    if (!string_buffer_append(buffer, "[")) {
        return false; // Buffer overflow
    }

    int64_t length = 0;
    JS_GetLength(ctx, value, &length);

    int64_t displayed_length = length;
    if (length > CONSOLE_MAX_DISPLAYED_ARRAY_ELEMENTS) {
        displayed_length = CONSOLE_MAX_DISPLAYED_ARRAY_ELEMENTS;
    }

    // Iterate the array elements
    for (int64_t i = 0; i < displayed_length; i++) {
        if (i > 0) {
            // Append separator
            if (!string_buffer_append(buffer, ", ")) {
                return false; // Buffer overflow
            }
        }

        JSValue element = JS_GetPropertyInt64(ctx, value, i);
        bool is_string = JS_IsString(element);
        if (is_string) {
            if (!string_buffer_append(buffer, "\"")) {
                JS_FreeValue(ctx, element);
                return false; // Buffer overflow
            }
        }

        bool ret = js_value_to_string(buffer, ctx, element, depth + 1, seen);
        JS_FreeValue(ctx, element);
        if (!ret) return false; // Buffer overflow

        if (is_string) {
            if (!string_buffer_append(buffer, "\"")) {
                return false; // Buffer overflow
            }
        }
    }

    // Ending
    char end[30];
    if (displayed_length < length) {
        int64_t more = length - displayed_length;
        snprintf(
            end,
            sizeof(end),
            ", ... %" PRId64 " more %s ]",
            more,
            (more > 1) ? "elements" : "element"
        );
    } else {
        snprintf(end, sizeof(end), "]");
    }

    if (!string_buffer_append(buffer, end)) {
        return false; // Buffer overflow
    }

    return true;
}


static bool js_array_to_string(
    string_buffer_t * buffer,
    JSContext * ctx,
    JSValue value,
    size_t depth,
    seen_value_set_t * seen
) {
    assert(JS_IsArray(value));
    void * ptr = JS_VALUE_GET_PTR(value);

    // Check if depth is exceeded
    if (depth >= CONSOLE_MESSAGE_MAX_DEPTH) {
        return string_buffer_append(buffer, "[Array]");
    }

    // Check if already seen
    if (seen_value_set_find(seen, ptr)) {
        return string_buffer_append(buffer, "[Circular]");
    }
    
    // Add to seen
    if (!seen_value_set_add(seen, ptr)) {
        // Exceeded max seen objects
        return string_buffer_append(buffer, "[Array]");
    }

    // Same depth
    bool ret = js_array_to_string2(buffer, ctx, value, depth, seen);

    // Remove from seen
    seen_value_set_remove(seen, ptr);
    return ret;
}


static bool js_object_to_string2(
    string_buffer_t * buffer,
    JSContext * ctx,
    JSValue value,
    size_t depth,
    seen_value_set_t * seen,
    JSPropertyEnum * props,
    uint32_t prop_count
) {

    uint32_t displayed_prop_count = prop_count;
    if (prop_count > CONSOLE_MAX_DISPLAYED_OBJECT_PROPERTIES) {
        displayed_prop_count = CONSOLE_MAX_DISPLAYED_OBJECT_PROPERTIES;
    }

    if (displayed_prop_count == 0) {
        return string_buffer_append(buffer, "{}");
    }

    // Append open tag
    if (!string_buffer_append(buffer, "{ ")) {
        return false; // Buffer overflow
    }

    for (uint32_t i = 0; i < displayed_prop_count; i++) {
        if (i > 0) {
            // Append separator
            if (!string_buffer_append(buffer, ", ")) {
                return false; // Buffer overflow
            }
        }

        JSAtom atom = props[i].atom;
        const char * name = JS_AtomToCString(ctx, atom);
        if (!name) continue; // Ignore

        if (!string_buffer_append(buffer, name)) {
            JS_FreeCString(ctx, name);
            return false; // Buffer overflow
        }

        if (!string_buffer_append(buffer, ": ")) {
            JS_FreeCString(ctx, name);
            return false; // Buffer overflow
        }

        JSValue element = JS_GetPropertyStr(ctx, value, name);
        if (JS_IsException(element)) {
            JS_FreeCString(ctx, name);
            JS_FreeValue(ctx, element);
            continue; // Ignore
        }

        JS_FreeCString(ctx, name); // We dont need name anymore
        bool is_string = JS_IsString(element);
        if (is_string) {
            if (!string_buffer_append(buffer, "\"")) {
                JS_FreeValue(ctx, element);
                return false; // Buffer overflow
            }
        }

        bool ret = js_value_to_string(buffer, ctx, element, depth + 1, seen);
        JS_FreeValue(ctx, element);
        if (!ret) return false; // Buffer overflow

        if (is_string) {
            if (!string_buffer_append(buffer, "\"")) {
                return false; // Buffer overflow
            }
        }
    }

    char end[30];
    if (displayed_prop_count < prop_count) {
        uint32_t more = prop_count - displayed_prop_count;
        snprintf(
            end,
            sizeof(end),
            ", ... %" PRIu32 " more %s }",
            more,
            (more > 1) ? "properties" : "property"
        );
    } else {
        snprintf(end, sizeof(end), " }");
    }
    
    // Append close tag
    if (!string_buffer_append(buffer, end)) {
        return false; // Buffer overflow
    }

    return true;
}


static bool js_object_to_string(
    string_buffer_t * buffer,
    JSContext * ctx,
    JSValue value,
    size_t depth,
    seen_value_set_t * seen
) {
    assert(JS_IsObject(value));
    void * ptr = JS_VALUE_GET_PTR(value);

    // Check if depth is exceeded
    if (depth >= CONSOLE_MESSAGE_MAX_DEPTH) {
        return string_buffer_append(buffer, "{Object}");
    }

    // Check if already seen
    if (seen_value_set_find(seen, ptr)) {
        return string_buffer_append(buffer, "{Circular}");
    }
    
    // Add to seen
    if (!seen_value_set_add(seen, ptr)) {
        // Exceeded max seen objects
        return string_buffer_append(buffer, "{Object}");
    }

    // Iterate the object properties
    JSPropertyEnum * props = NULL;
    uint32_t prop_count = 0;

    // Get all string-named properties
    JS_GetOwnPropertyNames(
        ctx, &props, &prop_count, value, 
        JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY
    );

    // Same depth
    bool ret = js_object_to_string2(
        buffer, ctx, value, depth, seen, props, prop_count
    );

    // Free properties table
    JS_FreePropertyEnum(ctx, props, prop_count);
    
    // Remove from seen
    seen_value_set_remove(seen, ptr);
    return ret;
}


/// @brief Convert JSValue to string
static bool js_value_to_string(
    string_buffer_t * buffer,
    JSContext * ctx,
    JSValue value,
    size_t depth,
    seen_value_set_t * seen
) {
    assert(buffer != NULL);
    assert(ctx != NULL);

    if (JS_IsFunction(ctx, value)) {
        return js_func_to_string(buffer, ctx, value);
    }
    
    if (JS_IsArray(value)) {
        // Array
        return js_array_to_string(buffer, ctx, value, depth, seen);
    }
    
    if (JS_IsObject(value)) {
        // Object
        return js_object_to_string(buffer, ctx, value, depth, seen);
    }
    
    // Basic types
    return js_basic_to_string(buffer, ctx, value);
}


/// @brief Common console function
static JSValue js_console_common(
    JSContext * ctx,
    int argc,
    JSValue * argv,
    pomelo_qjs_logger_level level
) {
    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    (void) level;
    (void) context;

    int nargc = argc;
    if (nargc > POMELO_QJS_CONSOLE_MAX_LOG_ARGS) {
        nargc = POMELO_QJS_CONSOLE_MAX_LOG_ARGS;
    }

    string_buffer_t buffer;
    seen_value_set_t seen;

    string_buffer_init(&buffer);
    seen_value_set_init(&seen);
    seen.max_elements = CONSOLE_MAX_SEEN_OBJECTS;

    for (int i = 0; i < nargc; i++) {
        if (!js_value_to_string(&buffer, ctx, argv[i], 0, &seen)) {
            break; // Buffer overflow, skip
        }
    }

    pomelo_qjs_logger_log(context, level, buffer.buffer);
    return JS_UNDEFINED;
}


/// console.log
static JSValue js_console_log(
    JSContext * ctx, JSValue this_val, int argc, JSValue * argv
) {
    (void) this_val;
    return js_console_common(ctx, argc, argv, POMELO_QJS_LOGGER_LEVEL_LOG);
}


/// console.info
static JSValue js_console_info(
    JSContext * ctx, JSValue this_val, int argc, JSValue * argv
) {
    (void) this_val;
    return js_console_common(ctx, argc, argv, POMELO_QJS_LOGGER_LEVEL_INFO);
}


/// console.warn
static JSValue js_console_warn(
    JSContext * ctx, JSValue this_val, int argc, JSValue * argv
) {
    (void) this_val;
    return js_console_common(ctx, argc, argv, POMELO_QJS_LOGGER_LEVEL_WARN);
}


/// console.error
static JSValue js_console_error(
    JSContext * ctx, JSValue this_val, int argc, JSValue * argv
) {
    (void) this_val;
    return js_console_common(ctx, argc, argv, POMELO_QJS_LOGGER_LEVEL_LOG);
}


/// @brief Initialize the console
static int init_console(JSContext * ctx) {
    assert(ctx != NULL);
    JSValue fn;

    // Create console object
    JSValue console = JS_NewObject(ctx);
    if (JS_IsException(console)) return -1;
    
    // console.log
    fn = JS_NewCFunction2(ctx, js_console_log, "log", 1, JS_CFUNC_generic, 0);
    if (JS_IsException(fn)) {
        JS_FreeValue(ctx, console);
        return -1;
    }
    if (JS_SetPropertyStr(ctx, console, "log", fn) < 0) {
        JS_FreeValue(ctx, fn);
        JS_FreeValue(ctx, console);
        return -1;
    }

    // console.info
    fn = JS_NewCFunction2(ctx, js_console_info, "info", 1, JS_CFUNC_generic, 0);
    if (JS_IsException(fn)) {
        JS_FreeValue(ctx, console);
        return -1;
    }
    if (JS_SetPropertyStr(ctx, console, "info", fn) < 0) {
        JS_FreeValue(ctx, fn);
        JS_FreeValue(ctx, console);
        return -1;
    }

    // console.warn
    fn = JS_NewCFunction2(ctx, js_console_warn, "warn", 1, JS_CFUNC_generic, 0);
    if (JS_IsException(fn)) {
        JS_FreeValue(ctx, console);
        return -1;
    }
    if (JS_SetPropertyStr(ctx, console, "warn", fn) < 0) {
        JS_FreeValue(ctx, fn);
        JS_FreeValue(ctx, console);
        return -1;
    }

    // console.error
    fn = JS_NewCFunction2(
        ctx, js_console_error, "error", 1, JS_CFUNC_generic, 0
    );
    if (JS_IsException(fn)) {
        JS_FreeValue(ctx, console);
        return -1;
    }
    if (JS_SetPropertyStr(ctx, console, "error", fn) < 0) {
        JS_FreeValue(ctx, fn);
        JS_FreeValue(ctx, console);
        return -1;
    }
    
    // Add console to global object
    JSValue global = JS_GetGlobalObject(ctx);
    if (JS_IsException(global)) {
        JS_FreeValue(ctx, console);
        return -1;
    }
    
    if (JS_SetPropertyStr(ctx, global, "console", console) < 0) {
        JS_FreeValue(ctx, global);
        JS_FreeValue(ctx, console);
        return -1;
    }
    
    // Cleanup
    JS_FreeValue(ctx, global);
    return 0;
}


int pomelo_qjs_console_init(pomelo_qjs_context_t * context) {
    assert(context != NULL);
    init_console(pomelo_qjs_context_get_js_context(context));
    return 0;
}


void pomelo_qjs_console_cleanup(pomelo_qjs_context_t * context) {
    (void) context;
}
