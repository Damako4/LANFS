#pragma once

enum LogLevel { None, Error, Info, Debug };

#ifdef DEBUG
    #define LOG_LEVEL LogLevel::Debug
#else
    #define DEBUG_LOG(msg)
#endif

#ifndef LOG_LEVEL
    #define LOG_LEVEL 0   // None, by default
#endif

#define LOG_ERROR(x) if (LOG_LEVEL >= 1) std::cerr << "[ERROR] " << x << std::endl
#define LOG_INFO(x)  if (LOG_LEVEL >= 2) std::cout << "[INFO] "  << x << std::endl
#define LOG_DEBUG(x) if (LOG_LEVEL >= 3) std::cout << "[DEBUG] " << x << std::endl