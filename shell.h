#ifndef SHELL_H
#define SHELL_H

    typedef struct redirectee_st {
        int dest;
        char *filename;
    } redirectee_t;

    typedef struct redirect_st {
        redirectee_t redirector;    // ... to be redirected
        redirectee_t redirectee;    // redirect to ...
    } redirect_t;

    /* e.g. ls -a -l */
    typedef struct wordlist_st {
      struct wordlist_st *next;
      char *word;
    } wordlist_t;

    /* e.g. ls -al > foo */
    typedef struct simple_cmd_st {
        wordlist_t *words;
        redirect_t *redirects;
    } simple_cmd_t;

    typedef struct element_st {
        char *word;
        redirect_t *redirect;
    } element_t;

    extern simple_cmd_t *parsed_command;

    void exec_cmd(simple_cmd_t *command);
    void print_prompt();
    simple_cmd_t *gen_simple_cmd(element_t element, simple_cmd_t *command);
    int wordlist_length(wordlist_t *words);
	char **gen_args(wordlist_t *words);
    int is_built_in(simple_cmd_t *command);
#endif