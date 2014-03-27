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
    pipeline_t *pipeline;
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

%type <command> command simple_command
%type <element> simple_command_element
%type <redirect> redirection redirection_list
%type <pipeline> pipeline

%start init
%%
/* productions and actions here */
/* should use left recursion */
/* The default action is {$$=$1;} */

init                            : pipeline                                  {
                                                                                parsed_pipeline = $1;
                                                                                background = 0;
                                                                            }
                                | pipeline '&'                              {
                                                                                parsed_pipeline = $1;
                                                                                background = 1;
                                                                            }
                                | /* empty */                               {
                                                                                parsed_pipeline = NULL;
                                                                            }
                                ;
pipeline                        : command                                   { $$ = gen_pipe($1, (pipeline_t *)NULL); }
                                | pipeline '|' command                      { $$ = gen_pipe($3, $1); }
                                ;
/* simple_command + redirects */
command                         : simple_command                            {
                                                                                $$ = $1;
                                                                                $$->redirects = (redirect_t *)NULL;
                                                                            }
                                | simple_command redirection_list           {
                                                                                $$ = $1;
                                                                                $$->redirects = $2;
                                                                            }
                                ;
/* a word list */
simple_command                  : simple_command_element                    {
                                                                                $$ = gen_simple_cmd($1, (simple_cmd_t *)0);
                                                                            }
                                | simple_command simple_command_element     {
                                                                                $$ = gen_simple_cmd($2, $1);
                                                                            }
                                ;
/* element */                   /* word */
simple_command_element          : WORD                                      {
                                                                                $$.word = $1;
                                                                                $$.redirect = 0;
                                                                            }
                                ;
/* redirect */
redirection                     : '>' WORD                                  { $$ = gen_redirect('>', $2); }
                                | '<' WORD                                  { $$ = gen_redirect('<', $2); }
                                | DLESS WORD                                { $$ = gen_redirect(RE_DLESS, $2); }
                                ;
redirection_list                : redirection                               { $$ = $1; }
                                | redirection_list redirection              {
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

pipeline_t *gen_pipe(simple_cmd_t *command, pipeline_t *pipeline) {
    pipeline_t *rt;
    rt = (pipeline_t *) malloc(sizeof(pipeline_t *));
    // a simple command
    if (pipeline == NULL) {
        rt->cmd = command;
        rt->next = NULL;
        return rt;
    } else {
        rt->cmd = command;
        rt->next = pipeline;
    }
    return rt;
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
    rt->next = (redirect_t *)NULL;
}
