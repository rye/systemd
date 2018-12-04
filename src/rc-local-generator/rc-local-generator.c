/* SPDX-License-Identifier: LGPL-2.1+ */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "generator.h"
#include "log.h"
#include "main-func.h"
#include "mkdir.h"
#include "string-util.h"
#include "util.h"

static const char *arg_dest = "/tmp";

/* So you are reading this, and might wonder: why is this implemented as a generator rather than as a plain, statically
 * enabled service that carries appropriate ConditionFileIsExecutable= lines? The answer is this: conditions bypass
 * execution of a service's binary, but they have no influence on unit dependencies. Thus, a service that is
 * conditioned out will still act as synchronization point in the dependency tree, and we'd rather not have that for
 * these two legacy scripts. */

static int add_symlink(const char *service, const char *where) {
        const char *from, *to;

        assert(service);
        assert(where);

        from = strjoina(SYSTEM_DATA_UNIT_PATH "/", service);
        to = strjoina(arg_dest, "/", where, ".wants/", service);

        (void) mkdir_parents_label(to, 0755);

        if (symlink(from, to) < 0) {
                if (errno == EEXIST)
                        return 0;

                return log_error_errno(errno, "Failed to create symlink %s: %m", to);
        }

        return 1;
}

static int run(int argc, char *argv[]) {
        int r = 0, k = 0;

        log_setup_generator();

        if (argc > 1 && argc != 4)
                return log_error_errno(SYNTHETIC_ERRNO(EINVAL), "This program takes three or no arguments.");

        if (argc > 1)
                arg_dest = argv[1];

        if (access(RC_LOCAL_SCRIPT_PATH_START, X_OK) < 0)
                log_full_errno(errno == ENOENT ? LOG_DEBUG : LOG_WARNING, errno,
                               RC_LOCAL_SCRIPT_PATH_START " is not executable: %m");
        else {
                log_debug("Automatically adding rc-local.service.");

                r = add_symlink("rc-local.service", "multi-user.target");
        }

        if (access(RC_LOCAL_SCRIPT_PATH_STOP, X_OK) < 0)
                log_full_errno(errno == ENOENT ? LOG_DEBUG : LOG_WARNING, errno,
                               RC_LOCAL_SCRIPT_PATH_STOP " is not executable: %m");
        else {
                log_debug("Automatically adding halt-local.service.");

                k = add_symlink("halt-local.service", "final.target");
        }

        return r < 0 ? r : k;
}

DEFINE_MAIN_FUNCTION(run);
