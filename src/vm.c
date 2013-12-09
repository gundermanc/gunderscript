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

#include "vm.h"
#include "vmdefs.h"
#include "gsbool.h"
#include "ophandlers.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

/* the initial size of the op stack */
static const int opStkInitSize = 60;
/* the number of bytes in size the op stack increases in each expansion */
static const int opStkBlockSize = 60;

/**
 * Initializes a VM with a preallocated maximum frame stack that is stackSize
 * bytes in size and can have up to callbacksSize callbacks registered to it.
 * stackSize: size of the frame stack in bytes.
 * callbacksSize: the maximum number of callbacks that may be registered.
 * returns: a new VM instance, or NULL if allocation fails.
 */
VM * vm_new(size_t stackSize, int callbacksSize) {

  assert(stackSize > 0);
  assert(callbacksSize > 0);

  VM * vm = calloc(1, sizeof(VM));

  if(vm == NULL) {
    return NULL;
  }

  vm->frmStk = frmstk_new(stackSize);
  if(vm->frmStk == NULL) {
    free(vm);
    return NULL;
  }

  vm->opStk = typestk_new(opStkInitSize, opStkBlockSize);
  if(vm->opStk == NULL) {
    frmstk_free(vm->frmStk);
    free(vm);
    return NULL;
  }

  vm->callbacksSize = callbacksSize;

  vm->callbacks = calloc(vm->callbacksSize, sizeof(VMCallback));
  if(vm->callbacks == NULL) {
    typestk_free(vm->opStk);
    frmstk_free(vm->frmStk);
    free(vm);
    return NULL;
  }

  vm->callbacksHT = ht_new(vm->callbacksSize, 10, 1.0);
  if(vm->callbacksHT == NULL) {
    free(vm->callbacks);
    typestk_free(vm->opStk);
    frmstk_free(vm->frmStk);
    free(vm);
    return NULL;
  }

  return vm;
}

/**
 * Registers a callback function to this VM instance.
 * vm: an instance of a VM.
 * name: the text representation of the VM.
 * nameLen: the length of the name, in characters.
 * callback: a function pointer to a VMCallback function that will be called
 * whenever name occurs in code.
 */
bool vm_reg_callback(VM * vm, char * name, size_t nameLen, VMCallback callback) {

  assert(vm != NULL);
  assert(name != NULL);
  assert(nameLen > 0);
  assert(callback != NULL);

  bool prevRegistered;
  DSValue newValue;
  DSValue oldValue;

  vm_set_err(vm, VMERR_SUCCESS);

  if(vm->numCallbacks >= vm->callbacksSize) {
    vm_set_err(vm, VMERR_CALLBACKS_BUFFER_FULL);
    return false;
  }

  newValue.intVal = vm->numCallbacks;
  vm->callbacks[vm->numCallbacks] = callback;

  if(!ht_put_raw_key(vm->callbacksHT, name, nameLen, 
		     &newValue, &oldValue, &prevRegistered)) {
    vm_set_err(vm, VMERR_ALLOC_FAILED);
    return false;
  }

  /* a function with this name was already registered, freak out */
  if(prevRegistered) {

    /* we changed the value, set it back to what it was */
    ht_put_raw_key(vm->callbacksHT, name, nameLen,
		   &oldValue, NULL, NULL);

    vm_set_err(vm, VMERR_CALLBACK_EXISTS);
    return false;
  }

  vm->numCallbacks++;
  return true;
}

/**
 * Gets a callback function's function pointer from its array index.
 * vm: an instance of VM.
 * index: the index of the function to call. You can find out a function's
 * index using the vm_callback_index() function.
 * returns: the function's function pointer, or NULL if the specified index does
 * not exist.
 */
VMCallback vm_callback_from_index(VM * vm, int index) {
  assert(vm != NULL);
  assert(index >= 0);

  /* handle index is out of range error case */
  if(index >= vm->numCallbacks) {
    vm_set_err(vm, VMERR_CALLBACK_NOT_EXIST);
    return NULL;
  }

  return vm->callbacks[index];
}

/**
 * Gets the index of a VM callback function from its name.
 * vm: an instance of VM.
 * name: the name of the function.
 * nameLen: the length of name, in chars.
 * returns: the index of the callback, > 0, or -1 if the operation
 * fails because the function does not exist.
 */
int vm_callback_index(VM * vm, char * name, size_t nameLen) {
  assert(vm != NULL);
  assert(name != NULL);
  assert(nameLen > 0);
  
  DSValue value;

  vm_set_err(vm, VMERR_SUCCESS);

  if(!ht_get_raw_key(vm->callbacksHT, name, nameLen, &value)) {
    vm_set_err(vm, VMERR_CALLBACK_NOT_EXIST);
    return -1;
  }

  return value.intVal;
}

/**
 * Gets the number of callbacks registered with the VM.
 * vm: an instance of VM.
 * returns: the number of callbacks registered.
 */
int vm_num_callbacks(VM * vm) {
  assert(vm != NULL);

  vm_set_err(vm, VMERR_SUCCESS);

  return vm->numCallbacks;
}

/**
 * Executes a VM bytecode. For more info on the bytecode format, see
 * ophandlers.c where the opcodes are described and implemented.
 * vm: an instance of VM.
 * byteCode: an array of chars that contain VM byte code.
 * byteCodeLen: the number of bytes to read from byteCode array.
 * startIndex: the index to start executing from. The entry point.
 */
bool vm_exec(VM * vm, char * byteCode, 
	     size_t byteCodeLen, int startIndex) {

  assert(vm != NULL);
  assert(startIndex >= 0);
  assert(startIndex < byteCodeLen);

  vm->index = startIndex;

  while(vm->index < byteCodeLen) {

    vm_set_err(vm, VMERR_SUCCESS);

    switch(byteCode[vm->index]) {
    case OP_VAR_PUSH:
      if(!op_var_push(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    case OP_VAR_STOR:
      if(!op_var_stor(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    case OP_FRM_PUSH:
      if(!op_frame_push(vm, byteCode, byteCodeLen, &vm->index, false)) {
	return false;
      }
      break;
    case OP_FRM_POP:
      if(!op_frame_pop(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    case OP_ADD:
      if(!op_add(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_MOD:
      if(!op_dual_operand_math(vm, byteCode, byteCodeLen, &vm->index,
			       byteCode[vm->index])) {
	return false;
      }
      break;
    case OP_LT:
    case OP_GT:
    case OP_LTE:
    case OP_GTE:
    case OP_EQUALS:
    case OP_NOT_EQUALS:
      if(!op_dual_comparison(vm, byteCode, byteCodeLen,
			     &vm->index, byteCode[vm->index])) {
	return false;
      }
      break;
    case OP_GOTO:
      printf("Not yet implemented!");
      break;
    case OP_BOOL_PUSH:
      if(!op_bool_push(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    case OP_NUM_PUSH:
      if(!op_num_push(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    
    case OP_EXIT:
      printf("Not yet implemented!");
      break;
    case OP_STR_PUSH:
      if(!op_str_push(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    case OP_CALL_STR_N:
      printf("Not yet implemented!");
      break;
    case OP_CALL_PTR_N:
      if(!op_call_ptr_n(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    case OP_CALL_B:
      if(!op_frame_push(vm, byteCode, byteCodeLen, &vm->index, true)) {
	return false;
      }
      break;
    case OP_NOT:
      if(!op_not(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    case OP_TCOND_GOTO:
      if(!op_cond_goto(vm, byteCode, byteCodeLen, &vm->index, false)) {
	return false;
      }
      break;
    case OP_FCOND_GOTO:
      if(!op_cond_goto(vm, byteCode, byteCodeLen, &vm->index, true)) {
	return false;
      }
      break;
    case OP_POP:
      if(!op_pop(vm, byteCode, byteCodeLen, &vm->index)) {
	return false;
      }
      break;
    default:
      printf("Invalid OpCode at Index: %i\n", vm->index);
      vm_set_err(vm, VMERR_INVALID_OPCODE);
      return false;
    }
  }
  return true;
}

/**
 * Sets the current error code in the VM.
 * vm: an instance of vm.
 * err: the error code.
 */
void vm_set_err(VM * vm, VMErr err) {
  assert(vm != NULL);

  vm->err = err;
}

/**
 * Gets the current error status of the VM. Call this after calling any public
 * VM function to get a diagnostic error code.
 * vm: the VM instance.
 * returns: the last error triggered. This will be VMERR_SUCCESS (0) if no error
 * occurred last call.
 */
VMErr vm_get_err(VM * vm) {
  assert(vm != NULL);

  return vm->err;
}

/**
 * Frees a VM instance
 * vm: a VM instance.
 */
void vm_free(VM * vm) {

  assert(vm != NULL);

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

  if(vm->callbacksHT != NULL) {
    ht_free(vm->callbacksHT);
  }

  if(vm->callbacks) {
    free(vm->callbacks);
  }

  free(vm);
}

/**
 * Gets the bytecode index at which the code exited. This function can be used
 * to get the approximate location at which an error occurred in the code. 
 * Please Note: this method returns the code execution index, which is the place
 * at which the VM last read bytes. These bytes are not neccessary VM opcodes,
 * but may in fact be parameters to another OP code.
 * vm: a virtual machine instance.
 * returns: the index in the bytecode at which the execution with vm_exec()
 * stopped.
 */
int vm_exit_index(VM * vm) {
  assert(vm != NULL);

  return vm->index;
}

/**
 * Gets the type of a VMArg.
 */
VarType vmarg_type(VMArg arg) {
  return arg.type;
}


/* TODO: do this without the pointless memcpy...
 * The following three functions would be better as MACROS
 */
/**
 * Converts a VMArg to a string.
 * arg: The arg to convert/
 * returns: a char*, or NULL if the arg is not a string.
 */
char * vmarg_string(VMArg arg) {

  if(arg.type == TYPE_STRING) {
    char * argument;
    memcpy(&argument, arg.data, sizeof(char*));
    return argument;
  }

  return NULL;
}

/* TODO: do this without the pointless memcpy */
/**
 * Converts an argument to a number.
 * arg: the argument to convert.
 * success: pointer to a boolean that will receive whether or not the operation
 * succeeded. It can be NULL. Operation fails if the argument is the wrong type.
 * returns: the number in the argument.
 */
double vmarg_number(VMArg arg, bool * success) {

  if(arg.type == TYPE_NUMBER) {
    double value;
    memcpy(&value, arg.data, sizeof(value));

    if(success != NULL) {
      *success = true;
    }

    return value;
  }

  if(success != NULL) {
      *success = false;
  }

  return 0;
}

/* TODO: do this without the pointless memcpy */
/**
 * Converts an argument to a boolean value.
 * arg: the argument to convert.
 * success: a pointer to a boolean that receives whether or not the operation
 * succeeded. It can be NULL. Operation fails if the argument is the wrong type.
 * returns: the boolean value.
 */
bool vmarg_boolean(VMArg arg, bool * success) {

  if(arg.type == TYPE_BOOLEAN) {
    bool value;
    memcpy(&value, arg.data, sizeof(bool));

    if(success != NULL) {
      *success = true;
    }

    return value;
  }

  if(success != NULL) {
      *success = false;
  }

  return false;
}
