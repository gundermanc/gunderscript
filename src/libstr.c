/**
 * libstr.c
 * (C) 2014 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Defines the Gunderscript functions and types for modifying strings.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libstr.h"
#include <string.h>

static void workshop_cleanup(VM * vm, VMLibData * data) {
  Buffer * buffer = vmlibdata_data(data);

  buffer_free(buffer);
}

static VMLibData * workshop_new(int bufferLen) {

  /* allocate workshop object */
  Buffer * buffer = buffer_new(bufferLen, LIBSTR_WORKSHOP_BLOCKSIZE);
  VMLibData * data;

  if(buffer == NULL) {
    return NULL;
  }

  /* allocate VMLibData */
  data = vmlibdata_new(LIBSTR_WORKSHOP_TYPE, LIBSTR_WORKSHOP_TYPE_LEN,
		       workshop_cleanup, buffer);
  if(data == NULL) {
    buffer_free(buffer);
    return NULL;
  }

  return data;
}

/**
 * VMNative: string_equals( string1, string2 )
 * Accepts two string arguments. Returns true if they are the same, and false if
 * not.
 */
static bool vmn_str_equals(VM * vm, VMArg * arg, int argc) {

  /* check for proper number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument type */
  if(!vmarg_is_string(arg[0]) || !vmarg_is_string(arg[1])) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* push result */
  vmarg_push_boolean(vm, strcmp(vmarg_string(arg[0]), 
				vmarg_string(arg[1])) == 0);

  /* this function does return a value */
  return true;
}

/**
 * VMNative: string_workshop( buffersize )
 * Accepts one number argument: the size of the string buffer.
 */
static bool vmn_str_workshop(VM * vm, VMArg * arg, int argc) {

  VMLibData * data;
  int bufferSize;

  /* check for proper number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument type */
  if(vmarg_type(arg[0]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  bufferSize = (int)vmarg_number(arg[0], NULL);

  /* check buffer size range */
  if(bufferSize < 1) {
    vm_set_err(vm, VMERR_ARGUMENT_OUT_OF_RANGE);
    return false;
  }

  /* allocate string workshop */
  data = workshop_new(bufferSize);
  if(data == NULL) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* push result */
  vmarg_push_libdata(vm, data);

  /* this function does return a value */
  return true;
}

/**
 * VMNative: string_workshop_length( workshop )
 * Accepts one number argument: the string workshop instance
 */
static bool vmn_str_workshop_length(VM * vm, VMArg * arg, int argc) {
  VMLibData * data;
  Buffer * buffer;

  /* check for proper number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument major type */
  if(vmarg_type(arg[0]) != TYPE_LIBDATA) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the libdata from the argument */
  data = vmarg_libdata(arg[0]);

  /* check libdata type */
  if(!vmlibdata_is_type(data, LIBSTR_WORKSHOP_TYPE, LIBSTR_WORKSHOP_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the buffer */
  buffer = vmlibdata_data(data);

  /* push string length */
  vmarg_push_number(vm, buffer_size(buffer));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: string_workshop_resize( workshop , newsize )
 * Accepts a workshop argument and a number. The newsize is the size to
 * reallocate the workshop buffer to. Preallocate buffer for best performance.
 */
static bool vmn_str_workshop_prealloc(VM * vm, VMArg * arg, int argc) {
  VMLibData * data;
  Buffer * buffer;
  int newSize;

  /* check for proper number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument 1 major type */
  if(vmarg_type(arg[0]) != TYPE_LIBDATA) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the libdata from the argument */
  data = vmarg_libdata(arg[0]);

  /* check libdata type */
  if(!vmlibdata_is_type(data, LIBSTR_WORKSHOP_TYPE, LIBSTR_WORKSHOP_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* check argument 2 type */
  if(vmarg_type(arg[1]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  newSize = (int)vmarg_number(arg[1], NULL);

  /* check buffer size range */
  if(newSize < 1) {
    vm_set_err(vm, VMERR_ARGUMENT_OUT_OF_RANGE);
    return false;
  }

  /* extract the workshop */
  buffer = vmlibdata_data(data);

  /* can't make it smaller, only bigger */
  buffer_resize(buffer, newSize >= buffer_size(buffer)
		? newSize : buffer_size(buffer));

  /* push null result */
  vmarg_push_null(vm);

  /* this function does return a value */
  return true;
}

/* TODO: finish implementing */
/**
 * VMNative: string_workshop_append( workshop , string )
 * Accepts a workshop argument and a string. The string will be allocated
 * onto the buffer.
 */
static bool vmn_str_workshop_append(VM * vm, VMArg * arg, int argc) {
  VMLibData * data;
  Buffer * buffer;
  char * appendStr;

  /* check for proper number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument 1 major type */
  if(vmarg_type(arg[0]) != TYPE_LIBDATA) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the libdata from the argument */
  data = vmarg_libdata(arg[0]);

  /* check libdata type */
  if(!vmlibdata_is_type(data, LIBSTR_WORKSHOP_TYPE, LIBSTR_WORKSHOP_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* check argument 2 type */
  if(!vmarg_is_string(arg[1])) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the buffer */
  buffer = vmlibdata_data(data);
  appendStr = vmarg_string(arg[1]);

  /* append the string to the buffer 
   * TODO: perhaps make it so this doesn't have to use strlen
   */
  if(!buffer_append_string(buffer, appendStr, strlen(appendStr))) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* push null result */
  vmarg_push_null(vm);

  /* this function does return a value */
  return true;
}

/**
 * VMNative: string_workshop_to_string( workshop )
 * Accepts one number argument: the string workshop instance
 */
static bool vmn_str_workshop_to_string(VM * vm, VMArg * arg, int argc) {
  VMLibData * data;
  VMLibData * newStringData;
  Buffer * buffer;

  /* check for proper number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument major type */
  if(vmarg_type(arg[0]) != TYPE_LIBDATA) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the libdata from the argument */
  data = vmarg_libdata(arg[0]);

  /* check libdata type */
  if(!vmlibdata_is_type(data, LIBSTR_WORKSHOP_TYPE, LIBSTR_WORKSHOP_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the buffer */
  buffer = vmlibdata_data(data);

  /* create container object for new string */
  newStringData = vmarg_new_string(buffer_get_buffer(buffer), 
				   buffer_size(buffer));
  if(newStringData == NULL) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* push new string */
  vmarg_push_libdata(vm, newStringData);

  /* this function does return a value */
  return true;
}

/**
 * VMNative: string_workshop_char_at( workshop, index )
 * Accepts one number argument: the string workshop instance
 */
static bool vmn_str_workshop_char_at(VM * vm, VMArg * arg, int argc) {
  VMLibData * data;
  VMLibData * newStringData;
  Buffer * buffer;
  int index;

  /* check for proper number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument major type */
  if(vmarg_type(arg[0]) != TYPE_LIBDATA ||
     vmarg_type(arg[1]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the libdata from the argument */
  data = vmarg_libdata(arg[0]);

  /* check libdata type */
  if(!vmlibdata_is_type(data, LIBSTR_WORKSHOP_TYPE, LIBSTR_WORKSHOP_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the buffer */
  buffer = vmlibdata_data(data);

  /* extract the index */
  index = vmarg_number(arg[1], NULL);

  /* check buffer size range */
  if(index < 0 || index >= buffer_size(buffer)) {
    vm_set_err(vm, VMERR_ARGUMENT_OUT_OF_RANGE);
    return false;
  }

  /* create container object for new string */
  newStringData = vmarg_new_string(buffer_get_buffer(buffer) + index, 
				   1);
  if(newStringData == NULL) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* push new string */
  vmarg_push_libdata(vm, newStringData);

  /* this function does return a value */
  return true;
}

/**
 * Installs the Libstr library in the given instance of Gunderscript.
 * gunderscript: the instance to receive the library.
 * returns: true upon success, and false upon failure. If failure occurs,
 * you probably did not allocate enough callbacks space in the call to 
 * gunderscript_new().
 */
bool libstr_install(Gunderscript * gunderscript) {
  if(!vm_reg_callback(gunderscript_vm(gunderscript), 
		      "string_equals", 13, vmn_str_equals)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
		      "string_workshop", 15, vmn_str_workshop)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
		      "string_workshop_length", 22, vmn_str_workshop_length)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
		      "string_workshop_prealloc", 24, vmn_str_workshop_prealloc)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "string_workshop_append", 22, vmn_str_workshop_append)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "string_workshop_to_string", 25, vmn_str_workshop_to_string)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "string_workshop_char_at", 23, vmn_str_workshop_char_at)
) {
    return false;
  }
  return true;
}
