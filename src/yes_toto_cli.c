/*
 * Chain-of-thought (Step 1 — file scope):
 *
 * 1. Single responsibility: discover --help/--version across argv, print
 *    help/version text, install SIGPIPE ignore via sigaction.
 * 2. sigaction: failure returns -1 with errno; emit helper reports it.
 * 3. No heap.
 * 4. Not hot-path.
 * 5. C11 + POSIX.1-2008 (sigaction).
 */

/*
 * Signal handling (Step 5 — setup):
 *
 * Consumer close must not raise SIGPIPE with default action; we ignore SIGPIPE
 * before the write loop so writes return EPIPE instead.
 */

#include "yes_toto_cli.h"

#include "yes_toto.h"
#include "yes_toto_emit.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#define YES_TOTO_ARG_HELP    "--help"
#define YES_TOTO_ARG_VERSION "--version"

/*
 * argv_has_exact — return whether any argv[1..argc-1] equals `needle`.
 *
 * Preconditions: argc >= 1; argv points to argc C strings.
 * Postconditions: returns 1 if found, 0 otherwise.
 */
static int argv_has_exact(int argc, char **argv, const char *needle)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], needle) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * scan_meta_flags — Step 7: scan all argv[1..] before interpreting strings.
 * --help wins over --version when both appear.
 *
 * Preconditions: argc >= 1.
 * Postconditions: sets *help or *version when those tokens appear anywhere.
 */
void scan_meta_flags(int argc, char **argv, int *help, int *version)
{
    *help = argv_has_exact(argc, argv, YES_TOTO_ARG_HELP);
    *version = 0;
    if (*help) {
        return;
    }
    *version = argv_has_exact(argc, argv, YES_TOTO_ARG_VERSION);
}

/*
 * print_help — write usage text to stdout.
 *
 * Preconditions: none.
 * Postconditions: formatted help emitted; returns void.
 */
void print_help(void)
{
    printf(
        "Usage: yes-toto [STRING]...\n"
        "       yes-toto OPTION\n"
        "\n"
        "Repeatedly print a line to standard output until stopped.\n"
        "\n"
        "With no STRING, print 'y'.\n"
        "\n"
        "Options:\n"
        "  --help     display this help and exit\n"
        "  --version  display version and exit\n"
    );
}

/*
 * print_version — write `yes-toto VERSION` line to stdout.
 *
 * Preconditions: none.
 * Postconditions: one line on stdout; returns void.
 */
void print_version(void)
{
    printf("yes-toto %s\n", YES_TOTO_VERSION_STRING);
}

/*
 * install_sigpipe_ignore — configure SIGPIPE as ignored for EPIPE-on-write.
 *
 * Preconditions: none.
 * Postconditions: returns 0 on success, -1 on failure (errno set).
 */
int install_sigpipe_ignore(void)
{
#if defined(_WIN32)
    /*
     * MinGW/Windows: no SIGPIPE. Broken pipes are reported via write()
     * (see try_write_all in yes_toto_write.c); nothing to configure here.
     */
    return 0;
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) != 0) {
        yes_toto_emit_error("sigaction(SIGPIPE)");
        return -1;
    }
    return 0;
#endif
}
