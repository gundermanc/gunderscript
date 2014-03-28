/**
 * compcommon.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Contains functions that are required by all components of the compiler.
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

#include "compcommon.h"
#include "lexer.h"
#include "langkeywords.h"
#include <assert.h>

/**
 * Gets the OP code associated with an operation from its string representation.
 * operator: the operator to get the OPCode from.
 * len: the number of characters in length that operator is.
 * returns: the OPCode, or -1 if the operator is unrecognized.
 */
OpCode operator_to_opcode(char * operator, size_t len) {

  /* TODO: improve running time complexity */
  if(tokens_equal(operator, len, LANG_OP_ADD, LANG_OP_ADD_LEN)) {
    return OP_ADD;
  } else if(tokens_equal(operator, len, LANG_OP_SUB, LANG_OP_SUB_LEN)) {
    return OP_SUB;
  } else if(tokens_equal(operator, len, LANG_OP_MUL, LANG_OP_MUL_LEN)) {
    return OP_MUL;
  } else if(tokens_equal(operator, len, LANG_OP_DIV, LANG_OP_DIV_LEN)) {
    return OP_DIV;
  } else if(tokens_equal(operator, len, LANG_OP_EQUALS, LANG_OP_EQUALS_LEN)) {
    return OP_EQUALS;
  } else if(tokens_equal(operator, len, LANG_OP_NOT_EQUALS, LANG_OP_NOT_EQUALS_LEN)) {
    return OP_NOT_EQUALS;
  } else if(tokens_equal(operator, len, LANG_OP_LT, LANG_OP_LT_LEN)) {
    return OP_LT;
  } else if(tokens_equal(operator, len, LANG_OP_GT, LANG_OP_GT_LEN)) {
    return OP_GT;
  } else if(tokens_equal(operator, len, LANG_OP_LTE, LANG_OP_LTE_LEN)) {
    return OP_LTE;
  } else if(tokens_equal(operator, len, LANG_OP_GTE, LANG_OP_GTE_LEN)) {
    return OP_GTE;
  } else if(tokens_equal(operator, len, LANG_OP_AND, LANG_OP_AND_LEN)) {
    return OP_AND;
  } else if(tokens_equal(operator, len, LANG_OP_OR, LANG_OP_OR_LEN)) {
    return OP_OR;
  } else if(tokens_equal(operator, len, LANG_OP_MOD, LANG_OP_MOD_LEN)) {
    return OP_MOD;
  }

  /* unknown operator */
  return -1;
}

/**
 * Checks if two strings, "tokens", are equal. To be equal, the must have the
 * same text up to the provided lengths.
 * token1: the first token to compare.
 * num1: the number of characters of the first token to compare.
 * token2: the second token to compare.
 * num2: the number of characters of token 2 to compare.
 * returns: true if the tokens are equal and false if they are different
 * or token1 or token2 is NULL.
 */
bool tokens_equal(char * token1, size_t num1,
			 char * token2, size_t num2) {

  if((token1 != NULL && token2 != NULL) && (num1 == num2)) {
    return strncmp(token1, token2, num1) == 0;
  }
  return false;
}

/**
 * Gets the top symbol table from the symbol table stack and returns a pointer,
 * without actually removing the table from the stack. This function is useful
 * for getting a reference to the current frame's symbol table so that you can
 * look up its symbols.
 * c: an instance of Compiler.
 * returns: a pointer to the top symbol table hashtable, or NULL if the stack
 * is empty.
 */
HT * symtblstk_peek(Compiler * c, int offset) {
  DSValue value;

  /* check that peek was success */
  if(!stk_peek_offset(c->symTableStk, offset, &value)) {
    return NULL;
  }

  return value.pointerVal;
}

/**
 * Place holder for a function that gets the precedence of an operator.
 * operator: the operator to check for precedence.
 * operatorLen: the number of characters to read from operator as the operator.
 * TODO: possibly reimplement using a hash set for multicharacter operators.
 * returns: an integer that represents an operator's precedence. Higher is
 * greater. Returns 1 if unknown operator.
 */
int operator_precedence(char * operator, size_t operatorLen) {

  if(operatorLen == 1) {
    switch(operator[0]) {
    case '*':
    case '/':
    case '%':
      return 5;
    case '+':
    case '-':
      return 4;
    case '<':
    case '>':
      return 3;
    }
  } else {
    if(tokens_equal(operator, operatorLen, LANG_OP_EQUALS, LANG_OP_EQUALS_LEN)
       || (tokens_equal(operator, operatorLen, LANG_OP_NOT_EQUALS, LANG_OP_NOT_EQUALS_LEN))) {
      return 2; /* ==  or != */
    } else if(tokens_equal(operator, operatorLen, LANG_OP_LTE, LANG_OP_LTE_LEN)
	      || tokens_equal(operator, operatorLen, LANG_OP_GTE, LANG_OP_GTE_LEN)) {
      return 3; /* <= or >=*/
    } else if(tokens_equal(operator, operatorLen, LANG_OP_AND, LANG_OP_AND_LEN)
	      || tokens_equal(operator, operatorLen, LANG_OP_OR, LANG_OP_OR_LEN)) {
      return 1; /* && or ||*/
    }
  }

  return 1;
}

/**
 * Gets the precedence of the operator at the top of the opStk. This function is
 * used by the straight code parser to get the precedence of the last operator
 * that was encountered.
 * stk: the operator stack...a stack of pointers to strings containing operators
 * lenStk: a stack of longs that contain the lengths of the operator strings.
 * returns: the precedence of the top stack operator, or 0 if the operator stack
 * is empty.
 */
int topstack_precedence(TypeStk * stk, Stk * lenStk) {

  DSValue value;
  char * token;
  size_t len;

  /* get the token from the operator stack */
  if(!typestk_peek(stk, &token, sizeof(char*), NULL)) {
    return 0;
  }

  /* get the token length from the operator length stack */
  if(!stk_peek(lenStk, &value)) {
    return 0;
  }
  len = value.longVal;

  /* return the precedence of the operator */
  return operator_precedence(token, len);
}

int topstack_type(TypeStk * stk, Stk * lenStk) {

  DSValue value;
  VarType type;
  char * token;

  /* get the token from the operator stack */
  if(!typestk_peek(stk, &token, sizeof(char*), &type)) {
    return 0;
  }

  /* get the token length from the operator length stack */
  if(!stk_peek(lenStk, &value)) {
    return 0;
  }

  /* return the precedence of the operator */
  return (int)type;
}


/**
 * Pushes a new symbol table onto the stack of symbol tables. The symbol table
 * is a structure that contains records of all variables and their respective
 * locations in the stack in the VM. The symbol table's position in the stack
 * represents the VM stack frame's position in the stack as well.
 * c: an instance of Compiler that will receive the new frame.
 * returns: true if success, false if an allocation failure occurs.
 */
bool symtblstk_push(Compiler * c) {

  assert(c != NULL);

  HT * symTbl = ht_new(COMPILER_INITIAL_HTSIZE, COMPILER_HTBLOCKSIZE, COMPILER_HTLOADFACTOR);

  /* check that hashtable was successfully allocated */
  if(symTbl == NULL) {
    return false;
  }

  if(!stk_push_pointer(c->symTableStk, symTbl)) {
    return false;
  }

  return true;
}

/**
 * Gets the top symbol table from the symbol table stack and returns a pointer,
 * and removes the table from the stack. This function does NOT free the table.
 * You must free the table manually when finished with symtbl_free(), or the memory
 * will leak.
 * c: an instance of Compiler.
 * returns: a pointer to the symbol table formerly at the top of the stack, or
 * NULL if the stack is empty.
 */
HT * symtblstk_pop(Compiler * c) {
  DSValue value;

  /* check that peek was success */
  if(!stk_pop(c->symTableStk, &value)) {
    return NULL;
  }

  return value.pointerVal;
}
