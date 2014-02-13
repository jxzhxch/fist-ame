/*
 * QEMU Object Model
 *
 * Copyright IBM, Corp. 2011
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qom/object.h"
#include "qemu-common.h"
#include "qapi/visitor.h"
#include "qapi/string-input-visitor.h"
#include "qapi/string-output-visitor.h"

/* TODO: replace QObject with a simpler visitor to avoid a dependency
 * of the QOM core on QObject?  */
#include "qom/qom-qobject.h"
#include "qapi/qmp/qobject.h"
#include "qapi/qmp/qbool.h"
#include "qapi/qmp/qint.h"
#include "qapi/qmp/qstring.h"

#define MAX_INTERFACES 32

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

typedef struct InterfaceImpl InterfaceImpl;
typedef struct TypeImpl TypeImpl;

struct InterfaceImpl
{
    const char *typename;
};

struct TypeImpl
{
    const char *name;

    size_t class_size;

    size_t instance_size;

    void (*class_init)(ObjectClass *klass, void *data);
    void (*class_base_init)(ObjectClass *klass, void *data);
    void (*class_finalize)(ObjectClass *klass, void *data);

    void *class_data;

    void (*instance_init)(Object *obj);
    void (*instance_finalize)(Object *obj);

    bool abstract;

    const char *parent;
    TypeImpl *parent_type;

    ObjectClass *class;

    int num_interfaces;
    InterfaceImpl interfaces[MAX_INTERFACES];
};

static Type type_interface;
static struct MyGListHead *type_table = 0;

static struct MyGListHead *type_table_get(void)
{
    return type_table;
}

static void type_table_add(TypeImpl *ti)
{
	struct MyGListHead* new_entry = malloc(sizeof(struct MyGListHead));
	new_entry->next = type_table_get();
	new_entry->data = ti;
	type_table = new_entry;
}

static TypeImpl *type_table_lookup(const char *name)
{
	struct MyGListHead* current = type_table_get();
	for(; current; current = current->next) {
		struct TypeImpl* ti = current->data;
		if(strcmp(ti->name, name) == 0)
			return ti;
	}
    return 0;
}

static TypeImpl *type_register_internal(const TypeInfo *info)
{
    TypeImpl *ti = malloc(sizeof(*ti));
    memset(ti, 0, sizeof(*ti));
    int i;

    assert(info->name != NULL);

    if (type_table_lookup(info->name) != NULL) {
        fprintf(stderr, "Registering `%s' which already exists\n", info->name);
        abort();
    }

    ti->name = strdup(info->name);
    ti->parent = strdup(info->parent);

    ti->class_size = info->class_size;
    ti->instance_size = info->instance_size;

    ti->class_init = info->class_init;
    ti->class_base_init = info->class_base_init;
    ti->class_finalize = info->class_finalize;
    ti->class_data = info->class_data;

    ti->instance_init = info->instance_init;
    ti->instance_finalize = info->instance_finalize;

    ti->abstract = info->abstract;

    for (i = 0; info->interfaces && info->interfaces[i].type; i++) {
        ti->interfaces[i].typename = strdup(info->interfaces[i].type);
    }
    ti->num_interfaces = i;

    type_table_add(ti);

    return ti;
}

TypeImpl *type_register(const TypeInfo *info)
{
    assert(info->parent);
    return type_register_internal(info);
}

TypeImpl *type_register_static(const TypeInfo *info)
{
    return type_register(info);
}

static TypeImpl *type_get_by_name(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    return type_table_lookup(name);
}

static TypeImpl *type_get_parent(TypeImpl *type)
{
    if (!type->parent_type && type->parent) {
        type->parent_type = type_get_by_name(type->parent);
        assert(type->parent_type != NULL);
    }

    return type->parent_type;
}

static bool type_has_parent(TypeImpl *type)
{
    return (type->parent != NULL);
}

static size_t type_class_get_size(TypeImpl *ti)
{
    if (ti->class_size) {
        return ti->class_size;
    }

    if (type_has_parent(ti)) {
        return type_class_get_size(type_get_parent(ti));
    }

    return sizeof(ObjectClass);
}

static size_t type_object_get_size(TypeImpl *ti)
{
    if (ti->instance_size) {
        return ti->instance_size;
    }

    if (type_has_parent(ti)) {
        return type_object_get_size(type_get_parent(ti));
    }

    return 0;
}

static bool type_is_ancestor(TypeImpl *type, TypeImpl *target_type)
{
    assert(target_type);

    /* Check if typename is a direct ancestor of type */
    while (type) {
        if (type == target_type) {
            return true;
        }

        type = type_get_parent(type);
    }

    return false;
}

static void type_initialize(TypeImpl *ti);

static void type_initialize_interface(TypeImpl *ti, const char *parent)
{
    InterfaceClass *new_iface;
    TypeInfo info = { };
    TypeImpl *iface_impl;

    info.parent = parent;
    char info_name_buf[MAX_PATH];
    sprintf(info_name_buf, "%s::%s", ti->name, info.parent);
    info.name = info_name_buf;
    info.abstract = true;

    iface_impl = type_register(&info);
    type_initialize(iface_impl);

    new_iface = (InterfaceClass *)iface_impl->class;
    new_iface->concrete_class = ti->class;

    struct MyGListHead* new_head = malloc(sizeof(struct MyGListHead));
    new_head->next = ti->class->interfaces;
    new_head->data = iface_impl->class;
    ti->class->interfaces = new_head;
}

static void type_initialize(TypeImpl *ti)
{
    TypeImpl *parent;

    if (ti->class) {
        return;
    }

    ti->class_size = type_class_get_size(ti);
    ti->instance_size = type_object_get_size(ti);

    ti->class = malloc(ti->class_size);
    memset(ti->class, 0, ti->class_size);

    parent = type_get_parent(ti);
    if (parent) {
        type_initialize(parent);
        struct MyGListHead *e;
        int i;

        assert(parent->class_size <= ti->class_size);
        memcpy(ti->class, parent->class, parent->class_size);

        for (e = parent->class->interfaces; e; e = e->next) {
            ObjectClass *iface = e->data;
            type_initialize_interface(ti, object_class_get_name(iface));
        }

        for (i = 0; i < ti->num_interfaces; i++) {
            TypeImpl *t = type_get_by_name(ti->interfaces[i].typename);
            for (e = ti->class->interfaces; e; e = e->next) {
                TypeImpl *target_type = OBJECT_CLASS(e->data)->type;

                if (type_is_ancestor(target_type, t)) {
                    break;
                }
            }

            if (e) {
                continue;
            }

            type_initialize_interface(ti, ti->interfaces[i].typename);
        }
    }

    ti->class->type = ti;

    while (parent) {
        if (parent->class_base_init) {
            parent->class_base_init(ti->class, ti->class_data);
        }
        parent = type_get_parent(parent);
    }

    if (ti->class_init) {
        ti->class_init(ti->class, ti->class_data);
    }


}

static void object_init_with_type(Object *obj, TypeImpl *ti)
{
    if (type_has_parent(ti)) {
        object_init_with_type(obj, type_get_parent(ti));
    }

    if (ti->instance_init) {
        ti->instance_init(obj);
    }
}

void object_initialize_with_type(void *data, TypeImpl *type)
{
    Object *obj = data;

    assert(type != NULL);
    type_initialize(type);

    assert(type->instance_size >= sizeof(Object));
    assert(type->abstract == false);

    memset(obj, 0, type->instance_size);
    obj->class = type->class;
    object_ref(obj);
    QTAILQ_INIT(&obj->properties);
    object_init_with_type(obj, type);
}

void object_initialize(void *data, const char *typename)
{
    TypeImpl *type = type_get_by_name(typename);

    object_initialize_with_type(data, type);
}

static inline bool object_property_is_child(ObjectProperty *prop)
{
    return strstart(prop->type, "child<", NULL);
}

static inline bool object_property_is_link(ObjectProperty *prop)
{
    return strstart(prop->type, "link<", NULL);
}

static void object_property_del_all(Object *obj)
{
    while (!QTAILQ_EMPTY(&obj->properties)) {
        ObjectProperty *prop = QTAILQ_FIRST(&obj->properties);

        QTAILQ_REMOVE(&obj->properties, prop, node);

        if (prop->release) {
            prop->release(obj, prop->name, prop->opaque);
        }

        free(prop->name);
        free(prop->type);
        free(prop);
    }
}

static void object_property_del_child(Object *obj, Object *child, Error **errp)
{
    ObjectProperty *prop;

    QTAILQ_FOREACH(prop, &obj->properties, node) {
        if (object_property_is_child(prop) && prop->opaque == child) {
            object_property_del(obj, prop->name, errp);
            break;
        }
    }
}

void object_unparent(Object *obj)
{
    object_ref(obj);
    if (obj->parent) {
        object_property_del_child(obj->parent, obj, NULL);
    }
    if (obj->class->unparent) {
        (obj->class->unparent)(obj);
    }
    object_unref(obj);
}

static void object_deinit(Object *obj, TypeImpl *type)
{
    if (type->instance_finalize) {
        type->instance_finalize(obj);
    }

    if (type_has_parent(type)) {
        object_deinit(obj, type_get_parent(type));
    }
}

static void object_finalize(void *data)
{
    Object *obj = data;
    TypeImpl *ti = obj->class->type;

    object_deinit(obj, ti);
    object_property_del_all(obj);

    assert(obj->ref == 0);
    if (obj->free) {
        obj->free(obj);
    }
}

Object *object_new_with_type(Type type)
{
    Object *obj;

    assert(type != NULL);
    type_initialize(type);

    obj = malloc(type->instance_size);
    object_initialize_with_type(obj, type);
    obj->free = free;

    return obj;
}

Object *object_new(const char *typename)
{
    TypeImpl *ti = type_get_by_name(typename);

    return object_new_with_type(ti);
}

Object *object_dynamic_cast(Object *obj, const char *typename)
{
    if (obj && object_class_dynamic_cast(object_get_class(obj), typename)) {
        return obj;
    }

    return NULL;
}

Object *object_dynamic_cast_assert(Object *obj, const char *typename)
{
    Object *inst;

    inst = object_dynamic_cast(obj, typename);

    if (!inst && obj) {
        fprintf(stderr, "Object %p is not an instance of type %s\n",
                obj, typename);
        abort();
    }

    return inst;
}

ObjectClass *object_class_dynamic_cast(ObjectClass *class,
                                       const char *typename)
{
    TypeImpl *target_type = type_get_by_name(typename);
    TypeImpl *type = class->type;
    ObjectClass *ret = NULL;

    if (type->num_interfaces && type_is_ancestor(target_type, type_interface)) {
        int found = 0;
        struct MyGListHead *i;

        for (i = class->interfaces; i; i = i->next) {
            ObjectClass *target_class = i->data;

            if (type_is_ancestor(target_class->type, target_type)) {
                ret = target_class;
                found++;
            }
         }

        /* The match was ambiguous, don't allow a cast */
        if (found > 1) {
            ret = NULL;
        }
    } else if (type_is_ancestor(type, target_type)) {
        ret = class;
    }

    return ret;
}

ObjectClass *object_class_dynamic_cast_assert(ObjectClass *class,
                                              const char *typename)
{
    ObjectClass *ret = object_class_dynamic_cast(class, typename);

    if (!ret) {
        fprintf(stderr, "Object %p is not an instance of type %s\n",
                class, typename);
        abort();
    }

    return ret;
}

const char *object_get_typename(Object *obj)
{
    return obj->class->type->name;
}

ObjectClass *object_get_class(Object *obj)
{
    return obj->class;
}

bool object_class_is_abstract(ObjectClass *klass)
{
    return klass->type->abstract;
}

const char *object_class_get_name(ObjectClass *klass)
{
    return klass->type->name;
}

ObjectClass *object_class_by_name(const char *typename)
{
    TypeImpl *type = type_get_by_name(typename);

    if (!type) {
        return NULL;
    }

    type_initialize(type);

    return type->class;
}

ObjectClass *object_class_get_parent(ObjectClass *class)
{
    TypeImpl *type = type_get_parent(class->type);

    if (!type) {
        return NULL;
    }

    type_initialize(type);

    return type->class;
}

typedef struct OCFData
{
    void (*fn)(ObjectClass *klass, void *opaque);
    const char *implements_type;
    bool include_abstract;
    void *opaque;
} OCFData;

int object_child_foreach(Object *obj, int (*fn)(Object *child, void *opaque),
                         void *opaque)
{
    ObjectProperty *prop;
    int ret = 0;

    QTAILQ_FOREACH(prop, &obj->properties, node) {
        if (object_property_is_child(prop)) {
            ret = fn(prop->opaque, opaque);
            if (ret != 0) {
                break;
            }
        }
    }
    return ret;
}

void object_ref(Object *obj)
{
    obj->ref++;
}

void object_unref(Object *obj)
{
    assert(obj->ref > 0);
    obj->ref--;

    /* parent always holds a reference to its children */
    if (obj->ref == 0) {
        object_finalize(obj);
    }
}

void object_property_add(Object *obj, const char *name, const char *type,
                         ObjectPropertyAccessor *get,
                         ObjectPropertyAccessor *set,
                         ObjectPropertyRelease *release,
                         void *opaque, Error **errp)
{
    ObjectProperty *prop = malloc(sizeof(*prop));

    prop->name = strdup(name);
    prop->type = strdup(type);

    prop->get = get;
    prop->set = set;
    prop->release = release;
    prop->opaque = opaque;

    QTAILQ_INSERT_TAIL(&obj->properties, prop, node);
}

ObjectProperty *object_property_find(Object *obj, const char *name,
                                     Error **errp)
{
    ObjectProperty *prop;

    QTAILQ_FOREACH(prop, &obj->properties, node) {
        if (strcmp(prop->name, name) == 0) {
            return prop;
        }
    }

    error_set(errp, QERR_PROPERTY_NOT_FOUND, "", name);
    return NULL;
}

void object_property_del(Object *obj, const char *name, Error **errp)
{
    ObjectProperty *prop = object_property_find(obj, name, errp);
    if (prop == NULL) {
        return;
    }

    if (prop->release) {
        prop->release(obj, name, prop->opaque);
    }

    QTAILQ_REMOVE(&obj->properties, prop, node);

    free(prop->name);
    free(prop->type);
    free(prop);
}

void object_property_get(Object *obj, Visitor *v, const char *name,
                         Error **errp)
{
    ObjectProperty *prop = object_property_find(obj, name, errp);
    if (prop == NULL) {
        return;
    }

    if (!prop->get) {
        error_set(errp, QERR_PERMISSION_DENIED);
    } else {
        prop->get(obj, v, prop->opaque, name, errp);
    }
}

void object_property_set(Object *obj, Visitor *v, const char *name,
                         Error **errp)
{
    ObjectProperty *prop = object_property_find(obj, name, errp);
    if (prop == NULL) {
        return;
    }

    if (!prop->set) {
        error_set(errp, QERR_PERMISSION_DENIED);
    } else {
        prop->set(obj, v, prop->opaque, name, errp);
    }
}

void object_property_set_str(Object *obj, const char *value,
                             const char *name, Error **errp)
{
    QString *qstr = qstring_from_str(value);
    object_property_set_qobject(obj, QOBJECT(qstr), name, errp);

    QDECREF(qstr);
}

char *object_property_get_str(Object *obj, const char *name,
                              Error **errp)
{
    QObject *ret = object_property_get_qobject(obj, name, errp);
    QString *qstring;
    char *retval;

    if (!ret) {
        return NULL;
    }
    qstring = qobject_to_qstring(ret);
    if (!qstring) {
        error_set(errp, QERR_INVALID_PARAMETER_TYPE, name, "string");
        retval = NULL;
    } else {
        retval = strdup(qstring_get_str(qstring));
    }

    QDECREF(qstring);
    return retval;
}

void object_property_set_bool(Object *obj, bool value,
                              const char *name, Error **errp)
{
    QBool *qbool = qbool_from_int(value);
    object_property_set_qobject(obj, QOBJECT(qbool), name, errp);

    QDECREF(qbool);
}

bool object_property_get_bool(Object *obj, const char *name,
                              Error **errp)
{
    QObject *ret = object_property_get_qobject(obj, name, errp);
    QBool *qbool;
    bool retval;

    if (!ret) {
        return false;
    }
    qbool = qobject_to_qbool(ret);
    if (!qbool) {
        error_set(errp, QERR_INVALID_PARAMETER_TYPE, name, "boolean");
        retval = false;
    } else {
        retval = qbool_get_int(qbool);
    }

    QDECREF(qbool);
    return retval;
}

void object_property_set_int(Object *obj, int64_t value,
                             const char *name, Error **errp)
{
    QInt *qint = qint_from_int(value);
    object_property_set_qobject(obj, QOBJECT(qint), name, errp);

    QDECREF(qint);
}

int64_t object_property_get_int(Object *obj, const char *name,
                                Error **errp)
{
    QObject *ret = object_property_get_qobject(obj, name, errp);
    QInt *qint;
    int64_t retval;

    if (!ret) {
        return -1;
    }
    qint = qobject_to_qint(ret);
    if (!qint) {
        error_set(errp, QERR_INVALID_PARAMETER_TYPE, name, "int");
        retval = -1;
    } else {
        retval = qint_get_int(qint);
    }

    QDECREF(qint);
    return retval;
}

void object_property_parse(Object *obj, const char *string,
                           const char *name, Error **errp)
{
    StringInputVisitor *mi;
    mi = string_input_visitor_new(string);
    object_property_set(obj, string_input_get_visitor(mi), name, errp);

    string_input_visitor_cleanup(mi);
}

char *object_property_print(Object *obj, const char *name,
                            Error **errp)
{
    StringOutputVisitor *mo;
    char *string;

    mo = string_output_visitor_new();
    object_property_get(obj, string_output_get_visitor(mo), name, errp);
    string = string_output_get_string(mo);
    string_output_visitor_cleanup(mo);
    return string;
}

const char *object_property_get_type(Object *obj, const char *name, Error **errp)
{
    ObjectProperty *prop = object_property_find(obj, name, errp);
    if (prop == NULL) {
        return NULL;
    }

    return prop->type;
}

Object *object_get_root(void)
{
    static Object *root;

    if (!root) {
        root = object_new("container");
    }

    return root;
}

char *object_get_canonical_path(Object *obj)
{
    Object *root = object_get_root();
    char newpath[MAX_PATH];
    char path[MAX_PATH];
    path[0] = 0;

    while (obj != root) {
        ObjectProperty *prop = NULL;

        assert(obj->parent != NULL);

        QTAILQ_FOREACH(prop, &obj->parent->properties, node) {
            if (!object_property_is_child(prop)) {
                continue;
            }

            if (prop->opaque == obj) {
                if (strlen(path) > 0) {
                	sprintf("%s/%s", prop->name, path);
                    strcpy(path, newpath);
                } else {
                    strcpy(path, prop->name);
                }
                break;
            }
        }

        assert(prop != NULL);

        obj = obj->parent;
    }

    sprintf("/%s", path);
    return strdup(newpath);
}


static void object_get_child_property(Object *obj, Visitor *v, void *opaque,
                                      const char *name, Error **errp)
{
    Object *child = opaque;
    char *path;

    path = object_get_canonical_path(child);
    visit_type_str(v, &path, name, errp);
    free(path);
}

static void object_finalize_child_property(Object *obj, const char *name,
                                           void *opaque)
{
    Object *child = opaque;

    object_unref(child);
}

void object_property_add_child(Object *obj, const char *name,
                               Object *child, Error **errp)
{
    char type[MAX_PATH];
    sprintf(type, "child<%s>", object_get_typename(OBJECT(child)));

    object_property_add(obj, name, type, object_get_child_property,
                        NULL, object_finalize_child_property, child, errp);

    object_ref(child);
    assert(child->parent == NULL);
    child->parent = obj;

}


Object *object_resolve_path_component(Object *parent, const char *part)
{
    ObjectProperty *prop = object_property_find(parent, part, NULL);
    if (prop == NULL) {
        return NULL;
    }

    if (object_property_is_link(prop)) {
        return *(Object **)prop->opaque;
    } else if (object_property_is_child(prop)) {
        return prop->opaque;
    } else {
        return NULL;
    }
}

static Object *object_resolve_abs_path(Object *parent,
                                          char **parts,
                                          const char *typename,
                                          int index)
{
    Object *child;

    if (parts[index] == NULL) {
        return object_dynamic_cast(parent, typename);
    }

    if (strcmp(parts[index], "") == 0) {
        return object_resolve_abs_path(parent, parts, typename, index + 1);
    }

    child = object_resolve_path_component(parent, parts[index]);
    if (!child) {
        return NULL;
    }

    return object_resolve_abs_path(child, parts, typename, index + 1);
}

static Object *object_resolve_partial_path(Object *parent,
                                              char **parts,
                                              const char *typename,
                                              bool *ambiguous)
{
    Object *obj;
    ObjectProperty *prop;

    obj = object_resolve_abs_path(parent, parts, typename, 0);

    QTAILQ_FOREACH(prop, &parent->properties, node) {
        Object *found;

        if (!object_property_is_child(prop)) {
            continue;
        }

        found = object_resolve_partial_path(prop->opaque, parts,
                                            typename, ambiguous);
        if (found) {
            if (obj) {
                if (ambiguous) {
                    *ambiguous = true;
                }
                return NULL;
            }
            obj = found;
        }

        if (ambiguous && *ambiguous) {
            return NULL;
        }
    }

    return obj;
}

Object *object_resolve_path_type(const char *path, const char *typename,
                                 bool *ambiguous)
{
    bool partial_path = true;
    Object *obj;
    char *parts[MAX_PATH / 2];
    int part_num = 0;

    if (!path || path[0] == 0) {
        return object_get_root();
    }

    const char* path_after_slash = path;
    do {
    	const char* slash = strchr(path_after_slash, '/');
    	char* path_part = malloc(slash - path_after_slash + 1);
    	path_part[slash - path_after_slash] = 0;
    	parts[part_num++] = 0;
    	path_after_slash = slash + 1;
    } while(strlen(path_after_slash) > 0);
    parts[part_num] = 0;

    if (strcmp(parts[0], "") == 0) {
        partial_path = false;
    }

    if (partial_path) {
        if (ambiguous) {
            *ambiguous = false;
        }
        obj = object_resolve_partial_path(object_get_root(), parts,
                                          typename, ambiguous);
    } else {
        obj = object_resolve_abs_path(object_get_root(), parts, typename, 1);
    }

    int i;
    for(i = 0; i < part_num; ++i) {
    	free(parts[i]);
    }

    return obj;
}

Object *object_resolve_path(const char *path, bool *ambiguous)
{
    return object_resolve_path_type(path, TYPE_OBJECT, ambiguous);
}

typedef struct StringProperty
{
    char *(*get)(Object *, Error **);
    void (*set)(Object *, const char *, Error **);
} StringProperty;

static void property_get_str(Object *obj, Visitor *v, void *opaque,
                             const char *name, Error **errp)
{
    StringProperty *prop = opaque;
    char *value;

    value = prop->get(obj, errp);
    if (value) {
        visit_type_str(v, &value, name, errp);
        free(value);
    }
}

static void property_set_str(Object *obj, Visitor *v, void *opaque,
                             const char *name, Error **errp)
{
    StringProperty *prop = opaque;
    char *value;
    Error *local_err = NULL;

    visit_type_str(v, &value, name, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }

    prop->set(obj, value, errp);
    free(value);
}

static void property_release_str(Object *obj, const char *name,
                                 void *opaque)
{
    StringProperty *prop = opaque;
    free(prop);
}

void object_property_add_str(Object *obj, const char *name,
                           char *(*get)(Object *, Error **),
                           void (*set)(Object *, const char *, Error **),
                           Error **errp)
{
    StringProperty *prop = malloc(sizeof(*prop));
    memset(prop, 0, sizeof(*prop));

    prop->get = get;
    prop->set = set;

    object_property_add(obj, name, "string",
                        get ? property_get_str : NULL,
                        set ? property_set_str : NULL,
                        property_release_str,
                        prop, errp);
}

typedef struct BoolProperty
{
    bool (*get)(Object *, Error **);
    void (*set)(Object *, bool, Error **);
} BoolProperty;

static void property_get_bool(Object *obj, Visitor *v, void *opaque,
                              const char *name, Error **errp)
{
    BoolProperty *prop = opaque;
    bool value;

    value = prop->get(obj, errp);
    visit_type_bool(v, &value, name, errp);
}

static void property_set_bool(Object *obj, Visitor *v, void *opaque,
                              const char *name, Error **errp)
{
    BoolProperty *prop = opaque;
    bool value;
    Error *local_err = NULL;

    visit_type_bool(v, &value, name, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }

    prop->set(obj, value, errp);
}

static void property_release_bool(Object *obj, const char *name,
                                  void *opaque)
{
    BoolProperty *prop = opaque;
    free(prop);
}

void object_property_add_bool(Object *obj, const char *name,
                              bool (*get)(Object *, Error **),
                              void (*set)(Object *, bool, Error **),
                              Error **errp)
{
    BoolProperty *prop = malloc(sizeof(*prop));
    memset(prop, 0, sizeof(*prop));

    prop->get = get;
    prop->set = set;

    object_property_add(obj, name, "bool",
                        get ? property_get_bool : NULL,
                        set ? property_set_bool : NULL,
                        property_release_bool,
                        prop, errp);
}

static char *qdev_get_type(Object *obj, Error **errp)
{
    return strdup(object_get_typename(obj));
}

static void object_instance_init(Object *obj)
{
    object_property_add_str(obj, "type", qdev_get_type, NULL, NULL);
}

static void register_types(void)
{
    static TypeInfo interface_info = {
        .name = TYPE_INTERFACE,
        .class_size = sizeof(InterfaceClass),
        .abstract = true,
    };

    static TypeInfo object_info = {
        .name = TYPE_OBJECT,
        .instance_size = sizeof(Object),
        .instance_init = object_instance_init,
        .abstract = true,
    };

    type_interface = type_register_internal(&interface_info);
    type_register_internal(&object_info);
}

type_init(register_types)
