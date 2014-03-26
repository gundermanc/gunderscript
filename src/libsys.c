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
#include <unistd.h>

#define LIBSYS_GETLINE_MAXLEN          255

/**
 * VMNative: sys_print( value1, value2, ... )
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
 * VMNative: sys_getline( )
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
 * VMNative: sys_getchar( )
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
 * VMNative: type( )
 * Accepts no arguments. Accepts a single parameter of any time. Returns the type
 * of the value as a string.
 */
static bool vmn_type(VM * vm, VMArg * arg, int argc) {
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
 * VMNative: file_delete( fileName )
 * Deletes file with path/name fileName. Returns true if success, false if fail
 */
static bool vmn_file_delete(VM * vm, VMArg * arg, int argc) {
  char * fileName;
  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* check argument 1 type */
  if((fileName = vmarg_string(arg[0])) == NULL) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }
   
  /* push return value */
  if(!vmarg_push_boolean(vm, unlink(fileName) == 0)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  return true;
}

/**
 * VMNative: file_exists( fileName )
 * Checks if file exists. Returns true if so
 */
static bool vmn_file_exists(VM * vm, VMArg * arg, int argc) {
  char * fileName;
  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* check argument 1 type */
  if((fileName = vmarg_string(arg[0])) == NULL) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }
   
  /* push return value */
  if(!vmarg_push_boolean(vm, access(fileName, F_OK) == 0)) {
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
  if(!vm_reg_callback(gunderscript_vm(gunderscript), "sys_print", 9, vmn_print)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "sys_shell", 9, vmn_shell)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "sys_getline", 11, vmn_getline)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "sys_getchar", 11, vmn_getchar)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "type", 4, vmn_type)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "file_delete", 11, vmn_file_delete)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "file_exists", 11, vmn_file_exists)) {
    return false;
  }

  return true;
}
