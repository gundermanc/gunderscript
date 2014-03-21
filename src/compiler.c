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
#include "strcodeparser.h"
#include "lexer.h"
#include "langkeywords.h"
#include "vm.h"
#include "vmdefs.h"
#include "typestk.h"

/* TODO: make symTableStk auto expand and remove this */
static const int maxFuncDepth = 100;
/* initial size of all hashtables */
static const int initialHTSize = 11;
/* number of items to add to hashtable on rehash */
static const int HTBlockSize = 12;
/* hashtable load factor upon which it will be rehashed */
static const float HTLoadFactor = 0.75;
/* number of bytes in each additional block of the buffer */
static const int bufferBlockSize = 1000;

/**
 * Creates a new compiler object that will contain the current state of the
 * compiler and its data structures.
 * returns: new compiler object, or NULL if the allocation fails.
 */
Compiler * compiler_new(VM * vm) {

  assert(vm != NULL);

  Compiler * compiler = calloc(1, sizeof(Compiler));

  /* check for failed allocation */
  if(compiler == NULL) {
    return NULL;
  }

  /* TODO: make this stack auto expand when full */
  compiler->symTableStk = stk_new(maxFuncDepth);
  compiler->functionHT = ht_new(initialHTSize, HTBlockSize, HTLoadFactor);
  compiler->outBuffer = buffer_new(bufferBlockSize, bufferBlockSize);
  compiler->vm = vm;

  /* check for further malloc errors */
  if(compiler->symTableStk == NULL 
     || compiler->functionHT == NULL 
     || compiler->outBuffer == NULL) {
    compiler_free(compiler);
    return NULL;
  }

  return compiler;
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
				       int index, int numArgs, int numVars,
				       bool exported) {
  assert(index >= 0);

  CompilerFunc * cf = calloc(1, sizeof(CompilerFunc));
  if(cf != NULL) {
    cf->name = calloc(nameLen + 1, sizeof(char));
    strncpy(cf->name, name, nameLen);
    cf->index = index;
    cf->numArgs = numArgs;
    cf->numVars = numVars;
    cf->exported = true;
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
  DSValue value;
  int numArgs = 0;

  while(true) {

    /* check current token is an argument, if not throw a fit */
    if(token == NULL || type != LEXERTYPE_KEYVAR) {

      /* if there was a closed parenth, there are no args, otherwise, give err */
      if(tokens_equal(LANG_CPARENTH, LANG_CPARENTH_LEN, token, len)) {
	return 0;
      } else {
	c->err = COMPILERERR_EXPECTED_VARNAME;
	return -1;
      }
    }

    /* store variable along with index at which its data will be stored in the
     * frame stack in the virtual machine
     */
    value.intVal = ht_size(symTbl);
    if(!ht_put_raw_key(symTbl, token, len, 
		       &value, NULL, &prevExisted)) {
      c->err = COMPILERERR_ALLOC_FAILED;
      return -1;
    }
    numArgs++;

    /* check for duplicate var names */
    if(prevExisted) {
      c->err = COMPILERERR_PREV_DEFINED_VAR;
      return -1;
    }

    /* get next token and check for anything but a comma arg delimiter */
    token = lexer_next(l, &type, &len);
    if(!tokens_equal(token, len, LANG_ARGDELIM, LANG_ARGDELIM_LEN)) {

      /* check for end of the args, or invalid token */
      if(tokens_equal(token, len, LANG_CPARENTH, LANG_CPARENTH_LEN)) {
	/* end of args */
	return numArgs;
      } else {
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
static bool function_store_definition(Compiler * c, char * name, size_t nameLen,
			   int numArgs, int numVars, bool exported) {

  /* TODO: might need a lexer_next() call to get correct token */
  bool prevValue;
  CompilerFunc * cp;
  DSValue value;

  /* check for proper CompilerFunc allocation */
  cp = compilerfunc_new(name, nameLen, buffer_size(c->outBuffer), numArgs,
			numVars, exported);
  if(cp == NULL) {
    c->err = COMPILERERR_ALLOC_FAILED;
    return false;
  }

  value.pointerVal = cp;
  ht_put_raw_key(c->functionHT, cp->name, nameLen, &value, NULL, &prevValue);

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

  /* make sure next token is a variable decl. keyword, otherwise, return */
  if(!tokens_equal(LANG_VAR_DECL, LANG_VAR_DECL_LEN, token, len)) {
    return false;
  }

  /* check that next token is a keyvar type, neccessary for variable names */
  varName = lexer_next(l, &type, &varNameLen);
  if(type != LEXERTYPE_KEYVAR) {
    c->err = COMPILERERR_EXPECTED_VAR_NAME;
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
static int func_do_var_defs(Compiler * c, Lexer * l) {
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
    if(var_def(c, l)) {
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
  int numVars;

  /* check that this is a function declaration token */
  if(!tokens_equal(token, len, LANG_FUNCTION, LANG_FUNCTION_LEN)) {
    return false;
  }

  /* advance to next token. if is is EXPORTED, take note for later */
  token = lexer_next(l, &type, &len);
  if(tokens_equal(token, len, LANG_EXPORTED, LANG_EXPORTED_LEN)) {
    exported = true;
    token = lexer_next(l, &type, &len);
  }

  /* this is the name token, store it and check for correct type */
  name = token;
  nameLen = len;
  if(type != LEXERTYPE_KEYVAR || is_keyword(name, nameLen)) {
    c->err = COMPILERERR_EXPECTED_FNAME;
    return true;
  }

  /* check for the open parenthesis */
  token = lexer_next(l, &type, &len);
  if(!tokens_equal(token, len, LANG_OPARENTH, LANG_OPARENTH_LEN)) {
    c->err = COMPILERERR_EXPECTED_OPARENTH;
    return true;
  }

  /* we're going down a level. push new symbol table to stack */
  if(!symtblstk_push(c)) {
    c->err = COMPILERERR_ALLOC_FAILED;
    return true;
  }

  /* parse the arguments, return if the process fails */
  if((numArgs = func_defs_parse_args(c, l)) == -1) {
    return true;
  }

  /* check for open brace defining start of function "{" */
  token = lexer_next(l, &type, &len);
  if(!tokens_equal(token, len, LANG_OBRACKET, LANG_OBRACKET_LEN)) {
    c->err = COMPILERERR_EXPECTED_OBRACKET;
    return true;
  }
  token = lexer_next(l, &type, &len);

  /****************************** Do function body ****************************/
  
  /* handle variable declarations */
  if((numVars = func_do_var_defs(c, l)) == -1) {
    return false;
  }

  /* retrieve next token (it was modified by func_do_var_def */
  token = lexer_current_token(l, &type, &len);

  /* store the function name, location in the output, and # of args and vars */
  if(!function_store_definition(c, name, nameLen, numArgs, numVars, exported)) {
    return true;
  }

  if(!func_do_body(c, l)) {
    return true;
  }

  /* retrieve current token (it was modified by func_do_body) */
  token = lexer_current_token(l, &type, &len);

  /****************************** End function body ***************************/

  /* check for closing brace defining end of body "}" */
  if(!tokens_equal(token, len, LANG_CBRACKET, LANG_CBRACKET_LEN)) {
    c->err = COMPILERERR_EXPECTED_CBRACKET;
    return true;
  }

  /* push default return value. if no other return is given, this value is 
   * returned */
  buffer_append_char(c->outBuffer, OP_BOOL_PUSH);
  buffer_append_char(c->outBuffer, false);

  /* pop function frame and return to calling function */
  buffer_append_char(c->outBuffer, OP_FRM_POP);

  token = lexer_next(l, &type, &len);

  /* we're done here! pop the symbol table for this function off the stack. */
  ht_free(symtblstk_pop(c));

  return true;
}

/**
 * Gets the number of bytes of byte code in the bytecode buffer that can be
 * copied out using compiler_bytecode().
 * compiler: an instance of compiler.
 * returns: the size of the byte code buffer.
 */
size_t compiler_bytecode_size(Compiler * compiler) {
  assert(compiler != NULL);
  return buffer_size(compiler->outBuffer);
}

/**
 * The compiler bytecode buffer.
 * compiler: an instance of compiler.
 * returns: The compiled bytecode or NULL if there is none.
 */
char * compiler_bytecode(Compiler * compiler) {
  assert(compiler != NULL);
  if(compiler->err != COMPILERERR_SUCCESS) {
    return NULL;
  }

  return buffer_get_buffer(compiler->outBuffer);
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
  size_t tokenLen;

  /* check that lexer alloc didn't fail, and push symtbl for global variables */
  if(lexer == NULL || !symtblstk_push(compiler)) {
    compiler_set_err(compiler, COMPILERERR_ALLOC_FAILED);
    lexer_free(lexer);
    return false;
  }
  compiler_set_err(compiler, COMPILERERR_SUCCESS);

  /* compile loop */
  lexer_next(lexer, &type, &tokenLen);
  while(build_parse_func_defs(compiler, lexer)) {
    /* TEMPORARILY COMMENTED OUT FOR STRAIGHT CODE PARSER DEVELOPMENT: */
    /* handle errors */
    if(compiler->err != COMPILERERR_SUCCESS) {
      lexer_free(lexer);
      return false;
    }
  }

  /* we're done here: pop globals symtable */
  symtbl_free(symtblstk_pop(compiler));

  lexer_free(lexer);
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
 * Gets the start index of the specified function from the compiler declared
 * functions hashtable. To get the index of a function, it must have been
 * declared with the exported keyword.
 * compiler: an instance of compiler.
 * name: the name of the function to locate.
 * len: the length of name.
 * returns: the index in the byte code where the function begins, or -1 if the
 * function does not exist or was not exported.
 */
CompilerFunc * compiler_function(Compiler * compiler, char * name, size_t len) {

  assert(compiler != NULL);
  assert(name != NULL);
  assert(len > 0);

  DSValue value;
  CompilerFunc * cf;

  /* get the specified function's struct */
  if(!ht_get_raw_key(compiler->functionHT, name, len, &value)) {
    return NULL; /* error occurred */
  }
  cf = value.pointerVal;

  /* make sure function was declared with exported keyword */
  if(!cf->exported) {
    return NULL;
  }

  return cf;
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
    HTIter htIterator;
    ht_iter_get(compiler->functionHT, &htIterator);

    while(ht_iter_has_next(&htIterator)) {
      DSValue value;
      ht_iter_next(&htIterator, NULL, 0, &value, NULL, true);
      compilerfunc_free(value.pointerVal);
    }

    ht_free(compiler->functionHT);
  }
 
  if(compiler->outBuffer != NULL) {
    buffer_free(compiler->outBuffer);
  }

  free(compiler);
}
