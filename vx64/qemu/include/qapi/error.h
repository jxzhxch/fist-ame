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
#ifndef ERROR_H
#define ERROR_H

#include "qemu/compiler.h"
#include "qapi-types.h"
#include <stdbool.h>



/**
 * A class representing internal errors within QEMU.  An error has a ErrorClass
 * code and a human message.
 */
typedef struct Error Error;

/**
 * Set an indirect pointer to an error given a ErrorClass value and a
 * printf-style human message.  This function is not meant to be used outside
 * of QEMU.
 */
void error_set(Error **err, ErrorClass err_class, const char *fmt, ...) GCC_FMT_ATTR(3, 4);

/**
 * Set an indirect pointer to an error given a ErrorClass value and a
 * printf-style human message, followed by a strerror() string if
 * @os_error is not zero.
 */
void error_set_errno(Error **err, int os_error, ErrorClass err_class, const char *fmt, ...) GCC_FMT_ATTR(4, 5);

/**
 * Same as error_set(), but sets a generic error
 */
#define error_setg(err, fmt, ...) \
    error_set(err, ERROR_CLASS_GENERIC_ERROR, fmt, ## __VA_ARGS__)
#define error_setg_errno(err, os_error, fmt, ...) \
    error_set_errno(err, os_error, ERROR_CLASS_GENERIC_ERROR, fmt, ## __VA_ARGS__)

/**
 * Returns true if an indirect pointer to an error is pointing to a valid
 * error object.
 */
bool error_is_set(Error **err);

/*
 * Get the error class of an error object.
 */
ErrorClass error_get_class(const Error *err);

/**
 * Returns an exact copy of the error passed as an argument.
 */
Error *error_copy(const Error *err);

/**
 * Get a human readable representation of an error object.
 */
const char *error_get_pretty(Error *err);

/**
 * Propagate an error to an indirect pointer to an error.  This function will
 * always transfer ownership of the error reference and handles the case where
 * dst_err is NULL correctly.  Errors after the first are discarded.
 */
void error_propagate(Error **dst_err, Error *local_err);

/**
 * Free an error object.
 */
void error_free(Error *err);


/*
 * QError class list
 * Please keep the definitions in alphabetical order.
 * Use scripts/check-qerror.sh to check.
 */
#define QERR_ADD_CLIENT_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Could not add client"

#define QERR_AMBIGUOUS_PATH \
    ERROR_CLASS_GENERIC_ERROR, "Path '%s' does not uniquely identify an object"

#define QERR_BAD_BUS_FOR_DEVICE \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' can't go on a %s bus"

#define QERR_BASE_NOT_FOUND \
    ERROR_CLASS_GENERIC_ERROR, "Base '%s' not found"

#define QERR_BLOCK_JOB_NOT_ACTIVE \
    ERROR_CLASS_DEVICE_NOT_ACTIVE, "No active block job on device '%s'"

#define QERR_BLOCK_JOB_PAUSED \
    ERROR_CLASS_GENERIC_ERROR, "The block job for device '%s' is currently paused"

#define QERR_BLOCK_JOB_NOT_READY \
    ERROR_CLASS_GENERIC_ERROR, "The active block job for device '%s' cannot be completed"

#define QERR_BLOCK_FORMAT_FEATURE_NOT_SUPPORTED \
    ERROR_CLASS_GENERIC_ERROR, "Block format '%s' used by device '%s' does not support feature '%s'"

#define QERR_BUFFER_OVERRUN \
    ERROR_CLASS_GENERIC_ERROR, "An internal buffer overran"

#define QERR_BUS_NO_HOTPLUG \
    ERROR_CLASS_GENERIC_ERROR, "Bus '%s' does not support hotplugging"

#define QERR_BUS_NOT_FOUND \
    ERROR_CLASS_GENERIC_ERROR, "Bus '%s' not found"

#define QERR_COMMAND_DISABLED \
    ERROR_CLASS_GENERIC_ERROR, "The command %s has been disabled for this instance"

#define QERR_COMMAND_NOT_FOUND \
    ERROR_CLASS_COMMAND_NOT_FOUND, "The command %s has not been found"

#define QERR_DEVICE_ENCRYPTED \
    ERROR_CLASS_DEVICE_ENCRYPTED, "'%s' (%s) is encrypted"

#define QERR_DEVICE_FEATURE_BLOCKS_MIGRATION \
    ERROR_CLASS_GENERIC_ERROR, "Migration is disabled when using feature '%s' in device '%s'"

#define QERR_DEVICE_HAS_NO_MEDIUM \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' has no medium"

#define QERR_DEVICE_INIT_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' could not be initialized"

#define QERR_DEVICE_IN_USE \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' is in use"

#define QERR_DEVICE_IS_READ_ONLY \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' is read only"

#define QERR_DEVICE_LOCKED \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' is locked"

#define QERR_DEVICE_MULTIPLE_BUSSES \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' has multiple child busses"

#define QERR_DEVICE_NO_BUS \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' has no child bus"

#define QERR_DEVICE_NO_HOTPLUG \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' does not support hotplugging"

#define QERR_DEVICE_NOT_ACTIVE \
    ERROR_CLASS_DEVICE_NOT_ACTIVE, "Device '%s' has not been activated"

#define QERR_DEVICE_NOT_ENCRYPTED \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' is not encrypted"

#define QERR_DEVICE_NOT_FOUND \
    ERROR_CLASS_DEVICE_NOT_FOUND, "Device '%s' not found"

#define QERR_DEVICE_NOT_REMOVABLE \
    ERROR_CLASS_GENERIC_ERROR, "Device '%s' is not removable"

#define QERR_DUPLICATE_ID \
    ERROR_CLASS_GENERIC_ERROR, "Duplicate ID '%s' for %s"

#define QERR_FD_NOT_FOUND \
    ERROR_CLASS_GENERIC_ERROR, "File descriptor named '%s' not found"

#define QERR_FD_NOT_SUPPLIED \
    ERROR_CLASS_GENERIC_ERROR, "No file descriptor supplied via SCM_RIGHTS"

#define QERR_FEATURE_DISABLED \
    ERROR_CLASS_GENERIC_ERROR, "The feature '%s' is not enabled"

#define QERR_INVALID_BLOCK_FORMAT \
    ERROR_CLASS_GENERIC_ERROR, "Invalid block format '%s'"

#define QERR_INVALID_OPTION_GROUP \
    ERROR_CLASS_GENERIC_ERROR, "There is no option group '%s'"

#define QERR_INVALID_PARAMETER \
    ERROR_CLASS_GENERIC_ERROR, "Invalid parameter '%s'"

#define QERR_INVALID_PARAMETER_COMBINATION \
    ERROR_CLASS_GENERIC_ERROR, "Invalid parameter combination"

#define QERR_INVALID_PARAMETER_TYPE \
    ERROR_CLASS_GENERIC_ERROR, "Invalid parameter type for '%s', expected: %s"

#define QERR_INVALID_PARAMETER_VALUE \
    ERROR_CLASS_GENERIC_ERROR, "Parameter '%s' expects %s"

#define QERR_INVALID_PASSWORD \
    ERROR_CLASS_GENERIC_ERROR, "Password incorrect"

#define QERR_IO_ERROR \
    ERROR_CLASS_GENERIC_ERROR, "An IO error has occurred"

#define QERR_JSON_PARSE_ERROR \
    ERROR_CLASS_GENERIC_ERROR, "JSON parse error, %s"

#define QERR_JSON_PARSING \
    ERROR_CLASS_GENERIC_ERROR, "Invalid JSON syntax"

#define QERR_KVM_MISSING_CAP \
    ERROR_CLASS_K_V_M_MISSING_CAP, "Using KVM without %s, %s unavailable"

#define QERR_MIGRATION_ACTIVE \
    ERROR_CLASS_GENERIC_ERROR, "There's a migration process in progress"

#define QERR_MIGRATION_NOT_SUPPORTED \
    ERROR_CLASS_GENERIC_ERROR, "State blocked by non-migratable device '%s'"

#define QERR_MISSING_PARAMETER \
    ERROR_CLASS_GENERIC_ERROR, "Parameter '%s' is missing"

#define QERR_NO_BUS_FOR_DEVICE \
    ERROR_CLASS_GENERIC_ERROR, "No '%s' bus found for device '%s'"

#define QERR_NOT_SUPPORTED \
    ERROR_CLASS_GENERIC_ERROR, "Not supported"

#define QERR_OPEN_FILE_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Could not open '%s'"

#define QERR_PERMISSION_DENIED \
    ERROR_CLASS_GENERIC_ERROR, "Insufficient permission to perform this operation"

#define QERR_PROPERTY_NOT_FOUND \
    ERROR_CLASS_GENERIC_ERROR, "Property '%s.%s' not found"

#define QERR_PROPERTY_VALUE_BAD \
    ERROR_CLASS_GENERIC_ERROR, "Property '%s.%s' doesn't take value '%s'"

#define QERR_PROPERTY_VALUE_IN_USE \
    ERROR_CLASS_GENERIC_ERROR, "Property '%s.%s' can't take value '%s', it's in use"

#define QERR_PROPERTY_VALUE_NOT_FOUND \
    ERROR_CLASS_GENERIC_ERROR, "Property '%s.%s' can't find value '%s'"

#define QERR_PROPERTY_VALUE_NOT_POWER_OF_2 \
    ERROR_CLASS_GENERIC_ERROR, "Property %s.%s doesn't take value '%" PRId64 "', it's not a power of 2"

#define QERR_PROPERTY_VALUE_OUT_OF_RANGE \
    ERROR_CLASS_GENERIC_ERROR, "Property %s.%s doesn't take value %" PRId64 " (minimum: %" PRId64 ", maximum: %" PRId64 ")"

#define QERR_QGA_COMMAND_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Guest agent command failed, error was '%s'"

#define QERR_QGA_LOGGING_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Guest agent failed to log non-optional log statement"

#define QERR_QMP_BAD_INPUT_OBJECT \
    ERROR_CLASS_GENERIC_ERROR, "Expected '%s' in QMP input"

#define QERR_QMP_BAD_INPUT_OBJECT_MEMBER \
    ERROR_CLASS_GENERIC_ERROR, "QMP input object member '%s' expects '%s'"

#define QERR_QMP_EXTRA_MEMBER \
    ERROR_CLASS_GENERIC_ERROR, "QMP input object member '%s' is unexpected"

#define QERR_RESET_REQUIRED \
    ERROR_CLASS_GENERIC_ERROR, "Resetting the Virtual Machine is required"

#define QERR_SET_PASSWD_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Could not set password"

#define QERR_TOO_MANY_FILES \
    ERROR_CLASS_GENERIC_ERROR, "Too many open files"

#define QERR_UNDEFINED_ERROR \
    ERROR_CLASS_GENERIC_ERROR, "An undefined error has occurred"

#define QERR_UNKNOWN_BLOCK_FORMAT_FEATURE \
    ERROR_CLASS_GENERIC_ERROR, "'%s' uses a %s feature which is not supported by this qemu version: %s"

#define QERR_UNSUPPORTED \
    ERROR_CLASS_GENERIC_ERROR, "this feature or command is not currently supported"

#define QERR_VIRTFS_FEATURE_BLOCKS_MIGRATION \
    ERROR_CLASS_GENERIC_ERROR, "Migration is disabled when VirtFS export path '%s' is mounted in the guest using mount_tag '%s'"

#define QERR_SOCKET_CONNECT_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Failed to connect to socket"

#define QERR_SOCKET_LISTEN_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Failed to set socket to listening mode"

#define QERR_SOCKET_BIND_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Failed to bind socket"

#define QERR_SOCKET_CREATE_FAILED \
    ERROR_CLASS_GENERIC_ERROR, "Failed to create socket"

#endif
