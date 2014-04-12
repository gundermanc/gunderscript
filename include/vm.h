/**
 * vm.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * See vm.c for full description.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VM__H__
#define VM__H__

#include "frmstk.h"
#include "typestk.h"
#include "ht.h"

/* Virtual Machine error codes */
typedef enum {
  VMERR_SUCCESS,                      /* operation succeeded */
  VMERR_INVALID_OPCODE,               /* current byte is not an opcode */
  VMERR_STACK_OVERFLOW,               /* overflowed the frmStk */
  VMERR_STACK_EMPTY,                  /* not enough items left in opStk */
  VMERR_ALLOC_FAILED,                 /* an alloc failed */
  VMERR_UNEXPECTED_END_OF_OPCODES,    /* expected a param, but no bytes left */
  VMERR_INVALID_TYPE_IN_OPERATION,    /* item(s) on stack wrong type for oper. */
  VMERR_DIVIDE_BY_ZERO,               /* OP_DIV tried to divide by zero */
  VMERR_FRMSTK_EMPTY,                 /* no frames on the frame stack */
  VMERR_FRMSTK_VAR_ACCESS_FAILED,     /* can't alloc or invalid arg index */
  VMERR_INVALID_PARAM,                /* incorrect op code param */
  VMERR_INVALID_ADDR,                 /* address in goto is out of range */
  VMERR_CALLBACKS_BUFFER_FULL,        /* too many callbacks were registered */
  VMERR_CALLBACK_EXISTS,              /* a function with this name exists */
  VMERR_CALLBACK_NOT_EXIST,           /* invalid callback string, or index */
  VMERR_INCORRECT_NUMARGS,            /* incorrect number of arguments */
  VMERR_INVALID_TYPE_ARGUMENT,        /* argument is wrong type */
  VMERR_FILE_CLOSED,                  /* trying to read or write to closed file */
  VMERR_ARGUMENT_OUT_OF_RANGE,        /* index argument is out of range */
} VMErr;

/* english translations of vm errors */
static const char * const vmErrorMessages [] = {
  "Success",
  "Invalid op code",
  "Stack overflow",
  "Operand stack is empty",
  "Memory allocation failed",
  "Reached end of OP codes mid instruction.",
  "Invalid type in operation",
  "Divide by zero",
  "Frame stack empty",
  "Unable to read desired frame stack variable slot",
  "Invalid parameter in instruction",
  "Invalid memory address in goto",
  "Cannot register callback, callbacks buffer full",
  "A callback with this name already exists",
  "The callback does not exist",
  "Incorrect number of arguments to native function",
  "Argument to native function is invalid type",
  "Trying to read or write to a closed file.",
  "Argument to native function is out of allowable range",
};

typedef struct VMArg {
  char data[VM_VAR_SIZE];
  VarType type;
} VMArg;

typedef struct VM VM;


/**
 * The function prototype for a native VM function.
 * vm: an instance of VM.
 * arg: an array of arguments that is argc in size. Check an argument's type
 * with vmarg_type(). Once you know its type, unbox it with vmarg_*() functions.
 * argc: the number of arguments that this call received.
 * returns True if this function pushed a return value to the stack, and false
 * if not...vm automatically pushes a default return.
 */
typedef bool (*VMCallback) (VM * vm, VMArg * arg, int argc);

/* VM instance struct */
struct VM {
  FrmStk * frmStk;                /* the stack of stack frames */
  TypeStk * opStk;                /* the stack of operands */
  VMCallback * callbacks;         /* the array of native bound functions */
  HT * callbacksHT;               /* a pointer to the callbacks hashtable */
  int callbacksSize;              /* the size of the callbacks array */
  int numCallbacks;               /* the number of callbacks in array */
  int index;                      /* current execution index */
  VMErr err;                      /* VM error state */
};


typedef struct VMLibData VMLibData;

VM * vm_new(size_t stackSize, int callbacksSize);

bool vm_exec(VM * vm, char * byteCode,
	     size_t byteCodeLen, int startIndex, int numArgs);

bool vm_reg_callback(VM * vm, char * name, size_t nameLen, VMCallback callback);

VMCallback vm_callback_from_index(VM * vm, int index);

int vm_callback_index(VM * vm, char * name, size_t nameLen);

int vm_num_callbacks(VM * vm);

void vm_set_err(VM * vm, VMErr err);

VMErr vm_get_err(VM * vm);

const char * vm_err_to_string(VMErr err);

void vm_free(VM * vm);

int vm_exit_index(VM * vm);

void * vmarg_data(VMArg * arg);

VarType vmarg_type(VMArg arg);

double vmarg_number(VMArg arg, bool * success);

bool vmarg_boolean(VMArg arg, bool * success);

char * vmarg_string(VMArg arg);

VMLibData * vmarg_new_string(char * string, size_t stringLen);

bool vmarg_is_string(VMArg arg) ;

bool vmarg_push_libdata(VM * vm, VMLibData * data);

bool vmarg_push_number(VM * vm, double value);

bool vmarg_push_boolean(VM * vm, bool value);

bool vmarg_push_null(VM * vm);

/* define built in LIBDATA object types */
#define VM_LIBDATA_TYPELEN    10


/**
 * The function prototype for a library data type cleanup routine.
 * For each TYPE_LIBDATA type (strings and other "objects") are defined
 * using VmLibData structs that can be used as values by pushing their
 * pointers to the operand stack with the type TYPE_LIBDATA. When these types
 * go out of scope, this function is called (if defined for the VMLibData) to
 * destroy any dynamic memory associated with value.
 * vm: an instance of vm.
 * data: the VMLibData structure that is to be destroyed. If the library
 * creator used the data->libData pointer, now is the time to free its target.
 */
typedef void (*VMLibDataCleanupCallback) (VM * vm, VMLibData * data);

/* a library data type struct */
struct VMLibData {
  char type[VM_LIBDATA_TYPELEN];          /* a type identifier */
  void * libData;                         /* pointer to library data */
  int refCount;                           /* # refs to this object */
  VMLibDataCleanupCallback cleanupCallback;
};


VMLibData * vmlibdata_new(char * type, size_t typeLen,
			  VMLibDataCleanupCallback cleanupCallback, void * libData);

void * vmlibdata_data(VMLibData * data);

void vmlibdata_set_data(VMLibData * data, void * setData);

void vmlibdata_inc_refcount(VMLibData * data);

void vmlibdata_dec_refcount(VMLibData * data);

void vmlibdata_check_cleanup(VM * vm, VMLibData * data);

bool vmlibdata_is_type(VMLibData * data, char * type, size_t typeLen);

void vmlibdata_free(VM * vm, VMLibData * data);

bool vmarg_push_data(VM * vm, void * data, VarType type);

VMLibData * vmarg_libdata(VMArg arg);
#endif /* VM__H__ */
