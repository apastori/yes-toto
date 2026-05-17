#ifndef YES_TOTO_CLI_H
#define YES_TOTO_CLI_H

void scan_meta_flags(int argc, char **argv, int *help, int *version);
void print_help(void);
void print_version(void);
int install_sigpipe_ignore(void);

#endif /* YES_TOTO_CLI_H */
