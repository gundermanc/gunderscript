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
 * numArgs: the number of items to pop off of stack to use as arguments.
 */
bool vm_exec(VM * vm, char * byteCode, 
	     size_t byteCodeLen, int startIndex, int numVarArgs) {

  assert(vm != NULL);
  assert(startIndex >= 0);
  assert(startIndex < byteCodeLen);
  assert(numVarArgs >= 0);

  vm->index = startIndex;

  /* push new frame with selected number of arguments and vars. */
  if(!frmstk_push(vm->frmStk, -1, numVarArgs)) {
     vm_set_err(vm, VMERR_STACK_OVERFLOW);
     return false;
  }

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
    case OP_AND:
    case OP_OR:
      if(!op_boolean_logic(vm, byteCode, byteCodeLen,
			     &vm->index, byteCode[vm->index])) {
	return false;
      }
      break;
    case OP_GOTO:
      if(!op_goto(vm, byteCode, byteCodeLen,
			     &vm->index)) {
	return false;
      }
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
    case OP_NULL_PUSH:
      if(!op_null_push(vm, byteCode, byteCodeLen, &vm->index)) {
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
 * Gets a string representation of the vm error. NOTE: these strings are
 * not dynamically allocated, but are constants that cannot be freed, and die
 * when this module terminates.
 * lexer: an instance of lexer.
 * err: the error to translate to text.
 * returns: the text form of this error.
 */
char * vm_err_to_string(VMErr err) {
  return vmErrorMessages[err];
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
    while(typestk_pop(vm->opStk, &value, sizeof(void*), &type));

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
 * Converts a VMArg to a libdata pointer.
 * arg: The arg to convert/
 * returns: a char*, or NULL if the arg is not a string.
 */
VMLibData * vmarg_libdata(VMArg arg) {

  if(arg.type == TYPE_LIBDATA) {
    VMLibData * argument;
    memcpy(&argument, arg.data, sizeof(VMLibData*));
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

/**
 * If this argument is a string, gets it.
 * arg: The arguement to unbox.
 * returns: the string if it is one, or null if not a string.
 */
char * vmarg_string(VMArg arg) {

  /* make sure its a string */
  if(!vmarg_is_string(arg)) {
    return NULL;
  }

  return vmlibdata_data((VMLibData*)vmarg_libdata(arg));
}

/**
 * Creates a new string encased in a VMLibData struct, ready to be
 * pushed to the stack as a native function return value.
 * string: the text for the string.
 * stringLen: the length of the new string.
 * returns: a new VMLibData struct, or NULL if the malloc fails.
 */
VMLibData * vmarg_new_string(char * string, size_t stringLen) {
  VMLibData * result;
  char * newString = calloc(stringLen + 1, sizeof(char));
  if(newString == NULL) {
    return NULL;
  }
  strncpy(newString, string, stringLen);

  result = vmlibdata_new(GXS_STRING_TYPE, GXS_STRING_TYPE_LEN, 
			 string_cleanup, newString);

  return result;
}

/**
 * Checks to see if the current vmarg is a string.
 * arg: a vm arg.
 * returns: true if a string, false if not.
 */
bool vmarg_is_string(VMArg arg) {
  if(arg.type == TYPE_LIBDATA
     && vmlibdata_is_type(vmarg_libdata(arg), GXS_STRING_TYPE, GXS_STRING_TYPE_LEN)) {
    return true;
  }

  return false;
}

/**
 * Pushes a return value onto the stack.
 * vm: an instance of VM.
 * data: pointer to a VMLibData struct.
 * returns: true if success, false if fails.
 */
bool vmarg_push_libdata(VM * vm, VMLibData * data) {
  vmlibdata_inc_refcount(data);
  vmlibdata_inc_refcount(data);
  return typestk_push(vm->opStk, &data, sizeof(VMLibData*), TYPE_LIBDATA);
}

/**
 * Pushes a return value onto the stack.
 * vm: an instance of VM.
 * value: value to push.
 * returns: true if success, false if fails.
 */
bool vmarg_push_number(VM * vm, double value) {
  return typestk_push(vm->opStk, &value, sizeof(double), TYPE_NUMBER);
}

/**
 * Pushes a return value onto the stack.
 * vm: an instance of VM.
 * value: value to push.
 * returns: true if success, false if fails.
 */
bool vmarg_push_boolean(VM * vm, bool value) {
  return typestk_push(vm->opStk, &value, sizeof(bool), TYPE_BOOLEAN);
}



/**
 * Creates a new VMLibData structure instance.
 * type: a string that specifies the type of this VMLibData struct. This string
 * is limited to VM_LIBDATA_TYPELEN in length.
 * typeLen: the length of the type string. Cannot be more than 
 * VM_LIBDATA_TYPELEN.
 * cleanupCallback: a function that will free any memory allocated by the lib
 * implementing this type when the object goes out of scope.
 * libData: a pointer allocated by the library implementor. This pointer can be
 * used to store data required to implement the desired functionality.
 * return: a new instance, or NULL if the malloc fails. NOTE: assert failure if
 * type is longer than VM_LIBDATA_TYPELEN.
 */

VMLibData * vmlibdata_new(char * type, size_t typeLen,
			  VMLibDataCleanupCallback cleanupCallback, void * libData) {
  assert(!(typeLen > VM_LIBDATA_TYPELEN));
  assert(type != NULL);

  VMLibData * data = calloc(1, sizeof(VMLibData));

  if(data == NULL) {
    return NULL;
  }

  strncpy(data->type, type, typeLen);
  data->libData = libData;
  data->cleanupCallback = cleanupCallback;
  data->refCount = 0;

  return data;
}

/**
 * Gets the pointer to the data associated with this type. For example, in the
 * string type, the string is stored at this pointer.
 * data: an instance of VMLibData.
 * return: the data pointer.
 */
void * vmlibdata_data(VMLibData * data) {
  assert(data != NULL);
  return data->libData;
}


/**
 * Used by the VM to track usage of an object, this increments the internal
 * reference counter for this VMLibData.
 * data: an instance.
 */
void vmlibdata_inc_refcount(VMLibData * data) {
  assert(data != NULL);
  data->refCount++;
}

/**
 * Used by the VM to track usage of an object, this decrements the internal
 * reference counter for this VMLibData.
 * data: an instance.
 */
void vmlibdata_dec_refcount(VMLibData * data) {
  assert(data != NULL);
  /*assert(data-> refCount > 0);*/
  data->refCount--;
}

/**
 * Used by the VM to track usage of an object, checks the reference counter
 * for the specified object. If the reference count is 0, the VM automatically
 * destroys this object.
 * data: an instance.
 */
void vmlibdata_check_cleanup(VM * vm, VMLibData * data) {
  assert(vm != NULL);
  assert(data != NULL);
  if(data->refCount <= 0) {
    vmlibdata_free(vm, data);
  }
}

/**
 * Checks if data is the specified type.
 * data: an instance.
 * type: type to check against.
 * typeLen: the length of the type string. Strings, for example are
 * GXS_STRING_TYPE.
 * returns: true if data is of type, type and false if not.
 */
bool vmlibdata_is_type(VMLibData * data, char * type, size_t typeLen) {
  assert(data != NULL);
  if(typeLen > VM_LIBDATA_TYPELEN) {
    return false;
  }

  return strncmp(data->type, type, typeLen) == 0;
}


/**
 * Frees a VMLibData structure and calls its cleanup method.
 * vm: the VM instance.
 * data: an instance of VMLibData.
 */
void vmlibdata_free(VM * vm, VMLibData * data) {
  assert(vm != NULL);
  assert(data != NULL);

  if(data->cleanupCallback != NULL) {
    ((*data->cleanupCallback)(vm, data));
  }
  free(data);
}
