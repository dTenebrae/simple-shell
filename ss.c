#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SS_BUFSIZE 1024
#define SS_TOKEN_BUFSIZE 64
#define SS_TOKEN_DELIM " \t\r\n\a"

int ss_cd(char **args);
int ss_exit(char **args);
int ss_pwd(char **args);

char *builtin_str[] = {
        "cd",
        "exit",
        "pwd",
};

int (*builtin_func[]) (char **) = {
        &ss_cd,
        &ss_exit,
        &ss_pwd,
};

int ss_num_builtins()
{
        return sizeof(builtin_str) / sizeof(char *);
}

int ss_cd(char **args)
{
        if (args[1] == NULL) {
                fprintf(stderr, "ss: unexpected argument\n");
        } else {
                if (chdir(args[1]) != 0)
                        perror("ss");
        }
        return 1;
}

int ss_pwd(char **args)
{
        char cwd[SS_TOKEN_BUFSIZE];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
        } else {
                perror("ss");
        }
        return 1;
}

int ss_exit(char **args)
{
        return 0;
}

char* ss_read_line(void)
{
        int bufsize = SS_BUFSIZE;
        int pos     = 0;
        int c;

        //allocate primary buffer size to read users input
        char *buffer = malloc(sizeof(char) * bufsize);

        if (!buffer) {
                fprintf(stderr, "ss: memory allocation error\n");
                exit(EXIT_FAILURE);
        }

        while (true) {
                c = getchar();

                if (c == '\n') {
                        buffer[pos] = '\0';
                        return buffer;
                } else if (c == EOF) {
                        buffer[pos] = EOF;
                        return buffer;
                } else {
                        buffer[pos] = c;
                }
                pos++;

                // if memory is not enough for input
                if (pos >= bufsize) {
                        bufsize += SS_BUFSIZE;
                        buffer = realloc(buffer, bufsize);
                        if (!buffer) {
                                fprintf(stderr, "ss: memory reallocation error\n");
                                exit(EXIT_FAILURE);
                        }
                }
        }
}

char** ss_parse_line(char* line)
{
        int    bufsize = SS_TOKEN_BUFSIZE;
        int    pos     = 0;
        char **tokens  = malloc(bufsize * sizeof(char*));
        char  *token;

        if (!tokens) {
                fprintf(stderr, "ss: memory allocation error\n");
                exit(EXIT_FAILURE);
        }

        token = strtok(line, SS_TOKEN_DELIM);
        if (*token == EOF) {
                tokens[pos] = "exit";
                return tokens;
        }
        while (token != NULL) {
                tokens[pos] = token;
                pos++;

                if (pos >= bufsize) {
                        bufsize += SS_TOKEN_BUFSIZE;
                        tokens = realloc(tokens, bufsize * sizeof(char*));
                        if (!tokens) {
                                fprintf(stderr, "ss: memory allocation error\n");
                                exit(EXIT_FAILURE);
                        }
                }
                token = strtok(NULL, SS_TOKEN_DELIM);
        }
        tokens[pos] = NULL;
        return tokens;
}

int ss_run(char **args)
{
        pid_t pid, wpid;
        int status;

        pid = fork();
        // It's a child
        if (pid == 0) {
                // trying to execute command
                if (execvp(args[0], args) == -1)
                        perror("ss");
                exit(EXIT_FAILURE);
        } else if (pid < 0) {
                // forking goes wrong
                perror("ss");
        
        } else {
                // it's parent
                do {
                        wpid = waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }

        return wpid;
}

int ss_exec(char **args)
{
        // input is empty
        if (args[0] == NULL)
                return 1;

        // check, if input from builtins
        for (int i = 0; i < ss_num_builtins(); i++) {
                if (!strcmp(args[0], builtin_str[i])) {
                        return (*builtin_func[i])(args);
                }
        }

        return ss_run(args);
}


void ss_loop(void)
{
        char *line;
        char **parsed;
        int   status = 1;

        do {
                // TODO replace with customizable input char
                printf("> ");
                line = ss_read_line();
                parsed = ss_parse_line(line);
                status = ss_exec(parsed);

                free(line);
                free(parsed);

        } while (status);
}

int main (int argc, char *argv[])
{
        // main shell loop
        ss_loop();

        return 0;
}
