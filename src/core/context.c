#include <assert.h>
#include <string.h>
#include "context.h"
#include "socket.h"
#include "session.h"
#include "channel.h"
#include "message.h"


pomelo_qjs_context_t * pomelo_qjs_context_create(
    pomelo_qjs_context_options_t * options
) {
    assert(options != NULL);
    if (!options->ctx || !options->rt || !options->platform) {
        return NULL; // Invalid args
    }

    pomelo_allocator_t * allocator = options->allocator;
    if (!allocator) {
        allocator = pomelo_allocator_default();
    }

    pomelo_qjs_context_t * context =
        pomelo_allocator_malloc_t(allocator, pomelo_qjs_context_t);
    if (!context) return NULL; // Failed to allocate new context
    memset(context, 0, sizeof(pomelo_qjs_context_t));

    context->error_handler = JS_NULL;

    JSContext * ctx = options->ctx;
    JSRuntime * rt = options->rt;

    // Setup the context
    context->allocator = allocator;
    context->rt = rt;
    context->ctx = ctx;
    context->platform = options->platform;

    // Create native context
    pomelo_context_root_options_t context_options = {
        .allocator = allocator,
        .message_capacity = options->message_capacity
    };
    context->context = pomelo_context_root_create(&context_options);
    if (!context->context) {
        pomelo_allocator_free(allocator, context);
        return NULL;
    }

    // Create pool of sockets
    pomelo_pool_root_options_t pool_options;
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_qjs_socket_t);
    pool_options.available_max = options->pool_socket_max;
    pool_options.alloc_data = context;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_qjs_socket_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_qjs_socket_cleanup;
    pool_options.zero_init = true;
    context->pool_socket = pomelo_pool_root_create(&pool_options);
    if (!context->pool_socket) {
        pomelo_qjs_context_destroy(context);
        return NULL;
    }

    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_qjs_message_t);
    pool_options.available_max = options->pool_message_max;
    pool_options.alloc_data = context;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_qjs_message_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_qjs_message_cleanup;
    pool_options.zero_init = true;
    context->pool_message = pomelo_pool_root_create(&pool_options);
    if (!context->pool_message) {
        pomelo_qjs_context_destroy(context);
        return NULL;
    }

    // Create pool of sessions
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_qjs_session_t);
    pool_options.available_max = options->pool_session_max;
    pool_options.alloc_data = context;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_qjs_session_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_qjs_session_cleanup;
    pool_options.zero_init = true;
    context->pool_session = pomelo_pool_root_create(&pool_options);
    if (!context->pool_session) {
        pomelo_qjs_context_destroy(context);
        return NULL;
    }

    // Create pool of channels
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_qjs_channel_t);
    pool_options.available_max = options->pool_channel_max;
    pool_options.alloc_data = context;
    pool_options.on_init = (pomelo_pool_init_cb)
        pomelo_qjs_channel_init;
    pool_options.on_cleanup = (pomelo_pool_cleanup_cb)
        pomelo_qjs_channel_cleanup;
    pool_options.zero_init = true;
    context->pool_channel = pomelo_pool_root_create(&pool_options);
    if (!context->pool_channel) {
        pomelo_qjs_context_destroy(context);
        return NULL;
    }

    // Create pool of send info
    memset(&pool_options, 0, sizeof(pomelo_pool_root_options_t));
    pool_options.allocator = allocator;
    pool_options.element_size = sizeof(pomelo_qjs_send_info_t);
    pool_options.zero_init = true;
    context->pool_send_info = pomelo_pool_root_create(&pool_options);
    if (!context->pool_send_info) {
        pomelo_qjs_context_destroy(context);
        return NULL;
    }

    // Create list of running sockets
    pomelo_list_options_t list_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_qjs_socket_t *)
    };
    context->running_sockets = pomelo_list_create(&list_options);
    if (!context->running_sockets) {
        pomelo_qjs_context_destroy(context);
        return NULL;
    }

    // Create temporary sending sessions array
    pomelo_array_options_t array_options = {
        .allocator = allocator,
        .element_size = sizeof(pomelo_session_t *)
    };
    context->tmp_send_sessions = pomelo_array_create(&array_options);
    if (!context->tmp_send_sessions) {
        pomelo_qjs_context_destroy(context);
        return NULL;
    }

    // Set opaque data
    JS_SetContextOpaque(ctx, context);
    JS_SetRuntimeOpaque(rt, context);

    return context;
}


pomelo_qjs_socket_t * pomelo_qjs_context_acquire_socket(
    pomelo_qjs_context_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->pool_socket, context);
}


void pomelo_qjs_context_release_socket(
    pomelo_qjs_context_t * context,
    pomelo_qjs_socket_t * qjs_socket
) {
    assert(context != NULL);
    pomelo_pool_release(context->pool_socket, qjs_socket);
}


pomelo_qjs_session_t * pomelo_qjs_context_acquire_session(
    pomelo_qjs_context_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->pool_session, context);
}


void pomelo_qjs_context_release_session(
    pomelo_qjs_context_t * context,
    pomelo_qjs_session_t * qjs_session
) {
    assert(context != NULL);
    if (qjs_session) {
        // Reset any session-specific state here if needed
        pomelo_pool_release(context->pool_session, qjs_session);
    }
}


pomelo_qjs_message_t * pomelo_qjs_context_acquire_message(
    pomelo_qjs_context_t * context
) {
    assert(context != NULL);
    pomelo_qjs_message_t *msg = pomelo_pool_acquire(context->pool_message, context);
    if (msg) {
        // Initialize message-specific state here if needed
        msg->message = NULL;
    }
    return msg;
}


void pomelo_qjs_context_release_message(
    pomelo_qjs_context_t * context,
    pomelo_qjs_message_t * qjs_message
) {
    assert(context != NULL);
    if (qjs_message) {
        // Clean up any message resources here
        if (qjs_message->message) {
            pomelo_message_unref(qjs_message->message);
            qjs_message->message = NULL;
        }
        pomelo_pool_release(context->pool_message, qjs_message);
    }
}


pomelo_qjs_channel_t * pomelo_qjs_context_acquire_channel(
    pomelo_qjs_context_t * context
) {
    assert(context != NULL);
    pomelo_qjs_channel_t *channel = pomelo_pool_acquire(context->pool_channel, context);
    if (channel) {
        // Initialize channel-specific state here if needed
        channel->channel = NULL;
        channel->thiz = JS_UNDEFINED;
    }
    return channel;
}


void pomelo_qjs_context_release_channel(
    pomelo_qjs_context_t * context,
    pomelo_qjs_channel_t * qjs_channel
) {
    assert(context != NULL);
    if (qjs_channel) {
        // Clean up any channel resources here
        if (!JS_IsUndefined(qjs_channel->thiz)) {
            JS_FreeValue(context->ctx, qjs_channel->thiz);
            qjs_channel->thiz = JS_UNDEFINED;
        }
        pomelo_pool_release(context->pool_channel, qjs_channel);
    }
}


pomelo_qjs_send_info_t * pomelo_qjs_context_acquire_send_info(
    pomelo_qjs_context_t * context
) {
    assert(context != NULL);
    return pomelo_pool_acquire(context->pool_send_info, NULL);
}


void pomelo_qjs_context_release_send_info(
    pomelo_qjs_context_t * context,
    pomelo_qjs_send_info_t * qjs_send_info
) {
    assert(context != NULL);
    pomelo_pool_release(context->pool_send_info, qjs_send_info);
}


uint8_t * pomelo_qjs_context_prepare_temp_buffer(
    pomelo_qjs_context_t * context,
    size_t size
) {
    assert(context != NULL);
    assert(!context->tmp_buffer_acquired);

    if (context->tmp_buffer_acquired) {
        return NULL; // Buffer is already acquired
    }

    context->tmp_buffer_acquired = true;
    if (context->tmp_buffer_size < size) {
        // Increase the buffer size
        uint8_t * new_buffer = pomelo_allocator_realloc(
            context->allocator,
            context->tmp_buffer,
            size
        );
        if (!new_buffer) {
            return NULL; // Cannot allocate new buffer
        }
        context->tmp_buffer = new_buffer;
        context->tmp_buffer_size = size;
    }

    return context->tmp_buffer;
}


void pomelo_qjs_context_release_temp_buffer(
    pomelo_qjs_context_t * context,
    uint8_t * buffer
) {
    assert(context != NULL);
    (void) buffer;
    assert(context->tmp_buffer_acquired);
    context->tmp_buffer_acquired = false;
}


void pomelo_qjs_context_set_extra(
    pomelo_qjs_context_t * context,
    void * extra
) {
    assert(context != NULL);
    context->extra = extra;    
}


void * pomelo_qjs_context_get_extra(pomelo_qjs_context_t * context) {
    return context->extra;
}


/*----------------------------------------------------------------------------*/
/*                               Private APIs                                 */
/*----------------------------------------------------------------------------*/

void pomelo_qjs_context_destroy(pomelo_qjs_context_t * context) {
    assert(context != NULL);

    // Cleanup all running sockets
    if (context->running_sockets) {
        pomelo_list_iterator_t it;
        pomelo_list_iterator_init(&it, context->running_sockets);

        pomelo_qjs_socket_t * socket = NULL;
        while (pomelo_list_pop_front(context->running_sockets, &socket) == 0) {
            socket->thiz_entry = NULL;
            pomelo_qjs_socket_stop_impl(socket);
        }
    }

    if (context->pool_socket) {
        pomelo_pool_destroy(context->pool_socket);
        context->pool_socket = NULL;
    }

    if (context->pool_message) {
        pomelo_pool_destroy(context->pool_message);
        context->pool_message = NULL;
    }

    if (context->pool_session) {
        pomelo_pool_destroy(context->pool_session);
        context->pool_session = NULL;
    }

    if (context->pool_channel) {
        pomelo_pool_destroy(context->pool_channel);
        context->pool_channel = NULL;
    }

    if (context->pool_send_info) {
        pomelo_pool_destroy(context->pool_send_info);
        context->pool_send_info = NULL;
    }
    
    if (context->running_sockets) {
        pomelo_list_destroy(context->running_sockets);
        context->running_sockets = NULL;
    }

    if (context->tmp_send_sessions) {
        pomelo_array_destroy(context->tmp_send_sessions);
        context->tmp_send_sessions = NULL;
    }

    if (context->tmp_buffer) {
        pomelo_allocator_free(context->allocator, context->tmp_buffer);
        context->tmp_buffer = NULL;
    }

    context->ctx = NULL;
    context->rt = NULL;

    if (!JS_IsNull(context->error_handler)) {
        JS_FreeValue(context->ctx, context->error_handler);
        context->error_handler = JS_NULL;
    }

    context->tmp_buffer_size = 0;

    // The platform does not belong to the qjs context
    context->platform = NULL;

    // Destroy context
    if (context->context) {
        pomelo_context_destroy(context->context);
        context->context = NULL;
    }

    // Free itself
    pomelo_allocator_free(context->allocator, context); 
}


JSContext * pomelo_qjs_context_get_js_context(pomelo_qjs_context_t * context) {
    assert(context != NULL);
    return context->ctx;
}


JSRuntime * pomelo_qjs_context_get_js_runtime(pomelo_qjs_context_t * context) {
    assert(context != NULL);
    return context->rt;
}
