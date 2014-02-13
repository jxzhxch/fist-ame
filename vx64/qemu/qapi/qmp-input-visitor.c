/*
 * Input Visitor
 *
 * Copyright IBM, Corp. 2011
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 *
 */

#include "qapi/qmp-input-visitor.h"
#include "qapi/visitor-impl.h"
#include "qemu/queue.h"
#include "qemu-common.h"
#include "qapi/qmp/types.h"

/*
 * patched by fshooter
 * QmpInputVisitor is patched and incomplete!!!
 * not support struct/list visit
 * detail see origin qemu QmpInputVisitor implemention
 */

struct QmpInputVisitor
{
    Visitor visitor;
    QObject *obj;
};

static QmpInputVisitor *to_qiv(Visitor *v)
{
    return container_of(v, QmpInputVisitor, visitor);
}


static void qmp_input_start_struct(Visitor *v, void **obj, const char *kind,
                                   const char *name, size_t size, Error **errp)
{
	assert(0);
}

static void qmp_input_end_struct(Visitor *v, Error **errp)
{
	assert(0);
}

static void qmp_input_start_list(Visitor *v, const char *name, Error **errp)
{
	assert(0);
}

static GenericList *qmp_input_next_list(Visitor *v, GenericList **list,
                                        Error **errp)
{
	assert(0);
}

static void qmp_input_end_list(Visitor *v, Error **errp)
{
	assert(0);
}

static void qmp_input_type_int(Visitor *v, int64_t *obj, const char *name,
                               Error **errp)
{
    QmpInputVisitor *qiv = to_qiv(v);
    QObject *qobj = qiv->obj;

    if (!qobj || qobject_type(qobj) != QTYPE_QINT) {
        error_set(errp, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                  "integer");
        return;
    }

    *obj = qint_get_int(qobject_to_qint(qobj));
}

static void qmp_input_type_bool(Visitor *v, bool *obj, const char *name,
                                Error **errp)
{
    QmpInputVisitor *qiv = to_qiv(v);
    QObject *qobj = qiv->obj;

    if (!qobj || qobject_type(qobj) != QTYPE_QBOOL) {
        error_set(errp, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                  "boolean");
        return;
    }

    *obj = qbool_get_int(qobject_to_qbool(qobj));
}

static void qmp_input_type_str(Visitor *v, char **obj, const char *name,
                               Error **errp)
{
    QmpInputVisitor *qiv = to_qiv(v);
    QObject *qobj = qiv->obj;

    if (!qobj || qobject_type(qobj) != QTYPE_QSTRING) {
        error_set(errp, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                  "string");
        return;
    }

    *obj = strdup(qstring_get_str(qobject_to_qstring(qobj)));
}

static void qmp_input_type_number(Visitor *v, double *obj, const char *name,
                                  Error **errp)
{
    QmpInputVisitor *qiv = to_qiv(v);
    QObject *qobj = qiv->obj;

    if (!qobj || (qobject_type(qobj) != QTYPE_QFLOAT &&
        qobject_type(qobj) != QTYPE_QINT)) {
        error_set(errp, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                  "number");
        return;
    }

    if (qobject_type(qobj) == QTYPE_QINT) {
        *obj = qint_get_int(qobject_to_qint(qobj));
    } else {
        *obj = qfloat_get_double(qobject_to_qfloat(qobj));
    }
}

static void qmp_input_start_optional(Visitor *v, bool *present,
                                     const char *name, Error **errp)
{
    QmpInputVisitor *qiv = to_qiv(v);
    QObject *qobj = qiv->obj;

    if (!qobj) {
        *present = false;
        return;
    }

    *present = true;
}

Visitor *qmp_input_get_visitor(QmpInputVisitor *v)
{
    return &v->visitor;
}

void qmp_input_visitor_cleanup(QmpInputVisitor *v)
{
    qobject_decref(v->obj);
    free(v);
}

QmpInputVisitor *qmp_input_visitor_new(QObject *obj)
{
    QmpInputVisitor *v;

    v = malloc(sizeof(*v));
    memset(v, 0, sizeof(*v));

    v->visitor.start_struct = qmp_input_start_struct;
    v->visitor.end_struct = qmp_input_end_struct;
    v->visitor.start_list = qmp_input_start_list;
    v->visitor.next_list = qmp_input_next_list;
    v->visitor.end_list = qmp_input_end_list;
    v->visitor.type_enum = input_type_enum;
    v->visitor.type_int = qmp_input_type_int;
    v->visitor.type_bool = qmp_input_type_bool;
    v->visitor.type_str = qmp_input_type_str;
    v->visitor.type_number = qmp_input_type_number;
    v->visitor.start_optional = qmp_input_start_optional;

    v->obj = obj;
    qobject_incref(obj);

    return v;
}

