/**
 * libsys.c
 * (C) 2014 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Defines the Gunderscript functions and types for interfacing with the
 * computer system.
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

#include "gunderscript.h"
#include "vm.h"
#include <string.h>

#define LIBSYS_GETLINE_MAXLEN          255

/**
 * VMNative: _sys_print( value1, value2, ... )
 * Accepts unlimited number of arguments. Prints them on the screen in their
 * string form.
 */
static bool vmn_print(VM * vm, VMArg * arg, int argc) {
  int i = 0;

  for(i = 0; i < argc; i++) {
    switch(vmarg_type(arg[i])) {
    case TYPE_LIBDATA : {
      if(vmarg_is_string(arg[i])) {
	printf("%s", vmarg_string(arg[i]));
      }
      break;
    }
    case TYPE_NUMBER:
      printf("%f", vmarg_number(arg[i], NULL));
      break;
    case TYPE_BOOLEAN:
      printf("%s", vmarg_boolean(arg[i], NULL) ? "true":"false");
      break;
    case TYPE_NULL:
      printf("null");
      break;
    default:
      printf("_sys_print: Unrecognised type.");
    }
  }

  /* this function does not return a value */
  return false;
}

/**
 * VMNative: _sys_getline( )
 * Accepts no arguments. Reads in a line from the console and returns it as a
 * string.
 */
static bool vmn_getline(VM * vm, VMArg * arg, int argc) {
  char line[LIBSYS_GETLINE_MAXLEN];
  VMLibData * result;
  /* check for correct number of arguments */
  if(argc != 0) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }


  /* get the input from the console */
  if(fgets(line, LIBSYS_GETLINE_MAXLEN, stdin) != NULL) {
    result = vmarg_new_string(line, strlen(line));
    
    /* check for malloc error */
    if(result == NULL) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }
   
    /* push return value */
    if(!vmarg_push_libdata(vm, result)) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    return true;
  } else {
    vmarg_push_null(vm);
  }

  return false;
}

/**
 * VMNative: _sys_getchar( )
 * Accepts no arguments. Reads in a char from the console and returns it as a
 * number.
 */
static bool vmn_getchar(VM * vm, VMArg * arg, int argc) {
  int c = 0;

  /* check for correct number of arguments */
  if(argc != 0) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  c = fgetc(stdin);

  /* get the input from the console */
  if(c != EOF) {

    /* push return value */
    if(!vmarg_push_number(vm, (double)c)) {
      vm_set_err(vm, VMERR_ALLOC_FAILED);
      return false;
    }

    return true;
  } else {
    vmarg_push_null(vm);
  }

  return false;
}

/**
 * VMNative: _type( )
 * Accepts no arguments. Reads in a line from the console and returns it as a
 * string.
 */
static bool vmn_type(VM * vm, VMArg * arg, int argc) {
  char line[LIBSYS_GETLINE_MAXLEN];
  VMLibData * result;
  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }


  /* get the input from the console */
  switch(arg[0].type) {
  case TYPE_NULL:
    result = vmarg_new_string("NULL", 4);
    break;
  case TYPE_BOOLEAN:
    result = vmarg_new_string("BOOLEAN", 7);
    break;
  case TYPE_NUMBER:
    result = vmarg_new_string("NUMBER", 6);
    break;
  case TYPE_LIBDATA: {
    char libDataType[20];
    strcpy(libDataType, "LIBDATA{");
    strcat(libDataType, vmarg_libdata(arg[0])->type);
    strcat(libDataType, "}");
    result = vmarg_new_string(libDataType, strlen(libDataType));
    break;
  }
  }
    
    
  /* check for malloc error */
  if(result == NULL) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }
   
  /* push return value */
  if(!vmarg_push_libdata(vm, result)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  return true;
}

/**
 * VMNative: _sys_shell( command )
 * Accepts one argument. Feeds the command into the shell.
 */
static bool vmn_shell(VM * vm, VMArg * arg, int argc) {
  VMLibData * data = vmarg_libdata(arg[0]);

  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* check 1st arg is string */
  if(arg[0].type != TYPE_LIBDATA
     || !vmlibdata_is_type(data, GXS_STRING_TYPE, GXS_STRING_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);

    /* this function does not return a value */
    return false;
  }

  system(vmlibdata_data(data));
  return false;
}

/**
 * Installs the Libsys library in the given instance of Gunderscript.
 * gunderscript: the instance to receive the library.
 * returns: true upon success, and false upon failure. If failure occurs,
 * you probably did not allocate enough callbacks space in the call to 
 * gunderscript_new().
 */
bool libsys_install(Gunderscript * gunderscript) {
  if(!vm_reg_callback(gunderscript_vm(gunderscript), "_sys_print", 10, vmn_print)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "_sys_shell", 10, vmn_shell)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "_sys_getline", 12, vmn_getline)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "_sys_getchar", 12, vmn_getchar)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "_type", 5, vmn_type)) {
    return false;
  }

  return true;
}
