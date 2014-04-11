/**
 * libarray.c
 * (C) 2014 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Defines the Gunderscript functions and types for interfacing with arrays.
 * TODO: There is some WEIRD pointer issue where I have to dereference a
 * pointer that shouldn't need to be dereferenced. Not sure why, but I
 * don't like it and want to change it when I have time to figure it out.
 * Search file for TODO.
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

#include "libarray.h"
#include <string.h>
#include <assert.h>

/**
 * Destroys an array contained in a VMLibData. This function is called
 * automatically by the VM when the array goes out of scope.
 * vm: an instance of VM.
 * data: the VMLibData containing the array.
 */
static void array_cleanup(VM * vm, VMLibData * data) {

  /* free references to objects */
  int i = 0;
  int bufferSize = libarray_array_size(data);
  Buffer * buffer = vmlibdata_data(data);

  for(i = 0; i < bufferSize; i++) {
    /* handle reference counters for old value */
    if((buffer_get_buffer(buffer) + i * LIBARRAY_ENTRY_SIZE + VM_VAR_SIZE)[0]
       == TYPE_LIBDATA) {
      VMLibData * oldValue;
      memcpy(&oldValue, (buffer_get_buffer(buffer) + i * LIBARRAY_ENTRY_SIZE), 
	     sizeof(VMLibData*));
      vmlibdata_dec_refcount(oldValue);
      vmlibdata_check_cleanup(vm, oldValue);
    }
  }
  buffer_free(vmlibdata_data(data));
}

/**
 * Creates a new array object encased in a VMLibData. This can be pushed
 * directly to the stack as an array variable.
 * size: the number of slots to have in the array.
 */
VMLibData * libarray_array_new(int size) {
  assert(size > 0);

  Buffer * array;
  VMLibData * data;

  /* create new expanding buffer for holding the array values */
  array = buffer_new(size * LIBARRAY_ENTRY_SIZE, 
		     LIBARRAY_BLOCK_COUNT * LIBARRAY_ENTRY_SIZE);
  if(array == NULL) {
    return NULL;
  }

  data = vmlibdata_new(LIBARRAY_ARRAY_TYPE, LIBARRAY_ARRAY_TYPE_LEN,
		       array_cleanup, array);
  if(data == NULL) {
    return NULL;
  }

  return data;
}

/**
 * Sets a value in an array.
 * vm: an instance of vm
 * data: the array to modify.
 * index: the index of the item to modify.
 * value: the value to store.
 * valueSize: the size of the value. Can be at MOST VM_VAR_SIZE.
 * type: the type of the value to store.
 */
bool libarray_array_set(VM * vm, VMLibData * data, int index, 
			void * value, int valueSize, VarType type) {
  assert(data != NULL);
  assert(valueSize <= VM_VAR_SIZE);
  assert(valueSize > 0);

  Buffer * buffer = vmlibdata_data(data);

  /* handle reference counters for new value */
  /* TODO: figure out why this WEIRD pointer shit is necessary..something is wrong */
  if(type == TYPE_LIBDATA) {
    vmlibdata_inc_refcount( *((VMLibData**)value) );
  }

  /* handle reference counters for old value */
  if((buffer_get_buffer(buffer) + index * LIBARRAY_ENTRY_SIZE + VM_VAR_SIZE)[0]
     == TYPE_LIBDATA) {
    VMLibData * oldValue;
    memcpy(&oldValue, (buffer_get_buffer(buffer) + index * LIBARRAY_ENTRY_SIZE), 
	   sizeof(VMLibData*));
    vmlibdata_dec_refcount(oldValue);
    vmlibdata_check_cleanup(vm, oldValue);
  }

  /* store the value in the specified index, last byte is the type */
  return buffer_set_string(buffer, value, 
			   valueSize, index * LIBARRAY_ENTRY_SIZE)
    && buffer_set_char(buffer, type, index * LIBARRAY_ENTRY_SIZE + VM_VAR_SIZE);
}

/**
 * Gets the number of slots in the array. Array autoexpands as neccessary.
 * data: the array.
 * returns: the number of slots in the array.
 */
int libarray_array_size(VMLibData * data) {
  assert(data != NULL);

  return buffer_buffer_size(vmlibdata_data(data)) / LIBARRAY_ENTRY_SIZE;
}

/**
 * Gets the type of the variable at the specified index in the array.
 * data: The array.
 * index: the index to look at.
 * type: pointer to a VarType that will receive the type.
 * returns: true on success, and false if index is bigger than possible indicies.
 */
bool libarray_array_get_type(VMLibData * data, int index, VarType * type) {
  assert(data != NULL);
  assert(type != NULL);

  Buffer * buffer = vmlibdata_data(data);

  /* make sure that there are enough items in the array */
  if(index < 0 || libarray_array_size(data) <= index) {
    return false;
  }

  *type = (buffer_get_buffer(buffer) 
	   + (index * LIBARRAY_ENTRY_SIZE + VM_VAR_SIZE)) [0];

  return true;
}

/**
 * Gets a value from an array.
 * data: the array.
 * index: the array index.
 * value: the buffer that will receive the value.
 * valueSize: the size of the value to copy. Can me VM_VAR_SIZE at max.
 * returns: true upon success, or false if the given index is out of range.
 */
bool libarray_array_get(VMLibData * data, int index,
			void * value, int valueSize) {
  assert(data != NULL);
  assert(index >= 0);
  assert(value != NULL);
  assert(valueSize > 0);
  assert(valueSize <= VM_VAR_SIZE);

  Buffer * buffer = vmlibdata_data(data);

  /* make sure that there are enough items in the array */
  if(libarray_array_size(data) <= index) {
    return false;
  }

  /* copy value to output buffer */
  memcpy(value, buffer_get_buffer(buffer) + (index * LIBARRAY_ENTRY_SIZE),
	 valueSize);

  return true;
}

/**
 * VMNative: array ( size )
 * Accepts one number argument. Creates new array of given size
 * not.
 */
static bool vmn_array(VM * vm, VMArg * arg, int argc) {

  VMLibData * newArray;
  int size;

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

  size = vmarg_number(arg[0], NULL);

  /* check array size is > 0 */
  if(size <= 0) {
    vm_set_err(vm, VMERR_ARGUMENT_OUT_OF_RANGE);
    return false;
  }

  newArray = libarray_array_new(size);

  /* check that array alloc succeeded */
  if(newArray == NULL || !vmarg_push_libdata(vm, newArray)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* this function does return a value */
  return true;
}

/**
 * VMNative: array_size ( array )
 * Accepts one array argument. Gets the size of the array.
 */
static bool vmn_array_size(VM * vm, VMArg * arg, int argc) {

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

  data = vmarg_libdata(arg[0]);

  /* check argument minor type */
  if(!vmlibdata_is_type(data, LIBARRAY_ARRAY_TYPE, LIBARRAY_ARRAY_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  /* check that array alloc succeeded */
  if(!vmarg_push_number(vm, libarray_array_size(data))) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* this function does return a value */
  return true;
}

/**
 * VMNative: array_set ( array , index , value )
 * Accepts one array argument, and two numbers. Sets the values in the array.
 * No return value.
 */
static bool vmn_array_set(VM * vm, VMArg * arg, int argc) {

  VMLibData * data;
  int index;

  /* check for proper number of arguments */
  if(argc != 3) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument major types */
  if(vmarg_type(arg[0]) != TYPE_LIBDATA
     || vmarg_type(arg[1]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  data = vmarg_libdata(arg[0]);
  
  /* check argument minor type */
  if(!vmlibdata_is_type(data, LIBARRAY_ARRAY_TYPE, LIBARRAY_ARRAY_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  index = vmarg_number(arg[1], NULL);

  /* make sure index is in valid range */
  if(index < 0) {
    vm_set_err(vm, VMERR_ARGUMENT_OUT_OF_RANGE);
    return false;
  }

  /* store value and check for failed alloc */
  if(!libarray_array_set(vm, data, index, vmarg_data(&arg[2]), 
			 VM_VAR_SIZE, vmarg_type(arg[2]))) {
       vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* this function does not return a value */
  return false;
}

/**
 * VMNative: array_get ( array , index )
 * Accepts one array argument, and one number. Gets the value in the array.
 */
static bool vmn_array_get(VM * vm, VMArg * arg, int argc) {

  VMLibData * data;
  char storedData[VM_VAR_SIZE];
  VarType type;
  int index;

  /* check for proper number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);
    return false;
  }

  /* check argument major types */
  if(vmarg_type(arg[0]) != TYPE_LIBDATA
     || vmarg_type(arg[1]) != TYPE_NUMBER) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  data = vmarg_libdata(arg[0]);
  
  /* check argument minor type */
  if(!vmlibdata_is_type(data, LIBARRAY_ARRAY_TYPE, LIBARRAY_ARRAY_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }

  index = vmarg_number(arg[1], NULL);

  /* make sure index is in valid range */
  if(index < 0 || index >= libarray_array_size(data)) {
    vm_set_err(vm, VMERR_ARGUMENT_OUT_OF_RANGE);
    return false;
  }

  libarray_array_get_type(data, index, &type);
  libarray_array_get(data, index, storedData, VM_VAR_SIZE);

  /* push value and check for failed alloc */
  if(!vmarg_push_data(vm, storedData, type)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* this function does not return a value */
  return true;
}

/**
 * Installs the Libdatastruct library in the given instance of Gunderscript.
 * gunderscript: the instance to receive the library.
 * returns: true upon success, and false upon failure. If failure occurs,
 * you probably did not allocate enough callbacks space in the call to 
 * gunderscript_new().
 */
bool libarray_install(Gunderscript * gunderscript) {
  if(!vm_reg_callback(gunderscript_vm(gunderscript), 
		      "array", 5, vmn_array)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "array_size", 10, vmn_array_size)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "array_set", 9, vmn_array_set)
     || !vm_reg_callback(gunderscript_vm(gunderscript), 
			 "array_get", 9, vmn_array_get)) {
    return false;
  }
  return true;
}
