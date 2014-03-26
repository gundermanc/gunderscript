/**
 * parsers.c
 * (C) 2013-2014 Christian Gunderman
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
#include "parsers.h"
#include "langkeywords.h"
#include "lexer.h"
#include "buffer.h"

/* preprocessor definitions: */
#define COMPILER_NO_PREV        -1
/* max number of digits in a number value */
#define  COMPILER_NUM_MAX_DIGITS   50

/* TODO: make STK type auto enlarge and remove */
static const int initialOpStkDepth = 100;
/* number of items to add to the op stack on resize */
static const int opStkBlockSize = 12;

/* private function declarations */
static bool parse_line(Compiler * c, Lexer * l, bool innerCall);
bool parse_block(Compiler * c, Lexer * l);
bool parse_body_statement(Compiler * c, Lexer * l);

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
 * returns: true
 */
static bool write_operator(Compiler * c,  char * token,
				   size_t len, LexerType type) {
  OpCode opCode = operator_to_opcode(token, len);

  /* check for invalid operators */
  if(opCode == -1) {
    c->err = COMPILERERR_UNKNOWN_OPERATOR;
    return false;
  }

  /* write operator OP code to output buffer */
  buffer_append_char(c->outBuffer, opCode);
   
  return true;
}

/**
 * Writes all operators from the provided stack to the output buffer in the 
 * Compiler instance, making allowances for operator precedences A.K.A., un-
 * "sidetracks"sidetracked tokens,in Dijikstra's postfix algorithm.
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

    /* get token string and length */
    typestk_pop(opStk, &token, sizeof(char*), &type);
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
     * to the output.
     */
    write_operator(c, token, len, type);
 
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
  char rawValue[COMPILER_NUM_MAX_DIGITS] = "";
  double value = 0;

  /* get double representation of number
   * TODO: length check, and double overflow check */
  strncpy(rawValue, token, len);
  value = atof(rawValue);

  /* write number to output */
  buffer_append_char(c->outBuffer, OP_NUM_PUSH);
  buffer_append_string(c->outBuffer, (char*)(&value), sizeof(double));

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
  buffer_append_char(c->outBuffer, OP_STR_PUSH);
  buffer_append_char(c->outBuffer, outLen);
  buffer_append_string(c->outBuffer, token, len);

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
			 Stk * opLenStk, LexerType * prevTokenType, LexerType type,
			 char * token, size_t len) {

  /* check for invalid types: */
  if(*prevTokenType != COMPILER_NO_PREV
     && *prevTokenType != LEXERTYPE_PARENTHESIS
     && *prevTokenType != LEXERTYPE_OPERATOR
     && *prevTokenType != LEXERTYPE_BRACKETS) {
    c->err = COMPILERERR_UNEXPECTED_TOKEN;
    return false;
  }

  /* delegate to line parser */
  if(!parse_line(c, l, true)) {
    return false;
  }

  /* get the current token, it was updated by parse_line */
  token = lexer_current_token(l, &type, &len);
  (*prevTokenType) = LEXERTYPE_PARENTHESIS;

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
    if(prevTokenType != COMPILER_NO_PREV &&
       prevTokenType != LEXERTYPE_OPERATOR &&
       prevTokenType != LEXERTYPE_ARGDELIM &&
       prevTokenType != LEXERTYPE_PARENTHESIS &&
       prevTokenType != LEXERTYPE_KEYVAR) {
      c->err = COMPILERERR_UNEXPECTED_TOKEN;
      return false;
    }
    (*parenthDepth)++;

  } else {

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
    printf("\nNOTE: this error case is experimental and may reflect a bug in Gunderscript"
	   ", not your code. After checking your code, if it seems ok, it may be a bug\n\n");
    c->err = COMPILERERR_UNEXPECTED_TOKEN;
    return false;
  }

  return true;
}

/**
 * The loop for parse_straight_code(). Evaluates math statements and dispatches
 * subparsers for assignment statements, function calls, constants, etc. as
 * needed.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * opStk: the stack of operators.
 * opLenStk: the stack of operator string lengths.
 * innerCall: must be true if this is a call inside of a set of parenthesis. If
 * this value is true, the function will automatically return upon reaching a 
 * LEXERTYPE_ARGDELIM. If false, the function will not return until it 
 * encounters a LEXERTYPE_ENDSTATEMENT that was not preceded by an expected 
 * open parenthesis.
 * returns: true upon success, and false upon an error.
 */
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

      /* if we reached an extra close parenth, its the end of the arguments
       * set end parenthesis encountered 
       */
      if(parenthDepth < 0 && innerCall) {
	if(tokens_equal(token, len, LANG_CPARENTH, LANG_CPARENTH_LEN) 
	   && parenthEncountered != NULL) {
	  *parenthEncountered = true;
	  return true;
	}
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
      break;

    case LEXERTYPE_ENDSTATEMENT: {
 
      /* if not an inner call, this line should end with a semicolon.
       * otherwise, it should end with a parenthesis. throw fits accordingly
       */
      if(!innerCall) {

 	/* check for invalid types: */
 	if(prevValType == LEXERTYPE_OPERATOR
 	   || prevValType == LEXERTYPE_ENDSTATEMENT) {
	  c->err = COMPILERERR_UNEXPECTED_TOKEN;
 	  return false;
 	}

	/* pop values from stack */
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
      c->err = COMPILERERR_UNEXPECTED_TOKEN;
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
 * returnCall: pointer to a buffer that recv's whether or not this function call
 * is a call to return().
 * result: returns true if success, and false if an error occurs. Upon error,
 * c->err receives the error code.
 */
static bool function_call(Compiler * c, char * functionName, 
			  size_t functionNameLen, int arguments, 
			  bool * returnCall) {

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
    buffer_append_char(c->outBuffer, OP_FRM_POP);
    *returnCall = true;
    return true;
  }

  *returnCall = false;

  /* check if the function name is a C built-in function */
  callbackIndex = vm_callback_index(c->vm, functionName, functionNameLen);
  if(callbackIndex != -1) {

    /* function is native, write the OPCodes for native call */
    buffer_append_char(c->outBuffer, OP_CALL_PTR_N);
    buffer_append_char(c->outBuffer, arguments);
    buffer_append_string(c->outBuffer, (char*)(&callbackIndex), sizeof(int));
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
      buffer_append_char(c->outBuffer, OP_CALL_B);
      buffer_append_char(c->outBuffer, funcDef->numArgs + funcDef->numVars);
      buffer_append_char(c->outBuffer, funcDef->numArgs);
      buffer_append_string(c->outBuffer, (char*)(&funcDef->index), sizeof(int));
    } else {
      c->err = COMPILERERR_UNDEFINED_FUNCTION;
      return false;
    }
  }
  return true;
}

/**
 * Parses arguments to a function call, if, while, etc. This function should be
 * called when the lexer is right inside of the "(". The arguments are converted to
 * the appropriate bytecode.
 * compiler: an instance of compiler.
 * token: the current token.
 * type: the LEXERTYPE of the token.
 * len: the length of the current token.
 * returns: the number of arguments.
 */
static int parse_arguments(Compiler * c, Lexer * l, char * token,
		    LexerType type, size_t len) {
  int argCount = 0;
  bool endOfArgs = false;

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
  return argCount;
}

/**
 * Parses while statements in script code, starting at the initial "while" token
 * and dispatches subparsers as needed until done.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * returns: true if this is a while statement, regardless of error. If error,
 * c->err is set.
 */
static bool parse_while_statement(Compiler * c, Lexer * l) {
  char * token;
  size_t len;
  LexerType type;
  int beforeWhileAddr;
  int jumpInstAddr;
  int address = 0;
  int argCount = 0;

  token = lexer_current_token(l, &type, &len);

  /* check if this is an while statement */
  if(!tokens_equal(token, len, LANG_WHILE, LANG_WHILE_LEN)) {
    return false;
  }

  token = lexer_next(l, &type, &len);

  /* check for an open parenthesis token */
  if(!tokens_equal(token, len, LANG_OPARENTH, LANG_OPARENTH_LEN)) {
    c->err = COMPILERERR_MALFORMED_IFORLOOP;
    return false;
  }

  token = lexer_next(l, &type, &len);

  /* store address where argument is pushed to stack, and 
   * compile argument code and get number of args 
   */
  beforeWhileAddr = buffer_size(c->outBuffer);
  argCount = parse_arguments(c, l, token, type, len);

  /* check for proper number of arguments */
  if(argCount != 1) {
    c->err = COMPILERERR_MALFORMED_IFORLOOP;
    return true;
  }

  /* fill jump instruction with placeholder bytes since we don't know the
   * end of the function address yet
   */
  buffer_append_char(c->outBuffer, OP_FCOND_GOTO);
  jumpInstAddr = buffer_size(c->outBuffer);
  buffer_append_string(c->outBuffer, (char*)(&address), sizeof(int));

  /* retrieve current token */
  token = lexer_current_token(l, &type, &len);

  if(!parse_body_statement(c, l)) {
    c->err = COMPILERERR_EXPECTED_OBRACKET;
    return true;
  }

  /* write jump to beginning of loop instruction */
  buffer_append_char(c->outBuffer, OP_GOTO);
  buffer_append_string(c->outBuffer, (char*)(&beforeWhileAddr), sizeof(int));

  /* write jump to end of body address for while statement */
  address = buffer_size(c->outBuffer);
  buffer_set_string(c->outBuffer, (char*)&address, sizeof(int), jumpInstAddr);

  return true;
}

/**
 * Parses while statements in script code, starting at the initial "while" token
 * and dispatches subparsers as needed until done.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * returns: true if this is a while statement, regardless of error. If error,
 * c->err is set.
 */
static bool parse_do_while_statement(Compiler * c, Lexer * l) {
  char * token;
  size_t len;
  LexerType type;
  int beforeDoAddr;
  int argCount = 0;

  token = lexer_current_token(l, &type, &len);

  /* check if this is a do while statement */
  if(!tokens_equal(token, len, LANG_DO, LANG_DO_LEN)) {
    return false;
  }

  token = lexer_next(l, &type, &len);

  /* store address before do block */
  beforeDoAddr = buffer_size(c->outBuffer);

  /* compile whatever is in our do statement/block */
  if(!parse_body_statement(c, l)) {
    c->err = COMPILERERR_EXPECTED_OBRACKET;
    return true;
  }

  /* update token, it was advanced by parse_body_statement */
  token = lexer_current_token(l, &type, &len);

  /* check for the while statement */
  if(!tokens_equal(token, len, LANG_WHILE, LANG_WHILE_LEN)) {
    c->err =  COMPILERERR_MALFORMED_IFORLOOP;
    return true;
  }

  token = lexer_next(l, &type, &len);

  /* check for an open parenthesis token */
  if(!tokens_equal(token, len, LANG_OPARENTH, LANG_OPARENTH_LEN)) {
    c->err = COMPILERERR_MALFORMED_IFORLOOP;
    return false;
  }

  token = lexer_next(l, &type, &len);

  /* compile the arguments */
  argCount = parse_arguments(c, l, token, type, len);

  /* get current token, it was updated by parse_arguments */
  token = lexer_current_token(l, &type, &len);

  /* check for proper number of arguments */
  if(argCount != 1) {
    c->err = COMPILERERR_MALFORMED_IFORLOOP;
    return true;
  }

  /* write the jump address */
  buffer_append_char(c->outBuffer, OP_TCOND_GOTO);
  buffer_append_string(c->outBuffer, (char*)(&beforeDoAddr), sizeof(int));

  /* check for a ';' token */
  if(!tokens_equal(token, len, LANG_ENDSTATEMENT, LANG_ENDSTATEMENT_LEN)) {
    c->err = COMPILERERR_EXPECTED_ENDSTATEMENT;
    return false;
  }

  token = lexer_next(l, &type, &len);

  return true;
}

/**
 * Parses if statements in script code, starting at the initial "if" token
 * and dispatches subparsers as needed until done.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * returns: true if this is an if statement, regardless of error. If error,
 * c->err is set.
 */
static bool parse_if_statement(Compiler * c, Lexer * l) {
  char * token;
  size_t len;
  LexerType type;
  int ifJumpInstAddr;
  int elseJumpInstAddr;
  int address = 0;
  int argCount = 0;

  token = lexer_current_token(l, &type, &len);

  /* check if this is an if statement */
  if(!tokens_equal(token, len, LANG_IF, LANG_IF_LEN)) {
    return false;
  }

  token = lexer_next(l, &type, &len);

  /* check for an open parenthesis token */
  if(!tokens_equal(token, len, LANG_OPARENTH, LANG_OPARENTH_LEN)) {
    c->err = COMPILERERR_MALFORMED_IFORLOOP;
    return true;
  }

  token = lexer_next(l, &type, &len);

  /* compile argument code and get number of args */
  argCount = parse_arguments(c, l, token, type, len);

  /* check for proper number of arguments */
  if(argCount != 1) {
    c->err = COMPILERERR_MALFORMED_IFORLOOP;
    return true;
  }

  /* fill jump instruction with placeholder bytes since we don't know the
   * end of the function address yet
   */
  buffer_append_char(c->outBuffer, OP_FCOND_GOTO);
  ifJumpInstAddr = buffer_size(c->outBuffer);
  buffer_append_string(c->outBuffer, (char*)(&address), sizeof(int));

  /* retrieve current token */
  token = lexer_current_token(l, &type, &len);

  /* do if body or statement */
  if(!parse_body_statement(c, l)) {
    return true;
  }

  token = lexer_current_token(l, &type, &len);

  /* check if this if block has an else as well */
  if(!tokens_equal(token, len, LANG_ELSE, LANG_ELSE_LEN)) {
    /* write jump address for if statement */
    address = buffer_size(c->outBuffer);
    buffer_set_string(c->outBuffer, (char*)&address, sizeof(int), ifJumpInstAddr);
    return true;
  }

  /* fill jump instruction with placeholder bytes since we don't know the
   * end of the function address yet
   */
  buffer_append_char(c->outBuffer, OP_GOTO);
  elseJumpInstAddr = buffer_size(c->outBuffer);
  buffer_append_string(c->outBuffer, (char*)(&address), sizeof(int));

  /* write jump address for if statement so that it is after the else jump */
  address = buffer_size(c->outBuffer);
  buffer_set_string(c->outBuffer, (char*)&address, sizeof(int), ifJumpInstAddr);

  token = lexer_next(l, &type, &len);

  /* do else body or statement */
  if(!parse_body_statement(c, l)) {
    c->err = COMPILERERR_EXPECTED_OBRACKET;
    return true;
  }

  /* write jump address for else statement */
  address = buffer_size(c->outBuffer);
  buffer_set_string(c->outBuffer, (char*)&address, sizeof(int), elseJumpInstAddr);

  return true;
}

/**
 * Parses a function call in script code, starting at the function name token
 * and dispatches subparsers as needed until done.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * noPop: pointer to a boolean that receives true if a "return" statement was
 * encountered and there should be no OP_POP to pop the return value of the
 * function.
 * returns: true if this is a function call, regardless of error. If error,
 * c->err is set.
 */
static bool parse_function_call(Compiler * c, Lexer * l, bool * noPop) {
  int argCount = 0;
  char * functionToken = NULL;
  size_t functionTokenLen = 0;
  char * token;
  size_t len;
  LexerType type;

  token = lexer_current_token(l, &type, &len);

  /* check if first token is a keyvar */
  if(type != LEXERTYPE_KEYVAR) {
    return false;
  }

  /* peek ahead one token */
  token = lexer_peek(l, NULL, &len);

  /* check if peeked token is a parenth */
  if(!tokens_equal(token, len,LANG_OPARENTH, LANG_OPARENTH_LEN)) {
    return false;
  }

  /* save function name */
  functionToken = lexer_current_token(l, NULL, &functionTokenLen);

  /* skip to the arguments tokens */
  token = lexer_next(l, &type, &len);
  token = lexer_next(l, &type, &len);

  argCount = parse_arguments(c, l, token, type, len);

  /* writes a call to the specified function, or returns if error */
  function_call(c, functionToken, functionTokenLen, argCount, noPop);

  return true;
}

/**
 * Writes OP codes to assign the top value on the stack to the specified
 * variable.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * variable: variable to recv the value.
 * variableLen: the length of variable string in chars.
 * returns: true if success, false if error.
 */
static bool assignment(Compiler * c, Lexer * l, char * variable, 
		       size_t variableLen) {

  DSValue value;
  char i = 0;
  HT * ht = symtblstk_peek(c, i);

  /* get variable depth */
  for(i = 0; true; i++, ht = symtblstk_peek(c, i)) {

    /* reached bottom of stack, variable not found */
    if(ht == NULL) {
      c->err = COMPILERERR_UNDEFINED_VARIABLE;
      return false;
    }

    /* found variable in this stack, stop iterating */
    if(ht_get_raw_key(ht, variable, variableLen, &value)) {
      break;
    }
  }

  /* write the variable data OPCodes
   * Moves the last value from the OP stack in the VM to the variable
   * storage slot in the frame stack. */
  buffer_append_char(c->outBuffer, OP_VAR_STOR);
  buffer_append_char(c->outBuffer, i);
  buffer_append_char(c->outBuffer, value.intVal);

  return true;
}

/**
 * Parses assignment statements in script code, starting at the variable token
 * and dispatches subparsers as needed until done.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * returns: true if this is an assignment, regardless of error. If error,
 * c->err is set.
 */
static bool parse_assignment_statement(Compiler * c, Lexer * l) {
  char * token;
  size_t len;
  LexerType type;

  char * varToken;
  size_t varTokenLen;

  /* get the current token */
  token = lexer_current_token(l, &type, &len);

  /* check if first token is a keyvar */
  if(type != LEXERTYPE_KEYVAR) {
    return false;
  }

   /* peek ahead one token */
  token = lexer_peek(l, NULL, &len);

  /* check if peeked token is an assignment statement */
  if(!tokens_equal(token, len, LANG_OP_ASSIGN, LANG_OP_ASSIGN_LEN)) {
    return false;
  }

  /* save variable name */
  varToken = lexer_current_token(l, NULL, &varTokenLen);

  /* skip to the value tokens */
  token = lexer_next(l, &type, &len);
  token = lexer_next(l, &type, &len);

  /* do line of code following '=" */
  if(!parse_straight_code(c, l, false, NULL)) {
    return true;
  }

  /* do assignment...return if fails..but we're already done, return anyways */
  assignment(c, l, varToken, varTokenLen);
  return true;
}

/**
 * Writes OP codes to push the specified variable onto the VM operand stack.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * variable: the variable to reference.
 * variableLen: the length of the variable in chars.
 * returns: true if this is an assignment, regardless of error. If error,
 * c->err is set.
 */
static bool reference(Compiler * c, Lexer * l, 
		      char * variable, size_t variableLen) {

  DSValue value;
  char i = 0;
  char varSlot;
  HT * ht = symtblstk_peek(c, i);

  /* get variable depth */
  for(i = 0; true; i++, ht = symtblstk_peek(c, i)) {

    /* reached bottom of stack, variable not found */
    if(ht == NULL) {
      c->err = COMPILERERR_UNDEFINED_VARIABLE;
      return false;
    }

    /* found variable in this stack, stop iterating */
    if(ht_get_raw_key(ht, variable, variableLen, &value)) {
      break;
    }
  }

  /* write the variable data read OPCodes
   * Moves the last value from the specified depth and slot of the frame
   * stack in the VM to the VM OP stack.
   */
  /* TODO: need to add ability to search LOWER frames for variables */
  varSlot = value.intVal;

  buffer_append_char(c->outBuffer, OP_VAR_PUSH);
  buffer_append_char(c->outBuffer, i);
  buffer_append_char(c->outBuffer, varSlot);

  return true;
}

/**
 * Parses constants such as "true" and "false".
 * c: an instance of compiler.
 * l: an instance of lexer.
 * returns: true if current token is a constant, and false if not.
 */
static bool parse_static_constant(Compiler * c, Lexer * l) {

  char * token;
  size_t len;
  LexerType type;

  /* get current token */
  token = lexer_current_token(l, &type, &len);

  /* check if current token is one of the possible static constants */
  if(tokens_equal(token, len, LANG_TRUE, LANG_TRUE_LEN)) {
    buffer_append_char(c->outBuffer, OP_BOOL_PUSH);
    buffer_append_char(c->outBuffer, true);
  } else if(tokens_equal(token, len, LANG_FALSE, LANG_FALSE_LEN)) {
    buffer_append_char(c->outBuffer, OP_BOOL_PUSH);
    buffer_append_char(c->outBuffer, false);
  } else if(tokens_equal(token, len, LANG_NULL, LANG_NULL_LEN)) {
    buffer_append_char(c->outBuffer, OP_NULL_PUSH);
  } else {
    return false;
  }

  /* advance token */
  token = lexer_next(l, &type, &len);

  return true;
}

/**
 * Parses constants variable references and writes OP codes for pushing the
 * variable to the VM stack.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * returns: true if current token is a variable reference, and false if not.
 */
static bool parse_variable_reference(Compiler * c, Lexer * l) {
  char * token;
  size_t len;
  LexerType type;

  /* get the current token */
  token = lexer_current_token(l, &type, &len);

  /* peek ahead one token */
  token = lexer_peek(l, NULL, &len);

  /* make sure this is not an assignment statement or function call */
  if(tokens_equal(token, len, LANG_OP_ASSIGN, LANG_OP_ASSIGN_LEN)
     || tokens_equal(token, len, LANG_OPARENTH, LANG_OPARENTH_LEN)) {
    return false;
  }

  /* get the current token */
  token = lexer_current_token(l, &type, &len);

  /* write the variable push code, return if fail */
  if(!reference(c, l, token, len)) {
    return true;
  }
  
  /* advance token */
  token = lexer_next(l, &type, &len);
  
  return true;
}

/**
 * Attempts to parse current location as a line of code. First, checks if this
 * is a function call. If so, dispatches subparsers to handle the arguments. If
 * not, other subparsers are dispatched until one is found that can correctly 
 * interpret the code.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * returns: false if an error occurs and sets c->err to the error code.
 */
static bool parse_line(Compiler * c, Lexer * l, bool innerCall) {
  bool noPop = false;

  /* feed line through various sub-parsers until one knows what to do */
  if(parse_function_call(c, l, &noPop)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  } else if(parse_assignment_statement(c, l)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
    noPop = true;
  } else if(parse_static_constant(c, l)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  } else if(parse_variable_reference(c, l)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  } else {

    /* nothing else worked, delegate to the straight code parser */
    if(!parse_straight_code(c, l, false, NULL)) {
      return false;
    }
  }

  /* if noPop is false (this line is NOT a return value): */
  if(!noPop && !innerCall) {
    /* pop line return value off of stack */
    buffer_append_char(c->outBuffer, OP_POP);
  }
  return true;
}

/**
 * Subparser for define_variables(). Performs definition of variables.
 * Looks at the current token. If it is 'var', the parser takes over and starts
 * defining the current variable. It defines ONLY 1 and returns. To define
 * additional, you must make multiple calls to this subparser.
 * Variable declarations take the form:
 *   var [variable_name];
 * c: an instance of Compiler.
 * l: an instance of lexer. The current token must be 'var' or else the function
 * will do nothing.
 * c: an instance of Compiler.
 * l: an instance of Lexer that will be used to parse the variable declarations.
 * returns: Returns true if the current token in Lexer l is 'var' and false if
 * the current token is something else. c->err variable is set to an error code
 * upon a mistake in the script or other error.
 */
static bool define_variable(Compiler * c, Lexer * l) {

  /* variable declarations: 
   *
   * var [variable_name];
   */
  HT * symTbl = symtblstk_peek(c, 0);
  LexerType type;
  char * token;
  size_t len;
  char * varName;
  size_t varNameLen;
  bool prevExisted;
  DSValue newValue;

  /* get current token cached in the lexer */
  token = lexer_current_token(l, &type, &len);

  /* make sure next token is a variable decl. keyword, otherwise, return */
  if(!tokens_equal(LANG_VAR_DECL, LANG_VAR_DECL_LEN, token, len)) {
    return false;
  }

  /* check that next token is a keyvar type, neccessary for variable names */
  varName = lexer_next(l, &type, &varNameLen);
  if(type != LEXERTYPE_KEYVAR) {
    c->err = COMPILERERR_EXPECTED_VARNAME;
    return true;
  }

  /* store variable along with index at which its data will be stored in the
   * frame stack in the virtual machine
   */
  newValue.intVal = ht_size(symTbl);
  if(!ht_put_raw_key(symTbl, varName, varNameLen,
		     &newValue, NULL, &prevExisted)) {
    c->err = COMPILERERR_ALLOC_FAILED;
    return true;
  }

  /* check for duplicate var names */
  if(prevExisted) {
    c->err = COMPILERERR_PREV_DEFINED_VAR;
    return true;
  }

  /* perform inline variable initialization if code provided */
  /*token = lexer_current_token(l, &type, &len);
  printf("Var init token: %s\n", token);
  if(!parse_function_call(c, l, NULL)) {
    return false;
    }*/

  token = lexer_next(l, &type, &len);
  /* check for terminating semicolon */
  if(type != LEXERTYPE_ENDSTATEMENT) {
    c->err = COMPILERERR_EXPECTED_ENDSTATEMENT;
    return false;
  }
  token = lexer_next(l, &type, &len);

  return true;
}

/**
 * Subparser for parse_function_definitions(). Parses function definitions. Where
 * define_variable() function defines ONE variable from a declaration, this function
 * continues defining variable until the first non-variable declaration token
 * is found.
 * c: an instance of Compiler.
 * l: an instance of lexer that provides the tokens being parsed.
 * returns: true if the variables were declared successfully, or there were
 * no variable declarations, and false if an error occurred. Upon an error,
 * c->err is set to a relevant error code.
 */
int define_variables(Compiler * c, Lexer * l) {
  LexerType type;
  char * token;
  size_t len;
  int varCount = 0;
 
  token = lexer_current_token(l, &type, &len);
  /* variable declarations look like so:
   * var [variable_name_1];
   * var [variable_name_2];
   */
  do {
    if(define_variable(c, l)) {
      varCount++;
      if(c->err != COMPILERERR_SUCCESS) {
	return -1;
      }
    } else {
      /* no more variables, leave the loop */
      break;
    }
  } while((token = lexer_current_token(l, &type, &len)) != NULL);

  return varCount;
}

/**
 * Evaluates a single line of function body code. In other words, all code
 * that may be found within a function body, including loops, ifs, straight 
 * code, assignment statements, but only one line, or one structure (if, while),
 * etc.
 * c: an instance of compiler.
 * l: an instance of lexer that will provide all tokens that will be parsed.
 * returns: true if successful, and false if an error occurs. In the case of
 * an error, c->err is set to a relevant error code.
 */
bool parse_body_statement(Compiler * c, Lexer * l) {
  LexerType type;
  size_t len;

  /* attempt each logical structure subparser, one by one */
  if(parse_if_statement(c, l)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  } else if(parse_block(c, l)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  } else if(parse_while_statement(c, l)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  } else if(parse_do_while_statement(c, l)) {
    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  } else {

    /* not a logical structure, evaluate as normal line of code */
    if(!parse_line(c, l, false)) {
      return false;
    }

    /* check for terminating semicolon */
    lexer_current_token(l, &type, &len);
    if(type != LEXERTYPE_ENDSTATEMENT) {
      c->err = COMPILERERR_EXPECTED_ENDSTATEMENT;
      return false;
    }
    lexer_next(l, &type, &len);
  }
  return true;
}

/**
 * Evaluates function body code. In other words, all code that may be found
 * within a function body, including loops, ifs, straight code, assignment
 * statements, but excluding variable declarations.
 * c: an instance of compiler.
 * l: an instance of lexer that will provide all tokens that will be parsed.
 * returns: true if successful, and false if an error occurs. In the case of
 * an error, c->err is set to a relevant error code.
 */
bool parse_body(Compiler * c, Lexer * l) {
  LexerType type;
  char * token;
  size_t len;

  token = lexer_current_token(l, &type, &len);

  /* keep executing lines of code until we hit a '}' */
  while(!tokens_equal(token, len, LANG_CBRACKET, LANG_CBRACKET_LEN)) {
    if(!parse_body_statement(c, l)) {
      return false;
    }
    token = lexer_current_token(l, &type, &len);
  }

  /* TODO: throw error if method doesn't end with curly brace */
  token = lexer_current_token(l, &type, &len);
 
  return true;
}

/**
 * Parses a block of code (encapsulated by "{" and "}") and pushes a new frame
 * to the stack so that this body of code has its own limited scope.
 * c: an instance of compiler.
 * l: an instance of lexer.
 * returns: true if this is a block, and false if the current token is not the
 * start of a block. c->err is set on an error.
 */
bool parse_block(Compiler * c, Lexer * l) {
  char * token;
  size_t len;
  LexerType type;
  int varCount = 0;
  int varCountAddr = 0;

  /* get current token */
  token = lexer_current_token(l, &type, &len);

  /* check if this is a block */
  if(!tokens_equal(token, len, LANG_OBRACKET, LANG_OBRACKET_LEN)) {
    return false;
  }

  /* get next token */
  token = lexer_next(l, &type, &len);

  /* we're going down a level. push new symbol table to stack */
  if(!symtblstk_push(c)) {
    c->err = COMPILERERR_ALLOC_FAILED;
    return true;
  }

  /* write push frame stack OP codes...we don't know the number of variables yet,
   * so we save the address of the number of args for pushing and push 0 to fill the
   * space.
   */
  buffer_append_char(c->outBuffer, OP_FRM_PUSH);
  varCountAddr = buffer_size(c->outBuffer);
  buffer_append_char(c->outBuffer, 0);

  /* define variables, return on error */
  if((varCount = define_variables(c, l)) == -1) {
    return true;
  }

  /* save number of variables */
  buffer_set_char(c->outBuffer, (char)varCount, varCountAddr);

  /* parse code in block */
  if(!parse_body(c, l)) {
    return true;
  }

  /* retrieve next token (it was modified by define_variables and parse_body */
  token = lexer_current_token(l, &type, &len);

  /* check for closing brace defining end of block "}" */
  if(!tokens_equal(token, len, LANG_CBRACKET, LANG_CBRACKET_LEN)) {
    c->err = COMPILERERR_EXPECTED_CBRACKET;
    return true;
  }

  /* pop block frame */
  buffer_append_char(c->outBuffer, OP_FRM_POP);

  /* we're done here! pop the symbol table for this block off the stack. */
  ht_free(symtblstk_pop(c));

  lexer_next(l, &type, &len);
  return true;
}
