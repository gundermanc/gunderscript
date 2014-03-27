/**
 * gunderscript.c
 * (C) 2014 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The Gunderscript library wrapper functions. Using these functions,
 * you can interact with a complete Gunderscript instance without
 * having to manage the individual components.
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

bool gunderscript_new(Gunderscript * instance, size_t stackSize,
		      int callbacksSize) {
  assert(instance != NULL);
  assert(stackSize > 0);
  assert(callbacksSize > 0);

  /* allocate virtual machine */
  instance->vm = vm_new(stackSize, callbacksSize);
  if(instance->vm == NULL) {
    return false;
  }

  /* initialize system libraries */
  if(!libsys_install(instance)
     || !libmath_install(instance)
     || !libstr_install(instance)) {
    vm_free(instance->vm);
    return false;
  }

  /* allocate compiler instance */
  instance->compiler = compiler_new(instance->vm);
  if(instance->compiler == NULL) {
    vm_free(instance->vm);
    return false;
  }

  return true;
}

Compiler * gunderscript_compiler(Gunderscript * instance) {
  assert(instance != NULL);
  return instance->compiler;
}

VM * gunderscript_vm(Gunderscript * instance) {
  assert(instance != NULL);
  return instance->vm;
}

bool gunderscript_build(Gunderscript * instance, char * input, size_t inputLen) {
  assert(instance != NULL);
  assert(input != NULL);
  assert(inputLen > 0);
  return compiler_build(instance->compiler, input, inputLen);
}

bool gunderscript_build_err(Gunderscript * instance) {
  assert(instance != NULL);
  return compiler_get_err(instance->compiler);
}

char * gunderscript_err_message(Gunderscript * instance) {
  assert(instance != NULL);

  if(compiler_get_err(instance->compiler) != COMPILERERR_SUCCESS) {
    if(compiler_get_err(instance->compiler) == COMPILERERR_LEXER_ERR) {
      return lexer_err_to_string(compiler_lex_err(instance->compiler));
    } else {
      return compiler_err_to_string(instance->compiler,
				  compiler_get_err(instance->compiler));
    }
  } else {
    return vm_err_to_string(vm_get_err(instance->vm));
  }
}

int gunderscript_err_line(Gunderscript * instance) {
  assert(instance != NULL);
  return compiler_err_line(instance->compiler);
}

bool gunderscript_function(Gunderscript * instance, char * entryPoint,
			   size_t entryPointLen) {
  CompilerFunc * function;

  /* get compiler function definitions */
  function = compiler_function(instance->compiler, entryPoint, entryPointLen);
  if(function == NULL) {
    return false;
  }
  
  /* execute function in the virtual machine */
  if(!vm_exec(instance->vm, compiler_bytecode(instance->compiler), 
	      compiler_bytecode_size(instance->compiler), function->index,
	      function->numArgs + function->numVars)) {
    return false;
  }

  return true;
}

bool gunderscript_function_err(Gunderscript * instance) {
  assert(instance != NULL);
  return vm_get_err(instance->vm);
}

void gunderscript_free(Gunderscript * instance) {
  assert(instance != NULL);
  compiler_free(instance->compiler);
  vm_free(instance->vm);
}
