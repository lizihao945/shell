#ifndef SHELL_H
#define SHELL_H

#define FILE_LENGTH     256
#define RE_DLESS        1000
#define RE_DGREAT       1001

#define CMD_FG          2000
#define CMD_BG          2001
#define CMD_EXIT        2002

typedef struct redirectee_st {
    int fd;
    char *filename;
} redirectee_t;

/* e.g. a.out < "text.in" */
/* here stdin is the redirector and "text.in" is the redirectee */
typedef struct redirect_st {
    /* a programm can have multiple redirections */
    struct redirect_st *next;
    int redirector;             // fd number to be redirected from
    // stands for the instruction('>', DLESS, etc.)
    int token_num;
    redirectee_t redirectee;    // redirect to ...(fd number or file name)
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

typedef struct job_st {
    struct job_st *next;
    int pid, num;
    char *cmd;
} job_t;

typedef struct pipeline_st {
    struct pipeline_st *next;
    simple_cmd_t *cmd;
} pipeline_t;

extern pipeline_t *parsed_pipeline;
extern int background;

void print_prompt();
int wordlist_length(wordlist_t *words);
char **gen_args(wordlist_t *words);
int is_built_in(simple_cmd_t *command);
pipeline_t *gen_pipe(simple_cmd_t *command, pipeline_t *pipeline);
simple_cmd_t *gen_simple_cmd(element_t element, simple_cmd_t *command);
redirect_t *gen_redirect(int token_num, char *filename);

#endif