/*
 * Device Container
 *
 * Copyright IBM, Corp. 2012
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qom/object.h"
#include "qemu/module.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

static const TypeInfo container_info = {
    .name          = "container",
    .instance_size = sizeof(Object),
    .parent        = TYPE_OBJECT,
};

static void container_register_types(void)
{
    type_register_static(&container_info);
}

Object *container_get(Object *root, const char *path)
{
    Object *obj, *child;
    char current_part[MAX_PATH];
    obj = root;
    const char* slash;
    for(slash = strchr(path, '/');
    		slash;
    		path = slash + 1) {
    	memcpy(current_part, path, slash - path);
    	current_part[slash - path] = 0;
    	child = object_resolve_path_component(obj, current_part);
    	if (!child) {
    	      child = object_new("container");
    	      object_property_add_child(obj, current_part, child, NULL);
    	}
    }

    return obj;
}


type_init(container_register_types)
