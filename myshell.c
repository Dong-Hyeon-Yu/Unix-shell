#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <setjmp.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_CMD_ARG 15
#define BUFSIZ 256

const char *prompt = "myshell> ";
char* cmdvector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];

/* SIGINT & SIGTSTP */
void interrupt_handler(int sig_no, siginfo_t *sig_info, void* param2){
    pid_t who_signal = sig_info->si_pid;

    /* root process(pid==0) sends signals to this shell */
    if (who_signal == 0) {
        printf("\n");
        cmdline[0] = '\0';
    }
}

/* SIGCHLD */
void child_handler(int sig_no){
    while(waitpid(-1, NULL, WNOHANG)>0);
}


/* parse cmd parameters */
void fatal(char *str){
    perror(str);
    exit(1);
}
int makelist(char *s, const char *delimiters, char** list, int MAX_LIST){
    int i = 0;
    int numtokens = 0;
    char *snew = NULL;

    if( (s==NULL) || (delimiters==NULL) ) return -1;

    snew = s + strspn(s, delimiters);	/* Skip delimiters */
    if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
        return numtokens;

    numtokens = 1;

    while(1){
        if( (list[numtokens]=strtok(NULL, delimiters)) == NULL) break;
        if(numtokens == (MAX_LIST-1)) return -1;
        numtokens++;
    }
    return numtokens;
}

int DoPipesExist(char** cmd){
    int cnt = 0;
    for (int i = 0; cmd[i] != NULL; i++)
        if (strcmp(cmd[i], "|") == 0)
            cnt += 1;

    return cnt;
}

int doesRedirectionExist(char** cmd, char* mode) {
    for (int i = 0; cmd[i] != NULL; i++)
        if (strcmp(cmd[i], mode) == 0) return i;

    return -1;
}

void _removeRedirectionChar(char** cmd, int idx){
    int i;
    for (i = idx; cmd[i+2] != NULL; i++)
        cmd[i] = cmd[i+2];
    cmd[i] = NULL;
}

int makeRedirection(char** cmd, int char_idx, char* mode) {
    int fd, i;
    char* filepath = cmd[char_idx + 1];

    if (strcmp("<", mode) == 0) { /* infile redirection */
        if ((fd = open(filepath, O_RDONLY)) == -1) {
            perror("syscall 'open' failed!!");
            return -1;
        }
        dup2(fd, STDIN_FILENO);
    }
    else if (strcmp(">", mode) == 0) { /* outfile redirection */
        if ((fd = open(filepath, O_WRONLY|O_TRUNC|O_CREAT, 0644)) == -1) {
            perror("syscall 'open' failed!!");
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
    }
    
    _removeRedirectionChar(cmd, char_idx);
    
    close(fd);

    return 0;
}

void _getSubCmd(char* cmd[MAX_CMD_ARG], char* subcmd[MAX_CMD_ARG], int *cmd_idx){
    int i;
    for(i = 0; cmd[(*cmd_idx)] != NULL && strcmp(cmd[(*cmd_idx)], "|") != 0 ; i++, (*cmd_idx)++)
        subcmd[i] = cmd[(*cmd_idx)];

    subcmd[i] = NULL;
}

void doPipe(char** cmd, int pipe_cnt){
    int pipe_fd[MAX_CMD_ARG][2];
    char* subcmd[MAX_CMD_ARG]; 	// i번째 파이프 명령만 추출
    char* filepath; 		// 리다이렉션 파일
    int fd; 			// 리다이렉션 파일 디스크립터
    int in_idx, out_idx; 	// 리다이렉션 문자 위치
    int cmd_idx = 0; 		// 다음 pipe 명령을 수행하기 위한 인덱스
    pid_t pid;

    // first cmd
    pipe(pipe_fd[0]);
    if((in_idx = doesRedirectionExist(cmd, "<")) > -1){
        filepath = cmd[in_idx + 1];
        if ((fd = open(filepath, O_RDONLY)) == -1) {
            perror("syscall 'open' failed!!");
            return;
        }
        _removeRedirectionChar(cmd, in_idx);
    }
    _getSubCmd(cmd, subcmd, &cmd_idx); /* extract pipe cmd */


    if ((pid = fork()) == -1) fatal("fail to fork for pipe!");
    else if (pid == 0) {
        if (in_idx > -1) {
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        dup2(pipe_fd[0][1], STDOUT_FILENO);
        close(pipe_fd[0][0]);
        close(pipe_fd[0][1]);

        execvp(subcmd[0], subcmd);
        fatal("doing pipe!1");
    }
    else {
        if (in_idx > -1) close(fd);
        close(pipe_fd[0][1]);
        waitpid(pid, NULL, 0);
    }

    // middle cmd
    int cur_cnt = 0;
    while(++cur_cnt < pipe_cnt){
        pipe(pipe_fd[cur_cnt]);
        cmd_idx += 1;
        _getSubCmd(cmd, subcmd, &cmd_idx);

        if((pid = fork()) == -1)
            fatal("fail to fork for pipe!");
        else if (pid == 0) {
            dup2(pipe_fd[cur_cnt - 1][0], STDIN_FILENO);
            dup2(pipe_fd[cur_cnt][1], STDOUT_FILENO);
            close(pipe_fd[cur_cnt - 1][0]);
            close(pipe_fd[cur_cnt][0]);
            close(pipe_fd[cur_cnt][1]);

            execvp(subcmd[0], subcmd);
            fatal("doing pipe!2");
        }
        else {
            close(pipe_fd[cur_cnt -1][0]);
            close(pipe_fd[cur_cnt][1]);
            waitpid(pid, NULL, 0);
        }
    }

    // last cmd
    if((out_idx = doesRedirectionExist(cmd, ">")) > -1){
        filepath = cmd[out_idx + 1];
        if ((fd = open(filepath, O_WRONLY|O_TRUNC|O_CREAT, 0644)) == -1) {
            perror("syscall 'open' failed!!");
            return;
        }
        _removeRedirectionChar(cmd, out_idx);
    }
    cmd_idx += 1;
    _getSubCmd(cmd, subcmd, &cmd_idx); /* extract pipe cmd */

    if ((pid = fork()) == -1)
        fatal("fail to fork for pipe!");
    else if (pid == 0) {
        if (out_idx > -1) {
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        dup2(pipe_fd[cur_cnt - 1][0], STDIN_FILENO);
        close(pipe_fd[cur_cnt - 1][0]);

        execvp(subcmd[0], subcmd);
        fatal("doing pipe!3");
    }
    else {
        if (out_idx > -1) close(fd);
        close(pipe_fd[cur_cnt -1][0]);
        waitpid(pid, NULL, 0);
    }
}

/* main of this shell */
int main(int argc, char**argv){
    static struct sigaction act, child;

    /* SIGINT & SIGTSTP handler*/
    act.sa_sigaction = interrupt_handler;
    act.sa_flags = SA_SIGINFO;
    sigfillset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTSTP, &act, NULL);

    /* SIGCHLD handler */
    child.sa_handler = child_handler;
    child.sa_flags = SA_RESTART;
    sigfillset(&child.sa_mask);
    sigaction(SIGCHLD, &child, NULL);


    int i = 0, pipe_cnt = 0;
    bool bg_flag = false;
    pid_t pid;
    while (1){
        fputs(prompt, stdout);
        fgets(cmdline, BUFSIZ, stdin);
        cmdline[strlen(cmdline) -1] = '\0';
        i = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

        if (i < 1) continue; // no user input like just enter

        /* chdir */
        if (i > 0 && strcmp(cmdvector[0], "cd")==0){
            chdir(cmdvector[1]);
            continue;
        }
        /* exit */
        else if (i > 0 && strcmp(cmdvector[0], "exit")==0)
            break;

        if ((bg_flag = (i>0 && strcmp("&", cmdvector[i-1])==0 ? 1 : 0)))
            cmdvector[i-1] = NULL;     /* background */

        switch(pid = fork()){
            case 0:
                if ((pipe_cnt = DoPipesExist(cmdvector)) > 0) {
                    /* pipe */
                    doPipe(cmdvector, pipe_cnt);
                    exit(0);
                }
                else {
                    int in_idx, out_idx;
                    /* infile redirection */
                    if ((in_idx = doesRedirectionExist(cmdvector, "<")) > -1)
                        if(makeRedirection(cmdvector, in_idx, "<") == -1) break;

                    /* outfile redirection */
                    if ((out_idx = doesRedirectionExist(cmdvector, ">")) > -1)
                        if(makeRedirection(cmdvector, out_idx, ">") == -1) break;

                    execvp(cmdvector[0], cmdvector);
                    fatal("main()");
                }
            case -1:
                fatal("main()");
            default:
                if (!bg_flag) waitpid(pid, NULL, 0);
        }
    }

    return 0;
}
