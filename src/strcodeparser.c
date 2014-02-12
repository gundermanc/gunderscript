/**
 * strcodeparser.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * Implements the straight code parser for the compiler.
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

#include <assert.h>
#include "strcodeparser.h"
#include "langkeywords.h"
#include "lexer.h"

/**
 * Gets the OP code associated with an operation from its string representation.
 * operator: the operator to get the OPCode from.
 * len: the number of characters in length that operator is.
 * returns: the OPCode, or -1 if the operator is unrecognized.
 */
OpCode operator_to_opcode(char * operator, size_t len) {
  if(tokens_equal(operator, len, LANG_OP_ADD, LANG_OP_ADD_LEN)) {
    return OP_ADD;
  } else if(tokens_equal(operator, len, LANG_OP_SUB, LANG_OP_SUB_LEN)) {
    return OP_SUB;
  } else if(tokens_equal(operator, len, LANG_OP_MUL, LANG_OP_MUL_LEN)) {
    return OP_MUL;
  } else if(tokens_equal(operator, len, LANG_OP_DIV, LANG_OP_DIV_LEN)) {
    return OP_DIV;
  } 

  /* unknown operator */
  return -1;
}

/**
 * Writes all operators from the provided stack to the output buffer in the 
 * Compiler instance, making allowances for specific behaviors for function
 * tokens, variable references, and assignments.
 * c: an instance of Compiler.
 * opStk: the stack of operator strings.
 * opLenStk: the stack of operator string lengths, in longs.
 * parenthExpected: tells whether or not the calling function is
 * expecting a parenthesis. If it is and one is not encountered, the
 * function sets c->err = COMPILERERR_UNMATCHED_PARENTH and returns false.
 * returns: true if successful, and false if an unmatched parenthesis is
 * encountered.
 */
bool write_operators_from_stack(Compiler * c, TypeStk * opStk, 
				       Stk * opLenStk, bool parenthExpected,
				       bool popParenth) {
  bool topOperator = true;
  DSValue value;
  char * token = NULL;
  size_t len = 0;
  OpCode opCode;
  VarType type;

  /* while items remain */
  while(typestk_size(opStk) > 0) {
    printf("TYPESTK SIZE: %i\n", typestk_size(opStk));
    /* get token string */
    typestk_pop(opStk, &token, sizeof(char*), &type);

    /* get token length */
    stk_pop(opLenStk, &value);
    len = value.longVal;

    /* if there is an open parenthesis...  */
    if(tokens_equal(LANG_OPARENTH, LANG_OPARENTH_LEN, token, len)) {
      printf("OPARENTHDSFLSKFJLSF\n");
      printf("TYPESTK SIZE: %i\n", typestk_size(opStk));
      /* if top of stack is a parenthesis, it is a mismatch! */
      if(!parenthExpected) {
	printf("UMP 1;\n");
	c->err = COMPILERERR_UNMATCHED_PARENTH;
	return false;
      } else if(popParenth) {
	printf("POPPING OPARENTH:\n");
	typestk_peek(opStk, &token, sizeof(char*), &type);
	stk_peek(opLenStk, &value);
	len = value.longVal;

	/* there is a function name before the open parenthesis,
	 * this is a function call:
	 */
	if(type == LEXERTYPE_KEYVAR) {
	  int callbackIndex = -1;
	  typestk_pop(opStk, &token, sizeof(char*), &type);
	  stk_pop(opLenStk, &value);

	  /* check if the function name is a provided function */
	  callbackIndex = vm_callback_index(c->vm, token, len);
	  if(callbackIndex == -1) {

	    /* the function is not a native function. check script functions */
	    if(ht_get_raw_key(c->functionHT, token, len, &value)) {
	      CompilerFunc * funcDef = value.pointerVal;
	      /* function exists, lets write the OPCodes */
	      sb_append_char(c->outBuffer, OP_CALL_B);
	      /* TODO: error check number of arguments */
	      sb_append_char(c->outBuffer, funcDef->numArgs);
	      sb_append_char(c->outBuffer, 1); /* Number of args ...TODO */
	      printf("Function Index: %i\n", funcDef->index);
	      sb_append_str(c->outBuffer, &funcDef->index, sizeof(int));
	      printf("OP_CALL_PTR_N: %s\n", token);
	    } else {
	      printf("Undefined function: %s\n", token);
	      c->err = COMPILERERR_UNDEFINED_FUNCTION;
	      return false;
	    }
	  }

	  /* function exists, lets write the OPCodes */
	  sb_append_char(c->outBuffer, OP_CALL_PTR_N);
	  /* TODO: make support mutiple arguments */
	  sb_append_char(c->outBuffer, 1);
	  sb_append_str(c->outBuffer, &callbackIndex, sizeof(int));
	  printf("OP_CALL_PTR_N: %s\n", token);
	  
	}
	return true;
      } else {
	/* Re-push the parenthesis:*/
	typestk_push(opStk, &token, sizeof(char*), type);
	stk_push_long(opLenStk, len);

	printf("CEASING WITHOUT POPPING:\n");
	printf("TYPESTK SIZE: %i\n", typestk_size(opStk));
	return true;
      }
    }

    /* handle OPERATORS and FUNCTIONS differently */
    if(type == LEXERTYPE_OPERATOR) {
      printf("POP: writing operator from OPSTK %s\n", token);
      printf("     writing operator from OPSTK LEN %i\n", len);

      /* if there is an '=' on the stack, this is an assignment,
       * the next token on the stack should be a KEYVAR type that
       * contains a variable name.
       */
      if(tokens_equal(LANG_OP_ASSIGN, LANG_OP_ASSIGN_LEN, token, len)) {

	/* get variable name string */
	if(!typestk_pop(opStk, &token, sizeof(char*), &type)
	     || type != LEXERTYPE_KEYVAR) {
	  printf("MALFORMED TOKEN: %s\n", token);
	  c->err = COMPILERERR_MALFORMED_ASSIGNMENT;
	  return false;
	}

	/* get variable name length */
	stk_pop(opLenStk, &value);
	len = value.longVal;

	assert(stk_size(c->symTableStk) > 0);

	/* check that the variable was previously declared */
	if(!ht_get_raw_key(symtblstk_peek(c), token, len, &value)) {
	  printf("UNDEFINED VAR: %s\n", token);
	  c->err = COMPILERERR_UNDEFINED_VARIABLE;
	  return false;
	}

	/* write the variable data OPCodes
	 * Moves the last value from the OP stack in the VM to the variable
	 * storage slot in the frame stack. */
	/* TODO: need to add ability to search LOWER frames for variables */
	sb_append_char(c->outBuffer, OP_VAR_STOR);
	sb_append_char(c->outBuffer, 0 /* this val should chng with depth */);
	sb_append_char(c->outBuffer, value.intVal);
	printf("ASSIGNED VALUE TO VAR: %s\n", token);
      } else {

	opCode = operator_to_opcode(token, len);

	/* check for invalid operators */
	if(opCode == -1) {

	  c->err = COMPILERERR_UNKNOWN_OPERATOR;
	  return false;
	}

	/* write operator OP code to output buffer */
	sb_append_char(c->outBuffer, opCode);
      }
    } else {
      /* Not an operator, must be a KEYVAR, and therefore, a variable read op */
      assert(stk_size(c->symTableStk) > 0);

      /* check that the variable was previously declared */
      if(!ht_get_raw_key(symtblstk_peek(c), token, len, &value)) {
	printf("Symbol table frame: %i\n", stk_size(c->symTableStk));
	printf("Undefined Var: %s %i\n", token, len);
	c->err = COMPILERERR_UNDEFINED_VARIABLE;
	return false;
      }

      /* write the variable data read OPCodes
       * Moves the last value from the specified depth and slot of the frame
       * stack in the VM to the VM OP stack.
       */
      /* TODO: need to add ability to search LOWER frames for variables */
      printf("Reading Variable\n");
      sb_append_char(c->outBuffer, OP_VAR_PUSH);
      sb_append_char(c->outBuffer, 0 /* this val should chng with depth */);
      sb_append_char(c->outBuffer, value.intVal);
    }

    /* we're no longer at the top of the stack */
    topOperator = false;
  }

  /* Is open parenthensis is expected SOMEWHERE in this sequence? If true,
   * and we reach this point, we never found one. Throw an error!
   */
  if(!parenthExpected) {
    return true;
  } else {
    printf("UMP 2;");
    /*** TODO: potentially catches false error cases */
    c->err = COMPILERERR_UNMATCHED_PARENTH;
    return false;
  }
}
