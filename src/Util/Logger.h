//
// Created by Kasimir on 15.10.2022.
//

#ifndef CMC_LOGGER_H
#define CMC_LOGGER_H

typedef enum CMC_LOG_LEVEL {
	DEBUG,
	INFO,
	WARN,
	ERR
} CMC_LOG_LEVEL;

static inline char *stringFromLevel(enum CMC_LOG_LEVEL level) {
	static char *strings[] = {"DEBUG", "INFO", "WARN", "ERR"};
	return strings[level];
}

void sc_log(enum CMC_LOG_LEVEL level, const char *message, ...);

#endif //CMC_LOGGER_H
