/* C declarations here */
%{
    void yyerror(char *s);
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include "shell.h"
    int word_count = 0;
%}


%union {
    int number;
    char *word;
    wordlist_t *wordlist;
    simple_cmd_t *command;
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
%token yacc_EOF

%type <command> simple_command
%type <element> simple_command_element
%type <redirect> redirection redirection_list

%start init
%%
/* productions and actions here */
/* should use left recursion */
/* The default action is {$$=$1;} */

init                            : simple_command                        {
                                                                            parsed_command = $1;
                                                                            parsed_command->redirects = (redirect_t *)NULL;
                                                                        }
                                | simple_command redirection_list       {
                                                                            parsed_command = $1;
                                                                            parsed_command->redirects = $2;
                                                                        }
                                ;
/* command */                   /* element */
simple_command                  : simple_command_element                { $$ = gen_simple_cmd($1, (simple_cmd_t *)0); }
                                | simple_command simple_command_element { $$ = gen_simple_cmd($2, $1); }
                                ;
/* element */                   /* word */
simple_command_element          : WORD                  {
                                                            $$.word = $1;
                                                            $$.redirect = 0;
                                                        }
                                ;
/* redirect */
redirection                     : '>' WORD                              { $$ = gen_redirect('>', $2); }
                                | '<' WORD                              { $$ = gen_redirect('<', $2); }
                                | DLESS WORD                            { $$ = gen_redirect(RE_DLESS, $2); }
                                ;
redirection_list                : redirection                           { $$ = $1; }
                                | redirection_list redirection          {
                                                                            // append the redirect to the end
                                                                            redirect_t *tmp;
                                                                            tmp = $1;
                                                                            while (tmp->next) tmp = tmp->next;
                                                                            tmp->next = $2;
                                                                            $$ = $1;
                                                                        }
                                ;
%%

void yyerror(char *s) {
	fprintf(stderr, "%s\n", s);
}

simple_cmd_t *gen_simple_cmd(element_t element, simple_cmd_t *command) {
    simple_cmd_t *rt;
    wordlist_t *tmp;
    // generate a bare command and append info later
    if (command == 0) {
        rt = (simple_cmd_t *) malloc(sizeof(simple_cmd_t));
        rt->words = (wordlist_t *) malloc(sizeof(wordlist_t));
        rt->words->word = element.word;
        rt->words->next = (wordlist_t *)NULL;
        rt->redirects = (redirect_t *)NULL;
        return rt;
    }
    // remember the arguments are added backwards
    if (element.word) {
        tmp = (wordlist_t *) malloc(sizeof(wordlist_t));
        tmp->word = element.word;
        tmp->next = command->words;
        command->words = tmp;
    } else if (element.redirect) { // redirection

    }
    return command;
}

redirect_t *gen_redirect(int token_num, char *filename) {
    redirect_t *rt;
    rt = (redirect_t *) malloc(sizeof(redirect_t));
    rt->redirectee.filename = filename;
    rt->token_num = token_num;
}
