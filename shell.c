#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "shell.h"

// functions
job_t *add_job(pid_t pid);
void exec_cmd(simple_cmd_t *command);

// varialbes
struct sigaction sa_val;
pipeline_t *parsed_pipeline;
job_t *jobs = NULL;
int jobs_count = 0;
char *line_str;
int background;
int status;

char *echo_prompt() {
    char *rt, *p;
    rt = (char *) malloc(sizeof(char) * FILE_LENGTH);
    rt[0] = '[';
    rt[1] = '\0';
    strcat(rt, getpwuid(getuid())->pw_name);
    p = (char *) malloc(sizeof(char) * FILE_LENGTH);
    gethostname(p, FILE_LENGTH);
    strcat(rt, "@");
    strcat(rt, p);
    getcwd(p, FILE_LENGTH);
    strcat(rt, ":");
    strcat(rt, p);
    strcat(rt, "]$ ");
    return rt;
    //return "myshell$ ";
}

pipeline_t *reverse_pipeline(pipeline_t *pipeline) {
    pipeline_t *prev, *cur, *next;
    if (!pipeline->next) return pipeline;
    // init
    cur = pipeline;
    next = pipeline->next;
    // first operation
    cur->next = (pipeline_t *)NULL;
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

void kill_job(pid_t pid) {
    job_t *p, *last;
    p = jobs;
    while (p && p->pid != pid) {
        last = p;
        p = p->next;
    }
    // the process is not in the job list
    if (p == NULL)
        return;
    if (p == jobs)
        jobs = jobs->next;
    else
        last->next = p->next;
    free(p);
}

job_t *find_job(pid_t pid) {
    job_t *p;
    for (p = jobs; p && p->pid != pid; p = p->next);
    // the process is not in the job list
    return p;
}

job_t *add_job(pid_t pid) {
    job_t *rt, *p;
    // check if job exists
    p = jobs;
    while (p && p->pid != pid)
        p = p->next;
    if (p) return p;
    // generate a new job
    rt = (job_t *) malloc(sizeof(job_t));
    rt->pid = pid;
    rt->cmd = (char *) malloc(sizeof(char) * FILE_LENGTH);
    strcpy(rt->cmd, line_str);
    rt->num = ++jobs_count;
    rt->next = NULL;
    // add it to the job list
    if (!jobs)
        jobs = rt;
    else {
        for (p = jobs; p->next != NULL; p = p->next);
        p->next = rt;
    }
    return rt;
}

void describe_pipeline(pipeline_t *pipeline) {
    wordlist_t *p;
    redirect_t *q;
    simple_cmd_t *command;
    for (; pipeline != NULL; pipeline = pipeline->next) {
        command = pipeline->cmd;
        p = command->words;
        printf("command: %s", command->words->word);
        if (background)
            printf(" &\n");
        else
            printf("\n");
        printf("arguments: ");
        while (p = p->next)
            printf("%s ", p->word);
        printf("\n");
        printf("redirects: ");
        q = command->redirects;
        while (q) {
            if (q->redirectee.filename)
                printf("%c %s; ", q->token_num, q->redirectee.filename);
            else
                printf("%c %d; ", q->token_num, q->redirectee.fd);
            q = q->next;
        }
        printf("\n");
    }
}

int wordlist_length(wordlist_t *words) {
    int count = 0;
    while (words = words->next) count++;
    return count;
}

void gen_redirects(pipeline_t *pipeline) {    
    redirect_t *p;
    int fd[2];
    for (; pipeline->next != NULL; pipeline = pipeline->next) {
        pipe(fd);
        // prev cmd writes to the write end
        p = (redirect_t *) malloc(sizeof(redirect_t));
        p->next = pipeline->cmd->redirects;
        p->token_num = '>';
        p->redirectee.fd = fd[1];
        p->redirectee.filename = NULL;
        pipeline->cmd->redirects = p;
        // next cmd reads from the read end
        p = (redirect_t *) malloc(sizeof(redirect_t));
        p->next = pipeline->next->cmd->redirects;
        p->token_num = '<';
        p->redirectee.fd = fd[0];
        p->redirectee.filename = NULL;
        pipeline->next->cmd->redirects = p;
    }
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

void check_on_exit() {
    job_t *p;
    for (p = jobs; p; p = p->next) {
        if (waitpid(p->pid, &status, WUNTRACED | WNOHANG))
            if (WIFSTOPPED(status)) {
                printf("[%d]  Terminated\t\t\t%s\n", p->pid, p->cmd);   
            }
    }
}

// builtin commands
void exec_history() {
    HIST_ENTRY *p;
    int i = 1;
    history_set_pos(0);
    while (p = current_history()) {
        printf("%5d  %s\n", i++, p->line);
        next_history();
    }
    
}

int is_built_in(simple_cmd_t *command) {
    if (!strcmp(command->words->word, "fg"))
        return CMD_FG;
    else if (!strcmp(command->words->word, "bg"))
        return CMD_BG;
    else if (!strcmp(command->words->word, "exit")) {
        check_on_exit();
        exit(0);
    } else if (!strcmp(command->words->word, "history"))
        return CMD_HISTORY;
    else if (!strcmp(command->words->word, "jobs"))
        return CMD_JOBS;
    else return 0;
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
    // give control
    kill(pid, SIGCONT);
    tcsetpgrp(STDIN_FILENO, pid);
    //printf("foreground group is %d(should be %d)\n", tcgetpgrp(STDIN_FILENO), pid);
    if (waitpid(pid, &status, WUNTRACED))
        if (WIFEXITED(status))
            kill_job(p->pid);
        else if (WIFSTOPPED(status)) {
            add_job(p->pid);
            printf("\n[%d]  Stopped\t\t\t%s\n", p->pid, p->cmd);
        }
    tcsetpgrp(STDIN_FILENO, getpgrp());
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
    printf("\n[%d]  Continued\t\t\t%s\n", p->pid, p->cmd);
    kill(p->pid, SIGCONT);
    if (waitpid(pid, &status, WUNTRACED | WNOHANG))
        if (WIFSTOPPED(status)) {
            add_job(p->pid);
            printf("\n[%d]  Stopped\t\t\t%s\n", p->pid, p->cmd);
        }
}

void exec_jobs() {
    job_t *p;
    for (p = jobs; p; p = p->next) {
        if (waitpid(p->pid, &status, WNOHANG)) {
            if (WIFSTOPPED(status))
                printf("[%d]  Stopped\t\t\t%s\n", p->pid, p->cmd);
            else if (WIFEXITED(status)) {
                printf("[%d]  Terminated\t\t\t%s\n", p->pid, p->cmd);
                kill_job(p->pid);
            }
        } else // status not available
            printf("[%d]  Running\t\t\t%s\n", p->pid, p->cmd);
    }
}
// builtin commands end

void sig_handler(int signo, siginfo_t *si, void *context) {
    job_t *p;
    switch (signo) {
        // Ctrl + c: should be ignored
        // if there's no foreground job
        case SIGINT:
            printf("\n%s", echo_prompt());
            break;
        // SIGCHLD is not handled here
        /*case SIGCHLD:
            waitpid(si->si_code, NULL, WNOHANG);
            switch (si->si_code) {
                case CLD_EXITED:
                case CLD_KILLED:
                    kill_job(si->si_pid);
                    fg_pid = 0;
                    break;
                case CLD_CONTINUED:
                    p = find_job(si->si_pid);
                    printf("%s\n", p->cmd);
                    waitpid(si->si_pid, &status, 0);
                    tcsetpgrp(STDIN_FILENO, getpgrp());
                    break;
                case CLD_STOPPED:
                    p = find_job(si->si_pid);
                    printf("\n[%d]  Stopped\t\t\t%s\n", p->pid, p->cmd);
                    break;
                default:
                    printf("SIGCHLD caught and not handled!\n");
            }*/
    }
}

void print_status(int status, pid_t pid, char *cmd) {
    if (WIFEXITED(status))
        printf("\n[%d]  Terminated\t\t\t%s\n", pid, cmd);
    if (WIFSTOPPED(status))
        printf("\n[%d]  Stopped\t\t\t%s\n", pid, cmd);
    if (WIFSIGNALED(status))
        printf("WIFSIGNALED: true\n");
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
            case CMD_HISTORY:
                exec_history();
                break;
            case CMD_JOBS:
                exec_jobs();
                break;
        }
    } else {
        if ((child_pid = fork()) < 0) {
            fprintf(stderr, "fork error!\n");
        } else if (child_pid == 0) { // child
            signal(SIGTSTP, SIG_DFL);
            //signal(SIGINT, SIG_DFL);
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
                } else {
                    fd = tmp->redirectee.fd;
                    if (tmp->token_num == '>') {
                        //printf("%s: write to %d\n", command->words->word, fd);
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    } else {
                        //printf("%s: read from %d\n", command->words->word, fd);
                        dup2(fd, STDIN_FILENO);
                        close(fd);
                    }
                }
                tmp = tmp->next;
            }
            // transfer arguments
            args = gen_args(command->words);
            // set child process to separate group
            setpgid(0, 0);
            // execute
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
            // parent should close these pipefds
            // so that read end won't hang
            tmp = command->redirects;
            while (tmp) {
                if (!tmp->redirectee.filename)
                    close(tmp->redirectee.fd);
                tmp = tmp->next;
            }
            if (!background) {
                // parent gives the control of terminal
                // child is in the child_pid group
                tcsetpgrp(STDIN_FILENO, child_pid);
                //printf("foreground group is %d(should be %d)\n", tcgetpgrp(STDIN_FILENO), child_pid);                
                // if SIGTSTP is sent to child
                // shell process shuold be notified
                if (waitpid(child_pid, &status, WUNTRACED))
                    if (WIFEXITED(status))
                        kill_job(child_pid);
                    else if (WIFSTOPPED(status)) {
                        add_job(child_pid);
                        printf("\n[%d]  Stopped\t\t\t%s\n", child_pid, line_str);
                    }
                tcsetpgrp(STDIN_FILENO, getpgrp());
            } else {
                printf("\n[%d]  %s\n", child_pid, line_str);
                add_job(child_pid);
                if (waitpid(child_pid, &status, WUNTRACED | WNOHANG))
                    if (WIFSTOPPED(status))
                        printf("\n[%d]  Stopped\t\t\t%s\n", child_pid, line_str);
            }
        }
    }
}

void exec_pipeline(pipeline_t *pipeline) {
    for (; pipeline; pipeline = pipeline->next)
        exec_cmd(pipeline->cmd);
}

void check_jobs() {
    job_t *p;
    for (p = jobs; p; p = p->next) {
    }
}

void eval_loop() {
    char *tmp, c;
    int i;
    pipeline_t *p;
    //printf("foreground group is %d\n", tcgetpgrp(STDIN_FILENO));
    while ((line_str = readline(echo_prompt())) != NULL) {
        add_history(line_str);
        yy_scan_string(line_str);
        yyparse();
        if (parsed_pipeline == NULL) continue;
        parsed_pipeline = reverse_pipeline(parsed_pipeline);
        for (p = parsed_pipeline; p != NULL; p = p->next)
            p->cmd->words = reverse_command(p->cmd->words);
        gen_redirects(parsed_pipeline);
        //describe_pipeline(parsed_pipeline);
        exec_pipeline(parsed_pipeline);
        check_jobs();
    }
    printf("exit\n");
    check_on_exit();
}

int main(void) {
    // when tcsetpgrp is called by a background process,
    // a SIGTTOU is sent to the background process group
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    //signal(SIGTTIN, SIG_IGN);
    // Specify that we will use a signal handler that takes three arguments
    // instead of one, which is the default.
    sa_val.sa_flags = SA_SIGINFO;
    sa_val.sa_sigaction = sig_handler;
    sigfillset(&sa_val.sa_mask);
    //sigaction(SIGCHLD, &sa_val, NULL);
    sigaction(SIGINT, &sa_val, NULL);
    line_str = (char *) malloc(sizeof(char) * FILE_LENGTH);
	eval_loop();
	return 0;
}
