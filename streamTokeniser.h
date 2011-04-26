/*
* Copyright (c) 1999-2006,2007, Craig S. Harrison
*
* All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef LOCAL_STREAMTOKENISER_H
#define LOCAL_STREAMTOKENISER_H

#include "utilExceptions.h"

#include "standardTokens.h"
#include "cshTypes_Collection.h"

typedef CSHCollection<token *>::collection tokenList;

template<class rClass,class streamClass>
class streamTokeniser
{
	public:
		streamTokeniser(rClass *rc,int caseS):reciever(rc),caseSensitive(caseS){}
		virtual ~streamTokeniser()
		{
			for(int i=0;i<tokList->getNumberOfItems();i++)
				delete tokList->getValueAtIndex(i);

			delete tokList;
		}

		virtual void initTokenList() = 0;
		virtual void tokenise(streamClass *);
		virtual void skipToNextToken();
		virtual int skipComments();
		virtual streamClass *getScObj()
		{
		  return scObj;
		}

		virtual void addToken(token *t);
		streamClass *scObj;
		char ach;
		int charsRead;

	private:
		virtual void internal_initTokenList();

		virtual void doReadNextChar(int skipComments=0);
		rClass *reciever;
		tokenList *tokList;
		int caseSensitive;
};

template<class rClass,class streamClass>
void streamTokeniser<rClass,streamClass>::addToken(token *t)
{
	tokList->add(t);
}

template<class rClass,class streamClass>
void streamTokeniser<rClass,streamClass>::internal_initTokenList()
{
	tokList = new tokenList;

	initTokenList();

	/*
	tokList->add(new identifierToken);

	tokList->add(new WORDToken("CREATE"));
	tokList->add(new WORDToken("TABLE"));
	tokList->add(new WORDToken("NUMBER"));
	tokList->add(new WORDToken("PROMPT"));
	tokList->add(new WORDToken("###"));
	tokList->add(new WORDToken("NVARCHAR"));
	tokList->add(new WORDToken("VARCHAR2"));
	tokList->add(new WORDToken("CLOB"));
	tokList->add(new WORDToken("NTEXT"));
	tokList->add(new WORDToken("BLOB"));
	tokList->add(new WORDToken("GO"));
	tokList->add(new WORDToken("ALTER"));
	tokList->add(new WORDToken("ADD"));
	tokList->add(new WORDToken("DROP"));
	tokList->add(new WORDToken("SEQUENCE"));
	tokList->add(new WORDToken("MINVALUE"));
	tokList->add(new WORDToken("FOREIGN"));
	tokList->add(new WORDToken("PRIMARY"));
	tokList->add(new WORDToken("KEY"));
	tokList->add(new WORDToken("CONSTRAINT"));
	tokList->add(new WORDToken("INDEX"));
	tokList->add(new WORDToken("REFERENCES"));
	tokList->add(new WORDToken("ADD"));
	tokList->add(new WORDToken("NOT"));
	tokList->add(new WORDToken("NULL"));
	tokList->add(new WORDToken("INT"));
	tokList->add(new WORDToken("BINARY"));
	tokList->add(new WORDToken("EXEC"));
	tokList->add(new WORDToken("EXECUTE"));
	tokList->add(new WORDToken("GRANT"));
	tokList->add(new WORDToken("SELECT"));
	tokList->add(new WORDToken("INSERT"));
	tokList->add(new WORDToken("DELETE"));
	tokList->add(new WORDToken("UPDATE"));
	tokList->add(new WORDToken("ON"));
	tokList->add(new WORDToken("TO"));
	tokList->add(new WORDToken("ROLE"));
	*/
	//tokList->add(new SYMBOLToken("("));
	//tokList->add(new SYMBOLToken("/"));
	//tokList->add(new SYMBOLToken(")"));
	//tokList->add(new SYMBOLToken(","));
	//tokList->add(new SYMBOLToken(";"));
	//tokList->add(new SYMBOLToken("'"));
	//tokList->add(new SYMBOLToken("--"));

	for(int i=0;i<tokList->getNumberOfItems();i++)
	{
		tokList->getValueAtIndex(i)->init();
	}
}

template<class rClass,class streamClass>
void streamTokeniser<rClass,streamClass>::tokenise(streamClass *sc)
{
	scObj = sc;
	internal_initTokenList();
	
	skipToNextToken();
	int readNextChar;
	int doMoreChars = 1;
	int reachedEnd = 0;
	while(doMoreChars)
	{
		readNextChar = 1;

		if(!caseSensitive)
			ach = toupper(ach);
		int matchCount=0;
		int notMatchedYetCount = 0;
		int lastMatchIndex = -1;
		for(int i=0;i<tokList->getNumberOfItems();i++)
		{
			token *t = tokList->getValueAtIndex(i);
			if(charsRead==1)
				t->injectChar(ach);
			else
			{
				t->reachedEndOfStream();
				doMoreChars = 0;
			}

			int tokState = t->getState();
			if(tokState==TOKEN_STATE_MATCH_FOUND)
			{
				matchCount++;
				lastMatchIndex = i;
			}
			if(tokState==TOKEN_STATE_MATCH_MORE_CHARS_NEEDED)
			{
				notMatchedYetCount++;
			}
			if(tokState==TOKEN_STATE_NO_MATCH_MORE_CHARS_NEEDED)
			{
				notMatchedYetCount++;
			}
		}

		if(matchCount>0)
		{
			//We have found a match

			token *t = tokList->getValueAtIndex(lastMatchIndex)->makeClone();

			reciever->newToken(t);

			readNextChar = 0;
			if(token::isWhitespace(ach))
			{
				skipToNextToken();

				if(charsRead==0)
					reachedEnd = 1;
			}

			for(int i=0;i<tokList->getNumberOfItems();i++)
			{
				tokList->getValueAtIndex(i)->init();
			}
		}

		if(!reachedEnd)
		{
			if((notMatchedYetCount==0) && (matchCount==0))
			{
				CSHString str("Invalid char in file stream:'");
				str.Cat(ach);
				str.Cat("'");
				throw generalFatalException(str.GetBuffer());
			}
			
			if(readNextChar)
				doReadNextChar();
		}
		else
		{
			doMoreChars = 0;
		}
	}
}

template<class rClass,class streamClass>
int streamTokeniser<rClass,streamClass>::skipComments()
{
	size_t currLoc = scObj->ftell();
	char tempBuff[2];
	charsRead = scObj->fread(tempBuff,2);
	int foundComment = 0;
	if(charsRead==2)
	{
		if((*tempBuff=='-') && (*(tempBuff+1)=='-'))
		{
			//We have a comment
			foundComment = 1;

			charsRead = scObj->fread(&ach,1);
			while((charsRead>0) && (ach!='\r') && (ach!='\n') )
				charsRead = scObj->fread(&ach,1);
		}
	}

	if(!foundComment)
		scObj->fseek(currLoc,SEEK_SET);

	return foundComment;
}

template<class rClass,class streamClass>
void streamTokeniser<rClass,streamClass>::doReadNextChar(int skipComments)
{
	if(skipComments)
	{
		while(this->skipComments());
	}

	charsRead = scObj->fread(&ach,1);
	#ifdef DEBUG_READ
	if(charsRead==1)
	{
		printf("%c",ach);
	}
	#endif
}


template<class rClass,class streamClass>
void streamTokeniser<rClass,streamClass>::skipToNextToken()
{
	doReadNextChar(1);
	while((charsRead==1) && ((ach==' ') || (ach=='\t') || (ach=='\r') || (ach=='\n')))
		doReadNextChar(1);
}

#endif
