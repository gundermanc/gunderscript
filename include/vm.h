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
  VMERR_INVALID_NUMBER_OF_ARGUMENTS,   /* too many/few arguments to function */
} VMErr;

typedef struct VMArg {
  char data[VM_VAR_SIZE];
  VarType type;
} VMArg;

typedef struct VM VM;

/* the function prototype for a native VM function */
typedef bool (*VMCallback) (VM * vm, VMArg * arg, int argc);

struct VM {
  FrmStk * frmStk;
  TypeStk * opStk;
  VMCallback * callbacks;
  HT * callbacksHT;
  int callbacksSize;
  int numCallbacks;
  int index;
  VMErr err;
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

void vm_free(VM * vm);

int vm_exit_index(VM * vm);

VarType vmarg_type(VMArg arg);

char * vmarg_string(VMArg arg);

double vmarg_number(VMArg arg, bool * success);

bool vmarg_boolean(VMArg arg, bool * success);



#endif /* VM__H__ */
