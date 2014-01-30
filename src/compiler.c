/**
 * compiler.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The compiler is the central component in the translation of the scripting
 * code into bytecode that the VM can understand. It is/will be designed in the
 * form of a recursive descent parser, similar in design to the lexer, that
 * uses the lexer object to tokenize the input code and feeds it into a cascade
 * of subparsers that automatically assume control in the correct context, or
 * pass control to the next subparser if they are unable to handle the current
 * situation. In this way, the complexity of the code can be managed.
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


/**
 * The QUALITY of code in THIS DOCUMENT is very low at the moment because it
 * is a work in progress. Pardon the dust.
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include "compiler.h"
#include "lexer.h"
#include "langkeywords.h"
#include "vmdefs.h"
#include "typestk.h"

/* preprocessor definitions: */
#define COMPILER_NO_PREV        -1

/* TODO: make STK type auto enlarge and remove */
static const int initialOpStkDepth = 100;
/* number of items to add to the op stack on resize */
static const int opStkBlockSize = 12;
/* TODO: make symTableStk auto expand and remove this */
static const int maxFuncDepth = 100;
/* initial size of all hashtables */
static const int initialHTSize = 11;
/* number of items to add to hashtable on rehash */
static const int HTBlockSize = 12;
/* hashtable load factor upon which it will be rehashed */
static const float HTLoadFactor = 0.75;
/* number of bytes in each additional block of the buffer */
static const int sbBlockSize = 100;
/* max number of digits in a number value */
static const int numValMaxDigits = 50;

/* !!UNDER CONSTRUCTION!! */

/**
 * Creates a new compiler object that will contain the current state of the
 * compiler and its data structures.
 * returns: new compiler object, or NULL if the allocation fails.
 */
Compiler * compiler_new() {
  Compiler * compiler = calloc(1, sizeof(Compiler));

  /* check for failed allocation */
  if(compiler == NULL) {
    return NULL;
  }

  /* TODO: make this stack auto expand when full */
  compiler->symTableStk = stk_new(maxFuncDepth);
  compiler->functionHT = ht_new(initialHTSize, sbBlockSize, HTLoadFactor);
  compiler->outBuffer = sb_new(sbBlockSize);

  /* check for further malloc errors */
  if(compiler->symTableStk == NULL 
     || compiler->functionHT == NULL 
     || compiler->outBuffer == NULL) {
    compiler_free(compiler);
    return NULL;
  }

  return compiler;
}

/* checks that two tokens are equal and not NULL */
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
static bool tokens_equal(char * token1, size_t num1,
			 char * token2, size_t num2) {

  if((token1 != NULL && token2 != NULL) && (num1 == num2)) {
    return strncmp(token1, token2, num1) == 0;
  }
  return false;
}

/**
 * Instantiates a CompilerFunc structure for storing information about a script
 * function in the functionHT member of Compiler struct. This struct is used to
 * store record of a function declaration, its number of arguments, and its
 * respective location in the bytecode.
 * name: A string with the text representation of the function. The text name
 * it is called by in the code.
 * nameLen: The number of characters to read from name for the function name.
 * index: the index of the byte where the function begins in the byte code.
 * numArgs: the number of arguments that the function expects.
 * returns: A new instance of CompilerFunc struct, NULL if the allocation fails.
 */
static CompilerFunc * compilerfunc_new(char * name, size_t nameLen,
				       int index, int numArgs) {
  assert(index >= 0);

  CompilerFunc * cf = calloc(1, sizeof(CompilerFunc));
  if(cf != NULL) {
    cf->name = calloc(nameLen + 1, sizeof(char));
    strncpy(cf->name, name, nameLen);
    cf->index = index;
    cf->numArgs = numArgs;
  }

  return cf;
}

/**
 * Frees a CompilerFunc struct. This must be done to every CompilerFunc
 * struct when it is removed from the hashtable.
 * cf: an instance of CompilerFunc to free the associated memory to.
 */
static void compilerfunc_free(CompilerFunc * cf) {

  assert(cf != NULL);

  free(cf->name);
  free(cf);
}

/* pushes another symbol table onto the stack of symbol tables
 * returns true if success, false if malloc fails
 */
/**
 * Pushes a new symbol table onto the stack of symbol tables. The symbol table
 * is a structure that contains records of all variables and their respective
 * locations in the stack in the VM. The symbol table's position in the stack
 * represents the VM stack frame's position in the stack as well.
 * c: an instance of Compiler that will receive the new frame.
 * returns: true if success, false if an allocation failure occurs.
 */
static bool symtblstk_push(Compiler * c) {

  assert(c != NULL);

  HT * symTbl = ht_new(initialHTSize, HTBlockSize, HTLoadFactor);

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
 * without actually removing the table from the stack. This function is useful
 * for getting a reference to the current frame's symbol table so that you can
 * look up its symbols.
 * c: an instance of Compiler.
 * returns: a pointer to the top symbol table hashtable, or NULL if the stack
 * is empty.
 */
static HT * symtblstk_peek(Compiler * c) {
  DSValue value;

  /* check that peek was success */
  if(!stk_peek(c->symTableStk, &value)) {
    return NULL;
  }

  return value.pointerVal;
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
static HT * symtblstk_pop(Compiler * c) {
  DSValue value;

  /* check that peek was success */
  if(!stk_pop(c->symTableStk, &value)) {
    return NULL;
  }

  return value.pointerVal;
}

/**
 * A wrapper function that frees a symbol table. This function should be used
 * after a symbol table is popped from the symbol table stack with
 * symtblstk_pop(). Do NOT free symbol table references obtained with
 * symtblstk_peek(). They are still being used. SEGFAULTS will ensue.
 * symTbl: a hashtable/symbol table to free.
 */
static void symtbl_free(HT * symTbl) {
  assert(symTbl != NULL);

  ht_free(symTbl);
}

/**
 * Checks a string to see if it is a reserved keyword for the scripting language
 * At the moment this function is a place holder for the keyword check
 * functionality. 
 * TODO: make more sophisticated, constant time lookup mechanism.
 * token: the string to check for being a keyword.
 * tokenLen: the number of characters to compare from the token string.
 * returns: true if token is a keyword, and false if it is not.
 */
static bool is_keyword(char * token, size_t tokenLen) {

  assert(token != NULL);
  assert(tokenLen > 0);

  if(tokens_equal(LANG_FUNCTION, LANG_FUNCTION_LEN, token, tokenLen)) {
    return true;
  }

  return false;
}

/**
 * A build_parse_func_def() subparser that parses the arguments from a function
 * definition. The function then stores the arguments in the current symbol
 * table and returns the number of arguments that were provided.
 * c: an instance of Compiler.
 * l: an instance of lexer.
 * returns: if the current token is a LEXERTYPE_KEYVAR, the parser starts to
 * parse function arguments. If not, it returns -1 and sets c->err to 
 * COMPILERERR_SUCCESS. If an error occurred, or there is a mistake in the
 * script, the function returns -1, but c->err is set to a relevant error code.
 */
static int func_defs_parse_args(Compiler * c, Lexer * l) {

  assert(c != NULL);
  assert(l != NULL);

  /* while the next token is not an end parenthesis and tokens remain, parse
   * the tokens and store each KEYVAR type token in the symbol table as a
   * function argument
   */
  HT * symTbl = symtblstk_peek(c);
  LexerType type;
  size_t len;
  bool prevExisted;
  char * token = lexer_next(l, &type, &len);
  int numArgs = 0;

  while(true) {

    /* check current token is an argument, if not throw a fit */
    if(token == NULL || type != LEXERTYPE_KEYVAR) {

      /* if there was a closed parenth, there are no args, otherwise, give err */
      printf("TOKENS EQS '%s'\n", token);
      if(tokens_equal(LANG_CPARENTH, LANG_CPARENTH_LEN, token, len)) {
	return 0;
      } else {
	c->err = COMPILERERR_EXPECTED_VARNAME;
	printf("COMPILERERR_EXPECTED_VARNAME\n");
	return -1;
      }
    }

    /* store variable along with index at which its data will be stored in the
     * frame stack in the virtual machine
     */
    if(!ht_put_int(symTbl, token, ht_size(symTbl), NULL, &prevExisted)) {
      c->err = COMPILERERR_ALLOC_FAILED;
      printf("COMPILER_ALLOC_FAILED\n");
      return -1;
    }
    numArgs++;

    /* check for duplicate var names */
    if(prevExisted) {
      c->err = COMPILERERR_PREV_DEFINED_VAR;
      printf("COMPILERERR_PREV_DEFINED_VAR\n");
      return -1;
    }

    /* get next token and check for anything but a comma arg delimiter */
    token = lexer_next(l, &type, &len);
    if(!tokens_equal(token, len, LANG_ARGDELIM, LANG_ARGDELIM_LEN)) {

      printf("Not Arg delim.\n");

      /* check for end of the args, or invalid token */
      if(tokens_equal(token, len, LANG_CPARENTH, LANG_CPARENTH_LEN)) {

	printf("End of Args\n");
	/* end of args */
	return numArgs;
      } else {
	printf("Unexpected token in args.");
	c->err = COMPILERERR_UNEXPECTED_TOKEN;
	return -1;
      }
    } else {
      /* skip over the comma / arg delimiter */
      token = lexer_next(l, &type, &len);
    }
  }

  return -1;
}

/**
 * Records a function declaration and stores the function's text name, callable
 * from the script, the number of arguments that the function accepts (we don't
 * need to record the type of the arguments because the language is dynamically
 * typed). These records are stored in the functionHT in the compiler struct and
 * can be referred to during function calls to verify proper usage of the
 * functions.
 * c: an instance of Compiler.
 * name: the string name of the function. This is the text that can be used to
 * call the function. e.g. print.
 * nameLen: the number of characters to read from name.
 * numArgs: the number of arguments that the function can accept.
 */
static bool func_store_def(Compiler * c, char * name, size_t nameLen,
			   int numArgs) {

  /* TODO: might need a lexer_next() call to get correct token */
  bool prevValue;
  CompilerFunc * cp;

  /* check for proper CompilerFunc allocation */
  cp = compilerfunc_new(name, nameLen, sb_size(c->outBuffer), numArgs);
  if(cp == NULL) {
    c->err = COMPILERERR_ALLOC_FAILED;
    return false;
  }

  ht_put_pointer(c->functionHT, cp->name, cp, NULL, &prevValue);

  /* check that function didn't previously exist */
  if(prevValue) {
    c->err = COMPILERERR_PREV_DEFINED_FUNC;
    return false;
  }

  return true;
}

/* returns true if current token is start of a variable declaration expression */

/**
 * Subparser for func_do_var_defs(). Performs definition of variables.
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
static bool var_def(Compiler * c, Lexer * l) {

  /* variable declarations: 
   *
   * var [variable_name];
   */
  HT * symTbl = symtblstk_peek(c);
  LexerType type;
  char * token;
  size_t len;
  char * varName;
  size_t varNameLen;
  bool prevExisted;
  DSValue newValue;

  /* get current token cached in the lexer */
  token = lexer_current_token(l, &type, &len);

  printf("   VAR DEF REACHED. Token: '%s'\n", token);
  printf("Token Len: %i\n", len);
  /* make sure next token is a variable decl. keyword, otherwise, return */
  if(!tokens_equal(LANG_VAR_DECL, LANG_VAR_DECL_LEN, token, len)) {
    return false;
  }

  printf("VAR TOKEN PROVIDED.\n");
  /* check that next token is a keyvar type, neccessary for variable names */
  varName = lexer_next(l, &type, &varNameLen);
  if(type != LEXERTYPE_KEYVAR) {
    c->err = COMPILERERR_EXPECTED_VAR_NAME;
    return true;
  }

  /**************************************************************************
   * TODO: place code for handling variable initialization
   **************************************************************************/

  /* check for terminating semicolon */
  token = lexer_next(l, &type, &len);
  if(type != LEXERTYPE_ENDSTATEMENT) {
    c->err = COMPILERERR_EXPECTED_ENDSTATEMENT;
    return true;
  }
  printf("Semicolon providided.\n");

  /* store variable along with index at which its data will be stored in the
   * frame stack in the virtual machine
   */
  newValue.intVal = ht_size(symTbl);
  if(!ht_put_raw_key(symTbl, varName, varNameLen,
		     &newValue, NULL, &prevExisted)) {
    c->err = COMPILERERR_ALLOC_FAILED;
    return true;
  }

  printf("Stored in HT: VAR '%s'\n", varName);

  /* check for duplicate var names */
  if(prevExisted) {
    c->err = COMPILERERR_PREV_DEFINED_VAR;
    printf("PREV EXISTED\n");
    return true;
  } else {
    printf("Not PREV EXISTED.\n");
  }

  return true;
}

/* performs variable declarations */
/**
 * Subparser for build_parse_func_defs(). Parses function definitions. Where
 * var_def() function defines ONE variable from a declaration, this function
 * continues defining variable until the first non-variable declaration token
 * is found.
 * c: an instance of Compiler.
 * l: an instance of lexer that provides the tokens being parsed.
 * returns: true if the variables were declared successfully, or there were
 * no variable declarations, and false if an error occurred. Upon an error,
 * c->err is set to a relevant error code.
 */
static bool func_do_var_defs(Compiler * c, Lexer * l) {
  LexerType type;
  char * token;
  size_t len;
 
  /* variable declarations look like so:
   * var [variable_name_1];
   * var [variable_name_2];
   */
  while((token = lexer_next(l, &type, &len)) != NULL) {
    printf("func_do_var_defs next token: '%s'\n", token);
    printf("func_do_var_defs next token len: '%i'\n", len);
    if(var_def(c, l)) {
      if(c->err != COMPILERERR_SUCCESS) {
	return false;
      }
    } else {
      /* no more variables, leave the loop */
      break;
    }
  }

  return true;
}

/**
 * Place holder for a function that gets the precedence of an operator.
 * operator: the operator to check for precedence.
 * operatorLen: the number of characters to read from operator as the operator.
 * TODO: possibly reimplement using a hash set for multicharacter operators.
 * returns: an integer that represents an operator's precedence. Higher is
 * greater. Returns 1 if unknown operator.
 */
static int operator_precedence(char * operator, size_t operatorLen) {

  printf("OPERATOR: %s\n", operator);
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
static int topstack_precedence(Stk * stk, Stk * lenStk) {

  DSValue value;
  char * token;
  size_t len;

  /* get the token from the operator stack */
  if(!stk_peek(stk, &value)) {
    return 0;
  }
  token = value.pointerVal;

  /* get the token length from the operator length stack */
  if(!stk_peek(lenStk, &value)) {
    return 0;
  }
  len = value.longVal;

  /* return the precedence of the operator */
  return operator_precedence(token, len);
}

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

/* returns false if invalid operator encountered */
/**
 * Writes all operators from the provided stack to the output buffer in the 
 * Compiler instance.
 * c: an instance of Compiler.
 * opStk: the stack of operator strings.
 * opLenStk: the stack of operator string lengths, in longs.
 * parenthExpected: tells whether or not the calling function is
 * expecting a parenthesis. If it is and one is not encountered, the
 * function sets c->err = COMPILERERR_UNMATCHED_PARENTH and returns false.
 * returns: true if successful, and false if an unmatched parenthesis is
 * encountered.
 */
static bool write_operators_from_stack(Compiler * c, TypeStk * opStk, 
				       Stk * opLenStk, bool parenthExpected) {
  bool topOperator = true;
  DSValue value;
  char * token = NULL;
  size_t len = 0;
  OpCode opCode;
  VarType type;

  /* while items remain */
  while(typestk_size(opStk) > 0) {

    /* get operator string */
    typestk_pop(opStk, &token, sizeof(char*), &type);

    /* get operator length */
    stk_pop(opLenStk, &value);
    len = value.longVal;

    /* if there is an open parenthesis, stop popping  */
    if(tokens_equal(LANG_OPARENTH, LANG_OPARENTH_LEN, token, len)) {

      /* if top of stack is a parenthesis, it is a mismatch! */
      if(!parenthExpected) {
	printf("Unmatched parenth!\n");
	c->err = COMPILERERR_UNMATCHED_PARENTH;
	return false;
      } else {
	printf("DEBUG: TRUE\n");
	return true;
      }
    }

    /* handle OPERATORS and FUNCTIONS differently */
    if(type == LEXERTYPE_OPERATOR) {
      printf("DEBUG: writing operator from OPSTK %s\n", token);
      printf("DEBUG: writing operator from OPSTK LEN %i\n", len);

      /* if there is an '=' on the stack, this is an assignment,
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
	c->err = COMPILERERR_UNDEFINED_VARIABLE;
	return false;
      }

      /* write the variable data read OPCodes
       * Moves the last value from the specified depth and slot of the frame
       * stack in the VM to the VM OP stack.
       */
      /* TODO: need to add ability to search LOWER frames for variables */
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
    c->err = COMPILERERR_UNMATCHED_PARENTH;
    return false;
  }
}

/**
 * Handles straight code:
 * This is a modified version of Djikstra's "Shunting Yard Algorithm" for
 * conversion to postfix notation. Code is converted to postfix and written to
 * the output buffer as a series of OP codes all in one step.
 * c: an instance of Compiler.
 * l: an instance of lexer that will provide the tokens.
 * returns: true if success, and false if errors occurred. If error occurred,
 * c->err is set.
 */
/* TODO: return false for every sb_append* function upon failure */
static bool func_body_straight_code(Compiler * c, Lexer * l) {
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

  /* straight code token parse loop */
  do {
    bool popAfterNextVal = true;

    /* output values, push operators to stack with precedence so that they can
     * be postfixed for execution by the VM
     */
    switch(type) {

    case LEXERTYPE_KEYVAR: {
      /* KEYVAR HANDLER:
       * Handles all word tokens. These can be variables or function names
       */
      
      /* push variable or function name to operator stack */
      typestk_push(opStk, &token, sizeof(char*), type);
      stk_push_long(opLenStk, len);
      printf("KEYVAR pushed to OPSTK: %s\n", token);
      printf("KEYVAR pushed to OPSTK Len: %i\n", len);
      break;
    }

    case LEXERTYPE_NUMBER: {
      /* Reads a number, converts to double and writes it to output as so:
       * OP_NUM_PUSH [value as a double]
       */
      char rawValue[numValMaxDigits];
      double value;

      /* get double representation of number
      /* TODO: length check, and double overflow check */
      strncpy(rawValue, token, len);
      value = atof(rawValue);

      printf("Output Number Written: %f\n", value);

      /* write number to output */
      sb_append_char(c->outBuffer, OP_NUM_PUSH);
      sb_append_str(c->outBuffer, (char*)(&value), sizeof(value));

      /* check for invalid types: */
      if(prevValType != COMPILER_NO_PREV
	 && prevValType != LEXERTYPE_PARENTHESIS
	 && prevValType != LEXERTYPE_OPERATOR) {
	c->err = COMPILERERR_UNEXPECTED_TOKEN;
	return false;
      }

      break;
    }

    case LEXERTYPE_STRING: {
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
      break;
    }

    case LEXERTYPE_PARENTHESIS: {
      if(tokens_equal(LANG_OPARENTH, LANG_OPARENTH_LEN, token, len)) {
	typestk_push(opStk, &token, sizeof(char*), type);
	stk_push_long(opLenStk, len);
	printf("OParenth pushed to OPSTK: %s\n", token);
	printf("OParenth pushed to OPSTK Len: %i\n", len);
      } else {
	/* pop operators from stack and write to output buffer
	 * FAIL if there is no open parenthesis in the stack.
	 */
	if(!write_operators_from_stack(c, opStk, opLenStk, true)) {
	  return false;
	}
      }

      /* check for invalid types: */
      if(prevValType != COMPILER_NO_PREV
	 && prevValType != LEXERTYPE_PARENTHESIS
	 && prevValType != LEXERTYPE_OPERATOR
	 && prevValType != LEXERTYPE_NUMBER
	 && prevValType != LEXERTYPE_STRING) {
	c->err = COMPILERERR_UNEXPECTED_TOKEN;
	return false;
      }
      break;
    }

    case LEXERTYPE_OPERATOR: {
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
      if(operator_precedence(token, len)
	 >= topstack_precedence(opStk, opLenStk)) {
	
	/* push operator to operator stack */
	typestk_push(opStk, &token, sizeof(char*), type);
	stk_push_long(opLenStk, len);
	printf("Operator pushed to OPSTK: %s\n", token);
	printf("Operator pushed to OPSTK Len: %i\n", len);
      } else {

	/* pop operators from stack and write to output buffer */
	if(!write_operators_from_stack(c, opStk, opLenStk, false)) {
	  return false;
	}
      }

      /* check for invalid types: */
      if(prevValType == LEXERTYPE_OPERATOR 
	 || prevValType == LEXERTYPE_PARENTHESIS) {
	c->err = COMPILERERR_UNEXPECTED_TOKEN;
	return false;
      }

      break;
    }

    case LEXERTYPE_ENDSTATEMENT: {

      /* write stack pop instruction:
       * this ensures that the last return value is popped off of the stack after
       * the statement completes.
       */
      sb_append_char(c->outBuffer, OP_POP);
      /* check for invalid types: */
      if(prevValType == LEXERTYPE_OPERATOR
	 || prevValType == LEXERTYPE_ENDSTATEMENT) {
	c->err = COMPILERERR_UNEXPECTED_TOKEN;
	return false;
      }
      break;
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

  /* check for a semicolon at the end of the line */
  if(prevValType != LEXERTYPE_ENDSTATEMENT) {
    c->err = COMPILERERR_EXPECTED_ENDSTATEMENT;
    return false;
  }

  /* reached the end of the input, empty the operator stack to the output */
  if(!write_operators_from_stack(c, opStk, opLenStk, false)) {
    return false;
  }

  /* no errors occurred */
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
static bool func_do_body(Compiler * c, Lexer * l) {
  LexerType type;
  char * token;
  size_t len;

  token = lexer_current_token(l, &type, &len);

  printf("BODY START TOKEN: '%s'\n", token);
  printf("BODY START TOKEN LEN: %i\n", len);

  /* handle variable declarations */
  if(!func_do_var_defs(c, l)) {
    return false;
  }

  /* retrieve current token (it was modified by func_do_var_defs */
  token = lexer_current_token(l, &type, &len);

  printf("Remaining token: %s\n", token);

  /*
  while((token = lexer_next(l, &type, &len)) != NULL) {
  
    if(func_do_var_def(c, l)) {
      break;
    } else {
      printf("UNHANDLED SITUATION\n");
    }

    if(c->err != COMPILERERR_SUCCESS) {
      return false;
    }
  }
  */
  return true;
}

/**
 * A subparser function for compiler_build() that looks at the current token,
 * checks for a 'function' token. If found, it proceeds to evaluate the function
 * declaration, make note of the number of arguments, and store the index where
 * the function will begin in the byte code. This function then dispatches
 * subparsers that define stack variables, evaluate logical blocks (if, else,
 * while, etc), and evaluate straight code.
 * c: an instance of Compiler.
 * l: an instance of lexer. 
 * returns: false if the current token is not the start of a function declaration
 * ('function'), and true if it is. If an error occurs, function returns true,
 * but c->err is set to a relevant error code.
 */
static bool build_parse_func_defs(Compiler * c, Lexer * l) {

  /*
   * A Function definition looks like so:
   *
   * function [EXPORTED] [NAME] ( [ARG1], [ARG2], ... ) {
   *   [code]
   * }
   * 
   * The code below parses these tokens and then dispatches the straight code
   * parsers to handle the function body.
   */
   
  bool exported = false;
  size_t len;
  LexerType type;
  char * token = lexer_current_token(l, &type, &len);
  char * name;
  size_t nameLen;
  int numArgs;

  /* check that this is a function declaration token */
  if(!tokens_equal(token, len, LANG_FUNCTION, LANG_FUNCTION_LEN)) {
    return false;
  }

  printf("Relevant Token.\n");

  /* advance to next token. if is is EXPORTED, take note for later */
  token = lexer_next(l, &type, &len);
  if(tokens_equal(token, len, LANG_EXPORTED, LANG_EXPORTED_LEN)) {
    exported = true;
    token = lexer_next(l, &type, &len);
  }

  printf("Passed Exported. Had: %i\n", exported);

  /* this is the name token, store it and check for correct type */
  name = token;
  nameLen = len;
  if(type != LEXERTYPE_KEYVAR || is_keyword(name, nameLen)) {
    printf("ERR: NOT KEYVAR or IS_KEYWORD\n");
    c->err = COMPILERERR_EXPECTED_FNAME;
    return true;
  }

  printf("Function Name is: %s\n", name);
  printf("Function nameLen is: %i\n", nameLen);

  /* check for the open parenthesis */
  token = lexer_next(l, &type, &len);
  printf("COMP TOKEN LEN: %i \n", len);
  if(!tokens_equal(token, len, LANG_OPARENTH, LANG_OPARENTH_LEN)) {
    c->err = COMPILERERR_EXPECTED_OPARENTH;
    return true;
  }

  printf("Passed OPARENTH\n");

  /* we're going down a level. push new symbol table to stack */
  if(!symtblstk_push(c)) {
    c->err = COMPILERERR_ALLOC_FAILED;
    return true;
  }

  printf("Pushed Stack Frame.\n");

  /* parse the arguments, return if the process fails */
  if((numArgs = func_defs_parse_args(c, l)) == -1) {
    return true;
  }

  printf("Parsed Args. Num args: %i\n", numArgs);

  /* check for open brace defining start of function "{" */
  token = lexer_next(l, &type, &len);
  if(!tokens_equal(token, len, LANG_OBRACKET, LANG_OBRACKET_LEN)) {
    c->err = COMPILERERR_EXPECTED_OBRACKET;
    return true;
  }

  printf("Passed OBRACKET.\n");

  /* store the function name, location in the output, and # of args */
  if(!func_store_def(c, name, nameLen, numArgs)) {
    return true;
  }

  /****************************** Do function body ****************************/

  if(!func_do_body(c, l)) {
    return true;
  }

  /* retrieve current token (it was modified by func_do_body */
  token = lexer_current_token(l, &type, &len);

  /****************************** End function body ***************************/

  /* check for closing brace defining end of body "}" */
  printf(token);
  if(!tokens_equal(token, len, LANG_CBRACKET, LANG_CBRACKET_LEN)) {
    c->err = COMPILERERR_EXPECTED_CBRACKET;
    return true;
  }

  printf("Passed last brace. Stored Function.\n");

  /* we're done here! pop the symbol table for this function off the stack. */
  ht_free(symtblstk_pop(c));

  return true;
}

/**
 * Reads the bytecode compiled with compiler_build() into a new buffer of the
 * appropriate size.
 * compiler: an instance of Compiler that has some compiled bytecode.
 * buffer: a pointer to a buffer that will receive the compiled bytecode.
 * bufferSize: the size of the buffer in bytes.
 * returns: true if the operation succeeds and false if the buffer is too small,
 * or another error occurs. Read c->err for a specific error code.
 */
bool compiler_bytecode(Compiler * compiler, char * buffer, size_t bufferSize) {
  /* TODO: write this function */
  return false;
}

/**
 * Builds a script file and adds its code to the bytecode output buffer and
 * stores references to its functions and variables in the Compiler object.
 * After several input buffers of scripts have been built, you can copy the
 * bytecode to a buffer for execution using compiler_bytecode().
 * compiler: an instance of compiler that will receive the bytecode.
 * input: an input buffer that contains the script code.
 * inputLen: the number of bytes in length of the input.
 * returns: true if the compile operation succeeds, and false if it fails.
 */
bool compiler_build(Compiler * compiler, char * input, size_t inputLen) {

  assert(compiler != NULL);
  assert(input != NULL);
  assert(inputLen > 0);

  Lexer * lexer = lexer_new(input, inputLen);
  LexerType type;
  char * token;
  size_t tokenLen;

  /* check that lexer alloc didn't fail, and push symtbl for global variables */
  if(lexer == NULL || !symtblstk_push(compiler)) {
    compiler_set_err(compiler, COMPILERERR_ALLOC_FAILED);
    return false;
  }
  compiler_set_err(compiler, COMPILERERR_SUCCESS);

  /* compile loop */
  while((token = lexer_next(lexer, &type, &tokenLen)) != NULL) {

    
    /* TEMPORARILY COMMENTED OUT FOR STRAIGHT CODE PARSER DEVELOPMENT: 
    if(build_parse_func_defs(compiler, lexer)) {
      break;
    } else {
      func_do_body(compiler, lexer);
    }
    */

    if(!func_body_straight_code(compiler, lexer)) {
      return false;
    }

    /* handle errors */
    if(compiler->err != COMPILERERR_SUCCESS) {
      return false;
    }
  }

  /* we're done here: pop globals symtable */
  symtbl_free(symtblstk_pop(compiler));

  /* TODO: Enforce actual return value */
  return true;
}

/**
 * Sets the compiler error code.
 * compiler: an instance of compiler to set the err on.
 * compiler: an instance of Compiler.
 * err: the error code to set.
 */
void compiler_set_err(Compiler * compiler, CompilerErr err) {
  assert(compiler != NULL);

  compiler->err = err;
}

/**
 * Gets the error code currently set on the provided Compiler err.
 * compiler: an instance of compiler.
 * returns: the COMPILERERR_ value.
 */
CompilerErr compiler_get_err(Compiler * compiler) {
  assert(compiler != NULL);

  return compiler->err;
}

/**
 * Frees a compiler object and all associated memory.
 * compiler: an instance of Compiler.
 * TODO: a lot of allocations don't have frees yet. These will be added in when
 * the program structure becomes final.
 */
void compiler_free(Compiler * compiler) {
  assert(compiler != NULL);

  if(compiler->symTableStk != NULL) {
    stk_free(compiler->symTableStk);
  }

  if(compiler->functionHT != NULL) {
    ht_free(compiler->functionHT);
  }

  if(compiler->outBuffer != NULL) {
    sb_free(compiler->outBuffer);
  }

  free(compiler);
}
