// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _LOG_H_
#define _LOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "zlog.h"

#define LOG_INIT(confpath, cname) dzlog_init(confpath, cname)

#define LOG_EXIT() zlog_fini()

#define LOG_DEBUG(...) dzlog_debug(__VA_ARGS__)

#define LOG_INFO(...) dzlog_info(__VA_ARGS__)

#define LOG_NOTICE(...) dzlog_notice(__VA_ARGS__)

#define LOG_WARN(...) dzlog_warn(__VA_ARGS__)

#define LOG_ERROR(...) dzlog_error(__VA_ARGS__)

#define LOG_FATAL(...) dzlog_fatal(__VA_ARGS__)

#define LOG_DEBUG_HEX(name, buffer, buffer_len)										\
	do																				\
	{																				\
		char _buffer[2048] = "";													\
		memset(_buffer, 0x00, 2048);												\
		for(int i = 0, count = 0; i < buffer_len && count <= 1021; i++, count += 3)	\
			snprintf(&_buffer[count], 2048 - count, "%02X ", *(buffer + i));		\
		dzlog_debug("%s%ld, %s\n", name, buffer_len, _buffer);						\
	} while(0)

#ifdef __cplusplus
}
#endif

#endif
