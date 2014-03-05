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

  /* check for proper number of arguments */
  if(argc < 2) {
    print_help();
    gunderscript_free(&ginst);
    return;
  }

  /* compile scripts one-by-one */
  for(i = 2; i < argc; i++) {
    char * fileContents = 
    compiler_build(gunderscript_compiler(&ginst), 
  }

  return 0;
}
