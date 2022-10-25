#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define DSH_BUFSIZE 1024
#define DSH_TOKEN_BUFSIZE 64
#define DSH_TOKEN_DELIM " \t\r\n\a"

int dsh_cd(char **args);
int dsh_exit(char **args);
int dsh_pwd(char **args);

char *builtin_str[] = {
        "cd",
        "exit",
        "pwd",
};

int (*builtin_func[]) (char **) = {
        &dsh_cd,
        &dsh_exit,
        &dsh_pwd,
};

int dsh_num_builtins()
{
        return sizeof(builtin_str) / sizeof(char *);
}

int dsh_cd(char **args)
{
        if (args[1] == NULL) {
                fprintf(stderr, "dsh: unexpected argument\n");
        } else {
                if (chdir(args[1]) != 0)
                        perror("dsh");
        }
        return 1;
}

int dsh_pwd(char **args)
{
        char cwd[DSH_TOKEN_BUFSIZE];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
        } else {
                perror("dsh");
        }
        return 1;
}

int dsh_exit(char **args)
{
        return 0;
}

char* dsh_readline(void)
{
        int bufsize = DSH_BUFSIZE;
        int pos     = 0;
        int c;

        //allocate primary buffer size to read users input
        char *buffer = malloc(sizeof(char) * bufsize);

        if (!buffer) {
                fprintf(stderr, "dsh: memory allocation error\n");
                exit(EXIT_FAILURE);
        }

        while (1) {
                c = getchar();

                if (c == EOF || c == '\n') {
                        buffer[pos] = '\0';
                        return buffer;
                } else {
                        buffer[pos] = c;
                }
                pos++;

                // if memory is not enough for input
                if (pos >= bufsize) {
                        bufsize += DSH_BUFSIZE;
                        buffer = realloc(buffer, bufsize);
                        if (!buffer) {
                                fprintf(stderr, "dsh: memory reallocation error\n");
                                exit(EXIT_FAILURE);
                        }
                }
        }
}

char** dsh_parseline(char* line)
{
        int    bufsize = DSH_TOKEN_BUFSIZE;
        int    pos     = 0;
        char **tokens  = malloc(bufsize * sizeof(char*));
        char  *token;

        if (!tokens) {
                fprintf(stderr, "dsh: memory allocation error\n");
                exit(EXIT_FAILURE);
        }

        token = strtok(line, DSH_TOKEN_DELIM);
        while (token != NULL) {
                tokens[pos] = token;
                pos++;

                if (pos >= bufsize) {
                        bufsize += DSH_TOKEN_BUFSIZE;
                        tokens = realloc(tokens, bufsize * sizeof(char*));
                        if (!tokens) {
                                fprintf(stderr, "dsh: memory allocation error\n");
                                exit(EXIT_FAILURE);
                        }
                }
                token = strtok(NULL, DSH_TOKEN_DELIM);
        }
        tokens[pos] = NULL;
        return tokens;
}

int dsh_run(char **args)
{
        pid_t pid, wpid;
        int status;

        pid = fork();
        // It's a child
        if (pid == 0) {
                if (execvp(args[0], args) == -1)
                        perror("dsh");
                exit(EXIT_FAILURE);
        } else if (pid < 0) {
                perror("dsh");
        
        } else {
                do {
                        wpid = waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
        return 1;
}

int dsh_exec(char **args)
{
        if (args[0] == NULL)
                return 1;

        for (int i = 0; i < dsh_num_builtins(); i++) {
                if (strcmp(args[0], builtin_str[i]) == 0) {
                        return (*builtin_func[i])(args);
                }
        }

        return dsh_run(args);
}


void dsh_loop(void)
{
        char *line;
        char **parsed;
        int   status = 1;

        do {
                // TODO replace with customizable input char
                printf("> ");
                line = dsh_readline();
                parsed = dsh_parseline(line);
                status = dsh_exec(parsed);

                free(line);
                free(parsed);

        } while (status);
}

int main (int argc, char *argv[])
{
        // load config files here

        // main shell loop
        dsh_loop();

        // shutdown/cleanup routines

        return 0;
}
