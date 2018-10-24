#pragma once

#define debug_log(CTX, FORMAT, ...) fc_ctx_dlog(fc::logger::get("debug"), CTX, FORMAT, __VA_ARGS__)
