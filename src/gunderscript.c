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
#include "libsys.h"
#include "libmath.h"
#include "libstr.h"
#include "libarray.h"

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
  instance->err = GUNDERSCRIPTERR_SUCCESS;
  if(instance->vm == NULL) {
    return false;
  }

  /* initialize system libraries */
  if(!libsys_install(instance)
     || !libmath_install(instance)
     || !libstr_install(instance)
     || !libarray_install(instance)) {
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
  bool result = compiler_build(instance->compiler, input, inputLen);

  if(!result) {
    instance->err = GUNDERSCRIPTERR_BUILDERR;
  }
  return result;
}

/**
 * Same as gunderscript_build, but builds a file.
 * instance: an instance of Gunderscript.
 * fileName: the file to build.
 * returns: true upon success, and false upon failure.
 */
bool gunderscript_build_file(Gunderscript * instance, char * fileName) {
  bool result = compiler_build_file(instance->compiler, fileName);
  
  if(!result) {
    instance->err = GUNDERSCRIPTERR_BUILDERR;
  }
  return result;
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
 * Run after gunderscript_build_file() to export the compiled bytecode to an
 * external bytecode file.
 * instance: a Gunderscript object.
 * fileName: The name of the file to export to. Caution: file will be overwritten
 * returns: true upon success, or false if file cannot be opened, 
 * no code has been built, or there was an error building code.
 */
bool gunderscript_export_bytecode(Gunderscript * instance, char * fileName) {
  FILE * outFile = fopen(fileName, "w");
  GSByteCodeHeader header;
  HTIter functionHTIter;
  char * byteCodeBuffer;

  /* check if file open fails */
  if(outFile == NULL) {
    instance->err = GUNDERSCRIPTERR_BAD_FILE_OPEN_WRITE;
    return false;
  }

  /* create header */
  strcpy(header.header, GS_BYTECODE_HEADER);
  strcpy(header.buildDate, __DATE__);
  header.byteCodeLen = vm_bytecode_size(instance->vm);
  header.numFunctions = ht_size(vm_functions(instance->vm));

  /* write header to file, return false on failure */
  if(fwrite(&header, sizeof(GSByteCodeHeader), 1, outFile) != 1) {
    instance->err = GUNDERSCRIPTERR_BAD_FILE_WRITE;
    return false;
  }
  
  ht_iter_get(vm_functions(instance->vm), &functionHTIter);

  /* write exported functions to file */
  while(ht_iter_has_next(&functionHTIter)) {
    DSValue value;
    char functionName[GS_MAX_FUNCTION_NAME_LEN];
    size_t functionNameLen;
    char outLen;

    /* get the next item from hashtable */
    ht_iter_next(&functionHTIter, functionName, GS_MAX_FUNCTION_NAME_LEN,
		 &value, &functionNameLen, false);

    /* check if the function should be exported. if so, write it to the file */
    if(((VMFunc*)value.pointerVal)->exported) {

      /* TODO: create an error handler case for this */
      assert(functionNameLen < GS_MAX_FUNCTION_NAME_LEN);
    
      /* write it's name and length to the file */
      outLen = functionNameLen;
      if(fwrite(&outLen, sizeof(char), 1, outFile) != 1 
	 || fwrite(functionName, sizeof(char), functionNameLen, outFile) 
	 != functionNameLen) {
	instance->err = GUNDERSCRIPTERR_BAD_FILE_WRITE;
	return false;
      }

      /* write its CompilerFunc to the file (stores function call information) */
      if(fwrite(value.pointerVal, sizeof(VMFunc), 1, outFile) != 1) {
	instance->err = GUNDERSCRIPTERR_BAD_FILE_WRITE;
	return false;
      }
    }
  }

  byteCodeBuffer = vm_bytecode(instance->vm);
  if(byteCodeBuffer == NULL) {
    instance->err = GUNDERSCRIPTERR_NO_SUCCESSFUL_BUILD;
    return false;
  }

  /* write bytecode */
  if(fwrite(byteCodeBuffer, 
	    vm_bytecode_size(instance->vm), 1, outFile) != 1) {
    instance->err = GUNDERSCRIPTERR_BAD_FILE_WRITE;
    return false;
  }

  fclose(outFile);

  return true;
}

bool gunderscript_import_bytecode(Gunderscript * instance, char * fileName) {
  DSValue value;
  FILE * inFile = fopen(fileName, "r");
  GSByteCodeHeader header;
  int i = 0;

  if(inFile == NULL) {
    instance->err = GUNDERSCRIPTERR_BAD_FILE_OPEN_READ;
    return false;
  }

  /* read header */
  if(fread(&header, sizeof(GSByteCodeHeader), 1, inFile) != 1) {
    instance->err = GUNDERSCRIPTERR_BAD_FILE_READ;
    fclose(inFile);
    return false;
  }

  /* check for header */
  if(strcmp(header.header, GS_BYTECODE_HEADER) != 0) {
    instance->err = GUNDERSCRIPTERR_NOT_BYTECODE_FILE;
    fclose(inFile);
    return false;
  }

  /* check that this build is the same as the one that created the file */
  if(strcmp(header.buildDate, GUNDERSCRIPT_BUILD_DATE) != 0) {
    instance->err = GUNDERSCRIPTERR_INCORRECT_RUNTIME_VERSION;
    fclose(inFile);
    return false;
  }

  /* check that the number of functions isn't negative or zero */
  if(header.numFunctions < 1) {
    instance->err = GUNDERSCRIPTERR_CORRUPTED_BYTECODE;
    return false;
  }

  /* import the exported functions definitions (script entry points) */
  for(i = 0; i < header.numFunctions; i++) {
    VMFunc * currentFunc = calloc(1, sizeof(VMFunc));
    char functionName[GS_MAX_FUNCTION_NAME_LEN];
    char functionNameLen;
    bool prevValue = false;

    /* check if VMFunc alloc failed */
    if(currentFunc == NULL) {
      instance->err = GUNDERSCRIPTERR_ALLOC_FAILED;
      fclose(inFile);
      return false;
    }

    /* read function name length */
    if(fread(&functionNameLen, sizeof(char), 1, inFile) != 1) {
      instance->err = GUNDERSCRIPTERR_BAD_FILE_READ;
      return false;
    }

    /* check function name length */
    if(functionNameLen > GS_MAX_FUNCTION_NAME_LEN) {
      instance->err = GUNDERSCRIPTERR_CORRUPTED_BYTECODE;
      fclose(inFile);
      return false;
    }

    /* read function name */
    if(fread(&functionName, sizeof(char), functionNameLen, inFile) 
       != functionNameLen) {
      instance->err = GUNDERSCRIPTERR_BAD_FILE_READ;
      fclose(inFile);
      return false;
    }
    
    /* read from file into new VMFunc */
    value.pointerVal = currentFunc;
    if(fread(currentFunc, sizeof(VMFunc), 1, inFile) != 1) {
      instance->err = GUNDERSCRIPTERR_BAD_FILE_READ;
      fclose(inFile);
      return false;
    }

    /* put functions into VM functions hashtable */
    if(!ht_put_raw_key(vm_functions(instance->vm), functionName, 
		       functionNameLen, &value, NULL, &prevValue)) {
      instance->err = GUNDERSCRIPTERR_ALLOC_FAILED;
      fclose(inFile);
      return false;
    }

    /* check for duplicate function names */
    if(prevValue) {
      instance->err = GUNDERSCRIPTERR_CORRUPTED_BYTECODE;
      fclose(inFile);
      return false;
    }
  }

  /* make space in buffer for the bytecode */
  if(!buffer_resize(vm_buffer(instance->vm), header.byteCodeLen)) {
    instance->err = GUNDERSCRIPTERR_ALLOC_FAILED;
    fclose(inFile);
    return false;
  }

  /* read raw bytecode from end of bytecode file */
  {
    int c, i;
    for(i = 0; (c = fgetc(inFile)) != -1; i++) {
      buffer_append_char(vm_buffer(instance->vm), (char)c);
    }

    if(i != (header.byteCodeLen)) {
      instance->err = GUNDERSCRIPTERR_BAD_FILE_READ;
      fclose(inFile);
      return false;
    }
  }

  fclose(inFile);
  return true;
}

GunderscriptErr gunderscript_get_err(Gunderscript * instance) {
  return instance->err;
}

static const char * err_to_string(GunderscriptErr err) {
  return gunderscriptErrorMessages[err];
}

/**
 * Gets a textual error message representing the last error, if there is one.
 * instance: an instance of Gunderscript.
 * returns: an error message, or NULL if there are no errors.
 */
const char * gunderscript_err_message(Gunderscript * instance) {
  assert(instance != NULL);

  if(gunderscript_get_err(instance) == GUNDERSCRIPTERR_BUILDERR) {
    if(compiler_get_err(instance->compiler) == COMPILERERR_LEXER_ERR) {
      return lexer_err_to_string(compiler_lex_err(instance->compiler));
    } else {
      return compiler_err_to_string(instance->compiler,
				  compiler_get_err(instance->compiler));
    }
  } else if(gunderscript_get_err(instance) == GUNDERSCRIPTERR_EXECERR) {
    return vm_err_to_string(vm_get_err(instance->vm));
  } else {
    return err_to_string(gunderscript_get_err(instance));
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
  VMFunc * function;

  /* get compiler function definitions */
  function = vm_function(instance->vm, entryPoint, entryPointLen);
  if(function == NULL) {
    return false;
  }
  
  /* execute function in the virtual machine */
  if(!vm_exec(instance->vm, vm_bytecode(instance->vm), 
	      vm_bytecode_size(instance->vm), function->index,
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
  if(instance->compiler != NULL) {
    compiler_free(instance->compiler);
  }

  if(instance->vm != NULL) {
    vm_free(instance->vm);
  }
}
