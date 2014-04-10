/**
 * libstr.c
 * (C) 2014 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Defines the Gunderscript functions and types for modifying string buffers.
 * These methods are not safe for public interfaces and should be used within
 * the VM only, due to the lack of type/error checking for some methods. Use
 * vmarg_new_string(), vmarg_push_libdata(),  vmarg_is_string(), and
 * vmarg_string() within your own libraries instead.
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
#include <limits.h>

/**
 * Frees a string buffer after it goes out of scope.
 * vm: an instance of VM.
 * data: the VMLIBDATA containing the string to be freed.
 * This method is called automatically by the VM when object goes out
 * of scope.
 */
static void string_cleanup(VM * vm, VMLibData * data) {
  Buffer * buffer = vmlibdata_data(data);

  buffer_free(buffer);
}

/**
 * Creates a new string buffer encased in a VMLibData. Use vmarg_push_libdata()
 * with TYPE_LIBDATA to push this string to the VM's stack.
 * bufferLen: the length of the string buffer.
 * returns: the new VMLibData object. See vmlibdata_*() functions for more info.
 */
VMLibData * libstr_string_new(int bufferLen) {

  /* allocate workshop object */
  Buffer * buffer = buffer_new(bufferLen, LIBSTR_STRING_BLOCKSIZE);
  VMLibData * data;

  if(buffer == NULL) {
    return NULL;
  }

  /* allocate VMLibData */
  data = vmlibdata_new(LIBSTR_STRING_TYPE, LIBSTR_STRING_TYPE_LEN,
		       string_cleanup, buffer);
  if(data == NULL) {
    buffer_free(buffer);
    return NULL;
  }

  return data;
}

/**
 * Gets the string contained inside of a VMLibData object.
 * data: the VMLibData containing the string buffer.
 * returns: pointer to the string. This pointer should NOT be written to. Use
 * libstr_*() functions instead. 
 * NOTE: this function does not check to make sure this VMLibData is a string
 * once asserts are disabled. You should error check accordingly.
 */
char * libstr_string(VMLibData * data) {
  assert(vmlibdata_is_type(data, LIBSTR_STRING_TYPE, LIBSTR_STRING_TYPE_LEN));

  return buffer_get_buffer( ((Buffer*)vmlibdata_data(data)) );
}

/**
 * Gets the length of this string.
 * NOTE: length refers to the number of characters in the buffer, not the
 * length of the buffer itself.
 * data: the VMLibData containing the string buffer.
 * returns: the length of the string as an integer.
 * NOTE: no type checking or error checking in this method. Not safe for
 * public interface.
 */
int libstr_string_length(VMLibData * data) {
  return buffer_size( ((Buffer*)vmlibdata_data(data)) );
}

/**
 * Appends the specified string to the end of the string in this VMLibData.
 * data: the VMLibData containing the string buffer.
 * string: the string to append.
 * stringLen: the length of the string to append.
 * returns: true if success, false if malloc fails.
 * NOTE: no type checking or error checking in this method. Not safe for
 * public interface.
 */
bool libstr_string_append(VMLibData * data, char * string, int stringLen) {
  return buffer_append_string(vmlibdata_data(data), string, stringLen);
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
 * VMNative: string ( buffersize )
 * Accepts one number argument: the size of the string buffer.
 */
static bool vmn_str(VM * vm, VMArg * arg, int argc) {

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
  data = libstr_string_new(bufferSize);
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
 * VMNative: string_length( workshop )
 * Accepts one number argument: the string workshop instance
 */
static bool vmn_str_length(VM * vm, VMArg * arg, int argc) {
  VMLibData * data;

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
  if(!vmlibdata_is_type(data, LIBSTR_STRING_TYPE, LIBSTR_STRING_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* push string length */
  vmarg_push_number(vm, libstr_string_length(data));

  /* this function does return a value */
  return true;
}

/**
 * VMNative: string_prealloc( workshop , newsize )
 * Accepts a workshop argument and a number. The newsize is the size to
 * reallocate the workshop buffer to. Preallocate buffer for best performance.
 */
static bool vmn_str_prealloc(VM * vm, VMArg * arg, int argc) {
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
  if(!vmlibdata_is_type(data, LIBSTR_STRING_TYPE, LIBSTR_STRING_TYPE_LEN)) {
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
 * VMNative: string_append( workshop , string )
 * Accepts a workshop argument and a string. The string will be allocated
 * onto the buffer.
 */
static bool vmn_str_append(VM * vm, VMArg * arg, int argc) {
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
  if(!vmlibdata_is_type(data, LIBSTR_STRING_TYPE, LIBSTR_STRING_TYPE_LEN)) {
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
 * VMNative: string_char_at( workshop, index )
 * Accepts one number argument: the string workshop instance
 * Returns the desired char as a TYPE_NUMBER.
 */
static bool vmn_str_char_at(VM * vm, VMArg * arg, int argc) {
  VMLibData * data;
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
  if(!vmlibdata_is_type(data, LIBSTR_STRING_TYPE, LIBSTR_STRING_TYPE_LEN)) {
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

  /* push char as a number */
  if(!vmarg_push_number(vm, buffer_get_buffer(buffer)[index] )) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* this function does return a value */
  return true;
}

/**
 * VMNative: char_to_string( char )
 * Accepts one number argument: the char to convert
 * Returns the desired char as a VM String.
 */
static bool vmn_char_to_str(VM * vm, VMArg * arg, int argc) {
  VMLibData * newStrData;
  char character[2] = "";

  /* check for proper number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument major type */
  if(vmarg_type(arg[0]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  character[0] = (char) vmarg_number(arg[0], NULL);

  /* check if value is out of range for char */
  if(character[0] > CHAR_MAX || character[0] < CHAR_MIN) {
    vm_set_err(vm, VMERR_ARGUMENT_OUT_OF_RANGE);
    return false;
  }

  newStrData = vmarg_new_string(character, 1);

  /* push char as a number */
  if(newStrData == NULL || !vmarg_push_libdata(vm, newStrData)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* this function does return a value */
  return true;
}

/**
 * VMNative: string_set_char_at( string, index, value )
 * Accepts one number argument: the string workshop instance
 * Returns NULL.
 */
static bool vmn_str_set_char_at(VM * vm, VMArg * arg, int argc) {
  VMLibData * data;
  Buffer * buffer;
  int index;
  char value;

  /* check for proper number of arguments */
  if(argc != 3) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument major type */
  if(vmarg_type(arg[0]) != TYPE_LIBDATA ||
     vmarg_type(arg[1]) != TYPE_NUMBER ||
     vmarg_type(arg[1]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the libdata from the argument */
  data = vmarg_libdata(arg[0]);

  /* check libdata type */
  if(!vmlibdata_is_type(data, LIBSTR_STRING_TYPE, LIBSTR_STRING_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* extract the buffer */
  buffer = vmlibdata_data(data);

  /* extract the index */
  index = vmarg_number(arg[1], NULL);
  value = (char) vmarg_number(arg[2], NULL);

  /* check buffer size range */
  if(index < 0) {
    vm_set_err(vm, VMERR_ARGUMENT_OUT_OF_RANGE);
    return false;
  }

  /* check char value */
  if(value > CHAR_MAX || value < CHAR_MIN) {
    vm_set_err(vm, VMERR_ARGUMENT_OUT_OF_RANGE);
    return false;
  }

  /* push char as a number */
  if(!buffer_set_char(buffer, value, index)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* this function does not return a value */
  return false;
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
		      "string", 6, vmn_str)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
		      "string_length", 13, vmn_str_length)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
		      "string_prealloc", 15, vmn_str_prealloc)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "string_append", 13, vmn_str_append)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "string_char_at", 14, vmn_str_char_at)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "char_to_string", 14, vmn_char_to_str)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "string_set_char_at", 18, vmn_str_set_char_at)
) {
    return false;
  }
  return true;
}
