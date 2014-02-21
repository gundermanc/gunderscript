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
#include <limits.h>
#include "strcodeparser.h"
#include "langkeywords.h"
#include "lexer.h"

/* preprocessor definitions: */
#define COMPILER_NO_PREV        -1


/* TODO: make STK type auto enlarge and remove */
static const int initialOpStkDepth = 100;
/* max number of digits in a number value */
static const int numValMaxDigits = 50;
/* number of items to add to the op stack on resize */
static const int opStkBlockSize = 12;

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

/* during parsing, all keyvars, including variables, are pushed to the stack.
 * this routine checks the top of stack for a variable, and if it is one, writes
 * its opcodes.
 * returns true if the top of the stack is a variable read op, and false if not.*/
static bool parse_topstack_variable_read(Compiler * c, char * token, size_t len,
					 TypeStk * opStk, Stk * opLenStk) {

  /* check and see if top of stack is a KEYVAR. If so, it might be a variable */
  if(topstack_type(opStk, opLenStk) == LEXERTYPE_KEYVAR) {
    printf("PTVR 1\n");
    DSValue value;
    char * varToken;
    LexerType varType;
    size_t varLen;
    
    /* Variable read op */
    /* get variable string */
    if(!tokens_equal(token, len, LANG_OP_ASSIGN, LANG_OP_ASSIGN_LEN)
       && typestk_peek(opStk, &varToken, sizeof(char*), &varType)) {

      printf("PTVR 2\n");
      /* get variable length */
      stk_peek(opLenStk, &value);
      varLen = value.longVal;

      assert(stk_size(c->symTableStk) > 0);

      /* check that the variable was previously declared */
      if(ht_get_raw_key(symtblstk_peek(c), varToken, varLen, &value)) {

	printf("PTVR 3\n");
	/* write the variable data read OPCodes
	 * Moves the last value from the specified depth and slot of the frame
	 * stack in the VM to the VM OP stack.
	 */
	/* TODO: need to add ability to search LOWER frames for variables */
	char varSlot = value.intVal;

	printf("Reading Variable: %s\n", varToken);
	printf("VarLen: %i\n", varLen);
	/* get variable string */
	typestk_pop(opStk, &varToken, sizeof(char*), &varType);

	/* get variable length */
	stk_pop(opLenStk, &value);
	sb_append_char(c->outBuffer, OP_VAR_PUSH);
	sb_append_char(c->outBuffer, 0 /* TODO: this val should chng with depth */);
	printf("Variable slot: %i\n", varSlot);
	sb_append_char(c->outBuffer, varSlot);
      } else {
	/* variable was not defined in the variable hashtable. Throw error. */
	c->err = COMPILERERR_UNDEFINED_VARIABLE;
      }
      return true;
    }
  }
  return false;
}

/* only returns false if there is an error */
static bool parse_stacked_function_call(Compiler * c, char * token, size_t len, 
				VarType type, TypeStk * opStk, 
				Stk * opLenStk) {
  DSValue value;

  /* peek top item in "sidetrack" stack */
  typestk_peek(opStk, &token, sizeof(char*), &type);
  stk_peek(opLenStk, &value);
  len = value.longVal;

  /* check if item is a KEYVAR, if so, it SHOULD be a function call */
  if(type == LEXERTYPE_KEYVAR) {
    int callbackIndex = -1;

    /* item is correct type, pop it now */
    typestk_pop(opStk, &token, sizeof(char*), &type);
    stk_pop(opLenStk, &value);

    /* check if the function name is a C built-in function */
    callbackIndex = vm_callback_index(c->vm, token, len);
    if(callbackIndex != -1) {

      /* function is native, lets write the OPCodes for native call */
      sb_append_char(c->outBuffer, OP_CALL_PTR_N);
      /* TODO: make support mutiple arguments */
      sb_append_char(c->outBuffer, 1);
      sb_append_str(c->outBuffer, &callbackIndex, sizeof(int));
      printf("OP_CALL_PTR_N: %s\n", token);
      return true;

    } else {

      /* the function is not a native function. check script functions */
      if(ht_get_raw_key(c->functionHT, token, len, &value)) {

	/* turn hashtable value into pointer to CompilerFunc struct */
	CompilerFunc * funcDef = value.pointerVal;

	/* function exists, lets write the OPCodes */
	sb_append_char(c->outBuffer, OP_CALL_B);
	/* TODO: error check number of arguments */
	sb_append_char(c->outBuffer, funcDef->numArgs);
	sb_append_char(c->outBuffer, 1); /* Number of args ...TODO */
	sb_append_str(c->outBuffer, &funcDef->index, sizeof(int));
      } else {
	printf("Undefined function: %s\n", token);
	c->err = COMPILERERR_UNDEFINED_FUNCTION;
	return false;
      }
    }
  }
  return true;
}

/* Handles operators that are in the "sidetrack" stack used by Dijikstra's
 * algorithm when handling operator precedence.
 */
static bool parse_stacked_operator(Compiler * c, char * token, size_t len, 
				    VarType type, TypeStk * opStk, 
				    Stk * opLenStk) {
  DSValue value;

  /* handle OPERATORS and FUNCTIONS differently */
  if(type == LEXERTYPE_OPERATOR) {

    /* if there is an '=' on the stack, this is an assignment, Variable Write
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
      printf("ASSIGNED\n");
    } else {
      OpCode opCode = operator_to_opcode(token, len);

      /* check for invalid operators */
      if(opCode == -1) {
	c->err = COMPILERERR_UNKNOWN_OPERATOR;
	return false;
      }

      /* write operator OP code to output buffer */
      sb_append_char(c->outBuffer, opCode);
    }
  }
  return true;
}

/**
 * Writes all tokens from the provided stack to the output buffer in the 
 * Compiler instance, making allowances for specific behaviors for function
 * tokens, variable references, and assignments. A.K.A., un-"sidetracks"
 * sidetracked tokens, in Dijikstra's postfix algorithm.
 * c: an instance of Compiler.
 * opStk: the stack of operator strings.
 * opLenStk: the stack of operator string lengths, in longs.
 * parenthExpected: tells whether or not the calling function is
 * expecting a parenthesis. If it is and one is not encountered, the
 * function sets c->err = COMPILERERR_UNMATCHED_PARENTH and returns false.
 * returns: true if successful, and false if an unmatched parenthesis is
 * encountered.
 */
static bool write_from_stack(Compiler * c, TypeStk * opStk, 
				       Stk * opLenStk, bool parenthExpected,
				       bool popParenth) {
  bool topOperator = true;
  DSValue value;
  char * token = NULL;
  size_t len = 0;
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

      /* if top of stack is a parenthesis, it is a mismatch! */
      if(!parenthExpected) {
	c->err = COMPILERERR_UNMATCHED_PARENTH;
	return false;
      } else if(popParenth) {
	
	/* let the parenthesis be popped, but don't push it back */
	/* parse code that looks like function calls */
	if(!parse_stacked_function_call(c, token, len, type, opStk, opLenStk)) {
	  return false;
	}
	return true;

      } else {
	/* Re-push the parenthesis, we need it for later*/
	typestk_push(opStk, &token, sizeof(char*), type);
	stk_push_long(opLenStk, len);
	return true;
      }
    }

    /* checks top of stack for an operator. if one exists, it is written
     * to the output. also handles variable assignment statements.
     */
    if(!parse_stacked_operator(c, token, len, type, opStk, opLenStk)) {
      return false;
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

static bool parse_number(Compiler * c, LexerType prevValType, 
			 char * token, size_t len) {
  /* Reads a number, converts to double and writes it to output as so:
   * OP_NUM_PUSH [value as a double]
   */
  char rawValue[numValMaxDigits];
  double value = 0;

  /* get double representation of number
     /* TODO: length check, and double overflow check */
  strncpy(rawValue, token, len);
  value = atof(rawValue);

  printf("OUTPUT: %f\n", value);

  /* write number to output */
  sb_append_char(c->outBuffer, OP_NUM_PUSH);
  sb_append_str(c->outBuffer, (char*)(&value), sizeof(double));

  /* check for invalid types: */
  if(prevValType != COMPILER_NO_PREV
     && prevValType != LEXERTYPE_PARENTHESIS
     && prevValType != LEXERTYPE_OPERATOR) {
    c->err = COMPILERERR_UNEXPECTED_TOKEN;
    return false;
  }

  return true;
}

static bool parse_string(Compiler * c, LexerType prevValType, 
			 char * token, size_t len) {
  /* Reads a string token and writes it raw to the output as so:
   * OP_STR_PUSH [strlen as 1 byte value] [string]
   */
  char outLen = len;

  /* string length byte has max value of CHAR_MAX. */
  if(len >= CHAR_MAX) {
    c->err = COMPILERERR_STRING_TOO_LONG;
    return false;
  }

  printf("Output string: %s\n", token);

  /* write output */
  sb_append_char(c->outBuffer, OP_STR_PUSH);
  sb_append_char(c->outBuffer, outLen);
  sb_append_str(c->outBuffer, token, len);

  /* check for invalid types: */
  if(prevValType != COMPILER_NO_PREV
     && prevValType != LEXERTYPE_PARENTHESIS
     && prevValType != LEXERTYPE_OPERATOR) {
    c->err = COMPILERERR_UNEXPECTED_TOKEN;
    return false;
  }
  return true;
}

static bool parse_keyvar(Compiler * c, TypeStk * opStk, Stk * opLenStk,
			 LexerType prevValType, LexerType type,
			 char * token, size_t len) {
  /* KEYVAR HANDLER:
   * Handles all word tokens. These can be variables or function names
   */
  /* push variable or function name to operator stack */
  typestk_push(opStk, &token, sizeof(char*), type);
  stk_push_long(opLenStk, len);
  printf("KEYVAR pushed to OPSTK: %s\n", token);
  printf("KEYVAR pushed to OPSTK Len: %i\n", len);
  return true;
}

static bool parse_parenthesis(Compiler * c, TypeStk * opStk, Stk * opLenStk,
			      LexerType prevValType, LexerType type, 
			      char * token, size_t len) {
  printf("PARENTH: %s\n", token);
  printf("PARENTH LEN: %i\n", len);
  if(tokens_equal(LANG_OPARENTH, LANG_OPARENTH_LEN, token, len)) {
    typestk_push(opStk, &token, sizeof(char*), type);
    stk_push_long(opLenStk, len);
    printf("OParenth pushed to OPSTK: %s\n", token);
    printf("OParenth pushed to OPSTK Len: %i\n", len);
  } else {

    /* we're about to start popping items off of the stack. First check for
     * variables at the top.
     */
    if(parse_topstack_variable_read(c, token, len, opStk, opLenStk)) {
      if(c->err != COMPILERERR_SUCCESS) {
	return false;
      }
    }

    /* pop operators from stack and write to output buffer
     * FAIL if there is no open parenthesis in the stack.
     */
    printf("**Close Parenth Pop\n");
    if(!write_from_stack(c, opStk, opLenStk, true, true)) {
      return false;
    }
  }

  /* check for invalid types: */
  if(prevValType != COMPILER_NO_PREV
     && prevValType != LEXERTYPE_PARENTHESIS
     && prevValType != LEXERTYPE_OPERATOR
     && prevValType != LEXERTYPE_NUMBER
     && prevValType != LEXERTYPE_STRING
     && prevValType != LEXERTYPE_KEYVAR) {
    c->err = COMPILERERR_UNEXPECTED_TOKEN; 
    return false;
  }
  return true;
}

static bool parse_operator(Compiler * c, TypeStk * opStk, Stk * opLenStk, 
			   LexerType prevValType, LexerType type,
			      char * token, size_t len) {
  /* Reads an operator from the lexer and decides whether or not to
   * place it in the opStk, in accordance with order of operations,
   * A.K.A. "precedence."
   */

  /* if the current operator precedence is >= to the one at the top of the
   * stack, push it to the stack. Otherwise, pop the stack empty. By doing
   * this, we modify the order of evaluation of operators based on their
   * precedence, a.k.a. order of operations.
   */

  /* TODO: add error checking for extra operators
   * and unmatched parenthesis */
  assert(token != NULL);

  /* check for pushed variables at the top of the stack */
  if(parse_topstack_variable_read(c, token, len, opStk, opLenStk)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  }

  if(operator_precedence(token, len)
     >= topstack_precedence(opStk, opLenStk)) {
	
    /* push operator to operator stack */
    typestk_push(opStk, &token, sizeof(char*), type);
    stk_push_long(opLenStk, len);
    printf("Operator pushed to OPSTK: %s\n", token);
    printf("Operator pushed to OPSTK Len: %i\n", len);
  } else {

    /* pop operators from stack and write to output buffer */
    printf("**Popping Operators.\n");
    if(!write_from_stack(c, opStk, opLenStk, true, false)) {
      return false;
    }

    /* push operator to operator stack */
    typestk_push(opStk, &token, sizeof(char*), type);
    stk_push_long(opLenStk, len);
  }

  /* check for invalid types: */
  if(prevValType != LEXERTYPE_STRING
     && prevValType != LEXERTYPE_NUMBER
     && prevValType != LEXERTYPE_KEYVAR
     && prevValType != LEXERTYPE_PARENTHESIS) {
    /* TODO: this error check does not distinguish between ( and ). Revise to
     * ensure that parenthesis is part of a matching pair too.
     */
    c->err = COMPILERERR_UNEXPECTED_TOKEN; 
    return false;
  }
  return true;
}

/**
 * Handles straight code:
 * This is a modified version of Djikstra's "Shunting Yard Algorithm" for
 * conversion to postfix notation. Code is converted to postfix and written to
 * the output buffer as a series of OP codes all in one step.
 * c: an instance of Compiler.
 * l: an instance of lexer that will provide the tokens.
 * bracketEncountered: a boolean that will recv whether or not the parser
 * stopped because a closed bracket was reached.
 * returns: true if success, and false if errors occurred. If error occurred,
 * c->err is set.
 */
/* TODO: return false for every sb_append* function upon failure */
bool parse_straight_code(Compiler * c, Lexer * l, bool * bracketEncountered) {
  LexerType type;
  LexerType prevValType = COMPILER_NO_PREV;
  char * token;
  size_t len;

  /* allocate stacks for operators and their lengths, a.k.a. 
   * the "side track in shunting yard" 
   */
  TypeStk * opStk = typestk_new(initialOpStkDepth, opStkBlockSize);
  Stk * opLenStk = stk_new(initialOpStkDepth);
  if(opStk == NULL || opLenStk == NULL) {
    c->err = COMPILERERR_ALLOC_FAILED;
    return false;
  }

  token = lexer_current_token(l, &type, &len);
  printf("First straight code token: %s\n", token);

  printf("FIRST: %s\n", token);

  if(bracketEncountered != NULL) {
    *bracketEncountered = false;
  }

  /* straight code token parse loop */
  do {

    /* output values, push operators to stack with precedence so that they can
     * be postfixed for execution by the VM
     */
    switch(type) {
    case LEXERTYPE_BRACKETS:
      /* TODO: make sure this is a CLOSE bracket, not open */
      if(prevValType != COMPILER_NO_PREV
	 && prevValType != LEXERTYPE_ENDSTATEMENT) {
	c->err = COMPILERERR_EXPECTED_ENDSTATEMENT;
	return false;
      }

      token = lexer_next(l, &type, &len);

      if(bracketEncountered != NULL) {
	*bracketEncountered = true;
      }
      printf("Hit Bracket\n");
      return true; 

    case LEXERTYPE_KEYVAR:
      if(!parse_keyvar(c, opStk, opLenStk, prevValType, type, token, len)) {
	return false;
      }
      break;

    case LEXERTYPE_NUMBER:
      if(!parse_number(c, prevValType, token, len)) {
	return false;
      }
      break;

    case LEXERTYPE_STRING:
      if(!parse_string(c, prevValType, token, len)) {
	return false;
      }
      break;

    case LEXERTYPE_PARENTHESIS:
      if(!parse_parenthesis(c, opStk, opLenStk, prevValType, type, token, len)) {
	return false;
      }
      break;

    case LEXERTYPE_OPERATOR:
      if(!parse_operator(c, opStk, opLenStk, prevValType, type, token, len)) {
	return false;
      }
      break;

    case LEXERTYPE_ENDSTATEMENT: {

      /* check for invalid types: */
      if(prevValType == LEXERTYPE_OPERATOR
	 || prevValType == LEXERTYPE_ENDSTATEMENT) {
	c->err = COMPILERERR_UNEXPECTED_TOKEN;
	return false;
      }
      printf("ENDSTATEMENT\n");

      /* reached the end of the input, empty the operator stack to the output */
      printf("**End Pop\n");
      if(!write_from_stack(c, opStk, opLenStk, false, true)) {
	return false;
      }
      /* write stack pop instruction:
       * this ensures that the last return value is popped off of the stack after
       * the statement completes.
       */
      printf("POPTOKEN: %s\n", token);
      sb_append_char(c->outBuffer, OP_POP);

      token = lexer_next(l, &type, &len);
      return true;
    }

    default:
      c->err = COMPILERERR_UNEXPECTED_TOKEN;
      printf("Unexpected Straight Code Token: %s\n", token);
      printf("Unexpected Straight Code Token Len: %i\n", len);
      return false;
    }

    /* store the type of this token for the next iteration: */
    prevValType = type;

  } while((token = lexer_next(l, &type, &len)) != NULL);

  /* no semicolon at the end of the line, throw a fit */
  /*if(prevValType != LEXERTYPE_ENDSTATEMENT) {*/
    c->err = COMPILERERR_EXPECTED_ENDSTATEMENT;
    return false;
    /*}*/

  /* no errors occurred */
  return true;
}
