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

    typedef struct command_st {
        simple_cmd_t *simple_cmd;
        redirect_t *redirects;
    } command_t;

    typedef struct element_st {
        char *word;
        redirect_t *redirect;
    } element_t;

    void exec_cmd(command_t *command);
    command_t *gen_simple_cmd(element_t element, command_t *command);
#endif