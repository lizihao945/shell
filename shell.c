#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"

simple_cmd_t *parsed_command;

void describe_command(simple_cmd_t *command) {
    wordlist_t *p;
    p = command->words;
    printf("command: %s\n", command->words->word);
    printf("arguments: ");
    while (p = p->next)
        printf("%s ", p->word);
    printf("\n");
}

int wordlist_length(wordlist_t *words) {
    int count = 0;
    while (words = words->next) count++;
    return count;
}

char **gen_args(wordlist_t *words) {
    char **rt;
    int i, count = wordlist_length(words);
    rt = (char **) malloc(sizeof(char *) * (count + 1));    // one for NUL end
    i = 0;
    rt[i++] = words->word;
    while (words = words->next) {
        rt[i] = words->word;
        i++;
    }
    rt[i] = (char *)NULL;
    return rt;
}

wordlist_t *reverse_command(wordlist_t *words) {
    wordlist_t *prev, *cur, *next;
    if (!words->next) return words;
    // init
    cur = words;
    next = words->next;
    // first operation
    cur->next = (wordlist_t *)NULL;
    while (next->next) {
        // pointers move on
        prev = cur;
        cur = next;
        next = cur->next;
        // operation
        cur->next = prev;
    }
    next->next = cur;
    return next;
}

int is_built_in(simple_cmd_t *command) {
    return 0;
}

void exec_cmd(simple_cmd_t *command) {
    int child_pid;
    char **args;
    if (is_built_in(command)) {
    } else {
        if ((child_pid = fork()) < 0) {
            fprintf(stderr, "fork error!\n");
        } else if (child_pid == 0) { // child            
            //execlp(command->simple_cmd->words[0], command->simple_cmd->words, (char *)0);
            args = gen_args(command->words);
            exit(execvp(command->words->word, args));
        } else {
            waitpid(child_pid, NULL, 0);
        }
    }
}

void eval_loop() {
    char *tmp, c;
    int i;
    tmp = (char *) malloc(sizeof(char) * 256);
	while ((tmp = readline("myshell$ ")) != NULL) {
		//printf("my_shell$ ");
        // fill the buffer
        /*i = 0;
        while ((c = getchar()) != '\n') {   // ASCII 10 stands for '\n'
            if (c == EOF) {
                printf("\n");
                exit(0);
            }
            tmp[i++] = c;
        }
        tmp[i] = '\n';
        tmp[i + 1] = '\0';
        if (!(strcmp(tmp, "\n"))) continue;*/
        add_history(tmp);
        yy_scan_string(tmp);
		yyparse();
        parsed_command->words = reverse_command(parsed_command->words);
        //describe_command(parsed_command);
        exec_cmd(parsed_command);
	}
    free(tmp);
}

int main(void) {
	eval_loop();
	return 0;
}
