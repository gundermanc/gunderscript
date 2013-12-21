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

static bool tokens_equal(char * token1, char * token2, size_t num) {
  return strncmp(token1, token2, num) == 0;
}

static CompilerFunc * compilerfunc_new(char * name, size_t nameLen, int index) {
  CompilerFunc * cf = calloc(1, sizeof(CompilerFunc));
  if(cf != NULL) {
    cf->name = calloc(nameLen + 1, sizeof(char));
    strncpy(cf->name, name, nameLen);
    cf->index = index;
  }

  return cf;
}

static void compilerfunc_free(CompilerFunc * cf) {
  free(cf->name);
  free(cf);
}

static bool build_parse_func_defs(Compiler * c, Lexer * l) {

  /*
   * A Function definition looks like so:
   *
   * function [EXPORTED] [NAME] ( [ARGUMENTS] ) {
   *   [code]
   * }
   * 
   * The code below parses these tokens
   */
   
  bool exported = false;
  size_t len;
  LexerType type;
  char * token = lexer_current_token(l, &type, &len);
  char * name;

  /* check that this is a function declaration token */
  if(!tokens_equal(token, LANG_FUNCTION, len)) {
    return false;
  }

  /* advance to next token. if is is EXPORTED, take note for later */
  token = lexer_next(l, &type, &len);
  if(tokens_equal(token, LANG_EXPORTED, len)) {
    exported = true;
    token = lexer_next(l, &type, &len);
  }

  /* this is the name token, take it down and check for correct type */
  name = token;
  if(type != LEXERTYPE_KEYVAR) {
    c->err = COMPILERERR_EXPECTED_FNAME;
    return true;
  }

  /* check for the open parenthesis */
  token = lexer_next(l, &type, &len);
  if(!token_equals(token, LANG_OPARENTH, len)) {
    c->err = COMPILERERR_EXPECTED_OPARENTH;
    return true;
  }

  /* make list of arguments */
  
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
