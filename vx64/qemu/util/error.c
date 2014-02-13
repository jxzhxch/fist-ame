/*
 * QEMU Error Objects
 *
 * Copyright IBM, Corp. 2011
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.  See
 * the COPYING.LIB file in the top-level directory.
 */

#include "qemu-common.h"
#include "qapi/error.h"
#include "qapi/qmp/qjson.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qapi-types.h"

struct Error
{
    char *msg;
    ErrorClass err_class;
};

void error_set(Error **errp, ErrorClass err_class, const char *fmt, ...)
{
    Error *err;
    char msg_buf[1024];
    va_list ap;

    if (errp == NULL) {
        return;
    }
    assert(*errp == NULL);

    err = malloc(sizeof(*err));
    memset(err, 0, sizeof(*err));

    va_start(ap, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, ap);
    msg_buf[sizeof(msg_buf) - 1] = 0;
    err->msg = strdup(msg_buf);
    va_end(ap);
    err->err_class = err_class;

    *errp = err;
}

void error_set_errno(Error **errp, int os_errno, ErrorClass err_class,
                     const char *fmt, ...)
{
    Error *err;
    char fmt_buf[1024];
    char msg_buf[1024];
    va_list ap;

    if (errp == NULL) {
        return;
    }
    assert(*errp == NULL);

    if (os_errno != 0) {
        snprintf(fmt_buf, sizeof(fmt_buf), "%s: %s", fmt, strerror(os_errno));
        fmt_buf[sizeof(fmt_buf) - 1] = 0;
    }
    va_start(ap, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt_buf, ap);
    msg_buf[sizeof(msg_buf) - 1] = 0;
    va_end(ap);
    err = malloc(sizeof(*err));
    memset(err, 0, sizeof(*err));
    err->msg = strdup(msg_buf);
    err->err_class = err_class;

    *errp = err;
}

Error *error_copy(const Error *err)
{
    Error *err_new;

    err_new = malloc(sizeof(*err));
    memset(err_new, 0, sizeof(*err));
    err_new->msg = strdup(err->msg);
    err_new->err_class = err->err_class;

    return err_new;
}

bool error_is_set(Error **errp)
{
    return (errp && *errp);
}

ErrorClass error_get_class(const Error *err)
{
    return err->err_class;
}

const char *error_get_pretty(Error *err)
{
    return err->msg;
}

void error_free(Error *err)
{
    if (err) {
        free(err->msg);
        free(err);
    }
}

void error_propagate(Error **dst_err, Error *local_err)
{
    if (dst_err && !*dst_err) {
        *dst_err = local_err;
    } else if (local_err) {
        error_free(local_err);
    }
}
