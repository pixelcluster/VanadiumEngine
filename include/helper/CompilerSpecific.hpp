#pragma once

#if defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE __builtin_unreachable();
#elif defined(_MSC_VER)
#define UNREACHABLE __assume(0);
#endif