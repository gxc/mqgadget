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

/* convert at the most `len' characters in `string' to uppercase */
char *strupr(char *restrict string, int len)
{
	char *retval;

	retval = string;
	for (; *string && len > 0; string++, len--)
		*string = toupper(*string);

	return retval;
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

/*
 * parse command-line argument which is a positive integer string
 * return the value of type int
 */
int parse_positive_int(const char *arg)
{
	long res;
	char *tail;

	errno = 0;
	res = strtol(arg, &tail, 0);
	/* errno must be ERANGE if nonzero */
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

/* return ture if TERM=`term' and false otherwise */
int is_term(const char *term)
{
	return strcmp(getenv("TERM"), term) == 0;
}
