/* C declarations here */
%{
    char *yytext;
    void yyerror(char *s);
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include "shell.h"
%}


%union {
    int number;
    char *word;
    wordlist_t *wordlist;
    command_t *command;
    redirect_t *redirect;
    element_t element;
}

/* When these tokens are defined with the %token command, they are assigned numerical id's, starting from 256. */
%token  AND_IF    OR_IF    DSEMI
/*      '&&'      '||'     ';;'  */
%token DLESS DGREAT LESSAND GREATAND LESSGREAT DLESSDASH
/*     '<<'  '>>'   '<&'    '>&'     '<>'      '<<-'      */
%token <word> WORD
%token <number> NUMBER
%token <command> COND_CMD
%token NEWLINE
%token IO_NUMBER

%type <command> command simple_command
%type <element> simple_command_element

%start command
%%
/* productions and actions here */
/* should use left recursion */
/* The default action is {$$=$1;} */
/*  */                          /* command */
command                         : simple_command                        { exec_cmd($1); }
                                ;
/* command */                   /* element */
simple_command                  : simple_command_element                { $$ = gen_simple_cmd($1, (command_t *)0); }
                                | simple_command simple_command_element { $$ = gen_simple_cmd($2, $1); }
                                ;
/* element */                   /* word */
simple_command_element          : WORD                                  { $$.word = $1; $$.redirect = 0; }
                                ;
%%
/* C code here */
int main(void) {
	/* initial jobs here */
	return yyparse();
}

void yyerror(char *s) {
	fprintf(stderr, "%s\n", s);
}

int is_built_in(command_t *command) {
    return 0;
}

command_t *gen_simple_cmd(element_t element, command_t *command) {
    command_t *rt;
    simple_cmd_t *tmp;
    // just bare command
    if (command == 0) {
        rt = (command_t *) malloc(sizeof(command_t));
        rt->simple_cmd = tmp = (simple_cmd_t *) malloc(sizeof(simple_cmd_t *));
        rt->redirects = (redirect_t *)NULL;
        return rt;
    }
    // 
    if (element.word) {

    }
}

void exec_cmd(command_t *command) {
    int child_pid;
    char **args;
    args = (char **) malloc(sizeof(char*) * 2);
    *args = (char *) malloc(sizeof(char) * 256);
    *args = "";
    if (is_built_in(command)) {
    } else {
        if ((child_pid = fork()) < 0) {
            fprintf(stderr, "fork error!\n");
        } else if (child_pid == 0) { // child
            //execlp(command->simple_cmd->words[0], command->simple_cmd->words, (char *)0);
            *args = "/bin/ls";
            execlp("ls", *args, (char *)0);
        } else {
            waitpid(child_pid, NULL, 0);
        }
    }
}
