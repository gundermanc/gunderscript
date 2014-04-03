/**
 * lexer.c
 * (C) 2013 Christian Gunderman
 * Modified by:
 * Author Email: gundermanc@gmail.com
 * Modifier Email:
 *
 * Description:
 * The Lexer object simply caches the initialization String and a current index
 * and returns the next "token" when the Next Token method is called. Tokens
 * are low level, non-language-specific patterns in text, such as symbols, (, )
 * {, }, etc. For more information on the Lexer's behavior, see
 * the Next Token method's description.
 *
 * This object acts to unify the interface for acquiring new tokens. The object
 * can be initialized using either a file, or String. Each time next is called,
 * the method peruses the selected input source until it finds the next token.
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

#include "lexer.h"
#include <string.h>
#include <assert.h>
#include "gsbool.h"

/**
 * Prevents subsequent lexer_next() calls, finalizing the lexer.
 * l: a lexer object instance.
 */
static void finalize_lexer(Lexer * l) {
  l->index = l->inputLen;
}



/**
 * Creates a new lexer object.
 * input: The input string to lex.
 * inputLen: the number of characters in the input buffer.
 * returns: A new lexer object, or NULL if the 
 */
Lexer * lexer_new(char * input, size_t inputLen) {

  assert(input != NULL);
  assert(inputLen > 0);

  /* allocate lexer, return NULL if fails */
  Lexer * lexer = calloc(1, sizeof(Lexer));
  if(lexer == NULL) {
    return NULL;
  }

  /* create operator set and allocate input storage string and handle failure */
  lexer->input = calloc(inputLen + 1, sizeof(char));
  if(lexer->input == NULL) {
    free(lexer);
    return NULL;
  }

  strncpy(lexer->input, input, inputLen);
  lexer->inputLen = inputLen;
  lexer->lineNum = 1;

  return lexer;
}

/**
 * Frees any memory associated with the Lexer Object and closes the input
 * file if there is one.
 * l: the lexer object to free.
 */ 
void lexer_free(Lexer * l) {
  assert(l != NULL);

  free(l->input);
  free(l);
}

/**
 * Checks to see if character is a whitespace character, such as end line,
 * return carriage, tab, or space.
 * c: a char value to check.
 * returns: true if c is a whitespace char, and false if not.
 */
static bool is_white_space(char c) {

  switch(c) {
  case ' ':
  case '\t':
  case '\r':
  case '\n':
    return true;
  }

  return false;
}

/**
 * Returns the next char in the input string. This wrapper is for convenience.
 * Warning: this method does not do bounds checking.
 * l: an instance of lexer.
 * returns: the next char from the input.
 */
static char next_char(Lexer * l) {
  return l->input[l->index];
}

/**
 * Advances the next_char index to the next char in the input string.
 * Warning: this method does no bounds checking.
 * l: an instance of lexer.
 */
static void advance_char(Lexer * l) {

  /* if new line, increment line number */
  if(next_char(l) == '\n') {
    l->lineNum++;
  }

  l->index++;
}

/**
 * Gets the previous character in the input string.
 * Warning: this method does not do bounds checking.
 * l: an instance of lexer.
 * returns: the previous character.
 */
static char prev_char(Lexer * l) {
  assert(l->index > 0);
  return l->input[l->index - 1];
}

/**
 * Returns the next character in the input string without advancing current char
 * iterator.
 * l: an instance of lexer.
 * returns: the char in the sequence after the current char.
 */
static char peek_char(Lexer * l) {
  return l->input[l->index + 1];
}

/**
 * Gets the number of uniterated characters remaining in the input string.
 * l: an instance of lexer
 * returns: the number of characters remaining.
 */
static int remaining_chars(Lexer * l) {
  return l->inputLen - l->index;
}

/**
 * Checks to make sure outType isn't null and then stored newType
 * in the buffer pointed to by outType.
 * outType: pointer to a LexerType that will receive the output.
 * newType: the value to store in outType.
 */
static void set_type(LexerType * outType, LexerType newType) {
  if(outType != NULL) {
    *outType = newType;
  }
}

/**
 * lexer_next() whitespace subparser.
 * Advances current character index until current contiguous block of whitespace
 * has been passed completed.
 * l: an instance of lexer object.
 * returns: true if current character was a whitespace char when subparser was
 * first called, or false if the current character was not a whitespace
 * character.
 */
static bool next_parse_whitespace(Lexer * l) {

  if(is_white_space(next_char(l))) {

    /* skip all whitespace characters */
    while(remaining_chars(l) > 0 && is_white_space(next_char(l))) {
      advance_char(l);
    }

    return true;
  }

  return false;
}

/**
 * lexer_next() comments subparser.
 * Whenever a comment start operator is encountered, advances current char index
 * until the matching comment terminator is encountered. If a multiline comment
 * is encountered, but it does not have a matching terminator, the l->err flag
 * is set to LEXERERR_UNTERMINATED_COMMENT, the next l->nextToken is set to NULL
 * but method still returns true.
 * l: a lexer object.
 * returns: true if this subparser was called when first chars were a start
 * comment operator, and returns false if current character was not comment
 * related and nothing was done.
 */
static bool next_parse_comments(Lexer * l) {

  /* if there is another character after this one, check this one and the next
   * one to see if they are single line comment characters: "//". If so, skip the
   * comment characters.
   */
  if(remaining_chars(l) > 0
     && next_char(l) == '/'
     && peek_char(l) == '/') {

    while(remaining_chars(l) > 0 && next_char(l) != '\n') {
      advance_char(l);
    }

    return true;
  }

  /* if there is another character after the current one, check it to see if they
   * are single line line comment characters. If so, skip the comment chars
   */
  if(remaining_chars(l) > 0
     && next_char(l) == '/'
     && peek_char(l) == '*') {

    while(remaining_chars(l) > 0) {
      if(l->index > 0 && prev_char(l) == '*' && next_char(l) == '/') {

	/* prevent the closing / from being re-evaluated next iteration */
	advance_char(l);
	return true;
      }
      advance_char(l);
    }

    l->err = LEXERERR_UNTERMINATED_COMMENT;
    finalize_lexer(l);
  }

  return false;
}

/**
 * lexer_next() strings subparser.
 * If current character is a quotation mark, ", this function begins parsing to
 * find the matching end quotation mark. Upon finding it, the l->nextToken
 * pointer is set to point to the beginning of the string, and l->nextTokenLen is
 * set to the length of the string. If no end quotation mark is encountered,
 * l->err is set to LEXERERR_UNTERMINATED_STRING and l->nextToken is set to NULL,
 * but function returns true;
 * l: an instance of lexer.
 * returns: true if the first character the lexer encountered was a quotation
 * mark, and false if not.
 */
static bool next_parse_strings(Lexer * l) {

  /* this is the beginning of a string */
  if(next_char(l) == '"') {
    int beginStrIndex = l->index + 1;

    /* add characters to the string */
    l->index++;
    for(; remaining_chars(l) > 0; advance_char(l)) {
      if(next_char(l) == '\\'){
        advance_char(l);
        advance_char(l);
      }
      /* encountered end of string, return it along with the quotes*/
      if(next_char(l) == '"') {
	l->nextTokenLen = (l->index - beginStrIndex);
	l->nextToken = l->input + beginStrIndex;
	l->err = LEXERERR_SUCCESS;

	l->nextTokenType = LEXERTYPE_STRING;

	/* increment index again to prevent the end quote from being evaluated
	 * next iteration.
	 */
	advance_char(l);
	return true;
      }

      /* prevent newlines from being put in strings */
      if(next_char(l) == '\n') {
	l->err = LEXERERR_NEWLINE_IN_STRING;
	finalize_lexer(l);
	return true;
      }
    
    }

    /* error occurred, set token to null and quit */
    l->nextToken = NULL;
    l->err = LEXERERR_UNTERMINATED_STRING;
    finalize_lexer(l);
    return true;
  }
  return false;
}

/**
 * Checks to determine if the character is a letter.
 * c: the letter to check.
 * returns: true if a letter, and false if not.
 */
static bool is_letter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/**
 * Checks if is digit.
 * c: digit to check.
 * returns: true if is digit, false if not.
 */
static bool is_digit(char c) {
  return (c >= '0' && c <= '9');
}

/**
 * lexer_next() keyvars subparser.
 * If current character is a letter, this method parses to the end of the
 * contiguous body of letters and numbers and then sets l->nextToken to the
 * start of the keyword or variable, and l->nextTokenLen to the the length
 * of the keyword or variable.
 * l: an instance of lexer.
 * returns: true if the first char was a letter, and false if not.
 */
static bool next_parse_keyvars(Lexer * l) {

  if(is_letter(next_char(l)) || next_char(l) == '_') {
    int beginStrIndex = l->index;

    /* extract entire word */
    while(remaining_chars(l) > 0 
	  && (is_letter(next_char(l)) 
	      || is_digit(next_char(l)) 
	      || next_char(l) == '_')) {
      advance_char(l);
    }

    l->nextTokenLen = (l->index - beginStrIndex);
    l->nextToken = l->input + beginStrIndex;
    l->err = LEXERERR_SUCCESS;
    l->nextTokenType = LEXERTYPE_KEYVAR;

    return true;
  }

  return false;
}

/**
 * lexer_next() numbers subparser.
 * If current character is a digit, begins parsing until end of digits is reached
 * making note of decimal points along the way. If more than one decimal point is
 * encountered, l->err is set to LEXERERR_DUPLICATE_DECIMAL_PT, l->nextToken is
 * set to NULL, and the function returns true. Otherwise, the function sets
 * l->nextToken to the pointer to the first digit in the number, and l->tokenLen
 * to the number of digits in the number (plus one if there is a decimal pt).
 * l: an instance of lexer.
 * returns: true if the first character encountered is a digit, and false if not.
 */
static bool next_parse_numbers(Lexer * l) {

  if(is_digit(next_char(l))) {
    int beginStrIndex = l->index;
    bool decimalDetected = false;

    /* move index to end of number */
    while(remaining_chars(l) > 0 
	  && (is_digit(next_char(l)) || next_char(l) == '.')) {

      /* prevent multiple decimal points in one number */
      if(next_char(l) == '.') {

	if(decimalDetected) {
	  l->err = LEXERERR_DUPLICATE_DECIMAL_PT;
	  finalize_lexer(l);
	  l->nextToken = NULL;
	  l->nextTokenLen = 0;
	  return true;
	}

	decimalDetected = true;
      }

      advance_char(l);
    }

    /* if last character in digit grouping is a '.' throw a fit.
     * Numbers must start and end with digits.
     */
    if(prev_char(l) == '.') {
      l->err = LEXERERR_TRAILING_DECIMAL_PT;
      finalize_lexer(l);
      l->nextToken = NULL;
      l->nextTokenLen = 0;
      return true;
    }

    /* success, save number to currentToken pointer */
    l->nextTokenLen = (l->index - beginStrIndex);
    l->nextToken = l->input + beginStrIndex;
    l->err = LEXERERR_SUCCESS;
    l->nextTokenType = LEXERTYPE_NUMBER;

    return true;
  }

  return false;
}

/**
 * Superficially decides if character is an operator character (not a letter,
 * digit, or whitespace).
 * c: the char to check.
 * returns: true if c is an operator, false if not.
 */
static bool is_operator(char c) {
  return (!is_digit(c) && !is_letter(c) && !is_white_space(c)
	  && c != '"' && c != ';');
}

/**
 * Checks if a character is a bracket or curly brace.
 * c: the char to check.
 * returns: true if c is a bracket, false if not.
 */
static bool is_bracket(char c) {
  return c == '{' || c == '}' || c == '[' || c == ']';
}

/**
 * Checks if a character is a parenthesis.
 * c: the char to check.
 * returns: true if c is a parenthesis, false if not.
 */
static bool is_parenthesis(char c) {
  return c == '(' || c == ')';
}

/**
 * lexer_next() operators subparser.
 * If current character is an operator char (see is_operator()), function copies
 * current character and all characters following operator that are operators
 * to the l->nextToken pointer, and sets l->nextTokenLen to the length of this
 * multicharacter operator.
 * l: an instance of lexer.
 * returns: true if the first character is an operator, and false if not.
 */
static bool next_parse_operators(Lexer * l) {

  if(is_operator(next_char(l))) {
    int beginStrIndex = l->index;
    char * token;
    size_t tokenLen;

    /* move index to end of symbols */
    while(remaining_chars(l) > 0 
	  && is_operator(next_char(l))) {
      advance_char(l);
    }

    tokenLen = (l->index - beginStrIndex);
    token  = l->input + beginStrIndex;

    l->nextToken = token;
    l->nextTokenLen = tokenLen;
    l->err = LEXERERR_SUCCESS;
    l->nextTokenType = LEXERTYPE_OPERATOR;

    return true;
  }
  return false;
}

/**
 * lexer_next() brackets subparser.
 * If current character is a bracket or curly brace), the current character
 * will be set to the l->nextToken pointer, and
 * l->nextTokenLen will be set to the length of the symbol (always 1).
 * l: an instance of lexer.
 * returns: true if the current character is a logiparenth and false if not.
 */
static bool next_parse_brackets(Lexer * l) {

  if(is_bracket(next_char(l))) {
    l->nextToken = l->input + l->index;
    l->nextTokenLen = 1;
    l->nextTokenType = LEXERTYPE_BRACKETS;
    l->err = LEXERERR_SUCCESS;
    advance_char(l);

    return true;
  }

  return false;
}

/**
 * lexer_next() parenthesis subparser.
 * If current character is a parenthesis, the current character
 * will be set to the l->nextToken pointer, and
 * l->nextTokenLen will be set to the length of the symbol (always 1).
 * l: an instance of lexer.
 * returns: true if the current character is a logiparenth and false if not.
 */
static bool next_parse_parenthesis(Lexer * l) {

  if(is_parenthesis(next_char(l))) {
    l->nextToken = l->input + l->index;
    l->nextTokenLen = 1;
    l->nextTokenType = LEXERTYPE_PARENTHESIS;
    l->err = LEXERERR_SUCCESS;
    advance_char(l);

    return true;
  }

  return false;
}

/**
 * lexer_next() end statement subparser. If current character is an endstatement
 * character, function returns true and sets l->nextToken to the current character.
 * l: an instance of lexer.
 * returns: true if current character is an endstatement.
 */
static bool next_parse_endstatement(Lexer * l) {
  if(next_char(l) == ';') {
    l->nextToken = l->input + l->index;
    l->nextTokenLen = 1;
    l->err = LEXERERR_SUCCESS;
    l->nextTokenType = LEXERTYPE_ENDSTATEMENT;
    advance_char(l);
    return true;
  }
  return false;
}

/**
 * lexer_next() comma subparser. If current character is a comma
 * character, function returns true and sets l->nextToken to the current character.
 * l: an instance of lexer.
 * returns: true if current character is a comma.
 */
static bool next_parse_argdelim(Lexer * l) {
  if(next_char(l) == ',') {
    l->nextToken = l->input + l->index;
    l->nextTokenLen = 1;
    l->err = LEXERERR_SUCCESS;
    l->nextTokenType = LEXERTYPE_ARGDELIM;
    advance_char(l);
    return true;
  }
  return false;
}

/**
 * Moves the currToken and nextToken fields of lexer object forward by one token.
 * l: an instance of lexer.
 */
static void update_next_token(Lexer * l) {

  assert(l != NULL);

  /* move "next" tokens into current */
  l->currToken = l->nextToken;
  l->currTokenLen = l->nextTokenLen;
  l->currTokenType = l->nextTokenType;

  /* Recursive Descent Parsing Loop:
   * Each iteration, the lexer attempts to handle the current set of characters
   * by feeding the current, previous, and next characters into a sub parser.
   * The sub parser will then look at the current character and decide whether
   * or not it can handle the current situation. For example, if the whitespace
   * parser sees a whitespace character, it will move the iterator index forward
   * until no more remain. It then returns true to signify that it could handle
   * the current situation, and the if/else chain restarts from the top. Lets say
   * the next symbol is a digit. The whitespace parser will be called again. Since
   * it doesn't know how to handle digits, it returns false and the digit is passed
   * down the chain until it reaches the number parser that can handle it.
   */
  while(remaining_chars(l) > 0) {

    l->nextTokenType = LEXERTYPE_UNKNOWN;

    /* Recursive Descent Chain:
     * I know the McCabe's complexity is CRAZY high, but in this I case, I
     * believe that it is excusible because it greatly simplifies the rest of
     * the code document and makes the code much more understandable...the 
     * primary goal of good computer science.
     *
     * Basically, control just cascades down until it finds a method that
     * can handle the data.
     */
    if(next_parse_whitespace(l)) {

      /* do nothing, but restart if/else chain when whitespace is detected */

    } else if(next_parse_comments(l)) {
      if(l->err != LEXERERR_SUCCESS) {
	break;
      }
    } else if(next_parse_strings(l)) {
      return;
    } else if(next_parse_keyvars(l)){
      return;
    } else if(next_parse_numbers(l)){
      return;
    } else if(next_parse_brackets(l)) {
      return;
    } else if(next_parse_argdelim(l)) {
      return;
    } else if(next_parse_parenthesis(l)) {
      return;
    } else if(next_parse_operators(l)) {
      return;
    } else if(next_parse_endstatement(l)) {
      return;
    } else {

      /* none of the parsers can handle the current situation */
      /* temporary warning that helps with debugging */
      printf("WARNING: Control reached end of Recursive Parse Loop\n");
      break;
    }
  }

  l->nextToken = NULL;
}

/**
 * Gets the token returned by the previous call to lexer_next.
 * l: an instance of lexer.
 * type: pointer that will receive the type. May be NULL.
 * len: the length of the token string. Can't be NULL.
 * returns: a string containing the token. Note: this string is not null
 * terminated, but instead requires use of the length provided through the
 * len parameter to ensure that only the characters from this token are compared.
 */
char * lexer_current_token(Lexer * l, LexerType * type, size_t * len) {

  assert(l != NULL);
  assert(len != NULL);

  if(l->err != LEXERERR_SUCCESS) {
    return NULL;
  }

  *len = l->currTokenLen;
  set_type(type, l->currTokenType);
  return l->currToken;

}

/**
 * Gets the last error experienced by lexer. If lexer_next() returns NULL,
 * call this method to find out the cause of the error. LEXERERR_SUCCESS means
 * that the operation succeeded. Any other code describes syntax errors in the
 * input.
 * l: an instnace of lexer.
 * returns: the last error experienced by lexer_next().
 */
LexerErr lexer_get_err(Lexer * l) {

  assert(l != NULL);

  return l->err;
}

/**
 * Gets the line number of the current token. This can be used to find out
 * which line caused an error while lexing scripts, or just to find current line.
 * l: an instance of lexer.
 * returns: line number of current token.
 */
int lexer_line_num(Lexer * l) {
  assert(l != NULL);
  return l->lineNum;
}

/**
 * Advances the currToken and nextToken fields of the lexer object to the next
 * token.
 * lexer: an instance of lexer.
 */
static void update_tokens(Lexer * lexer) {

  /* first time updating tokens, fill both currToken and nextToken by running
   * update_next_token a second time
   */
  if(lexer->index == 0) {
    update_next_token(lexer);
  }
  update_next_token(lexer);
}

/**
 * Returns the token after the current one without advancing the lexer forward.
 * lexer: an instance of lexer.
 * type: a pointer that will recv. the the type of the token being returned. May
 * be NULL.
 * len: a pointer that will recv. the length of the token in characters.
 * returns: the token after the current token.
 */
char * lexer_peek(Lexer * lexer, LexerType * type, size_t * len) {
  if(lexer->err != LEXERERR_SUCCESS) {
    return NULL;
  }

  set_type(type, lexer->nextTokenType);
  *len = lexer->nextTokenLen;
  return lexer->nextToken;
}

/**
 * Returns the next token string from this lexer. Characters are tokenized into:
 * - Strings: surrounded by quotes.
 * - Numbers: contiguous blocks of digits with up to one decimal pt.
 * - Comments: begin with slash star, end with star slash or begin with // end
 *             newline, and are removed during tokenization.
 * - Keywords/vars: any contiguous block of characters starting with a letter
 *                  and ending with letters or numbers.
 * - Operators: any contiguous blocks of characters that aren't whitespace,
 *              letters, or digits.
 * l: an instance of lexer.
 * type: the type of the token being returned.
 * len: the length of the string being returned.
 * returns: a string containing the next token. Note: this string is not NULL
 * terminated.
 */
char * lexer_next(Lexer * lexer, LexerType * type, size_t * len) {
  assert(lexer != NULL);
  assert(type != NULL);
  assert(len != NULL);

  if(lexer->err != LEXERERR_SUCCESS) {
    return NULL;
  }

  /* move lexer object's "nextToken" and "currentToken" fields
   * forward one token
   */
  update_tokens(lexer);

  set_type(type, lexer->currTokenType);
  *len = lexer->currTokenLen;
  return lexer->currToken;
}

/**
 * Gets a string representation of the lexer error. NOTE: these strings are
 * not dynamically allocated, but are constants that cannot be freed, and die
 * when this module terminates.
 * lexer: an instance of lexer.
 * err: the error to translate to text.
 * returns: the text form of this error.
 */
const char * lexer_err_to_string(LexerErr err) {
  return lexerErrorMessages[err];
}
