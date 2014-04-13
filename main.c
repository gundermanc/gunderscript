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
#include "libsys.h"
#include "ht.h"
#include <string.h>
#include "gunderscript.h"

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
  printf("Error compiling and executing bytecode.");
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
  int i = 0;

  /* process_arguments(argc, argv, &stackSize); */

  /* initialize gunderscript object */
  if(!gunderscript_new(&ginst, stackSize, callbacksSize)) {
    print_alloc_error();
    return 1;
  }

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
    if(!gunderscript_build(&ginst, fileContents, fileLen)) {
      print_compile_error(&ginst);
      print_build_fail();
      return 1;
    }
    free(fileContents);
  }

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
