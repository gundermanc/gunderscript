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
  LibstrWorkshop * ws = vmlibdata_data(data);

  free(ws->stringBuffer);
  free(ws);
}

static bool workshop_resize(LibstrWorkshop * ws, int newSize) {
  assert(newSize > 0);

  /* alloc new buffer */
  char * newBuffer = calloc(newSize + 1, sizeof(char));
  int i = 0;
  if(newBuffer == NULL) {
    return false;
  }

  /* copy bytes 1 by 1...do this manually in case this isn't a string */
  for(i = 0; i < ws->stringLen; i++) {
    newBuffer[i] = ws->stringBuffer[i];
  }

  /* free old buffer and switch to new one */
  free(ws->stringBuffer);
  ws->stringBuffer = newBuffer;
  ws->bufferLen = newSize;
  
  return true;
}

static VMLibData * workshop_new(int bufferLen) {

  /* allocate workshop object */
  LibstrWorkshop * ws = calloc(1, sizeof(LibstrWorkshop));
  VMLibData * data;

  if(ws == NULL) {
    return NULL;
  }

  /* allocate string buffer and store values */
  ws->stringBuffer = calloc(bufferLen + 1, sizeof(char));
  ws->bufferLen = bufferLen;
  ws->stringLen = 0;

  /* allocate VMLibData */
  data = vmlibdata_new(LIBSTR_WORKSHOP_TYPE, LIBSTR_WORKSHOP_TYPE_LEN,
		       workshop_cleanup, ws);
  if(data == NULL) {
    free(ws->stringBuffer);
    free(ws);
    return NULL;
  }

  return data;
}

static int workshop_string_length(LibstrWorkshop * ws) {
  return ws->stringLen;
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
  LibstrWorkshop * ws;

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

  /* extract the workshop */
  ws = vmlibdata_data(data);

  /* push string length */
  vmarg_push_number(vm, workshop_string_length(ws));

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
  LibstrWorkshop * ws;
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
  ws = vmlibdata_data(data);

  /* can't make it smaller, only bigger */
  workshop_resize(ws, newSize >= workshop_string_length(ws) 
		  ? newSize : workshop_string_length(ws));

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
  LibstrWorkshop * ws;

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

  /* extract the workshop */
  ws = vmlibdata_data(data);


  /* push null result */
  vmarg_push_null(vm);

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
		      "string_workshop_prealloc", 24, vmn_str_workshop_prealloc)) {
    return false;
  }
  return true;
}
