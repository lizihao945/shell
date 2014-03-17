#include <stdio.h>
#include <stdlib.h>
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
            exit(execve(command->words->word, args, (char **)0));
        } else {
            waitpid(child_pid, NULL, 0);
        }
    }
}

void eval_loop() {
    char *tmp, c;
    int i;
    tmp = (char *) malloc(sizeof(char) * 256);
	while (1) {
		printf("my_shell$ ");
        // fill the buffer
        i = 0;
        while ((c = getchar()) != '\n') tmp[i++] = c;   // ASCII 10 stands for '\n'
        tmp[i] = '\n';
        tmp[i + 1] = '\0';
        if (!(strcmp(tmp, "\n"))) continue;
        yy_scan_string(tmp);
		yyparse();
        //describe_command(parsed_command);
        exec_cmd(parsed_command);
	}
}

int main(void) {
	eval_loop();
	return 0;
}
