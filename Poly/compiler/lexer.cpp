
// ---- Include Files -------------------------------------------------------------------------

#include "lexer.h"

// ---- Globals -------------------------------------------------------------------------------

// ---- Lexer -----------------------------------------------------------------------------

LexerState g_CurrLexerState;                    // The current lexer state
LexerState g_PrevLexerState;                    // The previous lexer state (used for rewinding the token stream)

// ---- Operators -------------------------------------------------------------------------

// ---- First operator characters

OpState g_OpChars0[MAX_OP_STATE_COUNT] = { 
	{ '+', 0, 2, OP_TYPE_ADD },
	{ '-', 2, 2, OP_TYPE_SUB },
	{ '*', 4, 1, OP_TYPE_MUL },
	{ '/', 5, 1, OP_TYPE_DIV },
	{ '%', 6, 1, OP_TYPE_MOD },
	{ '&', 8, 2, OP_TYPE_BITWISE_AND },
	{ '|', 10, 2, OP_TYPE_BITWISE_OR },
	{ '~', 0, 0, OP_TYPE_BITWISE_NOT },
	{ '^', 11, 1, OP_TYPE_BITWISE_XOR },
	{ '!', 12, 1, OP_TYPE_LOGICAL_NOT },
	{ '=', 13, 1, OP_TYPE_ASSIGN },
	{ '<', 14, 2, OP_TYPE_LESS },
	{ '>', 16, 2, OP_TYPE_GREATER },
	{ '.', 0, 0, OP_TYPE_MEMBER_ACCESS },
	//{ ':', 18, 2, OP_TYPE_SEP },
};

// ---- Second operator characters

OpState g_OpChars1[MAX_OP_STATE_COUNT] = { 
	{ '=', 0, 0, OP_TYPE_ASSIGN_ADD }, { '+', 0, 0, OP_TYPE_INC },		// +=, ++
	{ '=', 0, 0, OP_TYPE_ASSIGN_SUB }, { '-', 0, 0, OP_TYPE_DEC },		// -=, --
	{ '=', 0, 0, OP_TYPE_ASSIGN_MUL },							// *=
	{ '=', 0, 0, OP_TYPE_ASSIGN_DIV },							// /=
	{ '=', 0, 0, OP_TYPE_ASSIGN_MOD },							// %=
	{ '=', 0, 0, OP_TYPE_ASSIGN_AND }, { '&', 0, 0, OP_TYPE_LOGICAL_AND },		// &=, &&
	{ '=', 0, 0, OP_TYPE_ASSIGN_OR }, { '|', 0, 0, OP_TYPE_LOGICAL_OR },		// |=, ||
	{ '=', 0, 0, OP_TYPE_ASSIGN_XOR },							// ^=
	{ '=', 0, 0, OP_TYPE_NOT_EQUAL },							// !=
	{ '=', 0, 0, OP_TYPE_EQUAL },							// ==
	{ '=', 0, 0, OP_TYPE_LESS_EQUAL }, { '<', 0, 1, OP_TYPE_BITWISE_SHIFT_LEFT },		// <=, <<
	{ '=', 0, 0, OP_TYPE_GREATER_EQUAL }, { '>', 1, 1, OP_TYPE_BITWISE_SHIFT_RIGHT },		// >=, >>
	//{ '=', 0, 0, OP_TYPE_DECL_VAR }, { ':', 0, 0, OP_TYPE_SCOPE },		// :=, ::
};

// ---- Third operator characters

OpState g_OpChars2[MAX_OP_STATE_COUNT] = { { '=', 0, 0, OP_TYPE_ASSIGN_SHIFT_LEFT }, { '=', 0, 0, OP_TYPE_ASSIGN_SHIFT_RIGHT } }; // <<=, >>=

// ---- Delimiters ------------------------------------------------------------------------

char cDelims[MAX_DELIM_COUNT] = { ',', '(', ')', '[', ']', '{', '}', ';' };

// ---- Functions -----------------------------------------------------------------------------

/******************************************************************************************
*
*   ResetLexer()
*
*   Resets the lexer.
*/

void ResetLexer()
{
	// Set the current line of code to the new line

	g_CurrLexerState.iCurrLineIndex = 0;
	g_CurrLexerState.pCurrLine = g_SourceCode.pHead;

	// Reset the start and end of the current lexeme to the beginning of the source

	g_CurrLexerState.iCurrLexemeStart = 0;
	g_CurrLexerState.iCurrLexemeEnd = 0;

	// Reset the current operator

	g_CurrLexerState.iCurrOp = 0;
}

/******************************************************************************************
*
*   CopyLexerState ()
*
*   Copies one lexer state structure into another.
*/

void CopyLexerState(LexerState & pDestState, const LexerState & pSourceState)
{
	// Copy each field individually to ensure a safe copy

	pDestState.iCurrLineIndex = pSourceState.iCurrLineIndex;
	pDestState.pCurrLine = pSourceState.pCurrLine;
	pDestState.CurrToken = pSourceState.CurrToken;
	strcpy(pDestState.pstrCurrLexeme, pSourceState.pstrCurrLexeme);
	pDestState.iCurrLexemeStart = pSourceState.iCurrLexemeStart;
	pDestState.iCurrLexemeEnd = pSourceState.iCurrLexemeEnd;
	pDestState.iCurrOp = pSourceState.iCurrOp;
}

/******************************************************************************************
*
*   GetOpStateIndex ()
*
*   Returns the index of the operator state associated with the specified character and
*   character index.
*/

int GetOpStateIndex(char cChar, int iCharIndex, int iSubStateIndex, int iSubStateCount)
{
	int iStartStateIndex;
	int iEndStateIndex;

	// Is the character index is zero?

	if (iCharIndex == 0)
	{
		// Yes, so there's no substates to worry about

		iStartStateIndex = 0;
		iEndStateIndex = MAX_OP_STATE_COUNT;
	}
	else
	{
		//  No, so save the substate information

		iStartStateIndex = iSubStateIndex;
		iEndStateIndex = iStartStateIndex + iSubStateCount;
	}

	// Loop through each possible substate and look for a match

	for (int iCurrOpStateIndex = iStartStateIndex; iCurrOpStateIndex < iEndStateIndex; ++iCurrOpStateIndex)
	{
		// Get the current state at the specified character index

		char cOpChar;
		switch (iCharIndex)
		{
		case 0:
			cOpChar = g_OpChars0[iCurrOpStateIndex].cChar;
			break;

		case 1:
			cOpChar = g_OpChars1[iCurrOpStateIndex].cChar;
			break;

		case 2:
			cOpChar = g_OpChars2[iCurrOpStateIndex].cChar;
			break;
		}

		// If the character is a match, return the index

		if (cChar == cOpChar)
			return iCurrOpStateIndex;
	}

	// Return -1 if no match is found

	return -1;
}

/******************************************************************************************
*
*   IsCharOpChar ()
*
*   Determines if the specified character is an operator character.
*/

int IsCharOpChar(char cChar, int iCharIndex)
{
	// Loop through each state in the specified character index and look for a match

	for (int iCurrOpStateIndex = 0; iCurrOpStateIndex < MAX_OP_STATE_COUNT; ++iCurrOpStateIndex)
	{
		// Get the current state at the specified character index

		char cOpChar;
		switch (iCharIndex)
		{
		case 0:
			cOpChar = g_OpChars0[iCurrOpStateIndex].cChar;
			break;

		case 1:
			cOpChar = g_OpChars1[iCurrOpStateIndex].cChar;
			break;

		case 2:
			cOpChar = g_OpChars2[iCurrOpStateIndex].cChar;
			break;
		}

		// If the character is a match, return TRUE

		if (cChar == cOpChar)
			return TRUE;
	}

	// Return FALSE if no match is found

	return FALSE;
}

/******************************************************************************************
*
*   GetOpState()
*
*   Returns the operator state associated with the specified index and state.
*/

OpState GetOpState(int iCharIndex, int iStateIndex)
{
	OpState State;

	// Save the specified state at the specified character index

	switch (iCharIndex)
	{
	case 0:
		State = g_OpChars0[iStateIndex];
		break;

	case 1:
		State = g_OpChars1[iStateIndex];
		break;

	case 2:
		State = g_OpChars2[iStateIndex];
		break;
	}

	return State;
}


/******************************************************************************************
*
*   IsCharDelim()
*
*   Determines whether a character is a delimiter.
*/

int IsCharDelim(char cChar)
{
	// Loop through each delimiter in the array and compare it to the specified character

	for (int iCurrDelimIndex = 0; iCurrDelimIndex < MAX_DELIM_COUNT; ++iCurrDelimIndex)
	{
		// Return TRUE if a match was found

		if (cChar == cDelims[iCurrDelimIndex])
			return TRUE;
	}

	// The character is not a delimiter, so return FALSE

	return FALSE;
}

/******************************************************************************************
*
*	IsCharWhitespace()
*
*	Returns a nonzero if the given character is whitespace, or zero otherwise.
*/

int IsCharWhitespace(char cChar)
{
	// Return true if the character is a space or tab.

	if (cChar == ' ' || cChar == '\t' || cChar == '\n')
		return TRUE;
	else
		return FALSE;
}

/******************************************************************************************
*
*	IsCharNumeric()
*
*	Returns a nonzero if the given character is numeric, or zero otherwise.
*/

int IsCharNumeric(char cChar)
{
	// Return true if the character is between 0 and 9 inclusive.

	if (cChar >= '0' && cChar <= '9')
		return TRUE;
	else
		return FALSE;
}

/******************************************************************************************
*
*	IsCharIdent()
*
*	Returns a nonzero if the given character is part of a valid identifier, meaning it's an
*	alphanumeric or underscore. Zero is returned otherwise.
*/

int IsCharIdent(char cChar)
{
	// Return true if the character is between 0 or 9 inclusive or is an uppercase or
	// lowercase letter or underscore

	if ((cChar >= '0' && cChar <= '9') ||
		(cChar >= 'A' && cChar <= 'Z') ||
		(cChar >= 'a' && cChar <= 'z') ||
		cChar == '_')
		return TRUE;
	else
		return FALSE;
}

/******************************************************************************************
*
*   GetNextChar()
*
*   Returns the next character in the source buffer.
*/

char GetNextChar()
{
	// Make a local copy of the string pointer, unless we're at the end of the source code

	char* pstrCurrLine;
	if (g_CurrLexerState.pCurrLine)
		pstrCurrLine = (char *)g_CurrLexerState.pCurrLine->pData;
	else
		return '\0';

	// If the current lexeme end index is beyond the length of the string, we're past the
	// end of the line

	if (g_CurrLexerState.iCurrLexemeEnd >= (int)strlen(pstrCurrLine))
	{
		// Move to the next node in the source code list

		g_CurrLexerState.pCurrLine = g_CurrLexerState.pCurrLine->pNext;

		// Is the line valid?

		if (g_CurrLexerState.pCurrLine)
		{
			// Yes, so move to the next line of code and reset the lexeme pointers

			pstrCurrLine = (char *)g_CurrLexerState.pCurrLine->pData;

			++g_CurrLexerState.iCurrLineIndex;
			g_CurrLexerState.iCurrLexemeStart = 0;
			g_CurrLexerState.iCurrLexemeEnd = 0;
		}
		else
		{
			// No, so return a null terminator to alert the lexer that the end of the
			// source code has been reached

			return '\0';
		}

	}

	// Return the character and increment the pointer

	return pstrCurrLine[g_CurrLexerState.iCurrLexemeEnd++];
}

/******************************************************************************************
*
*   GetNextToken()
*
*   Returns the next token in the source buffer.
*/

Token GetNextToken()
{
	// Save the current lexer state for future rewinding

	CopyLexerState(g_PrevLexerState, g_CurrLexerState);

	// Start the new lexeme at the end of the last one

	g_CurrLexerState.iCurrLexemeStart = g_CurrLexerState.iCurrLexemeEnd;

	// Set the initial state to the start state

	int iCurrLexState = LEX_STATE_START;

	// Set the current operator state

	int iCurrOpCharIndex = 0;
	int iCurrOpStateIndex = 0;
	OpState CurrOpState;

	// Flag to determine when the lexeme is done

	int iLexemeDone = FALSE;

	// ---- Loop until a token is completed

	// Current character

	char cCurrChar;

	// Current position in the lexeme string buffer

	int iNextLexemeCharIndex = 0;

	// Should the current character be included in the lexeme?

	int iAddCurrChar;

	// Begin the loop

	while (TRUE)
	{
		// Read the next character, and exit if the end of the source has been reached

		cCurrChar = GetNextChar();
		if (cCurrChar == '\0')
			break;

		// Assume the character will be added to the lexeme

		iAddCurrChar = TRUE;

		// Depending on the current state of the lexer, handle the incoming character

		switch (iCurrLexState)
		{
			// If an unknown state occurs, the token is invalid, so exit

		case LEX_STATE_UNKNOWN:

			iLexemeDone = TRUE;

			break;

			// The start state

		case LEX_STATE_START:

			// Just loop past whitespace, and don't add it to the lexeme

			if (IsCharWhitespace(cCurrChar))
			{
				++g_CurrLexerState.iCurrLexemeStart;
				iAddCurrChar = FALSE;
			}

			// An integer is starting

			else if (IsCharNumeric(cCurrChar))
			{
				iCurrLexState = LEX_STATE_INT;
			}

			// A float is starting

			else if (cCurrChar == '.')
			{
				iCurrLexState = LEX_STATE_FLOAT;
			}

			// An identifier is starting

			else if (IsCharIdent(cCurrChar))
			{
				iCurrLexState = LEX_STATE_IDENT;
			}

			// A delimiter has been read

			else if (IsCharDelim(cCurrChar))
			{
				iCurrLexState = LEX_STATE_DELIM;
			}

			// An operator is starting

			else if (IsCharOpChar(cCurrChar, 0))
			{
				// Get the index of the initial operand state

				iCurrOpStateIndex = GetOpStateIndex(cCurrChar, 0, 0, 0);
				if (iCurrOpStateIndex == -1)
					return TOKEN_TYPE_INVALID;

				// Get the full state structure

				CurrOpState = GetOpState(0, iCurrOpStateIndex);

				// Move to the next character in the operator (1)

				iCurrOpCharIndex = 1;

				// Set the current operator

				g_CurrLexerState.iCurrOp = CurrOpState.iIndex;

				iCurrLexState = LEX_STATE_OP;
			}

			// A string is starting, but don't add the opening quote to the lexeme

			else if (cCurrChar == '"')
			{
				iAddCurrChar = FALSE;
				iCurrLexState = LEX_STATE_STRING;
			}

			// It's invalid

			else
				iCurrLexState = LEX_STATE_UNKNOWN;

			break;

			// Integer

		case LEX_STATE_INT:

			// If a numeric is read, keep the state as-is

			if (IsCharNumeric(cCurrChar))
			{
				iCurrLexState = LEX_STATE_INT;
			}

			// If a radix point is read, the numeric is really a float

			else if (cCurrChar == '.')
			{
				iCurrLexState = LEX_STATE_FLOAT;
			}

			// If whitespace or a delimiter is read, the lexeme is done

			else if (IsCharWhitespace(cCurrChar) || IsCharDelim(cCurrChar))
			{
				iAddCurrChar = FALSE;
				iLexemeDone = TRUE;
			}

			// Anything else is invalid

			else
				iCurrLexState = LEX_STATE_UNKNOWN;

			break;

			// Floating-point

		case LEX_STATE_FLOAT:

			// If a numeric is read, keep the state as-is

			if (IsCharNumeric(cCurrChar))
			{
				iCurrLexState = LEX_STATE_FLOAT;
			}

			// If whitespace or a delimiter is read, the lexeme is done

			else if (IsCharWhitespace(cCurrChar) || IsCharDelim(cCurrChar))
			{
				iLexemeDone = TRUE;
				iAddCurrChar = FALSE;
			}

			// Anything else is invalid

			else
				iCurrLexState = LEX_STATE_UNKNOWN;

			break;

			// Identifier

		case LEX_STATE_IDENT:

			// If an identifier character is read, keep the state as-is

			if (IsCharIdent(cCurrChar))
			{
				iCurrLexState = LEX_STATE_IDENT;
			}

			// If whitespace or a delimiter is read, the lexeme is done

			else if (IsCharWhitespace(cCurrChar) || IsCharDelim(cCurrChar) || IsCharOpChar(cCurrChar, 0))
			{
				iAddCurrChar = FALSE;
				iLexemeDone = TRUE;
			}

			// Anything else is invalid

			else
				iCurrLexState = LEX_STATE_UNKNOWN;

			break;

			// Operator

		case LEX_STATE_OP:

			// If the current character within the operator has no substates, we're done

			if (CurrOpState.iSubStateCount == 0)
			{
				iAddCurrChar = FALSE;
				iLexemeDone = TRUE;
				break;
			}

			// Otherwise, find out if the new character is a possible substate

			if (IsCharOpChar(cCurrChar, iCurrOpCharIndex))
			{
				// Get the index of the next substate

				iCurrOpStateIndex = GetOpStateIndex(cCurrChar, iCurrOpCharIndex, CurrOpState.iSubStateIndex, CurrOpState.iSubStateCount);
				if (iCurrOpStateIndex == -1)
				{
					iCurrLexState = LEX_STATE_UNKNOWN;
				}
				else
				{
					// Get the next operator structure

					CurrOpState = GetOpState(iCurrOpCharIndex, iCurrOpStateIndex);

					// Move to the next character in the operator

					++iCurrOpCharIndex;

					// Set the current operator

					g_CurrLexerState.iCurrOp = CurrOpState.iIndex;
				}
			}

			// If not, the lexeme is done

			else
			{
				iAddCurrChar = FALSE;
				iLexemeDone = TRUE;
			}

			break;

			// Delimiter

		case LEX_STATE_DELIM:

			// Don't add whatever comes after the delimiter to the lexeme, because
			// it's done

			iAddCurrChar = FALSE;
			iLexemeDone = TRUE;

			break;

			// String

		case LEX_STATE_STRING:

			// If the current character is a closing quote, finish the lexeme

			if (cCurrChar == '"')
			{
				iAddCurrChar = FALSE;
				iCurrLexState = LEX_STATE_STRING_CLOSE_QUOTE;
			}

			// If it's a newline, the string token is invalid

			else if (cCurrChar == '\n')
			{
				iAddCurrChar = FALSE;
				iCurrLexState = LEX_STATE_UNKNOWN;
			}

			// If it's an escape sequence, switch to the escape state and don't add the
			// backslash to the lexeme

			else if (cCurrChar == '\\')
			{
				iAddCurrChar = TRUE;
				iCurrLexState = LEX_STATE_STRING_ESCAPE;
			}

			// Anything else gets added to the string

			break;

			// Escape sequence

		case LEX_STATE_STRING_ESCAPE:

			// Immediately switch back to the string state, now that the character's
			// been added

			iCurrLexState = LEX_STATE_STRING;

			break;

			// String closing quote

		case LEX_STATE_STRING_CLOSE_QUOTE:

			// Finish the string lexeme

			iAddCurrChar = FALSE;
			iLexemeDone = TRUE;

			break;
		}

		// Add the next character to the lexeme and increment the index

		if (iAddCurrChar)
		{
			g_CurrLexerState.pstrCurrLexeme[iNextLexemeCharIndex++] = cCurrChar;
		}

		// If the lexeme is complete, exit the loop

		if (iLexemeDone)
			break;
	}

	// Complete the lexeme string

	g_CurrLexerState.pstrCurrLexeme[iNextLexemeCharIndex] = '\0';

	// Retract the lexeme end index by one

	--g_CurrLexerState.iCurrLexemeEnd;

	// Determine the token type

	Token TokenType;
	switch (iCurrLexState)
	{
		// Unknown

	case LEX_STATE_UNKNOWN:
		TokenType = TOKEN_TYPE_INVALID;
		break;

		// Integer

	case LEX_STATE_INT:
		TokenType = TOKEN_TYPE_INT;
		break;

		// Float

	case LEX_STATE_FLOAT:
		TokenType = TOKEN_TYPE_FLOAT;
		break;

		// Identifier/Reserved Word

	case LEX_STATE_IDENT:

		// Set the token type to identifier in case none of the reserved words match

		TokenType = TOKEN_TYPE_IDENT;

		// ---- Determine if the "identifier" is actually a reserved word

		if (stricmp(g_CurrLexerState.pstrCurrLexeme, "true") == 0)
			TokenType = TOKEN_TYPE_RSRVD_TRUE;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "false") == 0)
			TokenType = TOKEN_TYPE_RSRVD_FALSE;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "if") == 0)
			TokenType = TOKEN_TYPE_RSRVD_IF;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "else") == 0)
			TokenType = TOKEN_TYPE_RSRVD_ELSE;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "break") == 0)
			TokenType = TOKEN_TYPE_RSRVD_BREAK;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "continue") == 0)
			TokenType = TOKEN_TYPE_RSRVD_CONTINUE;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "for") == 0)
			TokenType = TOKEN_TYPE_RSRVD_FOR;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "while") == 0)
			TokenType = TOKEN_TYPE_RSRVD_WHILE;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "print") == 0)
			TokenType = TOKEN_TYPE_RSRVD_PRINT;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "beep") == 0)
			TokenType = TOKEN_TYPE_RSRVD_BEEP;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "var") == 0)
			TokenType = TOKEN_TYPE_RSRVD_VAR;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "struct") == 0)
			TokenType = TOKEN_TYPE_RSRVD_STRUCT;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "static") == 0)
			TokenType = TOKEN_TYPE_RSRVD_STATIC;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "func") == 0)
			TokenType = TOKEN_TYPE_RSRVD_FUNC;

		else if (stricmp(g_CurrLexerState.pstrCurrLexeme, "return") == 0)
			TokenType = TOKEN_TYPE_RSRVD_RETURN;

		break;

		// Delimiter

	case LEX_STATE_DELIM:

		// Determine which delimiter was found

		switch (g_CurrLexerState.pstrCurrLexeme[0])
		{
		case ',':
			TokenType = TOKEN_TYPE_COMMA;
			break;

		case '(':
			TokenType = TOKEN_TYPE_OPEN_PAREN;
			break;

		case ')':
			TokenType = TOKEN_TYPE_CLOSE_PAREN;
			break;

		case '[':
			TokenType = TOKEN_TYPE_OPEN_BRACE;
			break;

		case ']':
			TokenType = TOKEN_TYPE_CLOSE_BRACE;
			break;

		case '{':
			TokenType = TOKEN_TYPE_OPEN_BLOCK;
			break;

		case '}':
			TokenType = TOKEN_TYPE_CLOSE_BLOCK;
			break;

		case ';':
			TokenType = TOKEN_TYPE_SEMICOLON;
			break;
		}

		break;

		// Operators

	case LEX_STATE_OP:
		TokenType = TOKEN_TYPE_OP;
		break;

		// Strings

	case LEX_STATE_STRING_CLOSE_QUOTE:
		TokenType = TOKEN_TYPE_STRING;
		break;

		// All that's left is whitespace, which means the end of the stream

	default:
		TokenType = TOKEN_TYPE_END_OF_STREAM;
	}

	// Return the token type and set the global copy

	g_CurrLexerState.CurrToken = TokenType;
	return TokenType;
}

/******************************************************************************************
*
*   ReserveCurrentState()
*
*   Moves the lexer back one token.
*/

void SaveLexerState(LexerState& state)
{
	CopyLexerState(state, g_CurrLexerState);
}

void RestoreLexerState(const LexerState& state)
{
	CopyLexerState(g_CurrLexerState, state);
}

/******************************************************************************************
*
*   RewindTokenStream ()
*
*   Moves the lexer back one token.
*/

void RewindTokenStream()
{
	CopyLexerState(g_CurrLexerState, g_PrevLexerState);
}

/******************************************************************************************
*
*   GetCurrToken ()
*
*   Returns the current token.
*/

Token GetCurrToken()
{
	return g_CurrLexerState.CurrToken;
}

/******************************************************************************************
*
*   GetCurrLexeme()
*
*   Returns a pointer to the current lexeme.
*/

char* GetCurrLexeme()
{
	return g_CurrLexerState.pstrCurrLexeme;
}

/******************************************************************************************
*
*   CopyCurrLexeme()
*
*   Makes a physical copy of the current lexeme into the specified string buffer.
*/

void CopyCurrLexeme(char* pstrBuffer)
{
	strcpy(pstrBuffer, g_CurrLexerState.pstrCurrLexeme);
}

/******************************************************************************************
*
*   GetCurrOp ()
*
*   Returns the current operator.
*/

int GetCurrOp()
{
	return g_CurrLexerState.iCurrOp;
}

/******************************************************************************************
*
*   GetLookAheadChar()
*
*   Returns the first character of the next token.
*/

char GetLookAheadChar()
{
	// Save the current lexer state

	LexerState PrevLexerState;
	CopyLexerState(PrevLexerState, g_CurrLexerState);

	// Skip any whitespace that may exist and return the first non-whitespace character

	char cCurrChar;
	while (TRUE)
	{
		cCurrChar = GetNextChar();
		if (!IsCharWhitespace(cCurrChar))
			break;
	}

	// Restore the lexer state

	CopyLexerState(g_CurrLexerState, PrevLexerState);

	// Return the look-ahead character

	return cCurrChar;
}

Token GetLookAheadToken()
{
	Token t = GetNextChar();
	RewindTokenStream();
	return t;
}

/******************************************************************************************
*
*   GetCurrSourceLine ()
*
*   Returns a pointer to the current source line string.
*/

char* GetCurrSourceLine()
{
	if (g_CurrLexerState.pCurrLine)
		return (char *)g_CurrLexerState.pCurrLine->pData;
	else
		return NULL;
}

/******************************************************************************************
*
*   GetCurrSourceLineIndex ()
*
*   Returns the current source line number.
*/

int GetCurrSourceLineIndex()
{
	return g_CurrLexerState.iCurrLineIndex;
}

/******************************************************************************************
*
*   GetLexemeStartIndex ()
*
*   Returns the pointer to the start of the current lexeme
*/

int GetLexemeStartIndex()
{
	return g_CurrLexerState.iCurrLexemeStart;
}
