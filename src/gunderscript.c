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

/**
 * Creates a new instance of Gunderscript object with a compiler and 
 * a Virtual Machine.
 * instance: pointer to a Gunderscript object that will receive
 * the instance.
 * stackSize: the size for the VM stack in bytes. The VM does not
 * dynamically resize so this value is absolute for this instance.
 * callbacksSize: the number of callbacks slots. This number defines
 * how many native functions can be bound to this instance. Increase
 * this value if vm_reg_callback() fails, or if gunderscript_new()
 * always returns false.
 * returns: true if creation succeeds, and false if fails. Failure can
 * occur due to malloc failure or if callbacksSize is too small to
 * contain all of the standard libraries.
 */
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

/**
 * Gets the instance of compiler for this instance of Gunderscript,
 * or NULL if this instance does not have a Compiler.
 * instance: an instance of Gunderscript.
 * returns: an instance of Compiler.
 */
Compiler * gunderscript_compiler(Gunderscript * instance) {
  assert(instance != NULL);
  return instance->compiler;
}

/**
 * Gets the instance of Virtual Machine for this instance of Gunderscript.
 * instance: an instance of Gunderscript.
 * returns: an instance of VM.
 */
VM * gunderscript_vm(Gunderscript * instance) {
  assert(instance != NULL);
  return instance->vm;
}

/**
 * Builds a gunderscript and appends its OPCode to the buffer of OP data for
 * any previously built files. You can build multiple files as long as the 
 * dependencies come first.
 * instance: an instance of Gunderscript.
 * input: a pointer to a buffer of Gunderscript code.
 * inputLen: the length of the input.
 * returns: an instance of VM.
 */
bool gunderscript_build(Gunderscript * instance, char * input, size_t inputLen) {
  assert(instance != NULL);
  assert(input != NULL);
  assert(inputLen > 0);
  return compiler_build(instance->compiler, input, inputLen);
}

/**
 * Gets any compiler errors that may have occurred.
 * instance: an instance of Gunderscript.
 * returns: a CompilerErr, defined in compiler.h.
 */
CompilerErr gunderscript_build_err(Gunderscript * instance) {
  assert(instance != NULL);
  return compiler_get_err(instance->compiler);
}

/**
 * Gets a textual error message representing the last error, if there is one.
 * instance: an instance of Gunderscript.
 * returns: an error message, or NULL if there are no errors.
 */
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

/**
 * Gets the line where the last known compiler error occurred.
 * instance: an instance of Gunderscript.
 * returns: a line number.
 */
int gunderscript_err_line(Gunderscript * instance) {
  assert(instance != NULL);
  return compiler_err_line(instance->compiler);
}

/**
 * Runs the specified Gunderscript "exported" function.
 * instance: an instance of Gunderscript.
 * entryPoint: the name of an entryPoint function to run.
 * entryPointLen: the length of entryPoint in chars.
 * returns: true if a success, and false if an error occurs.
 */
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

/**
 * Gets the error that occurred during the last call to gunderscript_function.
 * instance: an instance of Gunderscript.
 * returns: a VMErr
 */
VMErr gunderscript_function_err(Gunderscript * instance) {
  assert(instance != NULL);
  return vm_get_err(instance->vm);
}

/**
 * Frees a Gunderscript object.
 * instance: an instance of Gunderscript.
 * returns: a VMErr
 */
void gunderscript_free(Gunderscript * instance) {
  assert(instance != NULL);
  compiler_free(instance->compiler);
  vm_free(instance->vm);
}
