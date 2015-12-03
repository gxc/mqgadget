extern char *program_invocation_short_name;

bool is_vt100(void);
char *strupr(char *string, int len);
void err_exit(const char *format, ...);
int get_int_arg(const char *arg);
