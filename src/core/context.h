#ifndef POMELO_QUICKJS_CONTEXT_SRC_H
#define POMELO_QUICKJS_CONTEXT_SRC_H
#include "quickjs.h"
#include "pomelo/api.h"
#include "core.h"
#include "utils/pool.h"
#include "utils/array.h"
#include "utils/list.h"
#ifdef __cplusplus
extern "C" {
#endif


/// @brief Opaque type for context
typedef enum pomelo_qjs_context_opaque_type {
    POMELO_QJS_CONTEXT_OPAQUE_TYPE_LOGGER,
    POMELO_QJS_CONTEXT_OPAQUE_TYPE_COUNT,
} pomelo_qjs_context_opaque_type;


struct pomelo_qjs_context_s {
    /// @brief Extra data for context
    void * extra;

    /// @brief The allocator of module
    pomelo_allocator_t * allocator;

    /// @brief The context of pomelo
    pomelo_context_t * context;

    /// @brief The platform of pomelo
    pomelo_platform_t * platform;

    /// @brief The runtime of quickjs
    JSRuntime * rt;

    /// @brief The context of quickjs
    JSContext * ctx;

    /// @brief The class of socket
    JSClassID class_socket_id;

    /// @brief The class of session
    JSClassID class_session_id;

    /// @brief The class of channel
    JSClassID class_channel_id;

    /// @brief The class of message
    JSClassID class_message_id;

    /// @brief Pool of sockets
    pomelo_pool_t * pool_socket;

    /// @brief Pool of messages
    pomelo_pool_t * pool_message;

    /// @brief Pool of sessions
    pomelo_pool_t * pool_session;

    /// @brief Pool of channels
    pomelo_pool_t * pool_channel;

    /// @brief Pool of send info
    pomelo_pool_t * pool_send_info;

    /// @brief List of running sockets
    pomelo_list_t * running_sockets;

    /// @brief Std functions (NULL if it is not initialized)
    pomelo_qjs_std_t * std;

    /// @brief Error handler
    JSValue error_handler;

    /// @brief Temporary sessions for sending
    pomelo_array_t * tmp_send_sessions;

    /* Temporary buffer */

    /// @brief Temporary buffer
    uint8_t * tmp_buffer;

    /// @brief Temporary buffer size
    size_t tmp_buffer_size;

    /// @brief Whether the temporary buffer is acquired
    bool tmp_buffer_acquired;

    /// @brief Opaque data
    void * opaques[POMELO_QJS_CONTEXT_OPAQUE_TYPE_COUNT];
};


/// @brief Acquire new qjs socket
pomelo_qjs_socket_t * pomelo_qjs_context_acquire_socket(
    pomelo_qjs_context_t * context
);


/// @brief Release a qjs socket
void pomelo_qjs_context_release_socket(
    pomelo_qjs_context_t * context,
    pomelo_qjs_socket_t * qjs_socket
);


/// @brief Acquire a qjs session
pomelo_qjs_session_t * pomelo_qjs_context_acquire_session(
    pomelo_qjs_context_t * context
);


/// @brief Release a qjs session
void pomelo_qjs_context_release_session(
    pomelo_qjs_context_t * context,
    pomelo_qjs_session_t * qjs_session
);


/// @brief Acquire a qjs message
pomelo_qjs_message_t * pomelo_qjs_context_acquire_message(
    pomelo_qjs_context_t * context
);


/// @brief Release a qjs message
void pomelo_qjs_context_release_message(
    pomelo_qjs_context_t * context,
    pomelo_qjs_message_t * qjs_message
);


/// @brief Acquire a qjs channel
pomelo_qjs_channel_t * pomelo_qjs_context_acquire_channel(
    pomelo_qjs_context_t * context
);


/// @brief Destroy qjs channel
void pomelo_qjs_context_release_channel(
    pomelo_qjs_context_t * context,
    pomelo_qjs_channel_t * qjs_channel
);


/// @brief Acquire a qjs send info
pomelo_qjs_send_info_t * pomelo_qjs_context_acquire_send_info(
    pomelo_qjs_context_t * context
);


/// @brief Release a qjs send info
void pomelo_qjs_context_release_send_info(
    pomelo_qjs_context_t * context,
    pomelo_qjs_send_info_t * qjs_send_info
);


/**
 * The only temporary buffer is shared across the environment.
 * So that, make sure it is not used concurrently.
 */

/// @brief Prepare temporary buffer
uint8_t * pomelo_qjs_context_prepare_temp_buffer(
    pomelo_qjs_context_t * context,
    size_t size
);


/// @brief Release temporary buffer
void pomelo_qjs_context_release_temp_buffer(
    pomelo_qjs_context_t * context,
    uint8_t * buffer
);


#ifdef __cplusplus
}
#endif
#endif // POMELO_QUICKJS_CONTEXT_SRC_H
