#include <assert.h>
#include "token.h"
#include "context.h"
#include "pomelo/random.h"
#include "pomelo/constants.h"
#include "pomelo/token.h"


int pomelo_qjs_init_token_module(JSContext * ctx, JSModuleDef * m) {
    assert(ctx != NULL);
    assert(m != NULL);

    JSValue token = JS_NewObject(ctx);

    // Create static functions `encode`
    JSValue fn_encode = JS_NewCFunction(
        ctx,
        pomelo_qjs_token_encode,
        "encode",
        /* argc = */ 0
    );
    JS_SetPropertyStr(ctx, token, "encode", fn_encode);

    // Create static functions `randomBuffer`
    JSValue fn_random_buffer = JS_NewCFunction(
        ctx,
        pomelo_qjs_token_random_buffer,
        "randomBuffer",
        /* argc = */ 0
    );
    JS_SetPropertyStr(ctx, token, "randomBuffer", fn_random_buffer);

    // Add constants
    JS_SetPropertyStr(
        ctx,
        token, 
        "KEY_BYTES",
        JS_NewInt32(ctx, POMELO_KEY_BYTES)
    );
    
    JS_SetPropertyStr(
        ctx,
        token,
        "CONNECT_TOKEN_NONCE_BYTES",
        JS_NewInt32(ctx, POMELO_CONNECT_TOKEN_NONCE_BYTES)
    );
    
    JS_SetPropertyStr(
        ctx,
        token,
        "USER_DATA_BYTES",
        JS_NewInt32(ctx, POMELO_USER_DATA_BYTES)
    );

    JS_SetModuleExport(ctx, m, "Token", token);
    return 0;
}


JSValue pomelo_qjs_token_encode(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) thiz;
    if (argc < 11) {
        return JS_ThrowInternalError(ctx, "Missing required arguments");
    }

    pomelo_connect_token_t token = {0};
    int arg_index = 0;
    int ret;

    // 1. Parse private key: Uint8Array(POMELO_KEY_BYTES)
    size_t private_key_len;
    uint8_t * private_key = JS_GetUint8Array(
        ctx, &private_key_len, argv[arg_index]
    );
    if (!private_key || private_key_len != POMELO_KEY_BYTES) {
        return JS_ThrowRangeError(ctx, "Invalid private key length");
    }
    arg_index++;

    // 2. Parse protocol ID: number | bigint
    if (JS_IsBigInt(argv[arg_index])) {
        uint64_t temp;
        if (JS_ToBigUint64(ctx, &temp, argv[arg_index]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid protocol ID");
        }
        token.protocol_id = (uint64_t) temp;
    } else {
        uint32_t temp;
        if (JS_ToUint32(ctx, &temp, argv[arg_index]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid protocol ID");
        }
        token.protocol_id = temp;
    }
    arg_index++;

    // 3. Parse create timestamp: number | bigint
    if (JS_IsBigInt(argv[arg_index])) {
        int64_t temp;
        if (JS_ToBigInt64(ctx, &temp, argv[arg_index]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid create timestamp");
        }
        token.create_timestamp = (int64_t) temp;
    } else {
        double temp;
        if (JS_ToFloat64(ctx, &temp, argv[arg_index]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid create timestamp");
        }
        token.create_timestamp = (int64_t) temp;
    }
    arg_index++;

    // 4. Parse expire timestamp: number | bigint
    if (JS_IsBigInt(argv[arg_index])) {
        int64_t temp;
        if (JS_ToBigInt64(ctx, &temp, argv[arg_index]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid expire timestamp");
        }
        token.expire_timestamp = (int64_t) temp;
    } else {
        double temp;
        if (JS_ToFloat64(ctx, &temp, argv[arg_index]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid expire timestamp");
        }
        token.expire_timestamp = (int64_t) temp;
    }
    arg_index++;

    // 5. Parse connect token nonce: Uint8Array(POMELO_CONNECT_TOKEN_NONCE_BYTES)
    size_t nonce_len;
    uint8_t * nonce = JS_GetUint8Array(ctx, &nonce_len, argv[arg_index]);
    if (!nonce || nonce_len != POMELO_CONNECT_TOKEN_NONCE_BYTES) {
        return JS_ThrowRangeError(ctx, "Invalid nonce length");
    }
    memcpy(token.connect_token_nonce, nonce, POMELO_CONNECT_TOKEN_NONCE_BYTES);
    arg_index++;

    // 6. Parse timeout: number
    uint32_t timeout;
    if (JS_ToUint32(ctx, &timeout, argv[arg_index]) != 0) {
        return JS_ThrowTypeError(ctx, "Invalid timeout");
    }
    token.timeout = timeout;
    arg_index++;

    // 7. Parse addresses: string[]
    if (!JS_IsArray(argv[arg_index])) {
        return JS_ThrowTypeError(ctx, "addresses must be an array");
    }
    
    JSValue length_val = JS_GetPropertyStr(ctx, argv[arg_index], "length");
    uint32_t address_count;
    if (JS_ToUint32(ctx, &address_count, length_val) != 0) {
        JS_FreeValue(ctx, length_val);
        return JS_ThrowTypeError(ctx, "Invalid addresses array");
    }
    JS_FreeValue(ctx, length_val);

    if (address_count > POMELO_CONNECT_TOKEN_MAX_ADDRESSES) {
        return JS_ThrowRangeError(ctx, "Too many addresses");
    }

    for (uint32_t i = 0; i < address_count; i++) {
        JSValue addr_val = JS_GetPropertyUint32(ctx, argv[arg_index], i);
        const char *addr_str = JS_ToCString(ctx, addr_val);
        if (!addr_str) {
            JS_FreeValue(ctx, addr_val);
            return JS_ThrowTypeError(ctx, "Invalid address string");
        }
        
        ret = pomelo_address_from_string(
            &token.addresses[token.naddresses], addr_str
        );
        if (ret != 0) {
            JS_FreeCString(ctx, addr_str);
            JS_FreeValue(ctx, addr_val);
            return JS_ThrowTypeError(ctx, "Invalid address format");
        }
        
        JS_FreeCString(ctx, addr_str);
        JS_FreeValue(ctx, addr_val);
        token.naddresses++;
    }
    arg_index++;

    // 8. Parse client to server key: Uint8Array(POMELO_KEY_BYTES)
    size_t c2s_key_len;
    uint8_t *c2s_key = JS_GetUint8Array(ctx, &c2s_key_len, argv[arg_index]);
    if (!c2s_key || c2s_key_len != POMELO_KEY_BYTES) {
        return JS_ThrowRangeError(ctx, "Invalid client to server key length");
    }
    memcpy(token.client_to_server_key, c2s_key, POMELO_KEY_BYTES);
    arg_index++;

    // 9. Parse server to client key: Uint8Array(POMELO_KEY_BYTES)
    size_t s2c_key_len;
    uint8_t *s2c_key = JS_GetUint8Array(ctx, &s2c_key_len, argv[arg_index]);
    if (!s2c_key || s2c_key_len != POMELO_KEY_BYTES) {
        return JS_ThrowRangeError(ctx, "Invalid server to client key length");
    }
    memcpy(token.server_to_client_key, s2c_key, POMELO_KEY_BYTES);
    arg_index++;

    // 10. Parse client ID: number | bigint
    if (JS_IsBigInt(argv[arg_index])) {
        int64_t temp;
        if (JS_ToBigInt64(ctx, &temp, argv[arg_index]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid client ID");
        }
        token.client_id = temp;
    } else {
        int64_t temp;
        if (JS_ToInt64(ctx, &temp, argv[arg_index]) != 0) {
            return JS_ThrowTypeError(ctx, "Invalid client ID");
        }
        token.client_id = temp;
    }
    arg_index++;

    // 11. Parse user data: Uint8Array(POMELO_USER_DATA_BYTES)
    size_t user_data_len;
    uint8_t *user_data = JS_GetUint8Array(ctx, &user_data_len, argv[arg_index]);
    if (!user_data || user_data_len > POMELO_USER_DATA_BYTES) {
        return JS_ThrowRangeError(ctx, "Invalid user data length");
    }
    memcpy(token.user_data, user_data, user_data_len);

    // All parameters parsed, now encode the token
    pomelo_qjs_context_t *context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    uint8_t * buffer = pomelo_qjs_context_prepare_temp_buffer(
        context,
        POMELO_CONNECT_TOKEN_BYTES
    );
    if (!buffer) {
        return JS_ThrowInternalError(ctx, "Failed to allocate temporary buffer");
    }

    ret = pomelo_connect_token_encode(buffer, &token, private_key);
    if (ret < 0) {
        pomelo_qjs_context_release_temp_buffer(context, buffer);
        return JS_ThrowInternalError(ctx, "Failed to encode token");
    }

    // Create a new Uint8Array with the encoded token
    JSValue result = JS_NewUint8ArrayCopy(
        ctx, buffer, POMELO_CONNECT_TOKEN_BYTES
    );
    pomelo_qjs_context_release_temp_buffer(context, buffer);

    return result;
}


JSValue pomelo_qjs_token_random_buffer(
    JSContext * ctx, JSValue thiz, int argc, JSValue * argv
) {
    assert(ctx != NULL);
    (void) thiz;
    if (argc < 1) {
        return JS_ThrowInternalError(ctx, "Missing required arguments");
    }

    size_t length = 0;
    if (JS_IsBigInt(argv[0])) {
        uint64_t temp = 0;
        if (JS_ToBigUint64(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        length = (size_t) temp;
    } else if (JS_IsNumber(argv[0])) {
        uint32_t temp = 0;
        if (JS_ToUint32(ctx, &temp, argv[0]) != 0) {
            return JS_ThrowSyntaxError(ctx, "Invalid argument");
        }
        length = (size_t) temp;
    } else {
        return JS_ThrowSyntaxError(ctx, "Invalid argument");
    }

    pomelo_qjs_context_t * context = JS_GetContextOpaque(ctx);
    assert(context != NULL);

    uint8_t * buffer = pomelo_qjs_context_prepare_temp_buffer(context, length);
    if (!buffer) {
        return JS_ThrowSyntaxError(ctx, "Failed to prepare temporary buffer");
    }

    // Random the buffer
    pomelo_random_buffer(buffer, length);

    // Create new Uint8Array with copying
    JSValue result = JS_NewUint8ArrayCopy(ctx, buffer, length);
    pomelo_qjs_context_release_temp_buffer(context, buffer);

    return result;
}
