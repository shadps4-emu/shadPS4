#ifdef _MSC_VER
#define BREAKPOINT __debugbreak
#elif defined(__GNUC__)
#define BREAKPOINT __builtin_trap
#else
#error What the fuck is this compiler
#endif