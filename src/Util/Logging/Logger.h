//
// Created by Kasimir on 15.10.2022.
//

#ifndef CMC_LOGGER_H
#define CMC_LOGGER_H

typedef enum SCLogLevel {
	DEBUG,
	INFO,
	WARN,
	ERR
} SCLogLevel;

static inline char *stringFromLevel(SCLogLevel level) {
	static char *strings[] = {"DEBUG", "INFO", "WARN", "ERR"};
	return strings[level];
}

void cmc_log(SCLogLevel level, const char *message, ...);

#endif //CMC_LOGGER_H
