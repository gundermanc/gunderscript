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
 * During parsing, all keyvars, including variables, are pushed to the stack.
 * This routine checks the top of stack to see if it is a variable. If it is,
 * the routine writes its its opcodes to the output buffer.
 * c: an instance of compiler.
 * token: the current token.
 * len: the length of the current token.
 * varType: the type of the current token.
 * opStk: the stack of operands.
 * opLenStk: the stack of lengths of the opStk members.
 * returns: true if the top of the stack is a variable read op, and false if not.
 * If returns true, errors will be denoted by setting c->err value.
 */
static bool parse_topstack_variable_read(Compiler * c, char * token, size_t len,
					 LexerType varType, TypeStk * opStk,
					 Stk * opLenStk) {
  
  /* check and see if top of stack is a KEYVAR. If so, it should be a variable */
  if(topstack_type(opStk, opLenStk) == LEXERTYPE_KEYVAR) {
    DSValue value;
    char * varToken;
    size_t varLen;
    
    /* Variable read op:
     * check to see if current token is an assign '=' character. if not,
     * this is a variable read operation.
     */
    if(!tokens_equal(token, len, LANG_OP_ASSIGN, LANG_OP_ASSIGN_LEN)
       && typestk_peek(opStk, &varToken, sizeof(char*), NULL)) {

      /* get variable length */
      stk_peek(opLenStk, &value);
      varLen = value.longVal;

      assert(stk_size(c->symTableStk) > 0);

      /* check that the variable was previously declared */
      if(ht_get_raw_key(symtblstk_peek(c), varToken, varLen, &value)) {

	/* write the variable data read OPCodes
	 * Moves the last value from the specified depth and slot of the frame
	 * stack in the VM to the VM OP stack.
	 */
	/* TODO: need to add ability to search LOWER frames for variables */
	char varSlot = value.intVal;

	/* get variable string */
	typestk_pop(opStk, &varToken, sizeof(char*), NULL);

	/* get variable length */
	stk_pop(opLenStk, &value);
	sb_append_char(c->outBuffer, OP_VAR_PUSH);
	sb_append_char(c->outBuffer, 0 
		       /* TODO: this val should chng with depth */);
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

/**
 * Pops operators that are were pushed into the "sidetrack" stack used by 
 * Dijikstra's shunting yard algorithm when handling operator precedence. The 
 * respective OP codes are then written for each operator.
 * c: an instance of compiler.
 * token: the current token.
 * len: the length of the current token.
 * type: the type of the current token.
 * opStk: A stack containing the operands pushed by the shunting yard algorithm.
 * opLenStk: A stack containing the lengths of the operands on the opStk.
 * returns: false if an error occurs and true if not.
 */
static bool parse_stacked_operator(Compiler * c,  char * token,
				   size_t len, LexerType type, TypeStk * opStk, 
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
    } else {
      /* current operator is NOT an assignment, get its opcode and write */
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
 * Compiler instance, making allowances for specific behaviors variable
 * references, and assignments. A.K.A., un-"sidetracks"sidetracked tokens,
 * in Dijikstra's postfix algorithm.
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
  DSValue value;
  char * token = NULL;
  size_t len = 0;
  VarType type;

  /* while items remain */
  while(typestk_size(opStk) > 0) {

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
      } else if(!popParenth) {
	/* Re-push the parenthesis, we need it for later*/
	typestk_push(opStk, &token, sizeof(char*), type);
	stk_push_long(opLenStk, len);
      }
      return true;
    }

    /* checks top of stack for an operator. if one exists, it is written
     * to the output. also handles variable assignment statements.
     */
    if(!parse_stacked_operator(c, token, len, type, opStk, opLenStk)) {
      return false;
    }
  }

  /* Is open parenthensis is expected SOMEWHERE in this sequence? If true,
   * and we reach this point, we never found one. Throw an error!
   */
  /* TODO: make sure this is unneccessary before deleting */
  /*if(!parenthExpected) {
    return true;
  } else {
    printf("UMP 2;\n");
    c->err = COMPILERERR_UNMATCHED_PARENTH;
    return false;
    }*/
  return true;
}

/**
 * Subparser for parse_straight_code():
 * parses a number and writes its raw double form and OP_NUM_PUSH op code to the
 * output buffer.
 * c: an instance of compiler.
 * prevTokenType: The type of the token that came before the current one.
 * token: the current token. Should be a LEXERTYPE_NUMBER.
 * len: the length of the token in number of chars.
 * returns: true if operation succeeds and false if operation fails with an
 * error. Error is written to c->err
 */
static bool parse_number(Compiler * c, LexerType prevTokenType, 
			 char * token, size_t len) {
  /* Reads a number, converts to double and writes it to output as so:
   * OP_NUM_PUSH [value as a double]
   */
  char rawValue[numValMaxDigits];
  double value = 0;

  /* get double representation of number
   * TODO: length check, and double overflow check */
  strncpy(rawValue, token, len);
  value = atof(rawValue);

  /* write number to output */
  sb_append_char(c->outBuffer, OP_NUM_PUSH);
  sb_append_str(c->outBuffer, (char*)(&value), sizeof(double));

  /* check for invalid types in previous token: */
  if(prevTokenType != COMPILER_NO_PREV
     && prevTokenType != LEXERTYPE_PARENTHESIS
     && prevTokenType != LEXERTYPE_OPERATOR) {
    c->err = COMPILERERR_UNEXPECTED_TOKEN;
    return false;
  }

  return true;
}

/**
 * Subparser for parse_straight_code():
 * Parses a string token and produces respective OP_STR_PUSH op code and writes
 * to the outBuffer.
 * c: an instance of compiler.
 * prevTokenType: the type of the token before this one.
 * token: the current token.
 * len: the length of the current token.
 * returns: true if the operation succeeds and false if an error occured. Upon an
 * error, the error code is written to c->err.
 */
static bool parse_string(Compiler * c, LexerType prevTokenType, 
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

  /* write output */
  sb_append_char(c->outBuffer, OP_STR_PUSH);
  sb_append_char(c->outBuffer, outLen);
  sb_append_str(c->outBuffer, token, len);

  /* check for invalid types: */
  if(prevTokenType != COMPILER_NO_PREV
     && prevTokenType != LEXERTYPE_PARENTHESIS
     && prevTokenType != LEXERTYPE_OPERATOR) {
    c->err = COMPILERERR_UNEXPECTED_TOKEN;
    return false;
  }
  return true;
}

/**
 * Subparser for parse_straight_code():
 * Parses a KEYVAR type token and either interprets it as a variable read, a
 * variable write, or a function call, depending on the context of its usage.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * opStk: the stack of operators pushed for later use by the shunting yard
 * algorithm.
 * opLenStk: the stack of the lengths of the opStk members.
 * prevTokenType: pointer to the type of the token before the current one.
 * type: the type of the current token.
 * token: the current token.
 * len: the length of the current token.
 * returns true upon success, and false upon an error.
 */
static bool parse_keyvar(Compiler * c, Lexer * l, TypeStk * opStk,
			 Stk * opLenStk, LexerType * prevValType, LexerType type,
			 char * token, size_t len) {
  char * parenthToken;
  size_t parenthLen;
  LexerType parenthType;

  /* look ahead one token without advancing and see what's there */
  parenthToken = lexer_peek(l, &parenthType, &parenthLen);

  /* check if next token is a parenthesis, if so, this is a function call */
  if(tokens_equal(parenthToken, parenthLen, LANG_OPARENTH, LANG_OPARENTH_LEN)) {

    /* delegate function call to function call parser */
    if(!parse_line(c, l)) {
      return false;
    }
    token = lexer_current_token(l, &type, &len);

  } else {

    /* Not a function call. A variable operation. Store tokens on stack for
     * later parsing.
     */
    typestk_push(opStk, &token, sizeof(char*), type);
    stk_push_long(opLenStk, len);

    /* store type of current token for next iteration */
    (*prevValType) = type;

    /* advance to next token */
    token = lexer_next(l, &type, &len);
  }

  /* TODO: make sure that prevTokenType checks aren't neccessary here */

  return true;
}

/**
 * Subparser for parse_straight_code():
 * Handles parenthesis count and terminating the current function call upon
 * reaching the terminating parenthesis.
 * c: an instance of compiler.
 * opStk: a stack of operators used by shunting yard algorithm.
 * opLenStk: stack of length of operators in the opStk.
 * prevTokenType: type of the token that came before the current one.
 * type: the type of the current token.
 * token: the current token.
 * len: the length of the current token.
 * parenthDepth: pointer to the current nesting level of parenthesis. 0 is not
 * within any, and positive is within parenthesis.
 * returns: true if the operation succeeds and false if an error is encountered.
 * c->err receives the error code.
 */
static bool parse_parenthesis(Compiler * c, TypeStk * opStk, Stk * opLenStk,
			      LexerType prevTokenType, LexerType type, 
			      char * token, size_t len, int * parenthDepth) {

  /* handle open parenthesis:
   * push them onto the stack for order of operations handling
   */
  if(tokens_equal(LANG_OPARENTH, LANG_OPARENTH_LEN, token, len)) {
    typestk_push(opStk, &token, sizeof(char*), type);
    stk_push_long(opLenStk, len);

    /* check for invalid previous token types: */
    if(prevTokenType != LEXERTYPE_OPERATOR &&
       prevTokenType != LEXERTYPE_ARGDELIM &&
       prevTokenType != LEXERTYPE_PARENTHESIS &&
       prevTokenType != LEXERTYPE_KEYVAR) {
      c->err = COMPILERERR_UNEXPECTED_TOKEN; 
      return false;
    }
    (*parenthDepth)++;

  } else {

    /* handle close parenthesis:
     * we're about to start popping items off of the stack. First check for
     * variables at the top.
     */
    if(parse_topstack_variable_read(c, token, len, type, opStk, opLenStk)) {
      if(c->err != COMPILERERR_SUCCESS) {
	return false;
      }
    }

    /* pop operators from stack and write to output buffer
     * FAIL if there is no open parenthesis in the stack.
     */
    if(!write_from_stack(c, opStk, opLenStk, true, true)) {
      return false;
    }

    /* check for invalid previous token types: */
    if(prevTokenType != LEXERTYPE_PARENTHESIS
       && prevTokenType != LEXERTYPE_OPERATOR
       && prevTokenType != LEXERTYPE_NUMBER
       && prevTokenType != LEXERTYPE_STRING
       && prevTokenType != LEXERTYPE_KEYVAR) {
      c->err = COMPILERERR_UNEXPECTED_TOKEN; 
      return false;
    }

    (*parenthDepth)--;
  }
  return true;
}

/**
 * Subparser for parse_straight_code():
 * Handles precedence of operators by pushing them to the stack in order of 
 * precedence (order of operations). They are later popped after operands to
 * place the operands and operations in the opcode into postfix notation.
 * c: an instance of compiler.
 * opStk: the stack used for shunting the opcodes.
 * opLenStk: a stack containing the lengths of the tokens stored in the opStk.
 * prevTokenType: the type of the token before the current one.
 * type: type of the current token.
 * len: the length of the current token.
 * returns: true if the operators are handled with no errors, and false if an
 * error occurs.
 */
static bool parse_operator(Compiler * c, TypeStk * opStk, Stk * opLenStk, 
			   LexerType prevTokenType, LexerType type,
			      char * token, size_t len) {
  /* Reads an operator from the lexer and decides whether or not to
   * place it in the opStk, in accordance with order of operations,
   * A.K.A. "precedence." If the current operator precedence is >= to the one
   * at the top of the stack, push it to the stack. Otherwise, pop the stack
   * empty. By doing this, we modify the order of evaluation of operators based
   * on their precedence, a.k.a. order of operations.
   */
  assert(token != NULL);

  /* check for pushed variables at the top of the stack */
  if(parse_topstack_variable_read(c, token, len, type, opStk, opLenStk)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  }

  /* if current token has a higher precedence than top of stack, push it */
  if(operator_precedence(token, len)
     >= topstack_precedence(opStk, opLenStk)) {
	
    /* push operator to operator stack */
    typestk_push(opStk, &token, sizeof(char*), type);
    stk_push_long(opLenStk, len);
  } else {

    /* pop operators from stack and write to output buffer */
    if(!write_from_stack(c, opStk, opLenStk, true, false)) {
      return false;
    }

    /* push operator to operator stack */
    typestk_push(opStk, &token, sizeof(char*), type);
    stk_push_long(opLenStk, len);
  }

  /* check for invalid types: */
  if(prevTokenType != LEXERTYPE_STRING
     && prevTokenType != LEXERTYPE_NUMBER
     && prevTokenType != LEXERTYPE_KEYVAR
     && prevTokenType != LEXERTYPE_PARENTHESIS) {
    /* TODO: this error check does not distinguish between ( and ). Revise to
     * ensure that parenthesis is part of a matching pair too, and fix bug
     */
    /* c->err = COMPILERERR_UNEXPECTED_TOKEN;*/
    printf("UET parse_operators: %i\n", prevTokenType);
    /* return false; */
  }
  return true;
}

bool parse_straight_code_loop(Compiler * c, Lexer * l, TypeStk * opStk, 
			 Stk * opLenStk, bool innerCall, 
			 bool * parenthEncountered) {
  LexerType type;
  LexerType prevValType = COMPILER_NO_PREV;
  size_t len;
  char * token = lexer_current_token(l, &type, &len);
  int parenthDepth = 0;

  /* straight code token parse loop */
  do {

    /* output values, push operators to stack with precedence so that they can
     * be postfixed for execution by the VM
     */
    switch(type) {
    case LEXERTYPE_ARGDELIM:
      if(innerCall) {
	/* finishing parsing the current argument, return */
	if(!write_from_stack(c, opStk, opLenStk, true, true)) {
	  return false;
	}
	return true;
      } else {
	/* there shouldn't be any commas here */
	c->err = COMPILERERR_UNEXPECTED_TOKEN;
	return false;
      }

    case LEXERTYPE_NUMBER:
      if(!parse_number(c, prevValType, token, len)) {
	return false;
      }
      prevValType = type;
      token = lexer_next(l, &type, &len);
      break;

    case LEXERTYPE_STRING:
      if(!parse_string(c, prevValType, token, len)) {
	return false;
      }
      prevValType = type;
      token = lexer_next(l, &type, &len);
      break;

    case LEXERTYPE_PARENTHESIS:
      if(!parse_parenthesis(c, opStk, opLenStk, prevValType, 
			    type, token, len, &parenthDepth)) {
	return false;
      }

      /* set end parenthesis encountered */
      if(parenthDepth <= 0) {
	if(tokens_equal(token, len, LANG_CPARENTH, LANG_CPARENTH_LEN) 
	   && parenthEncountered != NULL) {
	  *parenthEncountered = true;
	}
	return true;
      }
      prevValType = type;
      token = lexer_next(l, &type, &len);
      break;

    case LEXERTYPE_OPERATOR:
      if(!parse_operator(c, opStk, opLenStk, prevValType, type, token, len)) {
	return false;
      }
      prevValType = type;
      token = lexer_next(l, &type, &len);
      break;

    case LEXERTYPE_KEYVAR:
      if(!parse_keyvar(c, l, opStk, opLenStk, &prevValType, 
		       type, token, len)) {
	return false;
      }
      token = lexer_current_token(l, &type, &len);
      prevValType = type;
      break;

    case LEXERTYPE_ENDSTATEMENT: {
 
      if(!innerCall) {
 	/* check for invalid types: */
 	if(prevValType == LEXERTYPE_OPERATOR
 	   || prevValType == LEXERTYPE_ENDSTATEMENT) {
	  c->err = COMPILERERR_UNEXPECTED_TOKEN;
 	  return false;
 	}
 
 	if(!write_from_stack(c, opStk, opLenStk, false, true)) {
 	  return false;
 	}
 	return true;
      } else {
 	c->err = COMPILERERR_UNEXPECTED_TOKEN;
 	return false;
      }
    }

    case LEXERTYPE_BRACKETS:
      c->err = COMPILERERR_UNMATCHED_PARENTH;
      return false;

    default:
      c->err = COMPILERERR_UNEXPECTED_TOKEN;
      return false;
    }
  } while((token = lexer_current_token(l, &type, &len)) != NULL);

  /* we're missing a closing parenthesis if we reached this point */
  c->err = COMPILERERR_UNMATCHED_PARENTH;
  return false;
}

/**
 * Handles straight code:
 * This is a modified version of Djikstra's "Shunting Yard Algorithm" for
 * conversion to postfix notation. Code is converted to postfix and written to
 * the output buffer as a series of OP codes all in one step.
 * c: an instance of Compiler.
 * l: an instance of lexer that will provide the tokens.
 * innerCall: should be true if the current code is an argument to a function
 * call.
 * parenthEncountered: parsing terminated because of a close parenthesis, rather
 * than a comma.
 * returns: true if success, and false if errors occurred. If error occurred,
 * c->err is set.
 */
bool parse_straight_code(Compiler * c, Lexer * l,
			 bool innerCall, bool * parenthEncountered) {
  /* TODO: return false for every sb_append* function upon failure */
  bool result;

  /* allocate stacks for operators and their lengths, a.k.a. 
   * the "side track in shunting yard" 
   */
  TypeStk * opStk = typestk_new(initialOpStkDepth, opStkBlockSize);
  Stk * opLenStk = stk_new(initialOpStkDepth);
  if(opStk == NULL || opLenStk == NULL) {
    c->err = COMPILERERR_ALLOC_FAILED;
    return false;
  }

  /* set initial parenthEncountered value */
  if(parenthEncountered != NULL) {
    *parenthEncountered = false;
  }

  result = parse_straight_code_loop(c, l, opStk, opLenStk, innerCall,
				    parenthEncountered);

  /* free stacks */
  typestk_free(opStk);
  stk_free(opLenStk);

  return result;
}

/**
 * Writes the function call op codes, telling VM to pop last 'arguments' number
 * of args off of stack to use as arguments to the function call.
 * c: compiler instance.
 * functionName: the name of the function to look up and write the call for.
 * functionNameLen: the length of the function name in chars.
 * arguments: the number of arguments that this function accepts.
 * result: returns true if success, and false if an error occurs. Upon error,
 * c->err receives the error code.
 */
static bool function_call(Compiler * c, char * functionName, 
		   size_t functionNameLen, int arguments) {

  DSValue value;
  int callbackIndex;

  /* check if function is "return" pseudo-function */
  if(tokens_equal(functionName, functionNameLen, LANG_RETURN, LANG_RETURN_LEN)) {

    /* make sure there is only one return value */
    if(arguments != 1) {
      c->err = COMPILERERR_INCORRECT_NUMARGS;
      return false;
    }

    /* TODO: pop multiple frames if current frame isn't a function frame */
    /* return from current function to return value stored in stack frame */
    sb_append_char(c->outBuffer, OP_FRM_POP);
    return true;
  }

  /* check if the function name is a C built-in function */
  callbackIndex = vm_callback_index(c->vm, functionName, functionNameLen);
  if(callbackIndex != -1) {

    /* function is native, write the OPCodes for native call */
    sb_append_char(c->outBuffer, OP_CALL_PTR_N);
    sb_append_char(c->outBuffer, arguments);
    sb_append_str(c->outBuffer, (char*)(&callbackIndex), sizeof(int));
    return true;

  } else {

    /* the function is not a native function. check script functions */
    if(ht_get_raw_key(c->functionHT, functionName, 
		      functionNameLen, &value)) {

      /* turn hashtable value into pointer to CompilerFunc struct */
      CompilerFunc * funcDef = value.pointerVal;

      /* check for correct number of arguments */
      if(funcDef->numArgs != arguments) {
	c->err = COMPILERERR_INCORRECT_NUMARGS;
	return false;
      }

      /* function exists, lets write the OPCodes */
      sb_append_char(c->outBuffer, OP_CALL_B);
      sb_append_char(c->outBuffer, funcDef->numArgs + funcDef->numVars);
      sb_append_char(c->outBuffer, funcDef->numArgs);
      sb_append_str(c->outBuffer, (char*)(&funcDef->index), sizeof(int));
    } else {
      c->err = COMPILERERR_UNDEFINED_FUNCTION;
      return false;
    }
  }
  return true;
}

/**
 * Attempts to parse current location as a line of code. First, checks if this
 * is a function call. If so, dispatches subparsers to handle the arguments. If
 * not, the straight code parser is dispatched to handle operations between
 * variables and constants.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * returns: false if an error occurs and sets c->err to the error code.
 */
bool parse_line(Compiler * c, Lexer * l) {
  LexerType type;
  char * token;
  size_t len;
  bool endOfArgs = false;
  size_t functionTokenLen;
  char * functionToken;

  /* get current token */
  token = lexer_current_token(l, &type, &len);
  
  /* check if this is a keyword or variable */
  if(type != LEXERTYPE_KEYVAR) {

    /* malformed expression,
     * does not start with KEYVAR...is not var assignment or function call
     */
    c->err = COMPILERERR_UNEXPECTED_TOKEN;
    return false;
  }

  /* peek ahead one token */
  token = lexer_peek(l, NULL, &len);

  /* check if this is a function call by seeing if peeked token is a parenth */
  if(tokens_equal(token, len,LANG_OPARENTH, LANG_OPARENTH_LEN)) {
    int argCount = 0;

    /* save function name */
    functionToken = lexer_current_token(l, NULL, &functionTokenLen);

    /* skip to the arguments tokens */
    token = lexer_next(l, &type, &len);
    token = lexer_next(l, &type, &len);

    /* check if this function has arguments */
    if(!tokens_equal(token, len, LANG_CPARENTH, LANG_CPARENTH_LEN)) {

      /* loop through the arguments one by one */
      do {

	/* delegate current argument to the straight code parser */
	if(!parse_straight_code(c, l, true, &endOfArgs)) {
	  return false;
	}

	/* skip the comma or closing parenth if there is one */
	token = lexer_next(l, &type, &len);
	argCount++;
      } while(!endOfArgs);
    } else {
      token = lexer_next(l, &type, &len);
    }

    /* writes a call to the specified function, or returns if error */
    if(!function_call(c, functionToken, functionTokenLen, argCount)) {
      return false;
    }

    /* function call write succeeded */
    return true;
  } else {

    /* not a function call, delegate to the straight code parser */
    if(!parse_straight_code(c, l, false, NULL)) {
      return false;
    }
  }
  return true;
}
