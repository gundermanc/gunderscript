/**
 * main.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The Gunderscript executable application. This application can be used
 * as a complete standalone Gunderscript environment.
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

#include "lexer.h"
#include "frmstk.h"
#include "vm.h"
#include "vmdefs.h"
#include "typestk.h"
#include "compiler.h"
#include "ht.h"
#include <string.h>
#include "gunderscript.h"

static void print_help() {
  printf("Gunderscript Scripting Environment\n");
  printf("By: Christian Gunderman\n\n");
  printf("Usage: gunderscript [entrypoint] [scripts]\n");
  /*printf("  -s [stackSize]         : sets the size of the stack in bytes\n");*/
}

static void print_alloc_error() {
  printf("Error allocating memory for Gunderscript object.");
}

static void print_build_fail() {
  printf("Error building script.");
}

static void print_exec_fail() {
  printf("Error executing compiled bytecode.");
}

/*static void process_arguments(int argc, char * argv[], size_t * stackSize) {

  int i = 0;

  for(i = 0; i < argc; i++) {
    if(argv[i][0] == '-' && strlen(argv[i]) == 2) {
      switch(argv[i][1]) {
      case 's':
	if(i < argc) {
	  return;
	}
	*stackSize = strtol(argv[i+1], NULL, 
      }
    } else {
      print_help();
    }
  }

  }*/


static char * load_file(char * file, size_t * size) {
  FILE * fp;
  long lSize;
  char * buffer;

  fp = fopen (file, "rb" );
  if(!fp) {
    perror(file);
    exit(1);
  }

  fseek(fp ,0L ,SEEK_END);
  lSize = ftell(fp);
  rewind(fp);

  /* allocate memory for entire content */
  buffer = calloc(1, lSize+1);
  if(!buffer) {
    fclose(fp);
    fputs("memory alloc fails",stderr);
    exit(1);
  }

  /* copy the file into the buffer */
  if(1 != fread( buffer , lSize, 1 , fp) ) {
    fclose(fp);
    free(buffer);
    fputs("entire read fails",stderr);
    exit(1);
  }

  *size = lSize;
  fclose(fp);
  return buffer;
}

static void print_compile_error(Gunderscript * ginst) {
  printf("Compiler Error Number: %i\n", gunderscript_build_err(ginst));
  printf("Detected around Line Number: %i\n", gunderscript_err_line(ginst));
  printf("Error: %s\n", gunderscript_err_message(ginst));
}

static void print_exec_error(Gunderscript * ginst) {
  printf("VM Error: %i\n", gunderscript_function_err(ginst));
}

/* vm native function */
static bool vmn_print(VM * vm, VMArg * arg, int argc) {
  int i = 0;

  for(i = 0; i < argc; i++) {
    switch(vmarg_type(arg[i])) {
    case TYPE_STRING:
      printf("%s", vmarg_string(arg[i]));
      break;
    case TYPE_NUMBER:
      printf("%f", vmarg_number(arg[i], NULL));
      break;
    case TYPE_BOOLEAN:
      printf("%s", vmarg_boolean(arg[i], NULL) ? "true":"false");
      break;
    default:
      printf("DEBUG: Unrecognised type.");
    }
  }

  /* this function does not return a value */
  return false;
}

int main(int argc, char * argv[]) {
  Gunderscript ginst;
  size_t stackSize = 100000;
  int callbacksSize = 10;
  int i = 0;

  /* process_arguments(argc, argv, &stackSize); */

  /* initialize gunderscript object */
  if(!gunderscript_new(&ginst, stackSize, callbacksSize)) {
    print_alloc_error();
    return 1;
  }

  /* register the native print function */
  vm_reg_callback(gunderscript_vm(&ginst), "print", 5, vmn_print);

  /* check for proper number of arguments */
  if(argc < 2) {
    print_help();
    gunderscript_free(&ginst);
    return 1;
  }

  /* compile scripts one-by-one */
  for(i = 2; i < argc; i++) {
    size_t fileLen = 0;
    char * fileContents = load_file(argv[i], &fileLen);
    printf("File Length: %i chars\n", (int)fileLen);
    if(!gunderscript_build(&ginst, fileContents, fileLen)) {
      print_compile_error(&ginst);
      print_build_fail();
      return 1;
    }
    free(fileContents);
  }

  printf("Script output:\n\n");

  /* execute the desired entry point */
  if(!gunderscript_function(&ginst, argv[1], strlen(argv[1]))) {
    print_exec_error(&ginst);
    print_exec_fail();
    return 1;
  }

  printf("\n\n");

  gunderscript_free(&ginst);

  return 0;
}
