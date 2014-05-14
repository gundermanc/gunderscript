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

#include <string.h>
#include "gunderscript.h"

#define GXSMAIN_BUILD_SCRIPT     "build-script"
#define GXSMAIN_RUN_SCRIPT       "run-script"
#define GXSMAIN_RUN_BYTECODE     "run-bytecode"

static void print_help() {
  printf("Gunderscript Scripting Environment ");
  
#ifdef _WIN64
  printf("- Win64 Build\n");
#elif _WIN32
  printf("- Win32 Build\n");
#elif __linux__
#ifdef __LP64__
  printf("- Linux64 Build\n");
#else
  printf("- Linux32 Build\n");
#endif /* defined(__LP64__) */
#else
  printf("- Unknown Platform;\n");
#endif /* defined(__linux__) */
  printf("(C) 2013-2014 Christian Gunderman\n");
  printf("http://github.com/gundermanc/gunderscript\n");
#if defined( __DATE__) && defined(__TIME__)
  printf("Built on %s at %s\n\n", __DATE__, __TIME__);
#else
  printf("Build date unavaiable; Not compiled with GCC;\n\n");
#endif /* defined(__linux__) */
  printf("Usage: gunderscript [operation] ... \n");
  printf("  Operations:\n");
  printf("    build-script [script.gxs] [outputfile.gxb] \n");
  printf("    run-script   [entrypoint] [script.gxs] \n");
  printf("    run-bytecode [entrypoint] [bytecode.gxb] \n");
  /*printf("  -s [stackSize]         : sets the size of the stack in bytes\n");*/
}

static void print_alloc_error() {
  printf("Error allocating memory for Gunderscript object.");
}

static void print_build_fail() {
  printf("Error building script.");
}

static void print_exec_fail() {
  printf("Error compiling and executing bytecode.");
}

static void print_compile_error(Gunderscript * ginst) {
  printf("\n\nCompiler Error Number: %i\n", gunderscript_build_err(ginst));
  printf("Detected around Line Number: %i\n", gunderscript_err_line(ginst));
  printf("Compiler Error: %s\n", gunderscript_err_message(ginst));
}

static void print_exec_error(Gunderscript * ginst) {
  printf("\n\nVM Error: %i\n", gunderscript_function_err(ginst));
  printf("Virtual Machine Error: %s\n", gunderscript_err_message(ginst));
}

int main(int argc, char * argv[]) {
  Gunderscript ginst;
  size_t stackSize = 100000;
  int callbacksSize = 55;

  /* initialize gunderscript object */
  if(!gunderscript_new(&ginst, stackSize, callbacksSize)) {
    print_alloc_error();
    return 1;
  }

  /* check for proper number of arguments */
  if(argc != 4) {
    print_help();
    gunderscript_free(&ginst);
    return 1;
  }

  /* build script */
  if(strcmp(argv[1], GXSMAIN_BUILD_SCRIPT) == 0) {

    /* compile the input script */
    if(!gunderscript_build_file(&ginst, argv[2])) {
      print_compile_error(&ginst);
      print_build_fail();
      gunderscript_free(&ginst);
      return 1;
    }

    /* export the compiled code to a BIN file */
    if(!gunderscript_export_bytecode(&ginst, argv[3])) {
      print_compile_error(&ginst);
      print_build_fail();
      gunderscript_free(&ginst);
      return 1;
    }
    gunderscript_free(&ginst);
    return 0;

  } else if(strcmp(argv[1], GXSMAIN_RUN_SCRIPT) == 0) {

    /* compile the input script */
    if(!gunderscript_build_file(&ginst, argv[3])) {
      print_compile_error(&ginst);
      print_build_fail();
      gunderscript_free(&ginst);
      return 1;
    }

    /* execute the desired entry point */
    if(!gunderscript_function(&ginst, argv[2], strlen(argv[2]))) {
      print_exec_error(&ginst);
      print_exec_fail();
      gunderscript_free(&ginst);
      return 1;
    }
    gunderscript_free(&ginst);
    return 0;

  } else if(strcmp(argv[1], GXSMAIN_RUN_BYTECODE) == 0) {

    /* import the compiled code from a BIN file */
    if(!gunderscript_import_bytecode(&ginst, argv[3])) {
      print_compile_error(&ginst);
      print_build_fail();
      gunderscript_free(&ginst);
      return 1;
    }
    
    /* execute the desired entry point */
    if(!gunderscript_function(&ginst, argv[2], strlen(argv[2]))) {
      print_exec_error(&ginst);
      print_exec_fail();
      gunderscript_free(&ginst);
      return 1;
    }
    gunderscript_free(&ginst);
    return 0;

  } else {
    print_help();
  }
  gunderscript_free(&ginst);
  return 0;
}
