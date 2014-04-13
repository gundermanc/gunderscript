/**
 * libsys.c
 * (C) 2014 Christian Gunderman + Kai Smith
 * Modified by: KaiSmith
 * Author Email: gundermanc@gmail.com
 * Modifier Email: kjs108@case.edu
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
#include "libsys.h"
#include <string.h>
#include <unistd.h>

#define LIBSYS_GETLINE_MAXLEN          255
#define LIBSYS_TOSTRING_MAXLEN         25

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
      printf("sys_print: Unrecognised type.");
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
 * Accepts a single parameter of any type. Returns the type
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
 * Frees a FILE* pointer in a VMLibData for the Gunderscript file type.
 * Called automatically by the VM when the file goes out of scope.
 */
void filepointer_free(VM * VM, VMLibData * data) {
  FILE * file = vmlibdata_data(data);
  if (file != NULL){
    fclose(file);
  }
}

/**
 * VMNative: file_open( fileName )
 * Open the file in the desired accessMode.
 */
static bool vmn_file_open(VM * vm, VMArg * arg, int argc) {
  char * fileName;
  char * accessMode;
  FILE * file;
  VMLibData * filePointer;

  /* check for correct number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* check argument 1 type */
  if((fileName = vmarg_string(arg[0])) == NULL 
     || (accessMode = vmarg_string(arg[1])) == NULL) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }
  
  file = fopen(fileName, accessMode);

  if (file == NULL){
    vmarg_push_null(vm);
  }

  filePointer = vmlibdata_new(LIBSYS_FILE_TYPE, LIBSYS_FILE_TYPE_LEN,
			      filepointer_free, file);
   
  /* push return value */
  if(!vmarg_push_libdata(vm, filePointer)){
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  return true;
}

/**
 * VMNative: file_open_read( fileName )
 * Open the file with read permission.
 */
static bool vmn_file_open_read(VM * vm, VMArg * arg, int argc) {
  char * fileName;
  FILE * file;
  VMLibData * filePointer;

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
  
  file = fopen(fileName, "r");

  if (file == NULL){
    vmarg_push_null(vm);
  }

  filePointer = vmlibdata_new(LIBSYS_FILE_TYPE, LIBSYS_FILE_TYPE_LEN, filepointer_free, file);
   
  /* push return value */
  if(!vmarg_push_libdata(vm, filePointer)){
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  return true;
}

/**
 * VMNative: file_open_write( fileName )
 * Open the file with write permission.
 */
static bool vmn_file_open_write(VM * vm, VMArg * arg, int argc) {
  char * fileName;
  FILE * file;
  VMLibData * filePointer;

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
  
  file = fopen(fileName, "w");

  if (file == NULL) {
    vmarg_push_null(vm);
  }

  filePointer = vmlibdata_new(LIBSYS_FILE_TYPE, LIBSYS_FILE_TYPE_LEN, filepointer_free, file);
   
  /* push return value */
  if(!vmarg_push_libdata(vm, filePointer)){
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  return true;
}



/**
 * VMNative: file_close( file )
 * Closes the desired file.
 */
static bool vmn_file_close(VM * vm, VMArg * arg, int argc) {
  VMLibData * filePointer;

  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* check argument 1 type */
  if((filePointer = vmarg_libdata(arg[0])) == NULL 
     || !vmlibdata_is_type(filePointer, 
			   LIBSYS_FILE_TYPE, LIBSYS_FILE_TYPE_LEN )) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }
  
  fclose(vmlibdata_data(filePointer));

  /* prevents the cleanup routine from double freeing */
  vmlibdata_set_data(filePointer, NULL);

  return false;
}


/**
 * VMNative: file_read_char( file )
 * Returns the next character in the file.
 */
static bool vmn_file_read_char(VM * vm, VMArg * arg, int argc) {
  VMLibData * filePointer;
  int c;

  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* check argument 1 type */
  if((filePointer = vmarg_libdata(arg[0])) == NULL ||
        !vmlibdata_is_type(filePointer, LIBSYS_FILE_TYPE, LIBSYS_FILE_TYPE_LEN)) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }
  
  if(filePointer->libData == NULL){
    vm_set_err(vm, VMERR_FILE_CLOSED);
    return false;
  }

  c = fgetc(vmlibdata_data(filePointer)); 

  vmarg_push_number(vm, c);

  return true;
}

/**
 * VMNative: file_write_char( file , char )
 * Writes a character to the file.
 */
static bool vmn_file_write_char(VM * vm, VMArg * arg, int argc) {
  VMLibData * filePointer;
  bool isNumber = false;
  int c;

  /* check for correct number of arguments */
  if(argc != 2) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  filePointer = vmarg_libdata(arg[0]);
  c = (int) vmarg_number(arg[1], &isNumber);

  /* check argument 1 type */
  if(!isNumber || filePointer == NULL
     || !vmlibdata_is_type(filePointer, 
			   LIBSYS_FILE_TYPE, LIBSYS_FILE_TYPE_LEN )) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }
 
  /* file already closed, throw error */
  if(filePointer->libData == NULL){
    vm_set_err(vm, VMERR_FILE_CLOSED);
    return false;
  }

  /* write failed, return false */
  if(fputc(c, vmlibdata_data(filePointer)) == EOF){
    vmarg_push_boolean(vm, false);
  }

  vmarg_push_boolean(vm, true);

  return true;
}

/**
 * VMNative: sys_shell( command )
 * Accepts one argument. Feeds the command into the shell.
 */
static bool vmn_shell(VM * vm, VMArg * arg, int argc) {

  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* check 1st arg is string */
  if(!vmarg_is_string(arg[0])) {
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);

    /* this function does not return a value */
    return false;
  }

  system(vmarg_string(arg[0]));
  return false;
}

/**
 * VMNative: is_boolean( value )
 * Accepts one arguments. Accepts a single parameter of any type
 */
static bool vmn_is_boolean(VM * vm, VMArg * arg, int argc) {
  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* push result of check*/
  vmarg_push_boolean(vm, arg[0].type == TYPE_BOOLEAN);
  return true;
}

/**
 * VMNative: is_number( value )
 * Accepts one arguments. Accepts a single parameter of any type.
 */
static bool vmn_is_number(VM * vm, VMArg * arg, int argc) {
  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* push result of check*/
  vmarg_push_boolean(vm, arg[0].type == TYPE_NUMBER);
  return true;
}

/**
 * VMNative: is_null( value )
 * Accepts one arguments. Accepts a single parameter of any type.
 */
static bool vmn_is_null(VM * vm, VMArg * arg, int argc) {
  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* push result of check*/
  vmarg_push_boolean(vm, arg[0].type == TYPE_NULL);
  return true;
}

/**
 * VMNative: is_string( value )
 * Accepts one arguments. Accepts a single parameter of any type.
 */
static bool vmn_is_string(VM * vm, VMArg * arg, int argc) {
  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  /* push result of check*/
  vmarg_push_boolean(vm, vmarg_is_string(arg[0]));
  return true;
}

/**
 * VMNative: to_string( value )
 * Accepts one arguments. Accepts a single parameter of any type
 * and converts it to a string.
 */
static bool vmn_to_string(VM * vm, VMArg * arg, int argc) {
  char newString[LIBSYS_TOSTRING_MAXLEN];
  VMLibData * result;

  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  switch(arg[0].type) {
  case TYPE_NULL:
    strcpy(newString, "null");
    break;
  case TYPE_NUMBER:
    snprintf(newString, LIBSYS_TOSTRING_MAXLEN, 
	     "%f", vmarg_number(arg[0], NULL));
    break;
  case TYPE_BOOLEAN:
    strcpy(newString, vmarg_boolean(arg[0], NULL) ? "true":"false");
    break;
  case TYPE_LIBDATA :
    if(vmarg_is_string(arg[0])) {
      vmarg_push_libdata(vm, vmarg_libdata(arg[0]));
      return true;
    } else {
      strcpy(newString, "LIBDATA{");
      strcat(newString, vmarg_libdata(arg[0])->type);
      strcat(newString, "}");
    }
    break;
  default:
    printf("to_string: unknown type.");
    return false;
  }

  /* allocate response string */
  result = vmarg_new_string(newString, strlen(newString));
  if(result == NULL) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* push response */
  vmarg_push_libdata(vm, result);
  return true;
}

/**
 * VMNative: to_number( value )
 * Accepts one arguments. Accepts a single parameter of any type
 * and converts it to a number if possible.
 */
static bool vmn_to_number(VM * vm, VMArg * arg, int argc) {
  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  switch(arg[0].type) {
  case TYPE_NULL:
    vmarg_push_number(vm, 0.0d);
    break;
  case TYPE_NUMBER:
    vmarg_push_number(vm, vmarg_number(arg[0], NULL));
    break;
  case TYPE_BOOLEAN:
    vmarg_push_number(vm, vmarg_boolean(arg[0], NULL) ? 1.0d : 0.0d);
    break;
  case TYPE_LIBDATA:
  default:
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }
  return true;
}

/**
 * VMNative: to_boolean( value )
 * Accepts one arguments. Accepts a single parameter of any type
 * and converts it to a boolean if possible.
 */
static bool vmn_to_boolean(VM * vm, VMArg * arg, int argc) {

  /* check for correct number of arguments */
  if(argc != 1) {
    vm_set_err(vm, VMERR_INCORRECT_NUMARGS);

    /* this function does not return a value */
    return false;
  }

  switch(arg[0].type) {
  case TYPE_NULL:
    vmarg_push_boolean(vm, false);
    break;
  case TYPE_NUMBER:
    vmarg_push_boolean(vm, vmarg_number(arg[0], NULL) == 0 ? false : true);
    break;
  case TYPE_BOOLEAN:
    vmarg_push_boolean(vm, vmarg_boolean(arg[0], NULL));
    break;
  case TYPE_LIBDATA:
    if(vmarg_is_string(arg[0])) {
      if(strcmp(vmarg_string(arg[0]), "true") == 0) {
	vmarg_push_boolean(vm, true);
      } else {
	/* push false if anything but true..this is by design */
	vmarg_push_boolean(vm, false);
      }
    } else {
      /* this is an object (not a string) that is not null, push true */
      vmarg_push_boolean(vm, true);
    }
    break;
  default:
    vm_set_err(vm, VMERR_INVALID_TYPE_ARGUMENT);
    return false;
  }
  return true;
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
     || !vm_reg_callback(gunderscript_vm(gunderscript), "file_exists", 11, vmn_file_exists)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "file_open", 9, vmn_file_open)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "file_open_read", 14, vmn_file_open_read)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "file_open_write", 15, vmn_file_open_write)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "file_close", 10, vmn_file_close)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "file_read_char", 14, vmn_file_read_char)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "file_write_char", 15, vmn_file_write_char)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "is_boolean", 10, vmn_is_boolean)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "is_number", 9, vmn_is_number)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "is_null", 7, vmn_is_null)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "is_string", 9, vmn_is_string)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "to_string", 9, vmn_to_string)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "to_number", 9, vmn_to_number)
     || !vm_reg_callback(gunderscript_vm(gunderscript), "to_boolean", 10, vmn_to_boolean)) {
    return false;
  }

  return true;
}
