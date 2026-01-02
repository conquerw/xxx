/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "log.h"

int main(int argc, char *argv[])
{
	LOG_INIT("/data/zlog.conf", "GetIo");
	
	for(int i = 0; i < 3; i++)
	{
		LOG_DEBUG("hello, zlog - debug, %d\n", i);
		LOG_INFO("hello, zlog - info\n");
		LOG_NOTICE("hello, zlog - notice, %d\n", i);
		LOG_WARN("hello, zlog - warn\n");
		LOG_ERROR("hello, zlog - error\n");
		LOG_FATAL("hello, zlog - fatal, %d\n", i);
	}
	
	char buffer[] = {0x01, 0x02, 0x03, 0x04, 0x05};
	LOG_DEBUG_HEX("hex_print", buffer, sizeof(buffer));
	
	LOG_DEBUG("hello, zlog - debug\n");
	LOG_INFO("hello, zlog - info\n");
	LOG_NOTICE("hello, zlog - notice\n");
	LOG_WARN("hello, zlog - warn\n");
	LOG_ERROR("hello, zlog - error\n");
	LOG_FATAL("hello, zlog - fatal\n");
	
	LOG_EXIT();
	
	return 1;
}
