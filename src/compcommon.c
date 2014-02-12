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
HT * symtblstk_peek(Compiler * c) {
  DSValue value;

  /* check that peek was success */
  if(!stk_peek(c->symTableStk, &value)) {
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
      return 2;
    }
  } else {

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
