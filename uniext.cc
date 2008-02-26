#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace Partty {

namespace {
int static_argc = 0;
char** static_argv = NULL;
char** static_envp = NULL;
}

void initprocname(int argc, char**& argv, char**& envp)
{
	static_argc = argc;
	static_argv = argv;
	static_envp = envp;
}

void setprocname(const char *fmt, ...)
{
	int i;
	for(i = 0; static_envp[i] != NULL; i++)
		;

	char* argv_end;
	if(i > 0) {
		argv_end = static_envp[i-1] + strlen(static_envp[i-1]);
	} else {
		argv_end = static_argv[static_argc - 1] + strlen(static_argv[static_argc-1]);
	}

	va_list va;
	va_start(va, fmt);
	vsnprintf(static_argv[0], argv_end - static_argv[0], fmt, va);
	va_end(va);

	for(char* p = &static_argv[0][strlen(static_argv[0])]; p < argv_end; ++p) {
		*p = NULL;
	}
}

}  // namespace Partty

