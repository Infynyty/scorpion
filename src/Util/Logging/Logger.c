//
// Created by Kasimir on 15.10.2022.
//

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "Logger.h"

const enum SCLogLevel ACTIVE_LOG_LEVEL = INFO;

void cmc_log(SCLogLevel level, const char *message, ...) {
	if (level < ACTIVE_LOG_LEVEL) {
		return;
	}
	va_list args;

	va_start(args, message);

	time_t mytime = time(NULL);
	char *time_str = ctime(&mytime);
	time_str[strlen(time_str) - 1] = '\0';
	printf("%s [%s]: ", time_str, stringFromLevel(level));

	vprintf(message, args);
	va_end(args);
	printf("\n");
}
