/**
 * vm.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The basic intention for the VM is to simplify the overall implementation of
 * the programming language by having a unified byte code that can be compiled to
 * 
 * The VM consists of two stacks: an opStk and a frmStk. The opStk is a stack
 * that holds values that represent individual machine instructions and variables
 * such as booleans and doubles. The frmStk is a stack of stack frames that are
 * capable of allocating memory for each logical block (if, else, while, etc.
 * block). These frames hold a return address and all variables and arguments
 * to the field.
 * The OP code is in the form of one byte OP codes (vmdefs.h) followed by a
 * variable number of bytes containing parameters. For example, to add two
 * numbers, they are pushed to the stack with OP_NUM_PUSH followed by 8 bytes
 * of a double. This is done twice, once for each number. Finally, the last
 * byte is a OP_ADD byte that signals the VM to pop both values, add them,
 * and push the result.
 *
 * TODO: This file needs to be cleaned up and still needs some OP codes
 * implemented.
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

#include "vm.h"
#include "vmdefs.h"
#include "gsbool.h"
#include "ophandlers.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define VM_TRUE             1
#define VM_FALSE            0

/* the initial size of the op stack */
static const int opStkInitSize = 60;
/* the number of bytes in size the op stack increases in each expansion */
static const int opStkBlockSize = 60;

VM * vm_new(size_t stackSize) {

  assert(stackSize > 0);

  VM * vm = calloc(1, sizeof(VM));

  if(vm != NULL) {
    vm->frmStk = frmstk_new(stackSize);
    vm->opStk = typestk_new(opStkInitSize, opStkBlockSize);

    if(vm->frmStk != NULL && vm->opStk != NULL) {
      return vm;
    }
    vm_free(vm);
  }

  return NULL;
}

bool vm_exec(VM * vm, char * byteCode, 
	     size_t byteCodeLen, size_t startIndex) {
  int i = startIndex;

  assert(vm != NULL);
  assert(startIndex >= 0);
  assert(startIndex < byteCodeLen);

  while(i < byteCodeLen) {

    vm_set_err(vm, VMERR_SUCCESS);

    switch(byteCode[i]) {
    case OP_VAR_PUSH:
      if(!op_var_push(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_VAR_STOR:
      if(!op_var_stor(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_FRM_PUSH:
      if(!op_frame_push(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_FRM_POP:
      if(!op_frame_pop(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_ADD:
      if(!op_add(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_SUB:
      if(!op_sub(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_MUL:
      if(!op_mul(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_DIV:
      if(!op_div(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_MOD:
      if(!op_mod(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_LT:
      if(!op_lt(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_GT:
      if(!op_gt(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_LTE:
      if(!op_lte(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_GTE:
      if(!op_gte(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_GOTO:
      printf("Not yet implemented!");
      break;
    case OP_BOOL_PUSH:
      if(!op_bool_push(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_NUM_PUSH:
      if(!op_num_push(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_EQUALS:
      if(!op_equals(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_EXIT:
      printf("Not yet implemented!");
      break;
    case OP_STR_PUSH:
      if(!op_str_push(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_CALL_STR_N:
      printf("Not yet implemented!");
      break;
    case OP_CALL_PTR_N:
      if(!op_call_ptr_n(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_CALL_B:
      printf("Not yet implemented!");
      break;
    case OP_NOT:
      if(!op_not(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_TCOND_GOTO:
      if(!op_cond_goto(vm, byteCode, byteCodeLen, &i, false)) {
	return false;
      }
      break;
    case OP_FCOND_GOTO:
      if(!op_cond_goto(vm, byteCode, byteCodeLen, &i, true)) {
	return false;
      }
      break;
    case OP_NOT_EQUALS:
      if(!op_not_equals(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    case OP_POP:
      if(!op_pop(vm, byteCode, byteCodeLen, &i)) {
	return false;
      }
      break;
    default:
      printf("Invalid OpCode at Index: %i\n", i);
      vm_set_err(vm, VMERR_INVALID_OPCODE);
      return false;
    }
  }
  return true;
}

void vm_set_err(VM * vm, VMErr err) {
  assert(vm != NULL);

  vm->err = err;
}

VMErr vm_get_err(VM * vm) {
  assert(vm != NULL);

  return vm->err;
}

void vm_free(VM * vm) {
  if(vm->opStk != NULL) {
    void * value;
    VarType type;

    /* pop all items off and free strings */
    while(typestk_pop(vm->opStk, &value, sizeof(void*), &type)) {
      if(type == TYPE_STRING) {
	free(value);
      }
    }

    typestk_free(vm->opStk);
  }

  if(vm->frmStk != NULL) {
    /* TODO: Free strings in frmstk */
    frmstk_free(vm->frmStk);
  }

  free(vm);
}
