#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#define MAX_CMD_ARG 10
#define BUFSIZ 256

const char *prompt = "myshell> ";
char* cmdvector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];

void fatal(char*);
int makelist(char*, const char*, char**, int);
void check_redirection(char* (*)[MAX_CMD_ARG], char*);
int check_pipe(char**);
void set_pipe_cmd(char**, int);
void remove_pipe_character(char**);
void interrupt_handler(int, siginfo_t*, void*);
void child_handler(int);

void printcmd(char** cmdvector) {
	fprintf(stdout, "[printcmd]");
	for (int i = 0; cmdvector[i] != NULL; i++)
		fprintf(stdout, "%s[%d] ", cmdvector[i], i);
}
/* main of this shell */
int main(int argc, char** argv) {

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


	int i = 0;
	pid_t pid;
	int pfd_wronly[2];
	int pfd_rdonly[2];
	//int pipe_fd[2];
	while (1) {

		
		fputs(prompt, stdout);
		fgets(cmdline, BUFSIZ, stdin);
		cmdline[strlen(cmdline) - 1] = '\0';
		i = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

		if (i < 1) continue; // no user input like just enter

		/* chdir */
		if (i > 0 && strcmp(cmdvector[0], "cd") == 0) {
			chdir(cmdvector[1]);
		}
		/* exit */
		else if (i > 0 && strcmp(cmdvector[0], "exit") == 0) {
			return 0;
		}
		else {
			int bg_flag = (i > 0 && strcmp("&", cmdvector[i - 1]) == 0 ? 1 : 0);
			if (bg_flag) cmdvector[i - 1] = '\0';     /* background */

			int pipe_num = check_pipe(cmdvector); // return count of a pipe character "|"

			for (int cmd_idx = 0; cmd_idx <= pipe_num; cmd_idx++) {
				//pipe(pfd_wronly);
				//pipe(pfd_rdonly);
				fprintf(stdout, "[cur_idx: %d, pipe_num: %d]\n", cmd_idx, pipe_num);
				switch (pid = fork()) {
				case 0:

					// redirection!
					if (cmd_idx == 0) check_redirection(&cmdvector, "<");

					//if (pipe_num > 0) {

					//	set_pipe_cmd(cmdvector, cmd_idx);
					//	printcmd(cmdvector);
					//	//printf("\n4\n");
					//	if (cmd_idx > 0) {
					//		// 파이프에서 입력을 받겠다. (단, 첫번째 명령은 제외)
					//		//fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK);
					//		dup2(pfd_rdonly[0], 0);
					//	}

					//	if (cmd_idx < pipe_num) {
					//		// 파이프로 출력하겠다. (단, 마지막 명령은 제외)
					//		dup2(pfd_wronly[1], 1);
					//	}
					//}
					//close(pfd_rdonly[0]);
					//close(pfd_rdonly[1]);
					//close(pfd_wronly[0]);
					//close(pfd_wronly[1]);

					if (cmd_idx == pipe_num) check_redirection(&cmdvector, ">");


					printcmd(cmdvector);
					execvp(cmdvector[0], cmdvector);
					fatal("main()");
				case -1:
					fatal("main()");
				default:
					/*close(pfd_wronly[1]);
					close(pfd_rdonly[0]);

					int nread;
					char buffer[BUFSIZ];
					if ((nread = read(pfd_wronly[0], buffer, BUFSIZ)) == -1) {
						perror("");
					}
					if (write(pfd_rdonly[1], buffer, BUFSIZ) == -1) {
						perror("");
					}
					close(pfd_rdonly[1]);*/
					
					if (!bg_flag) waitpid(pid, NULL, 0);
					/*close(pfd_wronly[0]);*/
					
					
				}
				//}
			}
		}
	}
		return 0;
}
 
/* parse cmd parameters */
void fatal(char* str) {
	perror(str);
	exit(1);
}
int makelist(char* s, const char* delimiters, char** list, int MAX_LIST) {
	int i = 0;
	int numtokens = 0;
	char* snew = NULL;

	if ((s == NULL) || (delimiters == NULL)) return -1;

	snew = s + strspn(s, delimiters);	/* Skip delimiters */
	if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
		return numtokens;

	numtokens = 1;

	while (1) {
		if ((list[numtokens] = strtok(NULL, delimiters)) == NULL) break;
		if (numtokens == (MAX_LIST - 1)) return -1;
		numtokens++;
	}
	return numtokens;
}

/* SIGINT & SIGTSTP */
void interrupt_handler(int sig_no, siginfo_t* sig_info, void* param2) {
	pid_t who_signal = sig_info->si_pid;

	/* root process(pid==0) sends signals to this shell */
	if (who_signal == 0) {
		printf("\n");
		cmdline[0] = '\0';
	}
}
/* SIGCHLD */
void child_handler(int sig_no) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* redirection */
void remove_redirect_character(char* (*cmdvector)[MAX_CMD_ARG], int i) {
	if ((*cmdvector)[i + 2] != NULL) (*cmdvector)[i] = '\0';
	
	for (; (*cmdvector)[i + 2] != NULL; i++)
		(*cmdvector)[i] = (*cmdvector)[i + 2];

	(*cmdvector)[i] = '\0';
}
void redirect_outfile(char** cmdvector, int i) {
	char* filepath = cmdvector[i + 1];

	int fd;
	if ((fd = open(filepath, O_WRONLY | O_APPEND | O_CREAT, 0644)) == -1) {
		perror("fail to redirect out: ");
		exit(1);
	}

	dup2(fd, 1);
	close(fd);
}
void redirect_infile(char** cmdvector, int i) {
	// open file path, if there is no file path, ERROR!
	char* filepath = cmdvector[i + 1];

	int fd;
	if ((fd = open(filepath, O_RDONLY)) == -1) {
		perror("fail to redirect out: ");
		exit(1);
	}

	dup2(fd, 0);
	close(fd);
}
void check_redirection(char* (*cmdvector)[MAX_CMD_ARG], char* redirect_char) {
	int in = 0; int out = 0;

	for (int i = 0; (*cmdvector)[i] != NULL; i++)
		if (strcmp((*cmdvector)[i], redirect_char) == 0) {
			if (strcmp(redirect_char, "<") == 0)
				redirect_infile(*cmdvector, i);
			else
				redirect_outfile(*cmdvector, i);

			remove_redirect_character(cmdvector, i);
		}
}

/* pipe */
void remove_pipe_character(char** cmdvector) {
	for (int i = 0; cmdvector[i + 2] != NULL; i++)
		if (strcmp(cmdvector[i], "|") == 0) {
			cmdvector[i] = '\0';
			break;
		}
}
int check_pipe(char** cmdvector) {
	int count = 0;
	for (int i = 0; cmdvector[i] != NULL; i++)
		if (strcmp(cmdvector[i], "|") == 0) count += 1;

	return count;
}
void set_pipe_cmd(char** cmdvector, int idx) {
	if (idx == 0) {
		int i;
		for (i = 0; cmdvector[i] != NULL && strcmp(cmdvector[i], "|"); i++) {}
		cmdvector[i] = '\0';
		return;
	}

	int dist = 0; int cnt = 0;
	for (int i = 0; cmdvector[i] != NULL; i++)
		if (strcmp(cmdvector[i], "|") == 0) {
			if (++cnt == idx) {
				dist = i + 1;
				while (cmdvector[i + 1] != NULL && strcmp(cmdvector[i + 1], "|") != 0) {
					cmdvector[i + 1 - dist] = cmdvector[i + 1];
					i += 1;
				}
				cmdvector[i + 1 - dist] = '\0';

				return;
			}
		}
}