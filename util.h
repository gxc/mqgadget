extern char *program_invocation_short_name; /* defined in main.c */

int is_term(const char *term);
char *strupr(char *string, int len);
void err_exit(const char *format, ...);
int parse_positive_int(const char *arg);
