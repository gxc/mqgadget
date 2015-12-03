#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>

#include "util.h"

char *program_invocation_short_name;
extern char **environ;

/* convert at the most `len' characters in `string' to uppercase */
char *strupr(char *restrict string, int len)
{
	char *ret;

	ret = string;
	while (*string && len-- > 0) {
		*string = toupper(*string);
		string++;
	}

	return ret;
}

/* write error messages to stderr and exit */
void err_exit(const char *format, ...)
{
	va_list ap;

	if (program_invocation_short_name)
		fprintf(stderr, "%s: ", program_invocation_short_name);
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

/**
 * get_int_arg() - parse positive integer string
 * @arg: string about to be parsed
 *
 * Return: int value of `arg'
 */
int get_int_arg(const char *arg)
{
	long res;
	char *tail;

	errno = 0;
	res = strtol(arg, &tail, 0);
	/* base is 0, then errno must be ERANGE if nonzero */
	if (errno)
		err_exit("number overflow (%s)", arg);
	/* Command-line args can never be empty strings
	   or strings with trailing whitespaces,
	   so *tail == '\0' imply that `arg' is a valid number */
	if (*tail)
		err_exit("invalid number (%s)\n", arg);

	if (res > INT_MAX)
		err_exit("integer out of range (%s)\n", arg);

	if (res < 1)
		err_exit("not a positive integer (%s)\n", arg);

	return (int)res;
}

/* return ture if `TERM=vt100' is set, otherwise false */
bool is_vt100(void)
{
	char **ep;

	for (ep = environ; *ep; ep++)
		if (strncmp(*ep, "TERM", 4) == 0
		    && strncmp(*ep + 5, "vt100", 5) == 0)
			return true;

	return false;
}
