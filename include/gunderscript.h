/**
 * gunderscript.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See gunderscript.c for description.
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
#ifndef GUNDERSCRIPT__H__
#define GUNDERSCRIPT__H__

#include <assert.h>
#include <stdlib.h>
#include "compiler.h"
#include "vm.h"

/* define module function exports */
#if defined(_MSC_VER)
    #define GSAPI __declspec(dllexport)
#elif defined(_GCC)
    #define GSAPI __attribute__((visibility("default")))
#else
    #define GSAPI
#endif

/* first 3 bytes of all gunderscript bytecode files. <= 3 chars in length */
#define GS_BYTECODE_HEADER   "GXS"
#define GS_BYTECODE_HEADER_SIZE 4
#define GUNDERSCRIPT_BUILD_DATE     __DATE__
#define GS_BYTECODE_BUILDDATE_SIZE  35
#define GS_MAX_FUNCTION_NAME_LEN    80

/* error codes */
typedef enum {
  GUNDERSCRIPTERR_SUCCESS,
  GUNDERSCRIPTERR_BAD_FILE_OPEN_READ,
  GUNDERSCRIPTERR_BAD_FILE_OPEN_WRITE,
  GUNDERSCRIPTERR_BAD_FILE_WRITE,
  GUNDERSCRIPTERR_BAD_FILE_READ,
  GUNDERSCRIPTERR_NO_SUCCESSFUL_BUILD,
  GUNDERSCRIPTERR_NOT_BYTECODE_FILE,
  GUNDERSCRIPTERR_INCORRECT_RUNTIME_VERSION,
  GUNDERSCRIPTERR_CORRUPTED_BYTECODE,
  GUNDERSCRIPTERR_ALLOC_FAILED,
  GUNDERSCRIPTERR_BUILDERR,
  GUNDERSCRIPTERR_EXECERR,
} GunderscriptErr;

/* english translations of Gunderscript errors */
static const char * const gunderscriptErrorMessages [] = {
  "Success",
  "Unable to open file for reading",
  "Unable to open file for writing",
  "Error writing to file",
  "Error reading from file",
  "No successful build completed yet", /* thrown if try to export before build */
  "Not a byte code file",
  "Incorrect runtime version",
  "Corrupted bytecode",
  "Memory Allocation Failed",
  "Compiler Error",
  "VM Error",
};

typedef struct {
  /* stores unique build date/string that is used to check byte code
   * compatibility before it is run
   */
  char header[GS_BYTECODE_HEADER_SIZE];
  char buildDate[GS_BYTECODE_BUILDDATE_SIZE];
  int byteCodeLen;
  int numFunctions;
} GSByteCodeHeader;

/* stores an instance of a Gunderscript environment */
typedef struct Gunderscript {
  Compiler * compiler;
  VM * vm;
  GunderscriptErr err;
  char * byteCode;
  int byteCodeLen;
} Gunderscript;

GSAPI bool gunderscript_new_full(Gunderscript * instance, size_t stackSize,
		      int callbacksSize);

GSAPI bool gunderscript_new_vm(Gunderscript * instance, size_t stackSize,
		    int callbacksSize);

GSAPI Compiler * gunderscript_compiler(Gunderscript * instance);

GSAPI VM * gunderscript_vm(Gunderscript * instance);

GSAPI bool gunderscript_build(Gunderscript * instance, char * input, size_t inputLen);

GSAPI bool gunderscript_build_file(Gunderscript * instance, char * fileName);


GSAPI CompilerErr gunderscript_build_err(Gunderscript * instance);

GSAPI LexerErr gunderscript_lexer_err(Gunderscript * instance);

GSAPI const char * gunderscript_err_message(Gunderscript * instance);

GSAPI bool gunderscript_function(Gunderscript * instance, char * entryPoint,
			   size_t entryPointLen);

GSAPI VMErr gunderscript_function_err(Gunderscript * instance);

GSAPI int gunderscript_err_line(Gunderscript * instance);

GSAPI void gunderscript_free(Gunderscript * instance);

GSAPI GunderscriptErr gunderscript_get_err(Gunderscript * instance);

GSAPI bool gunderscript_export_bytecode(Gunderscript * instance, char * fileName);

GSAPI bool gunderscript_import_bytecode(Gunderscript * instance, char * fileName);

GSAPI const char * gunderscript_build_date();


/* GunderscriptSharp.dll use only, these APIs are not meant for public use */
GSAPI Gunderscript * gunderscript_alloc_empty();
GSAPI void gunderscript_free_empty(Gunderscript * instance);

#endif /* GUNDERSCRIPT__H__ */
