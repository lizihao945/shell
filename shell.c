#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "shell.h"

// functions
job_t *add_job(pid_t pid);

// varialbes
simple_cmd_t *parsed_command;
job_t *jobs = NULL;
int jobs_count = 0;
pid_t fgPid = 0;

char *echo_prompt() {
    return "myshell$ ";
}

void sig_handler(int signo) {
    job_t *p;
    switch (signo) {
        // Ctrl + c: should be ignored
        // if there's no foreground job
        case SIGINT:
            if (fgPid != 0) return;
            printf("\n%s", echo_prompt());
            break;
        // Ctrl + z: ignored if there's no job(won't go here)
        case SIGTSTP:
            p = add_job(fgPid);
            p->stat = STOPPED;
            printf("\n[%d]+  Stopped\t\t\t%s\n", p->pid, p->cmd);
            // send stop signal to child
            kill(fgPid, SIGSTOP);
            fgPid = 0;
            // ignore subsequent SIGTSTP
            signal(SIGTSTP, SIG_IGN);
            break;
        // Ctrl + z: sent from stopped child
        case SIGCHLD:
            break;
    }
}

job_t *add_job(pid_t pid) {
    job_t *rt, *p;
    // add SIGTSTP handler
    // signal(SIGTSTP, sig_handler);
    // check if job exists
    p = jobs;
    while (p != NULL && p->pid != pid)
        p = p->next;
    if (p != NULL) return p;
    // generate a new job
    rt = (job_t *) malloc(sizeof(job_t));
    rt->pid = pid;
    rt->cmd = (char *) malloc(sizeof(char) * FILE_LENGTH);
    strcpy(rt->cmd, parsed_command->words->word);
    rt->stat = RUNNING;
    rt->num = ++jobs_count;
    rt->next = NULL;
    // add it to the job list
    if (jobs == NULL)
        jobs = rt;
    else {
        for (p = jobs; p->next != NULL; p = p->next);
        p->next = rt;
    }
    return rt;
}

void kill_job(pid_t pid) {
    job_t *p, *last;
    p = jobs;
    while (p != NULL && p->pid != pid) {
        last = p;
        p = p->next;
    }
    if (p == NULL) {
        printf("kill: no such job\n");
        return;
    }
    if (p == jobs)
        jobs = jobs->next;
    else
        last->next = p->next;
    free(p);
}

void describe_command(simple_cmd_t *command) {
    wordlist_t *p;
    redirect_t *q;
    p = command->words;
    printf("command: %s\n", command->words->word);
    printf("arguments: ");
    while (p = p->next)
        printf("%s ", p->word);
    printf("\n");
    printf("redirects: ");
    q = command->redirects;
    while (q) {
        printf("%c %s", q->token_num, q->redirectee.filename);
        q = q->next;
    }
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

void exec_fg(pid_t pid) {
    job_t *p;
    p = jobs;
    while (p != NULL && p->pid != pid)
        p = p->next;
    if (p == NULL) {
        printf("job not found!\n");
        return;
    }
    fgPid = p->pid;
    p->stat = RUNNING;
    // ready to stop it
    signal(SIGTSTP, sig_handler);
    kill(p->pid, SIGCONT);
    waitpid(fgPid, NULL, WUNTRACED);
}

void exec_bg(pid_t pid) {
    job_t *p;
    p = jobs;    
    while (p != NULL && p->pid != pid)
        p = p->next;
    if (p == NULL) {
        printf("job not found!\n");
        return;
    }
    p->stat = RUNNING;
    printf("\n[%d]+ %s &\n", p->pid, p->cmd);
    kill(p->pid, SIGCONT);
}

int is_built_in(simple_cmd_t *command) {
    if (!strcmp(command->words->word, "fg"))
        return CMD_FG;
    else if (!strcmp(command->words->word, "bg"))
        return CMD_BG;
    else return 0;
}

void exec_cmd(simple_cmd_t *command) {
    pid_t child_pid;
    char **args;
    redirect_t *tmp;
    int fd, flag, cmd_idx;
    if (cmd_idx = is_built_in(command)) {
        switch(cmd_idx) {
            case CMD_FG:
                exec_fg(atoi(command->words->next->word));
                break;
            case CMD_BG:
                exec_bg(atoi(command->words->next->word));
                break;
        }
    } else {
        if ((child_pid = fork()) < 0) {
            fprintf(stderr, "fork error!\n");
        } else if (child_pid == 0) { // child
            tmp = command->redirects;
            while (tmp) {
                // define open flag
                //printf("%c to %s\n", tmp->token_num, tmp->redirectee.filename);
                if (tmp->redirectee.filename) {
                    switch (tmp->token_num) {
                        case '>':
                            flag = O_TRUNC | O_WRONLY | O_CREAT;
                            fd = open(tmp->redirectee.filename, flag, S_IRUSR | S_IWUSR);
                            dup2(fd, STDOUT_FILENO);
                            close(fd);
                            break;
                        case '<':
                            flag = O_RDONLY;
                            fd = open(tmp->redirectee.filename, flag);
                            dup2(fd, STDIN_FILENO);
                            close(fd);
                            break;
                        case RE_DGREAT:
                            break;
                        case RE_DLESS:
                            break;
                    }
                } else
                    fd = tmp->redirectee.fd;
                tmp = tmp->next;
            }
            args = gen_args(command->words);
            if (execvp(command->words->word, args)) {
                switch (errno) {
                    case EACCES:
                        printf("%s: Permission denied\n", command->words->word);
                        break;
                    case ENOENT:
                        printf("%s: No such file or directory\n", command->words->word);
                        break;
                    default:
                    printf("Errno number %d not handled!\n", errno);
                }
            }
            exit(0);
        } else { // parent
            // foreground execution
            fgPid = child_pid;
            // ready to stop it
            signal(SIGTSTP, sig_handler);
            // if child process is suspended,
            // wait() will return
            waitpid(child_pid, NULL, WUNTRACED);
            // update status
            fgPid = 0;
            signal(SIGTSTP, SIG_IGN);
        }
    }
}

void eval_loop() {
    char *tmp, c;
    int i;
    tmp = (char *) malloc(sizeof(char) * 256);
    while ((tmp = readline(echo_prompt())) != NULL) {
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
        if (parsed_command == NULL) continue;
        parsed_command->words = reverse_command(parsed_command->words);
        describe_command(parsed_command);
        //exec_cmd(parsed_command);
    }
    free(tmp);
}

int main(void) {
    signal(SIGINT, sig_handler);
    //signal(SIGCHLD, sig_handler);
    signal(SIGTSTP, SIG_IGN);
	eval_loop();
	return 0;
}
