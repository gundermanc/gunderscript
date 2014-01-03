/**
 * typestk.h
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * A stack that can store the types of data used in the scripting language
 * and differentiate between them using a type field.
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
#include "typestk.h"
#include <assert.h>

/**
 * Allocates a new stack object.
 * initialDepth: how many indicies deep do you want the stack to be.
 * blockSize: the number of spaces that will be added each time the
 * stack overflows. If zero, stack will not expand.
 * returns: A new stack object, or NULL if unable to allocate.
 */
TypeStk * typestk_new(int initialDepth, int blockSize) {

  assert(initialDepth >= 0);
  assert(blockSize >= 0);

  /* if given depth is at least 2 */
  if(initialDepth >= 0) {

    /* allocate stack object */
    TypeStk * newList = (TypeStk*)calloc(1, sizeof(TypeStk));
    if(newList != NULL) {
      newList->size = 0;
      newList->depth = initialDepth;
      newList->blockSize = blockSize;

      /* allocate mem for items */
      newList->stack = (TypeStkData*)calloc(newList->depth,
					     sizeof(TypeStkData));
      /* check for successful alloc */
      if(newList->stack != NULL) {
	return newList;
      } else {
	free(newList);
      }
    }
  }
  return NULL;
}

/**
 * Frees a stack and all items currently in it.
 * Note: if stack contains pointers, you will have to pop those items and free
 * them yourself to ensure that the dynamically allocated memory is properly
 * freed.
 *
 * stack: an instance of stack.
 */
void typestk_free(TypeStk * stack) {
  assert(stack != NULL);
  assert(stack->stack != NULL);

  free(stack->stack);
  free(stack);
}

/**
 * Resizes the stack buffer to the specified capacity.
 * stack: an instance of stack.
 * newSize: The number of items that can be stored in the new stack buffer.
 * returns: true if the operation succeeds and false if the memory allocation
 * fails.
 */
static bool resize_stack(TypeStk * stack, int newSize) {
  TypeStkData * newBuffer = calloc(newSize, sizeof(TypeStkData));
  
  /* copy data to new buffer, free old one, and swap pointers */
  if(newBuffer != NULL) {
    memcpy(newBuffer, stack->stack, sizeof(TypeStkData) * stack->size);
    free(stack->stack);
    stack->stack = newBuffer;
    stack->depth = newSize;

    return true;
  }

  return false;
}

/**
 * Checks the size of the given stack and expands it if it is full.
 * returns: true if the stack doesn't need to be resized or it is
 * resized successfully, and false if it is unable to be resized.
 */
static bool check_size(TypeStk * stack) {

  /* if no space remains in the stack, expand it by blockSize */
  if((stack->depth - stack->size) != 0) {
    return true;
  }

  if(resize_stack(stack, stack->size + stack->blockSize)) {
    return true;
  }

  return false;
}

/**
 * Pushes a value onto the stack.
 * stack: an instance of TypeStk.
 * data: the data to put in the stack. This data must be more than 0 bytes and
 * less than VM_VAR_SIZE bytes.
 * dataSize: the number of bytes to copy to the stack. This must be greater than
 * 0 and less than VM_VAR_SIZE.
 * type: a one byte value that can be used to specify the type of the value.
 * returns: true if the operation succeeds and false if stack is full or alloc
 * fails during stack growing.
 */
bool typestk_push(TypeStk * stack, void * data, size_t dataSize, VarType type) {

  assert(stack != NULL);
  assert(data != NULL);
  assert(dataSize > 0);

  /* if stack is allowed to be resized, check to make sure it is large enough */
  if(stack->blockSize != 0) {
    if(!check_size(stack)) {
      return false;
    }
  }

  /* make sure there is enough space in stack, and then push item */
  if((stack->size < stack->depth) 
     && dataSize > 0 && dataSize <= VM_VAR_SIZE) {
    stack->stack[stack->size].type = type;
    memcpy(stack->stack[stack->size].data, data, dataSize);
    stack->size++;
    return true;
  } else {

    /* stack is full */
    return false;
  }
}

/**
 * Gets the value at the top of the stack without popping it off.
 * stack: an instance of TypeStk.
 * value: a buffer that will recv. the value contained within. Can be
 * NULL if you only want the type.
 * valueSize: the number of bytes to copy from the stack slot into
 * the buffer. This value must be greater than 0 and less than or equal
 * to VM_VAR_SIZE. You can use multiple calls to get the type and use it
 * to determine the size if neccessary.
 * type: a buffer that will recv. the type of the data.
 * returns: true if the value was copied to the buffer successfully and
 * false if valueSize was 0, greater than VM_VARS_SIZE, or if the stack
 * is empty.
 */
bool typestk_peek(TypeStk * stack, void * value, 
		  size_t valueSize, VarType * type) {

  assert(stack != NULL);
  assert(stack->stack != NULL);

  /* if there are items in the stack and valueSize is proper, copy out values */
  if(stack->size > 0 && valueSize > 0 && valueSize <= VM_VAR_SIZE) {
    if(value != NULL) {
      assert(valueSize > 0);
      memcpy(value, &stack->stack[stack->size-1].data, sizeof(valueSize));
    }

    if(type != NULL) {
      *type = stack->stack[stack->size-1].type;
    }
    return true;
  }

  return false;
}

/**
 * Gets the value at the top of the stack and pops it off.
 * stack: an instance of TypeStk.
 * value: a buffer that will recv. the value contained within.
 * valueSize: the number of bytes to copy from the stack slot into
 * the buffer. This value must be greater than 0 and less than or equal
 * to VM_VAR_SIZE. You can use multiple calls to get the type and use it
 * to determine the size if neccessary.
 * type: a buffer that will recv. the type of the data.
 * returns: true if the value was copied to the buffer successfully and
 * false if valueSize was 0, greater than VM_VARS_SIZE, or if the stack
 * is empty.
 */
bool typestk_pop(TypeStk * stack, void * value,
		 size_t valueSize, VarType * type) {

  assert(stack != NULL);
  assert(value != NULL);
  assert(valueSize > 0);

  /* get the value at the top of the stack and remove it */
  if(typestk_peek(stack, value, valueSize, type)) {
    stack->size--;
    return true;
  }
  return false;
}

/**
 * Gets the number of items in the stack.
 * stack: an instance of stack.
 * returns: the number of items.
 */
int typestk_size(TypeStk * stack) {
  assert(stack != NULL);

  return stack->size;
}
