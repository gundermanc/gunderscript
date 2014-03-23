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
};

typedef struct VMArg {
  char data[VM_VAR_SIZE];
  VarType type;
} VMArg;

typedef struct VM VM;

/* a library data type struct */
typedef struct {
  char type[10];            /* a type identifier */
  void * libData;           /* pointer to library data */
} VMLibData;

/* the function prototype for a library data type cleanup routine */
typedef bool (*VMLibDataCleanupCallback) (VM * vm, VMLibData * data);

/* the function prototype for a native VM function */
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


VM * vm_new(size_t stackSize, int callbacksSize);

bool vm_exec(VM * vm, char * byteCode,
	     size_t byteCodeLen, int startIndex, int numArgs);

bool vm_reg_callback(VM * vm, char * name, size_t nameLen, VMCallback callback);

VMCallback vm_callback_from_index(VM * vm, int index);

int vm_callback_index(VM * vm, char * name, size_t nameLen);

int vm_num_callbacks(VM * vm);

void vm_set_err(VM * vm, VMErr err);

VMErr vm_get_err(VM * vm);

char * vm_err_to_string(VMErr err);

void vm_free(VM * vm);

int vm_exit_index(VM * vm);

VarType vmarg_type(VMArg arg);

char * vmarg_string(VMArg arg);

double vmarg_number(VMArg arg, bool * success);

bool vmarg_boolean(VMArg arg, bool * success);



#endif /* VM__H__ */
