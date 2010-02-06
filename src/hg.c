#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "hg.h"

static int
hg_probe(vccontext_t* context)
{
    return isdir(".hg");
}

static int sum_bytes(const unsigned char* data, int size)
{
    int i, sum = 0;
    for (i = 0; i < size; ++i) {
        sum += data[i];
    }
    return sum;
}

static void
update_nodeid(vccontext_t* context, result_t* result)
{
    char buf[40];
    size_t readsize;

    if (!context->options->show_revision) return;

    readsize = read_file(".hg/dirstate", buf, 40);
    if (readsize == 40) {
        debug("read nodeids from .hg/dirstate");
        result->revision = malloc(32);  /* XXX mem leak */

        // first parent
        dump_hex(buf, result->revision, 6);
        if (!sum_bytes((unsigned char *) buf + 20, 20)) return;

        // second parent
        result->revision[12] = ',';
        dump_hex(buf + 20, result->revision + 13, 6);
    }
    else {
        debug("failed to read from .hg/dirstate");
    }
}

static void
update_mq_info(vccontext_t* context, result_t* result)
{
    char buf[1024], *patch;

    // we treat the name of the mq patch as the revision
    if (!context->options->show_revision) return;

    if (read_last_line(".hg/patches/status", buf, 1024)) {
        debug("read last line from .hg/patches/status: '%s'", buf);
        patch = strchr(buf, ':');
        if (!patch) return;
        patch += 1;
        debug("patch name found: '%s'", patch);
        result->revision = strdup(patch);   /* XXX mem leak */
    }
    else {
        debug("failed to read from .hg/patches/status: assuming no mq patch applied");
    }
}

static result_t*
hg_get_info(vccontext_t* context)
{
    result_t* result = init_result();
    char buf[1024];

    // prefers bookmark because it tends to be more informative
    if (read_first_line(".hg/bookmarks.current", buf, 1024) && buf[0]) {
        debug("read first line from .hg/bookmarks.current: '%s'", buf);
        result->branch = strdup(buf);  /* XXX mem leak */
    }
    else if (read_first_line(".hg/branch", buf, 1024)) {
        debug("read first line from .hg/branch: '%s'", buf);
        result->branch = strdup(buf);   /* XXX mem leak */
    }
    else {
        debug("failed to read from .hg/branch: assuming default branch");
        result->branch = "default";
    }

    update_mq_info(context, result);
    if (!result->revision) update_nodeid(context, result);

    return result;
}

vccontext_t* get_hg_context(options_t* options)
{
    return init_context("hg", options, hg_probe, hg_get_info);
}
