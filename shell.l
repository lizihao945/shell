%{
	#include <stdlib.h>
	#include <string.h>
	#include "shell.h"
	#include "y.tab.h"
	void yyerror(char *);
%}

%%
"<<"											{ return DLESS; }
">>"											{ return DGREAT; }
"<&"											{ return LESSAND; }
">&"											{ return GREATAND; }
"<>"											{ return LESSGREAT; }
"<<-"											{ return DLESSDASH; }
[0-9]*											{ return IO_NUMBER; }
[a-zA-Z]+_?										{ 
													yylval.word = (char *) malloc(sizeof(char) * 256);
													strcpy(yylval.word, yytext);
													return WORD;
												}
[ \t\r\n]+										{ ; }
.												{ 
													printf("Unsupported character: ");
													ECHO; printf("\n");
												}

%%

/*
 * called at the end of the input,
 * can be used to analyze multiple files(input),
 * return 1 to finish analyzing
 */
int yywrap(void) {
	return 1;
}