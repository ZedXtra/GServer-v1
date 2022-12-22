//
// Header to detect if compiler generate Linux or Windows executable
//
#ifndef OS
#define OS
// x86 Linux GCC
#if defined(linux)
#define OS_LINUX
#endif

// x86 Windows Cygwin
#if defined(__CYGWIN32__)
#define OS_WIN32
#endif

// If all fail :(
#if !(defined(OS_LINUX) || defined(OS_WIN32))
#error "Executable can only be compiled either Cygwin or Linux-GCC"
#endif
#endif
