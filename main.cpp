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
//#define DEBUG_READ

int doDebugToken = 0;

#define DO_DEBUG_TOKEN

#define _ASSERTE
#include "CSHMemDebug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "standardTokens.h"
#include "streamTokeniser.h"
#include "fileStream.h"

//#define CODETRACKING

#ifdef CODETRACKING
#include "typeinfo.h"
#endif

#define NEXT_COMMAND		0x00
#define JUMP_LABEL			0x01
#define EXIT_LOOP			0x02
#define NOT_IMPLEMENTED		0x03
#define EXIT_REQUESTED		0x04
#define ASSERT_FAILED		0x05
#define JUMP_TO_START		0x06
#define JUMP_FUNCTION		0x07
#define JUMP_RET			0x08

#include "cshTypes_Collection.h"

class noMoreTokens
{
	public:
		noMoreTokens(int){};
};

class couldNotFindLabel
{
	public:
		couldNotFindLabel(CSHString &l):labl(l)
		{
		}
	CSHString labl;
};

class invalidInstructionPointer
{
	public:
		invalidInstructionPointer(int)
		{
		}
};

class languageRuntimeException
{
	public:
		languageRuntimeException(int)
		{
		}
};

#ifdef DO_DEBUG_TOKEN
#define	DEBUG_TOKEN(ID) \
	if(doDebugToken) {printf("DEBUG_TOKEN ::: TYPE:%d REP:%s\n",ID->getTokType(),ID->getRep()->GetBuffer()); \
	csh_pause(); }
#else
#define	DEBUG_TOKEN(ID) 
#endif

class syntaxError
{
	public:
		syntaxError(int)
		{
			int t =34535;
		};
};
	
/*
void cshexit(int i)
{
}
*/

void csh_pause()
{
	int t =3454;
}

class noLabel
{
	public:
		noLabel(int)
		{
		}
};

class stringProcessorEngine;
class instruction
{
	public:
		instruction()
		{
		}

		virtual ~instruction()
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine) = 0;

		virtual void onceOnlyExecute(stringProcessorEngine *spe)
		{
		}

		virtual char *getLabelName()
		{
			throw noLabel(1);
		}

		virtual char *getJumpLabelName()
		{
			throw noLabel(1);
		}

		virtual void setJumpIndex(int index)
		{
		}

		virtual int getJumpIndex()
		{
			return -1;
		}

		virtual void extract(char *s, char *buffer,int from, int to)
		{
			if((from==-1) || (to==-1))
			{
				printf("From or To is -1\n");
				exit(1);
			}

			if(to-from+1<0)
			{
				printf("To and from are the wrong way round.\n");
				exit(1);
			}

			strncpy(buffer,s+from,to-from+1);
			*(buffer+to-from+1)=0;
		}
};

typedef CSHCollection<instruction *>::collection instructionList;
typedef CSHCollection<int>::collection intList;

#define NUMB_OF_STRING_VARS 20
#define NUMB_OF_NUM_VARS 20
#define LENGTH_OF_STRING_VARS 1024
class stringProcessorEngine
{
	public:
		class savedContext
		{
			public:
				savedContext(int *v,char **b,long gNum)
				{
					for(int i=0;i<NUMB_OF_NUM_VARS;i++)
						vars[i] = v[i];

					for(int i=0;i<NUMB_OF_STRING_VARS;i++)
					{
						buffers[i] = new char[LENGTH_OF_STRING_VARS];
						strcpy(buffers[i],b[i]);
					}

					globalNumber = gNum;
				}

				virtual ~savedContext()
				{
					for(int i=0;i<NUMB_OF_STRING_VARS;i++)
						delete []buffers[i];
				}

				virtual void retrieve(int *v,char **b,long *gNum)
				{
					for(int i=0;i<NUMB_OF_NUM_VARS;i++)
						v[i] = vars[i];

					for(int i=0;i<NUMB_OF_STRING_VARS;i++)
					{
						strcpy(b[i],buffers[i]);
					}

					*gNum = globalNumber;
				}

				int vars[NUMB_OF_NUM_VARS];
				long globalNumber;

				char *buffers[NUMB_OF_STRING_VARS];
		};
		typedef CSHCollection<savedContext *>::collection savedContextList;


		stringProcessorEngine(int ac,char *av[]):globalNumber(-1),repeatInstructions(0),textlineStream(NULL),argc(ac),argv(av),lastResultContext(NULL)
		{
			for(int i=0;i<NUMB_OF_NUM_VARS;i++)
				vars[i] = -1;

			for(int i=0;i<NUMB_OF_STRING_VARS;i++)
			{
				buffers[i] = new char[LENGTH_OF_STRING_VARS];
				*buffers[i] = 0;
			}
		}

		virtual ~stringProcessorEngine()
		{
			for(int i=0;i<theInstructionList.getNumberOfItems();i++)
				delete theInstructionList.getValueAtIndex(i);

			for(int i=0;i<NUMB_OF_STRING_VARS;i++)
				delete []buffers[i];

			delete lastResultContext;
			delete textlineStream;
		}

		virtual void initInstructionList()
		{
			while(theInstructionList.getNumberOfItems()>0)
				theInstructionList.removeItemAtIndex(0);
		}

		virtual void endOfInstructionList()
		{
			int insSize = theInstructionList.getNumberOfItems();
			for(int i=0;i<insSize;i++)
			{
				instruction *ins = theInstructionList.getValueAtIndex(i);
				ins->onceOnlyExecute(this);

				try
				{
					CSHString lab = ins->getJumpLabelName();

					int foundLabel = 0;
					for(int j=0;j<insSize;j++)
					{
						instruction *tempins = theInstructionList.getValueAtIndex(j);
						try
						{
							CSHString templab = tempins->getLabelName();
							if(lab.equal(templab.GetBuffer()))
							{
								//We have found a match for the label
								ins->setJumpIndex(j);
								foundLabel = 1;
								break;
							}
						}
						catch(noLabel &)
						{
						}
					}
					if(!foundLabel)
					{
						throw couldNotFindLabel(lab);
					}
				}
				catch(noLabel &)
				{
				}
			}
		}

		virtual void addInstruction(instruction *ins)
		{
			theInstructionList.add(ins);
		}

		virtual void setRepeatInstructions(int rep)
		{
			repeatInstructions = rep;
		}

		virtual void setStringSource(getsStream *gs)
		{
			textlineStream = gs;
		}

		virtual void run()
		{
			int pc = 0;
			int commandPCAction = NEXT_COMMAND;
			
			CSHString currString;

			try
			{
				if(textlineStream!=NULL)
					currString = textlineStream->gets();
			}
			catch(getsStream::noMoreStrings &)
			{
				commandPCAction = EXIT_REQUESTED;
			}

			if(theInstructionList.getNumberOfItems()>0)
			{
				int assertFailed = 0;
				while(commandPCAction!=EXIT_REQUESTED)
				{
					instruction *ins = theInstructionList.getValueAtIndex(pc);

					#ifdef CODETRACKING
					printf("%d,%s\n",pc,typeid(*ins).name());
					#endif
					
					commandPCAction = ins->execute(this,currString.GetBuffer());

					switch(commandPCAction)
					{
						case JUMP_RET:
						{
							//We are returning from a function call!
							int lastItemIndex = pcStack.getNumberOfItems()-1;
							if(lastItemIndex==-1)
								throw invalidInstructionPointer(1);

							pc = pcStack.getValueAtIndex(lastItemIndex);
							pcStack.removeItemAtIndex(lastItemIndex);
							popContext();
							break;
						}
						case JUMP_FUNCTION:
						{
							pushContext();
							pcStack.add(pc+1);
							//omitted break so that JUMP_LABEL is called!
						}
						case JUMP_LABEL:
						{
							pc = ins->getJumpIndex();
							if(pc==-1)
								throw invalidInstructionPointer(1);

							break;
						}
						case JUMP_TO_START:
						{
							pc = theInstructionList.getNumberOfItems()-1;
							//deliberate omission of break so that NEXT_COMMAND: runs after a JUMP_TO_START!
						}
						case NEXT_COMMAND:
						{
							pc++;
							if(pc>=theInstructionList.getNumberOfItems())
							{
								if(repeatInstructions)
								{
									pc = 0;
									if(textlineStream!=NULL)
									{
										try
										{
											currString = textlineStream->gets();
										}
										catch(getsStream::noMoreStrings &)
										{
											commandPCAction = EXIT_REQUESTED;
										}
									}
								}
								else
								{
									if(pc!=theInstructionList.getNumberOfItems())
										throw invalidInstructionPointer(1);
									else
										commandPCAction = EXIT_REQUESTED;
								}
							}
							break;
						}

						case ASSERT_FAILED:
						{
							assertFailed = 1;
							commandPCAction = EXIT_REQUESTED;
							break;
						}
					}
				}
				if(assertFailed)
					throw languageRuntimeException(1);
			}
			else
			{
				throw generalFatalException("no instructions to execute.");
			}
		}

		int printf(const char *format, ...)
		{
			va_list argptr;
	
			va_start( argptr,format);
			vprintf(format,argptr);
			va_end(argptr);

			return 1;
		}

		int vars[NUMB_OF_NUM_VARS];
		long globalNumber;

		char *buffers[NUMB_OF_STRING_VARS];
		char tempBuffer[1024];

		savedContext *lastResultContext;

		int argc;
		char **argv;

	private:
		virtual void pushContext()
		{
			savedContext *sc = new savedContext(vars,buffers,globalNumber);
			contextStack.add(sc);
		}

		virtual void popContext()
		{
			int popIndex = contextStack.getNumberOfItems() - 1;
			savedContext *sc = contextStack.getValueAtIndex(popIndex);
			contextStack.removeItemAtIndex(popIndex);

			if(lastResultContext!=NULL)
				delete lastResultContext;

			lastResultContext = new savedContext(vars,buffers,globalNumber);

			sc->retrieve(vars,buffers,&globalNumber);
			delete sc;
		}

		instructionList theInstructionList;
		int repeatInstructions;
		getsStream *textlineStream;
		intList pcStack;
		savedContextList contextStack;
};

class instructionClasses
{ public:


class instructionSTRMANIP
{
	public:
		class BEFORE_UPPER : public instruction
		{
			public:
				BEFORE_UPPER(int sVal,char *nStr):instruction(),strVal(sVal),additionalStr(nStr)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					CSHString lineString(curLine);

					CSHString str(spe->buffers[strVal]);
					char *strbuff = spe->buffers[strVal];

					CSHString newStr("");

					int size = str.GetLength();
					int count = 0;
					int lastPos = 0;
					for(int i=0;i<size;i++)
					{
						if(isupper(strbuff[i]))
						{
							if(lastPos!=i)
								newStr.Cat(str.extract(lastPos,i));
							newStr.Cat(additionalStr);
							lastPos = i;
							count++;
						}
					}

					if(count>0)
					{
						newStr.Cat(str.extract(lastPos,size));
						strcpy(spe->buffers[strVal],newStr.GetBuffer());
					}

					return NEXT_COMMAND;
				}

			private:
				CSHString additionalStr;
				int strVal;
		};

		class TOUPPER : public instruction
		{
			public:
				TOUPPER(int sVal):instruction(),strVal(sVal)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					CSHString str(spe->buffers[strVal]);
					char *strbuff = spe->buffers[strVal];


					str.toUpper();

					strcpy(strbuff,str.GetBuffer());

					return NEXT_COMMAND;
				}

			private:
				int strVal;
		};
};

class instructionFIND : public instruction
{
	public:
		instructionFIND(int nts,char *sVal,int fInd, int sIn):instruction(),numToSkip(nts),strVal(sVal),fromInd(fInd),storeIn(sIn)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			/*
				int numToSkip;
				CSHString strVal;
				int fromInd;
				int storeIn;
			*/

			CSHString lineString(curLine);
			//spe->vars[numVar] = 

			int startFrom = spe->vars[fromInd];
			int found = 1;
			for(int i=0;i<numToSkip;i++)
			{
				int newStartFromPos = lineString.find(strVal.GetBuffer(),startFrom);
				if(newStartFromPos==-1)
				{
					found = 0;
					break;
				}
				startFrom = newStartFromPos+1;
			}

			if(found)
			{
				spe->vars[storeIn] = startFrom - 1;
			}
			return NEXT_COMMAND;

		}

	private:
		int numToSkip;
		CSHString strVal;
		int fromInd;
		int storeIn;
};



class instructionITOSV : public instruction
{
	public:
		instructionITOSV(int nVar,int sVar):instruction(),numVar(nVar),stringVar(sVar)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			int num = spe->vars[numVar];
			int pos = 0;
			char *str = spe->tempBuffer;
			int num_digits = 0;
			while(num!=0)
			{
				num_digits++;
				div_t qr = div(num,10);
				str[pos++] = qr.rem + '0';
				num = qr.quot;
			}

			str = spe->buffers[stringVar];
			for(int i=0;i<num_digits;i++)
			{
				str[i] = spe->tempBuffer[num_digits-i-1];
			}
			str[num_digits] = 0;

			return NEXT_COMMAND;
		}

	private:
		int numVar;
		int stringVar;
};

class instructionEXTRACT : public instruction
{
	public:
		instructionEXTRACT(int from,int to,int sVar):instruction(),fromVar(from),toVar(to),stringVar(sVar)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			extract(curLine,spe->buffers[stringVar],spe->vars[fromVar],spe->vars[toVar]);

			return NEXT_COMMAND;
		}

	private:

		int toVar;
		int fromVar;
		int stringVar;
};

class instructionEXTRACTALL : public instruction
{
	public:
		instructionEXTRACTALL(int sVar):instruction(),stringVar(sVar)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			strcpy(spe->buffers[stringVar],curLine);
	
			return NEXT_COMMAND;
		}

	private:
		int stringVar;
};

class instructionPRINT : public instruction
{
	public:

		class SV : public instruction
		{
			public:
				SV(int sVar):instruction(),stringVar(sVar)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->printf("%s",spe->buffers[stringVar]);
					return NEXT_COMMAND;
				}

			private:
				int stringVar;
		};

		class NV : public instruction
		{
			public:
				NV(int nVar):instruction(),numVar(nVar)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->printf("%d",spe->vars[numVar]);
					return NEXT_COMMAND;
				}

			private:
				int numVar;
		};

		class NEWLINE : public instruction
		{
			public:
				NEWLINE():instruction()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->printf("\n");
					return NEXT_COMMAND;
				}

			private:
		};

		class WHOLELINE : public instruction
		{
			public:
				WHOLELINE():instruction()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->printf("%s",curLine);
					return NEXT_COMMAND;
				}

			private:
		};

		class DQUOTE : public instruction
		{
			public:
				DQUOTE():instruction()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->printf("\"");
					return NEXT_COMMAND;
				}

			private:
		};

		class HEAD : public instruction
		{
			public:
				HEAD(int nVar):instruction(),numVar(nVar)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					extract(curLine,spe->tempBuffer,0,spe->vars[numVar]);
					spe->printf("%s",spe->tempBuffer);
					return NEXT_COMMAND;
				}

			private:
				int numVar;
		};

		class TAIL : public instruction
		{
			public:
				TAIL(int nVar):instruction(),numVar(nVar)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					extract(curLine,spe->tempBuffer,spe->vars[numVar],strlen(curLine));
					spe->printf("%s",spe->tempBuffer);
					return NEXT_COMMAND;
				}

			private:
				int numVar;
		};

		class GLOBNUMINC : public instruction
		{
			public:
				GLOBNUMINC():instruction()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->printf("%ld",spe->globalNumber++);
					return NEXT_COMMAND;
				}

			private:
		};

		class GLOBNUMDEC : public instruction
		{
			public:
				GLOBNUMDEC():instruction()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->printf("%ld",spe->globalNumber--);
					return NEXT_COMMAND;
				}

			private:
		};

		class TAB : public instruction
		{
			public:
				TAB():instruction()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->printf("\t");
					return NEXT_COMMAND;
				}

			private:
		};
		class DQSTR : public instruction
		{
			public:
				DQSTR(char *s):instruction(),str(s)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->printf("%s",str.GetBuffer());
					return NEXT_COMMAND;
				}

			private:
				CSHString str;
		};


	private:
};

class instructionADJUST : public instruction
{
	public:
		instructionADJUST(int nVar,int f,int a):instruction(),numVar(nVar),factor(f),amount(a)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			spe->vars[numVar] += factor*amount;

			return NEXT_COMMAND;
		}

	private:
		int numVar;
		int factor;
		int amount;
};

class instructionSETGLOBALNUM : public instruction
{
	public:
		instructionSETGLOBALNUM(int n):instruction(),num(n)
		{
		}

		virtual void onceOnlyExecute(stringProcessorEngine *spe)
		{
				spe->globalNumber = num;
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			return NEXT_COMMAND;
		}

	private:
		int num;
};

class instructionENDWHENGLOBALEQUAL : public instruction
{
	public:
		instructionENDWHENGLOBALEQUAL(int n):instruction(),num(n)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			int exitAction = NEXT_COMMAND;

			if(spe->globalNumber == num)
				exitAction = EXIT_REQUESTED;

			return exitAction;
		}

	private:
		int num;
};

class instructionJUMPIF
{
	public:
		class JUMPIFBase : public instruction
		{
			public:
				JUMPIFBase(char *l):instruction(),lbl(l),jumpToIndex(-1)
				{
				}

				virtual char *getJumpLabelName()
				{
					return lbl.GetBuffer();
				}

				virtual void setJumpIndex(int index)
				{
					jumpToIndex = index;
				}

				virtual int getJumpIndex()
				{
					return jumpToIndex;
				}

			private:
				CSHString lbl;
				int jumpToIndex;

		};

		class NEG : public JUMPIFBase
		{
			public:
				NEG(int n,char *l):JUMPIFBase(l),num(n)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					int retVal = NEXT_COMMAND;
					if(spe->vars[num]==-1)
					{
						retVal = JUMP_LABEL;
					}

					return retVal;
				}

			private:
				int num;
		};

		class NNEG : public JUMPIFBase
		{
			public:
				NNEG(int n,char *l):JUMPIFBase(l),num(n)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					int retVal = NEXT_COMMAND;
					if(spe->vars[num]!=-1)
					{
						retVal = JUMP_LABEL;
					}

					return retVal;
				}

			private:
				int num;
		};

		class NZ : public JUMPIFBase
		{
			public:
				NZ(int n,char *l):JUMPIFBase(l),num(n)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					int retVal = NEXT_COMMAND;
					if(spe->vars[num]!=0)
					{
						retVal = JUMP_LABEL;
					}

					return retVal;
				}

			private:
				int num;
		};

		class ALL : public JUMPIFBase
		{
			public:
				ALL(char *l):JUMPIFBase(l)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					return JUMP_LABEL;
				}

			private:
		};

		class SMALL : public JUMPIFBase
		{
			public:
				SMALL(int fNum,int sNum,char *l):JUMPIFBase(l),firstNum(fNum),secondNum(sNum)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					int retVal = NEXT_COMMAND;
					if(spe->vars[firstNum]<spe->vars[secondNum])
					{
						retVal = JUMP_LABEL;
					}
					return retVal;
				}

			private:
				int firstNum;
				int secondNum;
		};

		class GNEG : public JUMPIFBase
		{
			public:
				GNEG(char *l):JUMPIFBase(l)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					int retVal = NEXT_COMMAND;
					if(spe->globalNumber<0)
					{
						retVal = JUMP_LABEL;
					}
					return retVal;
				}

			private:
		};

		class GNNEG : public JUMPIFBase
		{
			public:
				GNNEG(char *l):JUMPIFBase(l)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					int retVal = NEXT_COMMAND;
					if(spe->globalNumber>=0)
					{
						retVal = JUMP_LABEL;
					}
					return retVal;
				}

			private:
		};

};

class instructionLABEL : public instruction
{
	public:
		instructionLABEL(char *l):instruction(),lbl(l)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			return NEXT_COMMAND;
		}

		virtual char *getLabelName()
		{
			return lbl.GetBuffer();
		}


	private:
		CSHString lbl;
};

class instructionINDFIND : public instruction
{
	public:

		class SV : public instruction
		{
			public:
				SV(int sVar,int iV,int nVar):instruction(),strVar(sVar),indVar(iV),numVar(nVar)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					CSHString lineString(curLine);
					spe->vars[numVar] = lineString.find(spe->buffers[strVar],spe->vars[indVar]);

					return NEXT_COMMAND;
				}

			private:
				int strVar;
				int indVar;
				int numVar;
		};



		instructionINDFIND(char *s,int iV,int nVar):instruction(),string(s),indVar(iV),numVar(nVar)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			CSHString lineString(curLine);
			spe->vars[numVar] = lineString.find(string.GetBuffer(),spe->vars[indVar]);

			return NEXT_COMMAND;
		}

	private:
		CSHString string;
		int indVar;
		int numVar;
};

class instructionSWAP : public instruction
{
	public:
		instructionSWAP(int nVar1,int nVar2):instruction(),numVar1(nVar1),numVar2(nVar2)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			int temp = spe->vars[numVar1];
			spe->vars[numVar1] = spe->vars[numVar2];
			spe->vars[numVar2] = temp;
			
			return NEXT_COMMAND;
		}

	private:
		int numVar1;
		int numVar2;
};

class instructionINIT
{
	public:
		class NV : public instruction
		{
			public:
				NV(int nVar,int n):instruction(),numVar(nVar),num(n)
				{
				}

				virtual ~NV()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					spe->vars[numVar] = num;
					return NEXT_COMMAND;
				}

			private:
				int numVar;
				int num;
		};

		class SV : public instruction
		{
			public:
				SV(int sVar,char *s):instruction(),strVar(sVar),str(s)
				{
				}

				virtual ~SV()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					strcpy(spe->buffers[strVar],str.GetBuffer());
					return NEXT_COMMAND;
				}

			private:
				int strVar;
				CSHString str;
		};
};

class instructionREPLACE
{
	public:
		class ALL : public instruction
		{
			public:
				ALL(char *s1,char *s2,int nVar):instruction(),str1(s1),str2(s2),numVar(nVar)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					CSHString str;

					CSHString cLine(curLine);
					str.stringSubstitution(&cLine,str1.GetBuffer(),str2.GetBuffer());

					strcpy(spe->buffers[numVar],cLine.GetBuffer());

					return NEXT_COMMAND;
				}

			private:
				int numVar;
				CSHString str1;
				CSHString str2;
		};
};

class instructionINDENDOFCODESTR : public instruction
{
	public:
		instructionINDENDOFCODESTR(int nVar1,int nVar2):instruction(),numVar1(nVar1),numVar2(nVar2)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			char *string = curLine;

			int val = -1;
			int length = strlen(string);
			int startFromIndex = spe->vars[numVar1];

			int DBClast = 0;
			while(isLeadByte(string[startFromIndex]) || startFromIndex>=length)
			{
				startFromIndex+=2;
				DBClast = 1;
			}

			for(int i=startFromIndex;i<length;)
			{
				if(string[i]=='\"')
					if(!DBClast)
					{
						if(string[i-1]!='\\')
						{
							val = i;
							break;
						}
					}
					else
					{
						val = i;
						break;
					}


				if(!isLeadByte(string[i]))
				{
					i++;
					DBClast = 0;
				}
				else
					while(isLeadByte(string[i]) || i>=length)
					{
						i+=2;
						DBClast = 1;
					}

			}
			
			spe->vars[numVar2] = val;
				
			return NEXT_COMMAND;
		}

	private:

		int isLeadByte(unsigned char c)
		{
			/*
			if(!gotCPINFO)
			{
				GetCPInfo(codepageToUse,&info);
				gotCPINFO = 1;
			}

			int retVal = 0;
			int rangeIndex = 0;
			while( (info.LeadByte[rangeIndex]!=0) && (info.LeadByte[rangeIndex+1]!=0)) 
			{

				if((c>=info.LeadByte[rangeIndex]) && (c<=info.LeadByte[rangeIndex+1]))
				{
					retVal = 1;
					break;
				}

				rangeIndex+=2;
			}
			*/

			int retVal = 0;

			return retVal;
		}

		int numVar1;
		int numVar2;
};

class instructionSEARCHLINEINDEX : public instruction
{
	public:
		instructionSEARCHLINEINDEX(char *fName,int sVar,int nVar1):instruction(),fileName(fName),strVar(sVar),numVar1(nVar1)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			FILE *f=fopen(fileName.GetBuffer(),"rt");
			if(f==NULL)
			{
				spe->vars[numVar1] = -1;
				return NEXT_COMMAND;
			}

			char buffer[1024];
			char *bb = fgets(buffer,1024,f);
			int finalIndex = -1;
			int currLineIndex = 0;
			while(bb!=NULL)
			{
				if(strncmp(spe->buffers[strVar],buffer,strlen(spe->buffers[strVar]))==0)
				{
					finalIndex = currLineIndex;
					break;
				}
				currLineIndex++;
				bb = fgets(buffer,1024,f);
			}

			fclose(f);

			spe->vars[numVar1] = finalIndex;
				
			return NEXT_COMMAND;
		}

	private:
		CSHString fileName;
		int strVar;
		int numVar1;
};

class instructionINDSEARCHLINEINDEX : public instruction
{
	public:
		instructionINDSEARCHLINEINDEX(char *fName,int sVar,int nVar1,int nVar2):instruction(),fileName(fName),strVar(sVar),numVar1(nVar1),numVar2(nVar2)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			FILE *f=fopen(fileName.GetBuffer(),"rt");
			if(f==NULL)
			{
				spe->vars[numVar2] = -1;
				return NEXT_COMMAND;
			}

			char buffer[1024];

			char *bb;
			for(int i=0;i<spe->vars[numVar1];i++)
			{
				bb = fgets(buffer,1024,f);
				if(bb==NULL)
				{
					break;
				}
			}
			
			if(bb!=NULL)
			{

				char *bb = fgets(buffer,1024,f);
				int finalIndex = -1;
				int currLineIndex = spe->vars[numVar1];
				while(bb!=NULL)
				{
					if(strncmp(spe->buffers[strVar],buffer,strlen(spe->buffers[strVar]))==0)
					{
						finalIndex = currLineIndex;
						break;
					}
					currLineIndex++;
					bb = fgets(buffer,1024,f);
				}
				if(bb==NULL)
					spe->vars[numVar2] = -1;
				else
					spe->vars[numVar2] = finalIndex;
			}
			else
				spe->vars[numVar2] = -1;

			fclose(f);

			return NEXT_COMMAND;
		}

	private:
		CSHString fileName;
		int strVar;
		int numVar1;
		int numVar2;
};

class instructionLOADLINE : public instruction
{
	public:
		instructionLOADLINE(char *fName,int nVar1,int sVar):instruction(),fileName(fName),numVar1(nVar1),strVar(sVar)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			FILE *f=fopen(fileName.GetBuffer(),"rt");
			if(f==NULL)
			{
				*(spe->buffers[strVar]) = 0;
				return NEXT_COMMAND;
			}

			char buffer[1024];

			for(int i=0;i<=spe->vars[numVar1];i++)
			{
				fgets(buffer,1024,f);
			}

			fclose(f);

			for(unsigned int i2=0;i2<strlen(buffer);i2++)
				if((buffer[i2]=='\r') || (buffer[i2]=='\n') )
				{
					buffer[i2] = 0;
					break;
				}

			strcpy(spe->buffers[strVar],buffer);
				
			return NEXT_COMMAND;
		}

	private:
		CSHString fileName;
		int numVar1;
		int strVar;
};

class instructionINDFIRSTNONWS : public instruction
{
	public:
		instructionINDFIRSTNONWS(int nVar1,int nVar2):instruction(),numVar1(nVar1),numVar2(nVar2)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			CSHString lineString(curLine);

			int res = -1;
			for(int i=spe->vars[numVar1];i<lineString.length();i++)
			{
				if( (lineString[i]!=' ') &&
					(lineString[i]!='\t') &&
					(lineString[i]!='\r') &&
					(lineString[i]!='\n')
				  )
				{
					res = i;
					break;
				}
			}
			
			spe->vars[numVar2] = res;

			return NEXT_COMMAND;
		}

	private:
		CSHString fileName;
		int numVar1;
		int numVar2;
};

class instructionINDFIRSTWS : public instruction
{
	public:
		instructionINDFIRSTWS(int nVar1,int nVar2):instruction(),numVar1(nVar1),numVar2(nVar2)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			CSHString lineString(curLine);

			int res = -1;
			for(int i=spe->vars[numVar1];i<lineString.length();i++)
			{
				if( (lineString[i]==' ') ||
					(lineString[i]=='\t') ||
					(lineString[i]=='\r') ||
					(lineString[i]=='\n')
				  )
				{
					res = i;
					break;
				}
			}
			
			spe->vars[numVar2] = res;

			return NEXT_COMMAND;
		}

	private:
		CSHString fileName;
		int numVar1;
		int numVar2;
};

class instructionCURRENTLINETOSV : public instruction
{
	public:
		instructionCURRENTLINETOSV(int nVar1):instruction(),numVar1(nVar1)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			strcpy(spe->buffers[numVar1],curLine);

			return NEXT_COMMAND;
		}

	private:
		CSHString fileName;
		int numVar1;
};

class instructionSVTOI : public instruction
{
	public:
		instructionSVTOI(int sVar1,int nVar1):instruction(),strVar1(sVar1),numVar1(nVar1)
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			int num = 0;
			char *numStr = spe->buffers[strVar1];
			for(unsigned int i=0;i<strlen(numStr);i++)
			{
				if((*(numStr+i)>='0') && (*(numStr+i)<='9'))
				{
					num *= 10;
					num += *(numStr+i) - '0';
				}
				else
				{
					break;
				}
			}

			spe->vars[numVar1] = num;

			return NEXT_COMMAND;
		}

	private:
		CSHString fileName;
		int strVar1;
		int numVar1;
};

class instructionSOURCE
{
	public:
		class ARG : public instruction
		{
			public:
				ARG(int argn):instruction(),argNum(argn)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					return NEXT_COMMAND;
				}

				virtual void onceOnlyExecute(stringProcessorEngine *spe)
				{
					spe->setStringSource(new getsFileStream(spe->argv[argNum]));
				}


			private:
				int argNum;
		};

		class DIRECT : public instruction
		{
			public:
				DIRECT(char *fName):instruction(),fileName(fName)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					return NEXT_COMMAND;
				}

				virtual void onceOnlyExecute(stringProcessorEngine *spe)
				{
					spe->setStringSource(new getsFileStream(fileName.GetBuffer()));
				}


			private:
				CSHString fileName;
		};
};

class instructionREPEAT : public instruction
{
	public:
		instructionREPEAT():instruction()
		{
		}

		virtual void onceOnlyExecute(stringProcessorEngine *spe)
		{
			spe->setRepeatInstructions(1);
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			return NEXT_COMMAND;
		}

	private:
};

class instructionEXECMODE : public instruction
{
	public:
		instructionEXECMODE(int ftpi):instruction(),fileToProcessIndex(ftpi)
		{
		}

		virtual void onceOnlyExecute(stringProcessorEngine *spe)
		{
			spe->setRepeatInstructions(1);
			spe->setStringSource(new getsFileStream(spe->argv[fileToProcessIndex]));
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			return NEXT_COMMAND;
		}

	private:
		int fileToProcessIndex;
};

class instructionASSERT
{
	public:
		class SV : public instruction
		{
			public:
				SV(int n,char *sc):instruction(),varNum(n),scmp(sc)
				{
				}

				virtual void onceOnlyExecute(stringProcessorEngine *spe)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					int retVal = NEXT_COMMAND;
					if(strcmp(spe->buffers[varNum],scmp.GetBuffer())!=0)
					{
						retVal = ASSERT_FAILED;
					}

					return retVal;
				}

			private:
				int varNum;
				CSHString scmp;
		};

		class NV : public instruction
		{
			public:
				NV(int n,int nc):instruction(),varNum(n),ncmp(nc)
				{
				}

				virtual void onceOnlyExecute(stringProcessorEngine *spe)
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					int retVal = NEXT_COMMAND;
					if(spe->vars[varNum]!=ncmp)
					{
						retVal = ASSERT_FAILED;
					}

					return retVal;
				}

			private:
				int varNum;
				int ncmp;
		};
};

class instructionGLOBNUMINC : public instruction
{
	public:
		instructionGLOBNUMINC():instruction()
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			spe->globalNumber++;
			return NEXT_COMMAND;
		}

	private:
};

class instructionGLOBNUMDEC : public instruction
{
	public:
		instructionGLOBNUMDEC():instruction()
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			spe->globalNumber--;
			return NEXT_COMMAND;
		}

	private:
};


class instructionSTRIP
{
	public:
		class BOTHENDSWHITESPACE : public instruction
		{
			public:
				BOTHENDSWHITESPACE(int num):instruction(),svnum(num)
				{
				}

				virtual ~BOTHENDSWHITESPACE()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					CSHString sv(spe->buffers[svnum]);

					sv.removeWhitespaceFromBothEnds();

					strcpy(spe->buffers[svnum],sv.GetBuffer());

					return NEXT_COMMAND;
				}

			private:
				int svnum;
		};

		class HEADWHITESPACE : public instruction
		{
			public:
				HEADWHITESPACE(int num):instruction(),svnum(num)
				{
				}

				virtual ~HEADWHITESPACE()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					//Find the first non whitespace char
					CSHString sv(spe->buffers[svnum]);
					int index = -1;
					for(int i=0;i<sv.GetLength();i++)
					{
						//
						if((sv[i]!=' ') && (sv[i]!='\t'))
						{
							index = i;
							break;
						}
					}

					if(index!=-1)
					{
						CSHString ans = sv.extract(index,sv.GetLength());
						strcpy(spe->buffers[svnum],ans.GetBuffer());
					}

					return NEXT_COMMAND;
				}

			private:
				int svnum;
		};

		class TAILWHITESPACE : public instruction
		{
			public:
				TAILWHITESPACE(int num):instruction(),svnum(num)
				{
				}

				virtual ~TAILWHITESPACE()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					//Find the first non whitespace char
					CSHString sv(spe->buffers[svnum]);
					int index = -1;
					int startFrom = sv.GetLength();
					for(int i=startFrom-1;i>=0;i--)
					{
						//
						if((sv[i]!=' ') && (sv[i]!='\t'))
						{
							index = i;
							break;
						}
					}

					if(index!=-1)
					{
						CSHString ans = sv.extract(0,index);
						strcpy(spe->buffers[svnum],ans.GetBuffer());
					}

					return NEXT_COMMAND;
				}

			private:
				int svnum;
		};

		class LINEBREAKS : public instruction
		{
			public:
				LINEBREAKS(int num):instruction(),svnum(num)
				{
				}

				virtual ~LINEBREAKS()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					//Find the first non line break char
					CSHString sv(spe->buffers[svnum]);
					int index = -1;
					int startFrom = sv.GetLength();
					for(int i=startFrom-1;i>=0;i--)
					{
						//
						if((sv[i]!='\r') && (sv[i]!='\n'))
						{
							index = i;
							break;
						}
					}

					if(index!=-1)
					{
						CSHString ans = sv.extract(0,index);
						strcpy(spe->buffers[svnum],ans.GetBuffer());
					}

					return NEXT_COMMAND;
				}

			private:
				int svnum;
		};

};

class instructionDEBUG
{
public:
	class DUMPALL : public instruction
	{
		public:
			DUMPALL():instruction()
			{
			}

			virtual ~DUMPALL()
			{
			}

			virtual int execute(stringProcessorEngine *spe,char *curLine)
			{
				//CSHString sv(spe->buffers[svnum]);
				printf("\n---------------DEBUG-DUMPALL---------------------\n");
				for(int i=0;i<NUMB_OF_NUM_VARS;i++)
				{
					printf("numvar:[%d]:%d\n",i,spe->vars[i]);
				}

				for(int i=0;i<NUMB_OF_STRING_VARS;i++)
				{
					printf("strvar:[%d]:!%s!\n",i,spe->buffers[i]);
				}

				printf("global:%d\n",spe->globalNumber);
				printf("\n---------------DEBUG-DUMPALL---------------------\n");
				return NEXT_COMMAND;
			}

		private:
	};
};

class instructionSTRCAT : public instruction
{
	public:
		instructionSTRCAT(int dn,int sn):instruction(),svDestVarNum(dn),svSrcVarNum(sn)
		{
		}

		virtual ~instructionSTRCAT()
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			strcat(spe->buffers[svDestVarNum],spe->buffers[svSrcVarNum]);
			return NEXT_COMMAND;
		}

	private:
		int svDestVarNum;
		int svSrcVarNum;
};

class instructionSTREXEC : public instruction
{
	public:
		instructionSTREXEC(int sne):instruction(),execStrVarNum(sne)
		{
		}

		virtual ~instructionSTREXEC()
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			system(spe->buffers[execStrVarNum]);
			return NEXT_COMMAND;
		}

	private:
		int execStrVarNum;
};

class instructionCALL : public instruction
{
	public:
		instructionCALL(char *fn):instruction(),functionName(fn)
		{
		}

		virtual ~instructionCALL()
		{
		}

		virtual char *getJumpLabelName()
		{
			return functionName.GetBuffer();
		}

		virtual void setJumpIndex(int index)
		{
			jumpToIndex = index;
		}

		virtual int getJumpIndex()
		{
			return jumpToIndex;
		}


		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			return JUMP_FUNCTION;
		}

	private:
		CSHString functionName;
		int jumpToIndex;
};

class instructionQUIT : public instruction
{
	public:
		instructionQUIT():instruction()
		{
		}

		virtual ~instructionQUIT()
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			return JUMP_TO_START;
		}

	private:
};

class instructionFUNCTION : public instruction
{
	public:
		instructionFUNCTION(char *fn):instruction(),functionName(fn)
		{
		}

		virtual ~instructionFUNCTION()
		{
		}

		virtual char *getLabelName()
		{
			return functionName.GetBuffer();
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			return NEXT_COMMAND;
		}

	private:
		CSHString functionName;
};

class instructionRET : public instruction
{
	public:
		instructionRET():instruction()
		{
		}

		virtual ~instructionRET()
		{
		}

		virtual int execute(stringProcessorEngine *spe,char *curLine)
		{
			return JUMP_RET;
		}

	private:
};

class instructionGETFCTX : public instruction
{
	public:
		class SV : public instruction
		{
			public:
				SV(int dVar,int sVar):instruction(),destVar(dVar),srcVar(sVar)
				{
				}

				virtual ~SV()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					if(spe->lastResultContext!=NULL)
						strcpy(spe->buffers[destVar],spe->lastResultContext->buffers[srcVar]);
					return NEXT_COMMAND;
				}

			private:
				int srcVar;
				int destVar;
		};
		class NV : public instruction
		{
			public:
				NV(int dVar,int sVar):instruction(),destVar(dVar),srcVar(sVar)
				{
				}

				virtual ~NV()
				{
				}

				virtual int execute(stringProcessorEngine *spe,char *curLine)
				{
					if(spe->lastResultContext!=NULL)
						spe->vars[destVar] = spe->lastResultContext->vars[srcVar];
					return NEXT_COMMAND;
				}

			private:
				int srcVar;
				int destVar;
		};
};

};//End of instructionClasses


class tokenChecker
{
	public:
		tokenChecker()
		{
		}

		virtual ~tokenChecker()
		{
		}

		virtual int checkToken(token *t) = 0;
		virtual void dumpTokenSyntax() = 0;
};

class tokenChecker_string : public tokenChecker
{
	public:
		tokenChecker_string(CSHString &s):tokenChecker(),theString(s)
		{
		}
		virtual int checkToken(token *t)
		{
			int retVal =0;
			if(t->like(theString.GetBuffer()))
			{
				retVal = 1;
			}

			return retVal;
		}

		virtual void dumpTokenSyntax()
		{
			printf("%s",theString.GetBuffer());
		}
	
	private:
		CSHString theString;
};

class tokenChecker_type : public tokenChecker
{
	public:
		tokenChecker_type(int t):tokenChecker(),type(t)
		{
		}
		virtual int checkToken(token *t)
		{
			int retVal =0;
			if(t->getTokType() == type)
			{
				retVal = 1;
			}

			return retVal;
		}

		virtual void dumpTokenSyntax()
		{
			if(type==TOKEN_TYPE_UNDEF)
				printf("TOKEN_TYPE_UNDEF");
			else if(type==TOKEN_TYPE_ID)
				printf("TOKEN_TYPE_ID");
			else if(type==TOKEN_TYPE_WORD)
				printf("TOKEN_TYPE_WORD");
			else if(type==TOKEN_TYPE_SYM)
				printf("TOKEN_TYPE_SYM");
			else if(type==TOKEN_TYPE_NUM)
				printf("TOKEN_TYPE_NUM");
			else if(type==TOKEN_TYPE_DQ_STRING)
				printf("TOKEN_TYPE_DQ_STRING");
			else if(type==TOKEN_TYPE_SQ_STRING)
				printf("TOKEN_TYPE_SQ_STRING");
		}

	private:
		int type;
};

typedef CSHCollection<tokenChecker *>::collection tokenCheckerList;
typedef CSHCollection<CSHString *>::collection CSHStringList;

class tokenStream
{
	public:
		tokenStream()
		{
		}
		
		virtual ~tokenStream()
		{
		}

		virtual token *getNextToken() = 0;
};

class textFileManipSyntaxError
{
	public:
		textFileManipSyntaxError(int)
		{
		}
};

class commandFactory
{
	public:
		commandFactory(char *cName):parsedTokenList(0),commandName(cName)
		{
		}

		virtual ~commandFactory()
		{
		  int i;
			for(i=0;i<numOfTokCheckStrings;i++)
			{
				for(int j=0;j<theTokenCheckLists[i].getNumberOfItems();j++)
				{
					delete theTokenCheckLists[i].getValueAtIndex(j);
				}
			}

			delete []theTokenCheckLists;

			for(i=0;i<orderStrings.getNumberOfItems();i++)
				delete orderStrings.getValueAtIndex(i);
		}

		virtual void initTokenList();

		virtual instruction *make(token *,tokenStream *) = 0;

		virtual void skipNextToken(tokenStream *ts)
		{
			getNextToken(ts);
		}

		virtual token *getNextToken(tokenStream *ts)
		{
			token *t = ts->getNextToken();

			int numOfMatches = 0;

			for(int i=0;i<numOfTokCheckStrings;i++)
			{
				if(theTokenCheckLists[i].getNumberOfItems()>currentTokenIndex)
				{
					numOfMatches += theTokenCheckLists[i].getValueAtIndex(currentTokenIndex)->checkToken(t);
				}
			}

			if(numOfMatches==0)
				throw textFileManipSyntaxError(1);

			currentTokenIndex++;
			return t;
		}

		virtual int getNum(token *t)
		{
			return atoi(t->getRep()->GetBuffer());
		}

		virtual void dumpCommandSyntax()
		{
			for(int i=0;i<numOfTokCheckStrings;i++)
			{
				printf("%s ",getCommandName()->GetBuffer());
				for(int j=0;j<theTokenCheckLists[i].getNumberOfItems();j++)
				{
					tokenChecker *tc = theTokenCheckLists[i].getValueAtIndex(j);
					tc->dumpTokenSyntax();
					printf(" ");
				}
				printf("\n");
			}
		}

		virtual CSHString *getCommandName()
		{
			return &commandName;
		}

	protected:
		virtual void addOrderString(CSHString *s)
		{
			orderStrings.add(s);
		}

		virtual CSHStringList *getTokenOrderStrings()
		{
			return &orderStrings;
		}

		virtual CSHString getUnquotedString(CSHString &s)
		{
			int length = s.length();
			if(length<2)
				throw textFileManipSyntaxError(1);

			char *buff = s.GetBuffer();
			if(buff[0]=='\"')
			{
				if(buff[length-1]!='\"')
					throw textFileManipSyntaxError(1);
			}
			else if(buff[0]=='\'')
			{
				if(buff[length-1]!='\'')
					throw textFileManipSyntaxError(1);
			}

			CSHString retVal = s.extract(1,length-1);

			return retVal;
		}

	private:
		int parsedTokenList;
		int currentTokenIndex;
		tokenCheckerList *theTokenCheckLists;
		CSHStringList orderStrings;
		int numOfTokCheckStrings;
		CSHString commandName;
};

void commandFactory::initTokenList()
{
	if(!parsedTokenList)
	{
		CSHStringList *tokOrderStrings = getTokenOrderStrings();

		numOfTokCheckStrings = tokOrderStrings->getNumberOfItems();
		theTokenCheckLists = new tokenCheckerList[numOfTokCheckStrings];
		
		for(int i=0;i<numOfTokCheckStrings;i++)
		{
			CSHString *tokOrderString = tokOrderStrings->getValueAtIndex(i);
			int tokStrSize = tokOrderString->length();

			int currPos = 0;
			//':':NUM:',':NUM:;
			int nextPos = -1;
			while((*tokOrderString)[currPos]!=';')
			{
				if((*tokOrderString)[currPos]=='\'')
				{
					//The token should be check for a like
					int endSQ = tokOrderString->find("'",currPos+1);
					CSHString s = tokOrderString->extract(currPos+1,endSQ);

					theTokenCheckLists[i].add(new tokenChecker_string(s));

					nextPos = endSQ+1;
				}
				else if(tokOrderString->find("NUM",currPos)==currPos)
				{
					//The token should be check for a numeric type
					theTokenCheckLists[i].add(new tokenChecker_type(TOKEN_TYPE_NUM));
					nextPos = currPos+3;
				}
				else if(tokOrderString->find("SQSTR",currPos)==currPos)
				{
					//The token should be check for a numeric type
					theTokenCheckLists[i].add(new tokenChecker_type(TOKEN_TYPE_SQ_STRING));
					nextPos = currPos+5;
				}
				else if(tokOrderString->find("DQSTR",currPos)==currPos)
				{
					//The token should be check for a numeric type
					theTokenCheckLists[i].add(new tokenChecker_type(TOKEN_TYPE_DQ_STRING));
					nextPos = currPos+5;
				}
				else if(tokOrderString->find("ID",currPos)==currPos)
				{
					//The token should be check for a numeric type
					theTokenCheckLists[i].add(new tokenChecker_type(TOKEN_TYPE_ID));
					nextPos = currPos+2;
				}
				else if(tokOrderString->find("SYM",currPos)==currPos)
				{
					//The token should be check for a numeric type
					theTokenCheckLists[i].add(new tokenChecker_type(TOKEN_TYPE_SYM));
					nextPos = currPos+3;
				}
				else if(tokOrderString->find("WORD",currPos)==currPos)
				{
					//The token should be check for a numeric type
					theTokenCheckLists[i].add(new tokenChecker_type(TOKEN_TYPE_WORD));
					nextPos = currPos+4;
				}
				else
					throw generalFatalException("INTERNAL Syntax Description engine error.");

				//These instructions should be separted by a colon.
				currPos = nextPos;
				if((*tokOrderString)[currPos]!=':')
					throw generalFatalException("INTERNAL Syntax Description engine error.");
				
				currPos++;
			}

			parsedTokenList = 1;
		}
	}

	currentTokenIndex = 0;
}


class commandFactory;

typedef CSHCollection<commandFactory *>::collection commandFactoryList; 


class textFileManipScriptParser : public tokenStream
{
	public:
		textFileManipScriptParser(stringProcessorEngine *spe,int ftpi):tokenStream(),theStringProcessorEngine(spe),tokenIndex(0),fileToProcessIndex(ftpi)
		{
		}

		virtual ~textFileManipScriptParser()
		{
		  int i;
			for(i=0;i<theCommandFactoryList.getNumberOfItems();i++)
				delete theCommandFactoryList.getValueAtIndex(i);

			for(i=0;i<theTokenList.getNumberOfItems();i++)
				delete theTokenList.getValueAtIndex(i);
		}

		virtual void newToken(token *t)
		{
			DEBUG_TOKEN(t);
			theTokenList.add(t);
		}

		virtual void initCommandFactoryList()
		{
			for(int i=0;i<theCommandFactoryList.getNumberOfItems();i++)
			{
				theCommandFactoryList.getValueAtIndex(i)->initTokenList();
			}
		}

		virtual void parse()
		{
			theStringProcessorEngine->initInstructionList();

			parseTokens();
			theStringProcessorEngine->endOfInstructionList();
		}

		void parseTokens();
		void dumpCommandSyntax(CSHString *com);

		virtual token *getNextToken()
		{
			if(tokenIndex==theTokenList.getNumberOfItems())
				throw noMoreTokens(1);

			token *t = theTokenList.getValueAtIndex(tokenIndex++);
			DEBUG_TOKEN(t);
			return t;
		}

	private:
		void initClassFactoryList();

		tokenList theTokenList;
		int tokenIndex;

		stringProcessorEngine *theStringProcessorEngine;
		commandFactoryList theCommandFactoryList;

		int fileToProcessIndex;
};

template<class StreamClass>
class textFileManipScriptTokeniser : public streamTokeniser<textFileManipScriptParser,StreamClass>
{
public:
  textFileManipScriptTokeniser(textFileManipScriptParser *mc):streamTokeniser<textFileManipScriptParser,StreamClass>(mc,1)
  {
  }
  
  int skipComments()
  {
    size_t currLoc = streamTokeniser<textFileManipScriptParser,StreamClass>::getScObj()->ftell();
    char tempBuff[2];
    streamTokeniser<textFileManipScriptParser,StreamClass>::charsRead = streamTokeniser<textFileManipScriptParser,StreamClass>::getScObj()->fread(tempBuff,2);
    int foundComment = 0;
    if(streamTokeniser<textFileManipScriptParser,StreamClass>::charsRead==2)
      {
	if((*tempBuff=='#') && (*(tempBuff+1)=='#'))
	  {
	    //We have a comment
	    foundComment = 1;
	    
	    char tach;
	    streamTokeniser<textFileManipScriptParser,StreamClass>::charsRead = streamTokeniser<textFileManipScriptParser,StreamClass>::getScObj()->fread(&tach,1);
	    streamTokeniser<textFileManipScriptParser,StreamClass>::ach = tach;
	    
	    while((streamTokeniser<textFileManipScriptParser,StreamClass>::charsRead>0) && (streamTokeniser<textFileManipScriptParser,StreamClass>::ach!='\r') && (streamTokeniser<textFileManipScriptParser,StreamClass>::ach!='\n') )
	      {
		streamTokeniser<textFileManipScriptParser,StreamClass>::charsRead = streamTokeniser<textFileManipScriptParser,StreamClass>::getScObj()->fread(&tach,1);
		streamTokeniser<textFileManipScriptParser,StreamClass>::ach = tach;
	      }
	  }
      }
    
    if(!foundComment)
      streamTokeniser<textFileManipScriptParser,StreamClass>::getScObj()->fseek(currLoc,SEEK_SET);
    
    return foundComment;
  }
  
  virtual ~textFileManipScriptTokeniser(){};
  
  virtual void initTokenList();
};

template<class StreamClass>
void textFileManipScriptTokeniser<StreamClass>::initTokenList()
{
	textFileManipScriptTokeniser<StreamClass>::addToken(new SYMBOLToken(","));
	textFileManipScriptTokeniser<StreamClass>::addToken(new SYMBOLToken(";"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new SYMBOLToken(":"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new SYMBOLToken(","));
	textFileManipScriptTokeniser<StreamClass>::addToken(new SYMBOLToken("+"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new SYMBOLToken("-"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new SYMBOLToken("["));
	textFileManipScriptTokeniser<StreamClass>::addToken(new SYMBOLToken("]"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("ITOSV"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("EXTRACT"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("ALL"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("PRINT"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("SV"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("NV"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("NEWLINE"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("WHOLELINE"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("DQUOTE"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("HEAD"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("TAIL"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("GLOBNUMINC"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("GLOBNUMDEC"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("TAB"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("ADJUST"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("SETGLOBALNUM"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("ENDWHENGLOBALEQUAL"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("JUMPIF"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("NEG"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("NNEG"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("NZ"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("ALL"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("SMALL"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("GNEG"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("GNNEG"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("LABEL"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("INDFIND"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("SWAP"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("INIT"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("REPLACE"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("ALL"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("BLANK"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("INDENDOFCODESTR"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("SEARCHLINEINDEX"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("INDSEARCHLINEINDEX"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("LOADLINE"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("INDFIRSTNONWS"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("INDFIRSTWS"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("CURRENTLINETOSV"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("SVTOI"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("SOURCE"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("FILE"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("ARG"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("DIRECT"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("REPEAT"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("EXECMODE"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("COMPAT"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("ASSERT"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("FIND"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("STRIP"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("BOTHENDS"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("LINEBREAKS"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("WHITESPACE"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("CALL"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("QUIT"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("FUNCTION"));
	textFileManipScriptTokeniser<StreamClass>::addToken(new simpleToken("RET"));

	textFileManipScriptTokeniser<StreamClass>::addToken(new identifierToken);

	textFileManipScriptTokeniser<StreamClass>::addToken(new numberToken);
	textFileManipScriptTokeniser<StreamClass>::addToken(new doubleQuotedStringToken);
	textFileManipScriptTokeniser<StreamClass>::addToken(new singleQuotedStringToken);
}

class notValidToken
{
	public:
		notValidToken(int)
		{
		}
};

class commandFactoryClasses
{ public:

class commandFactoryITOSV : public commandFactory
{
	public:
		commandFactoryITOSV():commandFactory("ITOSV")
		{
			addOrderString(new CSHString("':':NUM:',':NUM:';':;"));
		}

		virtual ~commandFactoryITOSV()
		{
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("ITOSV"))
			{
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar = getNum(t);
				skipNextToken(ts);
				t = getNextToken(ts);
				int stringVar = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionITOSV(numVar,stringVar);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryEXTRACT : public commandFactory
{
	public:

		commandFactoryEXTRACT():commandFactory("EXTRACT")
		{
			addOrderString(new CSHString("':':NUM:',':NUM:',':NUM:';':;"));
			addOrderString(new CSHString("'ALL':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("EXTRACT"))
			{
				t = getNextToken(ts);
				if(t->like("ALL"))
				{
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionEXTRACTALL(numVar);
				}
				else
				{
					t = getNextToken(ts);
					int from = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					int to = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					int stringVar = getNum(t);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionEXTRACT(from,to,stringVar);
				}
			}
			else
				throw notValidToken(6);

			return retVal;
		}
};

class commandFactoryPRINT : public commandFactory
{
	public:
		commandFactoryPRINT():commandFactory("PRINT")
		{
			addOrderString(new CSHString("'SV':NUM:';':;"));
			addOrderString(new CSHString("'NV':NUM:';':;"));
			addOrderString(new CSHString("'NEWLINE':';':;"));
			addOrderString(new CSHString("'WHOLELINE':';':;"));
			addOrderString(new CSHString("'DQUOTE':';':;"));
			addOrderString(new CSHString("'HEAD':NUM:';':;"));
			addOrderString(new CSHString("'TAIL':NUM:';':;"));
			addOrderString(new CSHString("'GLOBNUMINC':';':;"));
			addOrderString(new CSHString("'GLOBNUMDEC':';':;"));
			addOrderString(new CSHString("'TAB':';':;"));
			addOrderString(new CSHString("DQSTR:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("PRINT"))
			{
				t = getNextToken(ts);
				if(t->like("SV"))
				{
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::SV(numVar);
				}
				else if(t->like("NV"))
				{
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::NV(numVar);
				}
				else if(t->like("NEWLINE"))
				{
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::NEWLINE;
				}
				else if(t->like("WHOLELINE"))
				{
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::WHOLELINE;
				}
				else if(t->like("DQUOTE"))
				{
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::DQUOTE;
				}
				else if(t->like("HEAD"))
				{
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::HEAD(numVar);
				}
				else if(t->like("TAIL"))
				{
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::TAIL(numVar);
				}
				else if(t->like("GLOBNUMINC"))
				{
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::GLOBNUMINC;
				}
				else if(t->like("GLOBNUMDEC"))
				{
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::GLOBNUMDEC;
				}
				else if(t->like("TAB"))
				{
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionPRINT::TAB;
				}
				else if(t->getTokType()==TOKEN_TYPE_DQ_STRING)
				{
					skipNextToken(ts);
					CSHString str = getUnquotedString(*(t->getRep()));
					retVal = new instructionClasses::instructionPRINT::DQSTR(str.GetBuffer());
				}
				else
					throw notValidToken(6);
			}
			else
				throw notValidToken(6);

			return retVal;
		}
};

class commandFactoryADJUST : public commandFactory
{
	public:

		commandFactoryADJUST():commandFactory("ADJUST")
		{
			addOrderString(new CSHString("NUM:'+':NUM:';':;"));
			addOrderString(new CSHString("NUM:'-':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("ADJUST"))
			{
				t = getNextToken(ts);
				int numVar = getNum(t);

				token *action = getNextToken(ts);

				t = getNextToken(ts);
				int amount = getNum(t);

				skipNextToken(ts);

				if(action->like("+"))
				{
					//We now have enough info to create the command
					retVal = new instructionClasses::instructionADJUST(numVar,1,amount);
				}
				else if(action->like("-"))
				{
					//We now have enough info to create the command
					retVal = new instructionClasses::instructionADJUST(numVar,-1,amount);
				}
			}
			else
				throw notValidToken(6);

			return retVal;
		}
};

class commandFactorySETGLOBALNUM : public commandFactory
{
	public:

		commandFactorySETGLOBALNUM():commandFactory("SETGLOBALNUM")
		{
			addOrderString(new CSHString("':':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("SETGLOBALNUM"))
			{
				skipNextToken(ts);
				t = getNextToken(ts);
				int numVar = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionSETGLOBALNUM(numVar);
			}
			else
				throw notValidToken(6);

			return retVal;
		}
};

class commandFactoryENDWHENGLOBALEQUAL : public commandFactory
{
	public:

		commandFactoryENDWHENGLOBALEQUAL():commandFactory("ENDWHENGLOBALEQUAL")
		{
			addOrderString(new CSHString("NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("ENDWHENGLOBALEQUAL"))
			{
				t = getNextToken(ts);
				int numVar = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionENDWHENGLOBALEQUAL(numVar);
			}
			else
				throw notValidToken(6);

			return retVal;
		}
};

class commandFactoryJUMPIF : public commandFactory
{
	public:
		commandFactoryJUMPIF():commandFactory("JUMPIF")
		{
			addOrderString(new CSHString("'NEG':NUM:':':ID:';':;"));
			addOrderString(new CSHString("'NNEG':NUM:':':ID:';':;"));
			addOrderString(new CSHString("'NZ':NUM:':':ID:';':;"));
			addOrderString(new CSHString("'ALL':':':ID:';':;"));
			addOrderString(new CSHString("'SMALL':NUM:',':NUM:':':ID:';':;"));
			addOrderString(new CSHString("'GNEG':':':ID:';':;"));
			addOrderString(new CSHString("'GNNEG':':':ID:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("JUMPIF"))
			{
				t = getNextToken(ts);
				if(t->like("NEG"))
				{
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionJUMPIF::NEG(numVar,t->getRep()->GetBuffer());
				}
				else if(t->like("NNEG"))
				{
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionJUMPIF::NNEG(numVar,t->getRep()->GetBuffer());
				}
				else if(t->like("NZ"))
				{
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionJUMPIF::NZ(numVar,t->getRep()->GetBuffer());
				}
				else if(t->like("ALL"))
				{
					skipNextToken(ts);
					t = getNextToken(ts);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionJUMPIF::ALL(t->getRep()->GetBuffer());
				}
				else if(t->like("SMALL"))
				{
					t = getNextToken(ts);
					int firstVar = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					int secondVar = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionJUMPIF::SMALL(firstVar,secondVar,t->getRep()->GetBuffer());
				}
				else if(t->like("GNEG"))
				{
					skipNextToken(ts);
					t = getNextToken(ts);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionJUMPIF::GNEG(t->getRep()->GetBuffer());
				}
				else if(t->like("GNNEG"))
				{
					skipNextToken(ts);
					t = getNextToken(ts);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionJUMPIF::GNNEG(t->getRep()->GetBuffer());
				}
			}
			else
				throw notValidToken(6);

			return retVal;
		}
};

class commandFactoryLABEL : public commandFactory
{
	public:
		commandFactoryLABEL():commandFactory("LABEL")
		{
			addOrderString(new CSHString("':':ID:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("LABEL"))
			{
				skipNextToken(ts);

				t = getNextToken(ts);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionLABEL(t->getRep()->GetBuffer());
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryINDFIND : public commandFactory
{
	public:
		commandFactoryINDFIND():commandFactory("INDFIND")
		{
			addOrderString(new CSHString("':':DQSTR:',':NUM:',':NUM:';':;"));
			addOrderString(new CSHString("':':SQSTR:',':NUM:',':NUM:';':;"));
			addOrderString(new CSHString("'SV':':':NUM:',':NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("INDFIND"))
			{
				t = getNextToken(ts);
				if(t->like(":"))
				{
					token *qst = getNextToken(ts);
					skipNextToken(ts);

					t = getNextToken(ts);
					int indVar1 = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					int indVar2 = getNum(t);
					skipNextToken(ts);

					CSHString qs = getUnquotedString(*(qst->getRep()));
					//We now have enough info to create the command
					retVal = new instructionClasses::instructionINDFIND(qs.GetBuffer(),indVar1,indVar2);
				}
				else if(t->like("SV"))
				{
					skipNextToken(ts);

					t = getNextToken(ts);
					int strVar = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					int indVar1 = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					int indVar2 = getNum(t);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionINDFIND::SV(strVar,indVar1,indVar2);
				}
				else
					throw notValidToken(6);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactorySWAP : public commandFactory
{
	public:
		commandFactorySWAP():commandFactory("SWAP")
		{
			addOrderString(new CSHString("NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("SWAP"))
			{
				t = getNextToken(ts);
				int numVar1 = getNum(t);
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar2 = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionSWAP(numVar1,numVar2);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryINIT : public commandFactory
{
	public:
		commandFactoryINIT():commandFactory("INIT")
		{
			addOrderString(new CSHString("'NV':':':NUM:',':NUM:';':;"));
			addOrderString(new CSHString("'SV':':':NUM:',':DQSTR:';':;"));
			addOrderString(new CSHString("'SV':':':NUM:',':SQSTR:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("INIT"))
			{
				t = getNextToken(ts);

				if(t->like("NV"))
				{
					skipNextToken(ts);
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					int num = getNum(t);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionINIT::NV(numVar,num);
				}
				else if(t->like("SV"))
				{
					skipNextToken(ts);
					t = getNextToken(ts);
					int strVar = getNum(t);
					skipNextToken(ts);

					t = getNextToken(ts);
					CSHString str1 = getUnquotedString(*(t->getRep()));
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionINIT::SV(strVar,str1.GetBuffer());
				}
				else
					throw notValidToken(6);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryREPLACE : public commandFactory
{
	public:
		commandFactoryREPLACE():commandFactory("REPLACE")
		{
			addOrderString(new CSHString("'ALL':':':DQSTR:',':DQSTR:',':NUM:';':;"));
			addOrderString(new CSHString("'ALL':':':SQSTR:',':SQSTR:',':NUM:';':;"));
			addOrderString(new CSHString("'BLANK':':':DQSTR:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("REPLACE"))
			{
				t = getNextToken(ts);

				if(t->like("ALL"))
				{
					skipNextToken(ts);
					token *str1t = getNextToken(ts);
					skipNextToken(ts);
					token *str2t = getNextToken(ts);
					skipNextToken(ts);
					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);


					CSHString str1 = getUnquotedString(*(str1t->getRep()));
					CSHString str2 = getUnquotedString(*(str2t->getRep()));
					//We now have enough info to create the command
					retVal = new instructionClasses::instructionREPLACE::ALL(str1.GetBuffer(),str2.GetBuffer(),numVar);
				}
				else if(t->like("BLANK"))
				{
					skipNextToken(ts);
					token *str1t = getNextToken(ts);
					skipNextToken(ts);

					t = getNextToken(ts);
					int numVar = getNum(t);
					skipNextToken(ts);

					CSHString str1 = getUnquotedString(*(str1t->getRep()));

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionREPLACE::ALL(str1.GetBuffer(),"",numVar);
				}
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryINDENDOFCODESTR : public commandFactory
{
	public:
		commandFactoryINDENDOFCODESTR():commandFactory("INDENDOFCODESTR")
		{
			addOrderString(new CSHString("NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("INDENDOFCODESTR"))
			{
				t = getNextToken(ts);
				int numVar1 = getNum(t);
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar2 = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionINDENDOFCODESTR(numVar1,numVar2);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactorySEARCHLINEINDEX : public commandFactory
{
	public:
		commandFactorySEARCHLINEINDEX():commandFactory("SEARCHLINEINDEX")
		{
			addOrderString(new CSHString("':':DQSTR:',':NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("SEARCHLINEINDEX"))
			{
				skipNextToken(ts);

				token *fName = getNextToken(ts);
				skipNextToken(ts);

				t = getNextToken(ts);
				int sVar = getNum(t);
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar1 = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionSEARCHLINEINDEX(fName->getRep()->GetBuffer(),sVar,numVar1);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryINDSEARCHLINEINDEX : public commandFactory
{
	public:
		commandFactoryINDSEARCHLINEINDEX():commandFactory("INDSEARCHLINEINDEX")
		{
			addOrderString(new CSHString("':':DQSTR:',':NUM:',':NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("INDSEARCHLINEINDEX"))
			{
				skipNextToken(ts);

				token *fName = getNextToken(ts);
				skipNextToken(ts);

				t = getNextToken(ts);
				int sVar = getNum(t);
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar1 = getNum(t);
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar2 = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionINDSEARCHLINEINDEX(fName->getRep()->GetBuffer(),sVar,numVar1,numVar2);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryLOADLINE : public commandFactory
{
	public:
		commandFactoryLOADLINE():commandFactory("LOADLINE")
		{
			addOrderString(new CSHString("':':DQSTR:',':',':NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("LOADLINE"))
			{
				skipNextToken(ts);

				token *fName = getNextToken(ts);
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar1 = getNum(t);
				skipNextToken(ts);

				t = getNextToken(ts);
				int sVar1 = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionLOADLINE(fName->getRep()->GetBuffer(),numVar1,sVar1);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryINDFIRSTNONWS : public commandFactory
{
	public:
		commandFactoryINDFIRSTNONWS():commandFactory("INDFIRSTNONWS")
		{
			addOrderString(new CSHString("':':NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("INDFIRSTNONWS"))
			{
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar1 = getNum(t);
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar2 = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionINDFIRSTNONWS(numVar1,numVar2);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryINDFIRSTWS : public commandFactory
{
	public:
		commandFactoryINDFIRSTWS():commandFactory("INDFIRSTWS")
		{
			addOrderString(new CSHString("':':NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("INDFIRSTWS"))
			{
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar1 = getNum(t);
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar2 = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionINDFIRSTWS(numVar1,numVar2);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryCURRENTLINETOSV : public commandFactory
{
	public:
		commandFactoryCURRENTLINETOSV():commandFactory("CURRENTLINETOSV")
		{
			addOrderString(new CSHString("':':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("CURRENTLINETOSV"))
			{
				skipNextToken(ts);

				t = getNextToken(ts);
				int numVar1 = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionCURRENTLINETOSV(numVar1);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactorySVTOI : public commandFactory
{
	public:
		commandFactorySVTOI():commandFactory("SVTOI")
		{
			addOrderString(new CSHString("':':NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("SVTOI"))
			{
				skipNextToken(ts);

				t = getNextToken(ts);
				int strVar1 = getNum(t);
				skipNextToken(ts);
	
				t = getNextToken(ts);
				int numVar1 = getNum(t);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionSVTOI(strVar1,numVar1);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactorySOURCE : public commandFactory
{
	public:
		commandFactorySOURCE():commandFactory("SOURCE")
		{
			addOrderString(new CSHString("'FILE':':':'ARG':'[':NUM:']':';':;"));
			addOrderString(new CSHString("'FILE':':':'DIRECT':DQSTR:';':;"));
			//:ARG[1];
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("SOURCE"))
			{
				skipNextToken(ts);
				skipNextToken(ts);

				token *t = getNextToken(ts);
				if(t->like("ARG"))
				{
					skipNextToken(ts);

					t = getNextToken(ts);
					int argNum = getNum(t);

					skipNextToken(ts);
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionSOURCE::ARG(argNum);
				}
				else if(t->like("DIRECT"))
				{
					t = getNextToken(ts);
					CSHString filename = getUnquotedString(*(t->getRep()));
					
					skipNextToken(ts);

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionSOURCE::DIRECT(filename.GetBuffer());
				}
				else
					throw notValidToken(6);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryREPEAT : public commandFactory
{
	public:
		commandFactoryREPEAT():commandFactory("REPEAT")
		{
			addOrderString(new CSHString("'SOURCE':';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("REPEAT"))
			{
				skipNextToken(ts);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionREPEAT;
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryEXECMODE : public commandFactory
{
	public:
		commandFactoryEXECMODE(int ftpi):commandFactory("EXECMODE"),fileToProcessIndex(ftpi)
		{
			addOrderString(new CSHString("'COMPAT':';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("EXECMODE"))
			{
				skipNextToken(ts);
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionEXECMODE(fileToProcessIndex);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
		int fileToProcessIndex;
};

class commandFactoryASSERT : public commandFactory
{
	public:
		commandFactoryASSERT():commandFactory("ASSERT")
		{
			addOrderString(new CSHString("'SV':NUM:':':DQSTR:';':;"));
			addOrderString(new CSHString("'NV':NUM:':':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("ASSERT"))
			{
				t = getNextToken(ts);
				if(t->like("SV"))
				{
					token *numt = getNextToken(ts);
					skipNextToken(ts);
					token *strt = getNextToken(ts);
					skipNextToken(ts);

					int varNum = getNum(numt);
					CSHString str = getUnquotedString(*(strt->getRep()));

					//We now have enough info to create the command
					retVal = new instructionClasses::instructionASSERT::SV(varNum,str.GetBuffer());
				}
				else if(t->like("NV"))
				{
					token *numt = getNextToken(ts);
					skipNextToken(ts);
					token *cmpt = getNextToken(ts);
					skipNextToken(ts);

					int varNum = getNum(numt);
					int ncmp = getNum(cmpt);
					//We now have enough info to create the command
					retVal = new instructionClasses::instructionASSERT::NV(varNum,ncmp);
				}
				else
					throw notValidToken(6);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryGLOBNUMDEC : public commandFactory
{
	public:
		commandFactoryGLOBNUMDEC():commandFactory("GLOBNUMDEC")
		{
			addOrderString(new CSHString("';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("GLOBNUMDEC"))
			{
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionGLOBNUMDEC();
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryGLOBNUMINC : public commandFactory
{
	public:
		commandFactoryGLOBNUMINC():commandFactory("GLOBNUMINC")
		{
			addOrderString(new CSHString("';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("GLOBNUMINC"))
			{
				skipNextToken(ts);

				//We now have enough info to create the command
				retVal = new instructionClasses::instructionGLOBNUMINC();
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryFIND : public commandFactory
{
	public:
		commandFactoryFIND():commandFactory("FIND")
		{
			//FIND 1,"XXX",1,0;
			addOrderString(new CSHString("NUM:',':DQSTR:',':NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("FIND"))
			{
				//1
				token *numToSkipt = getNextToken(ts);
				int numToSkip = getNum(numToSkipt);

				//,
				skipNextToken(ts);
			
				//"XXX"
				t = getNextToken(ts);
				CSHString str = getUnquotedString(*(t->getRep()));

				//,
				skipNextToken(ts);

				token *numt = getNextToken(ts);
				int fromInd = getNum(numt);

				//,
				skipNextToken(ts);
				token *dest = getNextToken(ts);
				int storeIn = getNum(dest);

				//;
				skipNextToken(ts);


				//We now have enough info to create the command
				retVal = new instructionClasses::instructionFIND(numToSkip,str.GetBuffer(),fromInd,storeIn);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactorySTRMANIP : public commandFactory
{
	public:
		commandFactorySTRMANIP():commandFactory("STRMANIP")
		{
			//STRMANIP BEFORE UPPER ADD "_",0;
			//STRMANIP BEFORE UPPER ADD '_',1;
			//STRMANIP TOUPPER,3;

			addOrderString(new CSHString("'BEFORE':'UPPER':'ADD':DQSTR:',':NUM:';':;"));
			addOrderString(new CSHString("'BEFORE':'UPPER':'ADD':SQSTR:',':NUM:';':;"));
			addOrderString(new CSHString("'TOUPPER':',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("STRMANIP"))
			{
				t = getNextToken(ts);

				if(t->like("BEFORE"))
				{
					skipNextToken(ts);
					skipNextToken(ts);

					token *qstrt = getNextToken(ts);
					skipNextToken(ts);

					token *numt = getNextToken(ts);

					CSHString qstr = getUnquotedString(*(qstrt->getRep()));
					int num = getNum(numt);

					retVal = new instructionClasses::instructionSTRMANIP::BEFORE_UPPER(num,qstr.GetBuffer());
				}
				else if(t->like("TOUPPER"))
				{
					skipNextToken(ts);
					token *numt = getNextToken(ts);
					int num = getNum(numt);
					retVal = new instructionClasses::instructionSTRMANIP::TOUPPER(num);
				}

				skipNextToken(ts);

				/*
				//1
				token *numToSkipt = getNextToken(ts);
				int numToSkip = getNum(numToSkipt);

				//,
				skipNextToken(ts);
			
				//"XXX"
				t = getNextToken(ts);
				CSHString str = getUnquotedString(*(t->getRep()));

				//,
				skipNextToken(ts);

				token *numt = getNextToken(ts);
				int fromInd = getNum(numt);

				//,
				skipNextToken(ts);
				token *dest = getNextToken(ts);
				int storeIn = getNum(dest);

				//;
				skipNextToken(ts);


				//We now have enough info to create the command
				retVal = new instructionClasses::instructionFIND(numToSkip,str.GetBuffer(),fromInd,storeIn);
				*/
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};


class commandFactorySTRIP : public commandFactory
{
	public:
		commandFactorySTRIP():commandFactory("STRIP")
		{
			//e.g. 
			//STRIP SV 1,BOTHENDS WHITESPACE;
			//STRIP SV 1,HEAD WHITESPACE;
			//STRIP SV 1,TAIL WHITESPACE;
			//STRIP SV 1,LINEBREAKS;

			addOrderString(new CSHString("'SV':NUM:',':'BOTHENDS':'WHITESPACE':';':;"));
			addOrderString(new CSHString("'SV':NUM:',':'HEAD':'WHITESPACE':';':;"));
			addOrderString(new CSHString("'SV':NUM:',':'TAIL':'WHITESPACE':';':;"));
			addOrderString(new CSHString("'SV':NUM:',':'LINEBREAKS':';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("STRIP"))
			{
				skipNextToken(ts);
				t = getNextToken(ts);
				int strVarNum = getNum(t);

				skipNextToken(ts);

				t = getNextToken(ts);
				if(t->like("BOTHENDS"))
				{
					skipNextToken(ts);
					retVal = new instructionClasses::instructionSTRIP::BOTHENDSWHITESPACE(strVarNum);
				}
				else if(t->like("HEAD"))
				{
					skipNextToken(ts);
					retVal = new instructionClasses::instructionSTRIP::HEADWHITESPACE(strVarNum);
				}
				else if(t->like("TAIL"))
				{
					skipNextToken(ts);
					retVal = new instructionClasses::instructionSTRIP::TAILWHITESPACE(strVarNum);
				}
				else if(t->like("LINEBREAKS"))
				{
					retVal = new instructionClasses::instructionSTRIP::LINEBREAKS(strVarNum);
				}
				else
					throw notValidToken(6);

				skipNextToken(ts);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

/////////////////////////////////////////////////////
class commandFactoryDEBUG : public commandFactory
{
	public:
		commandFactoryDEBUG():commandFactory("DEBUG")
		{
			//e.g. 
			//DEBUG DUMP;
			addOrderString(new CSHString("'DUMPALL':';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("DEBUG"))
			{
				t = getNextToken(ts);

				if(t->like("DUMPALL"))
				{
					retVal = new instructionClasses::instructionDEBUG::DUMPALL;
				}
				else
					throw notValidToken(6);

				skipNextToken(ts);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactorySTRCAT : public commandFactory
{
	public:
		commandFactorySTRCAT():commandFactory("STRCAT")
		{
			//e.g. 
			//STRCAT 0,1;

			addOrderString(new CSHString("NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("STRCAT"))
			{
				t = getNextToken(ts);
				int strDestVarNum = getNum(t);

				skipNextToken(ts);

				t = getNextToken(ts);
				int strSrcVarNum = getNum(t);

				retVal = new instructionClasses::instructionSTRCAT(strDestVarNum,strSrcVarNum);


				skipNextToken(ts);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactorySTREXEC : public commandFactory
{
	public:
		commandFactorySTREXEC():commandFactory("STREXEC")
		{
			//e.g. 
			//STREXEC 0;

			addOrderString(new CSHString("NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("STREXEC"))
			{
				t = getNextToken(ts);
				int execStrVarNum = getNum(t);

				retVal = new instructionClasses::instructionSTREXEC(execStrVarNum);

				skipNextToken(ts);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryCALL : public commandFactory
{
	public:
		commandFactoryCALL():commandFactory("CALL")
		{
			//e.g. 
			//STREXEC 0;

			addOrderString(new CSHString("ID:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("CALL"))
			{
				t = getNextToken(ts);

				retVal = new instructionClasses::instructionCALL(t->getRep()->GetBuffer());

				skipNextToken(ts);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryQUIT : public commandFactory
{
	public:
		commandFactoryQUIT():commandFactory("QUIT")
		{
			//e.g. 
			//STREXEC 0;

			addOrderString(new CSHString("';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("QUIT"))
			{
				t = getNextToken(ts);

				retVal = new instructionClasses::instructionQUIT;
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryFUNCTION : public commandFactory
{
	public:
		commandFactoryFUNCTION():commandFactory("FUNCTION")
		{
			//e.g. 
			//STREXEC 0;

			addOrderString(new CSHString("ID:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("FUNCTION"))
			{
				t = getNextToken(ts);

				retVal = new instructionClasses::instructionFUNCTION(t->getRep()->GetBuffer());
				skipNextToken(ts);
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryRET : public commandFactory
{
	public:
		commandFactoryRET():commandFactory("RET")
		{
			//e.g. 
			//STREXEC 0;

			addOrderString(new CSHString("';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("RET"))
			{
				t = getNextToken(ts);

				retVal = new instructionClasses::instructionRET;
			}
			else
				throw notValidToken(6);

			return retVal;
		}

	private:
};

class commandFactoryGETFCTX : public commandFactory
{
	public:
		commandFactoryGETFCTX():commandFactory("GETFCTX")
		{
			addOrderString(new CSHString("'SV':NUM:',':NUM:';':;"));
			addOrderString(new CSHString("'NV':NUM:',':NUM:';':;"));
		}

		virtual instruction *make(token *t,tokenStream *ts)
		{
			instruction *retVal;
			if(t->like("GETFCTX"))
			{
				t = getNextToken(ts);

				if(t->like("SV"))
				{
					t = getNextToken(ts);
					int destVar = getNum(t);
					skipNextToken(ts);
					t = getNextToken(ts);
					int srcVar = getNum(t);

					retVal = new instructionClasses::instructionGETFCTX::SV(destVar,srcVar);
				}
				else if(t->like("NV"))
				{
					t = getNextToken(ts);
					int destVar = getNum(t);
					skipNextToken(ts);
					t = getNextToken(ts);
					int srcVar = getNum(t);
					retVal = new instructionClasses::instructionGETFCTX::NV(destVar,srcVar);
				}
				else
					throw notValidToken(6);
			}
			else
				throw notValidToken(6);

			skipNextToken(ts);
			return retVal;
		}

	private:
};

/////////////////////////////////////////////////////


}; //End of classFactoryClasses

/*
	addToken(new simpleToken("ITOSV"));
	addToken(new simpleToken("EXTRACT"));
	addToken(new simpleToken("PRINT"));
*/


void textFileManipScriptParser::dumpCommandSyntax(CSHString *com)
{
	initClassFactoryList();

	for(int i=0;i<theCommandFactoryList.getNumberOfItems();i++)
	{
		commandFactory *cFactory = theCommandFactoryList.getValueAtIndex(i);
		int dumpCommand = 0;
		if(com==NULL)
			dumpCommand = 1;
		else if(cFactory->getCommandName()->equal(com->GetBuffer()))
			dumpCommand = 1;

		if(dumpCommand)
		{
			cFactory->initTokenList();
			if(com==NULL)
				printf("%s\n",cFactory->getCommandName()->GetBuffer());
			else
				cFactory->dumpCommandSyntax();
		}
	}
}

void textFileManipScriptParser::initClassFactoryList()
{
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryITOSV);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryEXTRACT);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryPRINT);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryADJUST);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySETGLOBALNUM);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryENDWHENGLOBALEQUAL);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryJUMPIF);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryLABEL);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDFIND);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySWAP);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINIT);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryREPLACE);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDENDOFCODESTR);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySEARCHLINEINDEX);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDSEARCHLINEINDEX);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryLOADLINE);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDFIRSTNONWS);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDFIRSTWS);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryCURRENTLINETOSV);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySVTOI);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySOURCE);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryREPEAT);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryEXECMODE(fileToProcessIndex));
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryASSERT);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryGLOBNUMDEC);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryGLOBNUMINC);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryFIND);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySTRMANIP);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySTRIP);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryDEBUG);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySTRCAT);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySTREXEC);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryCALL);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryQUIT);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryFUNCTION);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryRET);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryGETFCTX);


	/*
	//theCommandFactoryList.add(new commandFactoryClasses::commandFactoryITOSV);
	//theCommandFactoryList.add(new commandFactoryClasses::commandFactoryEXTRACT);
	--theCommandFactoryList.add(new commandFactoryClasses::commandFactoryPRINT);
	//theCommandFactoryList.add(new commandFactoryClasses::commandFactoryADJUST);
	--theCommandFactoryList.add(new commandFactoryClasses::commandFactorySETGLOBALNUM);
	--theCommandFactoryList.add(new commandFactoryClasses::commandFactoryENDWHENGLOBALEQUAL);
	--theCommandFactoryList.add(new commandFactoryClasses::commandFactoryJUMPIF);
	--theCommandFactoryList.add(new commandFactoryClasses::commandFactoryLABEL);
	//theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDFIND);
	//theCommandFactoryList.add(new commandFactoryClasses::commandFactorySWAP);
	//theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINIT);
	//theCommandFactoryList.add(new commandFactoryClasses::commandFactoryREPLACE);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDENDOFCODESTR);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySEARCHLINEINDEX);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDSEARCHLINEINDEX);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryLOADLINE);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDFIRSTNONWS);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryINDFIRSTWS);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryCURRENTLINETOSV);
	//theCommandFactoryList.add(new commandFactoryClasses::commandFactorySVTOI);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactorySOURCE);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryREPEAT);
	theCommandFactoryList.add(new commandFactoryClasses::commandFactoryEXECMODE);
	--theCommandFactoryList.add(new commandFactoryClasses::commandFactoryASSERT);
	*/


}

void textFileManipScriptParser::parseTokens()
{
	initClassFactoryList();
	//SOURCE FILE:ARG[1];
	//REPEAT SOURCE;

	try
	{
		while(1)
		{
			token *t = getNextToken();
			initCommandFactoryList();

			int numberOfInstuctionsAdded = 0;
			for(int i=0;i<theCommandFactoryList.getNumberOfItems();i++)
			{
				commandFactory *cli = theCommandFactoryList.getValueAtIndex(i);

				try
				{
					instruction *newInst = cli->make(t,this);
					theStringProcessorEngine->addInstruction(newInst);
					numberOfInstuctionsAdded++;
					break;
				}
				catch(notValidToken &)
				{
				}
			}
			if(numberOfInstuctionsAdded==0)
			{
				CSHString str("Invalid Command:");
				str.Cat(t->getRep()->GetBuffer());
				throw generalFatalException(str.GetBuffer());
			}
		}
	}
	catch(noMoreTokens &)
	{
	}
}

//SOURCE FILE:ARG[1];
//REPEAT SOURCE;

//#define USE_SIMPLE_STRING

void outputUsage()
{
	printf("Usage XX [-dTOK/-s/-sACOMMAND] .skr textfile\n");
	exit(0);
}

void checkFileExists(char *fn)
{
	FILE *f = fopen(fn,"rb");
	if(f==NULL)
	{
		printf("ERROR: Cannot open file:%s\n",fn);
		exit(1);
	}
	else
		fclose(f);
}

int main(int argc,char *argv[])
{
	#ifndef USE_SIMPLE_STRING
	if(argc<=1)
	{
		outputUsage();
	}
	#endif

	stringProcessorEngine *aStringProcessorEngine = NULL;
	try
	{
		#ifdef USE_SIMPLE_STRING
		char s[] = ";\r\n  \r \n \r\n";
		#endif

		//char *s = argv[1];
		aStringProcessorEngine = new stringProcessorEngine(argc,argv);

		#ifdef USE_SIMPLE_STRING
		simpleStringStream *msc = new simpleStringStream(s,strlen(s));
		#else
		fileStream *msc;
		int fileToProcessIndex = 2;
		int dumpCommandSyntax = 0;
		CSHString *commandToDump = NULL;
		if(argc==2)
		{
			if(strncmp(argv[1],"-s",2)==0)
			{
				dumpCommandSyntax = 1;

				if(strlen(argv[1])>3)
				{
					commandToDump = new CSHString(argv[1]+2);
				}
			}
			else
				outputUsage();
		}
        else if(argc==4)
		{
			if(strncmp(argv[1],"-d",2)==0)
			{
				if(strncmp(argv[1]+2,"TOK",3)==0)
					doDebugToken = 1;
			}
			else if(strncmp(argv[1],"-s",2)==0)
			{
				if(strlen(argv[1])>3)
				{
					commandToDump = new CSHString(argv[1]+2);
				}
				dumpCommandSyntax = 1;
			}
			else if(strncmp(argv[1],"-h",2)==0)
			{
				outputUsage();
			}
			msc = new fileStream(argv[2]);
			fileToProcessIndex = 3;
		}
		else if((argc==1) || (argc==2))
		{
			if(strncmp(argv[1],"-h",2)==0)
			{
				outputUsage();
			}
		}
		else
		{ 
			checkFileExists(argv[1]);
			msc = new fileStream(argv[1]);
		}
		#endif

		textFileManipScriptParser *parserObj = new textFileManipScriptParser(aStringProcessorEngine,fileToProcessIndex);

		if(dumpCommandSyntax)
		{
			parserObj->dumpCommandSyntax(commandToDump);

			delete commandToDump;
			exit(0);
		}

		checkFileExists(argv[fileToProcessIndex]);

		#ifdef USE_SIMPLE_STRING
		tempTokeniser<simpleStringStream> *sft = new tempTokeniser<simpleStringStream>(arithParserObj);
		#else
		textFileManipScriptTokeniser<fileStream> *sft = new textFileManipScriptTokeniser<fileStream>(parserObj);
		#endif
		sft->tokenise(msc);
		delete msc;

		try
		{
			parserObj->parse();
			aStringProcessorEngine->run();
		}
		catch(couldNotFindLabel &e)
		{
			printf("ERROR:could not find label:'%s'\n",e.labl.GetBuffer());
		}
		delete sft;
		delete parserObj;
	}
	catch(invalidInstructionPointer &)
	{
		printf("invalid intruction pointer encountered.\n");
	}
	catch(textFileManipSyntaxError &)
	{
		printf("ERROR:syntax error.\n");
	}
	catch(CSHStreamExceptions::couldNotOpenStream &e)
	{
		printf("ERROR:Could not open stream:%s\n",e.errorString.GetBuffer());
	}
	catch(generalFatalException &e)
	{
		printf("ERROR:%s\n",e.hint.GetBuffer());
	}
	catch(languageRuntimeException &)
	{
		printf("ERROR:Assert failed.\n");
	}

	delete aStringProcessorEngine;

	//debugMem::debug();

	return 0;
}

