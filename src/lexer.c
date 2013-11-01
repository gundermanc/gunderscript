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
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lexer.h"
#include <string.h>
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
 * inputType: an enum value specifying the input source for the lexer. Can be
 * STRING, or FILE.
 * inputFn: If inputType is a String, inputFn must be a string to lex. If
 * inputType is FILE, inputFn must be the path to a file containing input to
 * lex.
 * inputFnLen: The length of inputFn. No hand holding is done here. This value
 * must be a positive value, <= the size of the buffer.
 */
Lexer * lexer_new(char * input, size_t inputLen) {

  /* allocate lexer, return NULL if fails */
  Lexer * lexer = calloc(1, sizeof(Lexer));
  if(lexer == NULL) {
    return NULL;
  }

  /* create operator set and allocate input storage string and handle failure */
  lexer->input = calloc(inputLen + 1, sizeof(char));
  if(/*!initialize_operator_set(lexer) ||*/ lexer->input == NULL) {

    /* TODO: this NULL check is here. Make sure it works. */
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
 * is set to LEXERERR_UNTERMINATED_COMMENT, the next l->currToken is set to NULL
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
   * are single line line comment characters: "/*". If so, skip the comment chars
   */
  if(remaining_chars(l) > 0
     && next_char(l) == '/'
     && peek_char(l) == '*') {

    while(remaining_chars(l) > 0) {
      if(prev_char(l) == '*' && next_char(l) == '/') {

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
 * find the matching end quotation mark. Upon finding it, the l->currToken
 * pointer is set to point to the beginning of the string, and l->currTokenLen is
 * set to the length of the string. If no end quotation mark is encountered,
 * l->err is set to LEXERERR_UNTERMINATED_STRING and l->currToken is set to NULL,
 * but function returns true;
 * l: an instance of lexer.
 * returns: true if the first character the lexer encountered was a quotation
 * mark, and false if not.
 */
bool next_parse_strings(Lexer * l) {

  /* this is the beginning of a string */
  if(next_char(l) == '"') {
    int beginStrIndex = l->index;

    /* add characters to the string */
    l->index++;
    for(; remaining_chars(l) > 0; advance_char(l)) {

      /* encountered end of string, return it along with the quotes*/
      if(next_char(l) == '"' && prev_char(l) != '\\') {
	l->currTokenLen = (l->index - beginStrIndex) + 1;
	l->currToken = l->input + beginStrIndex;
	l->err = LEXERERR_SUCCESS;

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
    l->currToken = NULL;
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
 * contiguous body of letters and numbers and then sets l->currToken to the
 * start of the keyword or variable, and l->currTokenLen to the the length
 * of the keyword or variable.
 * l: an instance of lexer.
 * returns: true if the first char was a letter, and false if not.
 */
static bool next_parse_keyvars(Lexer * l) {

  if(is_letter(next_char(l))) {
    int beginStrIndex = l->index;

    /* extract entire word */
    while(remaining_chars(l) > 0 
	  && (is_letter(next_char(l)) || is_digit(next_char(l)) || next_char(l) == '_')) {
      advance_char(l);
    }

    l->currTokenLen = (l->index - beginStrIndex);
    l->currToken = l->input + beginStrIndex;
    l->err = LEXERERR_SUCCESS;

    return true;
  }

  return false;
}

/**
 * lexer_next() numbers subparser.
 * If current character is a digit, begins parsing until end of digits is reached
 * making note of decimal points along the way. If more than one decimal point is
 * encountered, l->err is set to LEXERERR_DUPLICATE_DECIMAL_PT, l->currToken is
 * set to NULL, and the function returns true. Otherwise, the function sets
 * l->currToken to the pointer to the first digit in the number, and l->tokenLen
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
	  l->currToken = NULL;
	  l->currTokenLen = 0;
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
      l->currToken = NULL;
      l->currTokenLen = 0;
      return true;
    }

    /* success, save number to currentToken pointer */
    l->currTokenLen = (l->index - beginStrIndex);
    l->currToken = l->input + beginStrIndex;
    l->err = LEXERERR_SUCCESS;

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
 * lexer_next() operators subparser.
 * If current character is an operator char (see is_operator()), function copies
 * current character and all characters following operator that are operators
 * to the l->currToken pointer, and sets l->currTokenLen to the length of this
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

    l->currToken = token;
    l->currTokenLen = tokenLen;
    l->err = LEXERERR_SUCCESS;

    return true;
  }
  return false;
}

/**
 * lexer_next() end statement subparser. If current character is an endstatement
 * character, function returns true and sets l->currToken to the current character.
 * l: an instance of lexer.
 * returns: true if current character is an endstatement.
 */
static bool next_parse_endstatement(Lexer * l) {
  if(next_char(l) == ';') {
    l->currToken = l->input + l->index;
    l->currTokenLen = 1;
    l->err = LEXERERR_SUCCESS;
    advance_char(l);
    return true;
  }
  return false;
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
 * len: the length of the string being returned.
 * returns: a string containing the next token. Note: this string is not NULL
 * terminated.
 */
char * lexer_next(Lexer * l, size_t * len) {

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
      *len = l->currTokenLen;
      return l->currToken;
    } else if(next_parse_keyvars(l)){
      *len = l->currTokenLen;
      return l->currToken;
    } else if(next_parse_numbers(l)){
      *len = l->currTokenLen;
      return l->currToken;
    } else if(next_parse_operators(l)){
      *len = l->currTokenLen;
      return l->currToken;
    } else if(next_parse_endstatement(l)) {
      *len = l->currTokenLen;
      return l->currToken;
    } else {

      /* none of the parsers can handle the current situation */
      /* temporary warning that helps with debugging */
      printf("WARNING: Control reached end of Recursive Parse Loop\n");
      break;
    }
  }
  return NULL;
}

/**
 * Gets the token returned by the previous call to lexer_next.
 * l: an instance of lexer.
 * len: the length of the token string.
 * returns: a string containing the token. Note: this string is not null
 * terminated, but instead requires use of the length provided through the
 * len parameter to ensure that only the characters from this token are compared.
 */
char * lexer_current_token(Lexer * l, size_t * len) {
  *len = l->currTokenLen;
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
  return l->err;
}

/**
 * Gets the line number of the current token. This can be used to find out
 * which line caused an error while lexing scripts, or just to find current line.
 * l: an instance of lexer.
 * returns: line number of current token.
 */
int lexer_line_num(Lexer * l) {
  return l->lineNum;
}

/**
 * Checks a string to make sure that it is composed entirely of operator characters.
 * input: a string.
 * len: the number of characters to compare.
 * definitive: if true, method iterates entire token length to make sure that it is
 * valid. If false, it uses clues, such as quotes and first and last character type
 * to make a quick judgement. All tokens coming from lexer_next() will be valid, so
 * this argument can be false with lexer_next() generated tokens.
 * returns: true if the string is composed entirely of characters that are not letters,
 * digits, or whitespace.
 */
static bool is_valid_operator(char * input, size_t len, bool definitive) {
  int i = 0;

  if(!definitive) {
    if(is_operator(input[0]) && is_operator(input[len-1])) {
      return true;
    } else {
      return false;
    }
  }

  /* for each character, if not an operator character, return false */
  for(i = 0; i < len; i++) {
    if(!is_operator(input[i])) {
      return false;
    }
  }

  return true;
}

/**
 * Checks a string to make sure that it is a string token.
 * input: a string.
 * len: the number of characters to compare.
 * definitive: if true, method iterates entire token length to make sure that it is
 * valid. If false, it uses clues, such as quotes and first and last character type
 * to make a quick judgement. All tokens coming from lexer_next() will be valid, so
 * this argument can be false with lexer_next() generated tokens.
 * returns: true if the string is enclosed by quotes and does not contain any \n, or
 * unescaped '"'.
 */
static bool is_valid_string(char * input, size_t len, bool definitive) {
  int i = 0;

  if(!definitive) {
    if(input[0] == '"' && input[len-1] == '"') {
      return true;
    } else {
      return false;
    }
  }

  /* for each character, if an unescaped quotation mark, or \n, return false */
  for(i = 0; i < len; i++) {
    if((i > 0 && input[i] == '"' && input[i-1] != '\\')
       || (input[i] == '\n')) {
      return false;
    }
  }

  return true;
}

/**
 * Checks a string to make sure that it is a valid number token.
 * input: a string.
 * len: the number of characters to compare.
 * definitive: if true, method iterates entire token length to make sure that it is
 * valid. If false, it uses clues, such as quotes and first and last character type
 * to make a quick judgement. All tokens coming from lexer_next() will be valid, so
 * this argument can be false with lexer_next() generated tokens.
 * returns: true if the string is made up of numbers with only one decimal place.
 */
static bool is_valid_number(char * input, size_t len, bool definitive) {
  int i = 0;
  bool decPtEncountered = false;

  if(!definitive) {
    if(is_digit(input[0]) && is_digit(input[len-1])) {
      return true;
    } else {
      return false;
    }
  }

  /* for each character, if not a digit or a decimal pt, return false*/
  for(i = 0; i < len; i++) {

    if(!decPtEncountered && input[i] == '.') {
      decPtEncountered = true;
    } else {
      return false;
    }

    if(!is_digit(input[i])
       && (input[i] != '.' || decPtEncountered)) {
      return false;
    }
  }

  return true;
}

/**
 * Checks a string to make sure that it is a valid keyvar token.
 * input: a string.
 * len: the number of characters to compare.
 * definitive: if true, method iterates entire token length to make sure that it is
 * valid. If false, it uses clues, such as quotes and first and last character type
 * to make a quick judgement. All tokens coming from lexer_next() will be valid, so
 * this argument can be false with lexer_next() generated tokens.
 * returns: true if the string starts with a letter and contains only letters and
 * numbers.
 */
static bool is_valid_keyvar(char * input, size_t len, bool definitive) {
  int i = 0;

  if(!definitive) {
    if(is_letter(input[0]) && (is_letter(input[len-1]) || is_digit(input[i-1]))) {
      return true;
    } else {
      return false;
    }
  }

  if(!is_letter(input[0]) || !is_digit(input[0])) {
    return false;
  }

  /* for each character, if not a letter or an underscore, return false */
  for(i = 0; i < len; i++) {
    if(!is_letter(input[i]) && input[i] != '_') {
      return false;
    }
  }

  return true;
}

/**
 * Determines a token string's type.
 * input: a string.
 * len: the number of characters to compare.
 * definitive: if true, method iterates entire token length to make sure that it is
 * valid. If false, it uses clues, such as quotes and first and last character type
 * to make a quick judgement. All tokens coming from lexer_next() will be valid, so
 * this argument can be false with lexer_next() generated tokens.
 * returns: the LexerType of the token, or LEXERTYPE_UNKNOWN if the token is not a
 * valid type.
 */
LexerType lexer_token_type(char * token, size_t len, bool definitive) {

  LexerType type = LEXERTYPE_UNKNOWN;

  if(is_valid_string(token, len, definitive)) {
    type = LEXERTYPE_STRING;
  } else if(is_valid_operator(token, len, definitive)) {
    type = LEXERTYPE_OPERATOR;
  } else if(is_valid_number(token, len, definitive)) {
    type = LEXERTYPE_NUMBER;
  } else if(is_valid_keyvar(token, len, definitive)) {
    type = LEXERTYPE_KEYVAR;
  } else if(len == 1 && token[0] == ';') {
    type = LEXERTYPE_ENDSTATEMENT;
  }

  return type;
}
