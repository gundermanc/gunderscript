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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "compiler.h"
#include "lexer.h"
#include "langkeywords.h"
#include "vmdefs.h"

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

/* !!UNDER CONSTRUCTION!! */

Compiler * compiler_new() {
  Compiler * compiler = calloc(1, sizeof(Compiler));

  if(compiler == NULL) {
    return NULL;
  }

  /* TODO: make this stack auto expand when full */
  compiler->symTableStk = stk_new(maxFuncDepth);
  compiler->functionHT = ht_new(initialHTSize, sbBlockSize, HTLoadFactor);
  compiler->outBuffer = sb_new(sbBlockSize);

  /* check for malloc errors */
  if(compiler->symTableStk == NULL 
     || compiler->functionHT == NULL 
     || compiler->outBuffer == NULL) {
    compiler_free(compiler);
    return NULL;
  }

  return compiler;
}

/* checks that two tokens are equal and not NULL */
static bool tokens_equal(char * token1, size_t num1, char * token2, size_t num2) {
  if((token1 != NULL && token2 != NULL) && (num1 == num2)) {
    return strncmp(token1, token2, num1) == 0;
  }
  return false;
}

static CompilerFunc * compilerfunc_new(char * name, size_t nameLen,
				       int index, int numArgs) {
  CompilerFunc * cf = calloc(1, sizeof(CompilerFunc));
  if(cf != NULL) {
    cf->name = calloc(nameLen + 1, sizeof(char));
    strncpy(cf->name, name, nameLen);
    cf->index = index;
    cf->numArgs = numArgs;
  }

  return cf;
}

static void compilerfunc_free(CompilerFunc * cf) {
  free(cf->name);
  free(cf);
}

/* pushes another symbol table onto the stack of symbol tables
 * returns true if success, false if malloc fails
 */
static bool symtblstk_push(Compiler * c) {
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

static HT * symtblstk_peek(Compiler * c) {
  DSValue value;

  /* check that peek was success */
  if(!stk_peek(c->symTableStk, &value)) {
    return NULL;
  }

  return value.pointerVal;
}

static HT * symtblstk_pop(Compiler * c) {
  DSValue value;

  /* check that peek was success */
  if(!stk_pop(c->symTableStk, &value)) {
    return NULL;
  }

  return value.pointerVal;
}

static void symtbl_free(HT * symTbl) {
  assert(symTbl != NULL);

  ht_free(symTbl);
}

/* TODO: make more sophisticated mechanism */
static bool is_keyword(char * token, size_t tokenLen) {
  if(tokens_equal(LANG_FUNCTION, LANG_FUNCTION_LEN, token, tokenLen)) {
    return true;
  }

  return false;
}

/* return the number of arguments, or -1 if error occurred */
static int func_defs_parse_args(Compiler * c, Lexer * l) {
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

static bool func_store_def(Compiler * c, char * name, size_t nameLen, int numArgs) {
  /* record function definition:
   * store function definition, associated function name string, index of the
   * function in the output, and the number of arguments that the function can
   * accept.
   */
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
static bool var_def(Compiler * c, Lexer * l) {
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
static bool func_do_var_defs(Compiler * c, Lexer * l) {
  LexerType type;
  char * token;
  size_t len;
 
  /* variable declarations look like so:
   * var [varName];
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

/* does the function body code */
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

/* builds a file and adds it to the output buffer */
bool compiler_build(Compiler * compiler, char * input, size_t inputLen) {

  assert(compiler != NULL);
  assert(input != NULL);
  assert(inputLen > 0);

  Lexer * lexer = lexer_new(input, inputLen);
  LexerType type;
  char * token;
  size_t tokenLen;

  /* check that lexer alloc didn't fail */
  if(lexer == NULL) {
    compiler_set_err(compiler, COMPILERERR_ALLOC_FAILED);
    return false;
  }
  compiler_set_err(compiler, COMPILERERR_SUCCESS);

  /* compile loop */
  while((token = lexer_next(lexer, &type, &tokenLen)) != NULL) {

    build_parse_func_defs(compiler, lexer);

  }


  /* TODO: Enforce actual return value */
  return true;
}

void compiler_set_err(Compiler * compiler, CompilerErr err) {
  assert(compiler != NULL);

  compiler->err = err;
}

CompilerErr compiler_get_err(Compiler * compiler) {
  assert(compiler != NULL);

  return compiler->err;
}

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
