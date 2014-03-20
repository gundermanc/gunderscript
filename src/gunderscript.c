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

bool gunderscript_function(Gunderscript * instance, char * entryPoint,
			   size_t entryPointLen) {
  CompilerFunc * function;
  size_t byteCodeLen = compiler_bytecode_size(instance->compiler);
  char * byteCode = calloc(sizeof(char), byteCodeLen + 1);
  if(byteCode == NULL) {
    return false;
  }

  /* copy bytecode to buffer */
  if(!compiler_bytecode(instance->compiler, byteCode, byteCodeLen + 1)) {
    free(byteCode);
    return false;
  }

  /* get compiler function definitions */
  function = compiler_function(instance->compiler, entryPoint, entryPointLen);
  if(function == NULL) {
    free(byteCode);
    return false;
  }
  
  /* execute function in the virtual machine */
  if(!vm_exec(instance->vm, byteCode, byteCodeLen, 
	      function->index, function->numArgs + function->numVars)) {
    free(byteCode);
    return false;
  }

  free(byteCode);
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