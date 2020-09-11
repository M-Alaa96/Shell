#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#define BUFFER_MAX_LENGTH 200
#define HISTORY_MAX_LENGTH 10

FILE * file = NULL;
char* history[HISTORY_MAX_LENGTH];
char* paths[BUFFER_MAX_LENGTH];

int paths_length;
int history_pointer = 0;
int empty_file = 0;
int background = 1;
char history_name[8] = "history";
char exit_command[5] = "exit";
char execute_recent_commad[3] = "!!";


// for user_command >> 0 , for history = 1
int open_file(char* path)
{

    file = fopen(path, "r");
    if (!file)
    {
        fprintf(stderr, "error: could not open textfile : %s\n", path);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    unsigned long len = (unsigned long)ftell(file);
    if(len > 0)
    {
        rewind(file);
        return (!empty_file);
    }
    return empty_file;
}

void add_command_to_history(char* command)
{
    if(history_pointer == HISTORY_MAX_LENGTH)
    {
        int i = 0;
        for(i = 0 ; i < HISTORY_MAX_LENGTH-1 ; i++)
        {
            history[i] = history[i+1];
        }
        history[HISTORY_MAX_LENGTH-1] = command;
    }
    else
    {
        history[history_pointer++] = command;
    }
}

int check_mode(int length ,char* arguments[])
{

    if(strcmp(arguments[length-1],"&") == 0)
    {
        arguments[length-1] =NULL;
        return background;
    }
    else
    {
        arguments[length] = NULL;
        return (!background);
    }
}
int splite(char* command , char* delim ,char* arguments[])
{
    int pointer = 0;
    char* token;
    for (token = strtok(command, delim); token; token = strtok(NULL, delim))
    {
        arguments[pointer++] = token;
    }
    return pointer;
}

int command_exceed_max_char(char* command)
{
    int size_of_line = strlen (command);
    if(size_of_line > 80)
    {
        fprintf(stderr, "error: line is too long.it exceed 80 character \n");
        return 1;
    }
    return 0;
}

char* read_from_user()
{
    char* line = malloc(BUFFER_MAX_LENGTH);
    fgets(line,BUFFER_MAX_LENGTH,stdin);
    if(line[0] == '\0')
    {
        printf("\n");
        return exit_command;
    }
    line[strlen(line)-1] = '\0';
    return line;
}

char* read_line_from_file()
{
    char* line = malloc(BUFFER_MAX_LENGTH);
    int tempChar;
    unsigned int tempCharIdx = 0U;
    int reading_line = 1;

    /* get a character from the file pointer */
    while(reading_line)
    {

        tempChar = fgetc(file);
        /* test character value */
        if (tempChar == EOF)
        {
            line[tempCharIdx] = '\0';
            fclose(file);
            break;
        }
        else if(tempChar == '\n' && tempCharIdx == 0)
        {
            continue;
        }
        else if (tempChar == '\n')
        {
            line[tempCharIdx] = '\0';
            break;
        }
        else
        {
            line[tempCharIdx++] = (char)tempChar;
        }

    }
    return line;
}


void display_history()
{
    int i ;
    if( history_pointer == 0)
    {
        fprintf(stdout, "history is empty\n");
    }
    else
    {
        for(i =  1; i < history_pointer+1 ; i++)
        {
            fprintf(stdout, "%d %s\n",i,history[i-1]);
        }
    }

}
void load_history()
{
    if(open_file("history.txt") != empty_file)
    {
        char* line = malloc(BUFFER_MAX_LENGTH);
        int corrupted_history_file = 0;
        line = read_line_from_file();
        while(line[0] != '\0')
        {

            if(history_pointer == 10 && (!corrupted_history_file))
            {
                fprintf(stderr, "error: the max size of history ia 10.please don't play in the history file\n");
                corrupted_history_file = 1;
            }
            add_command_to_history(line);
            line =  read_line_from_file();
        }
    }
    //fprintf(stdout,"leave this fun\n");
}
void save_history()
{
    FILE *file_history = fopen("history.txt", "w");

    if (file_history == NULL)
    {
        fprintf(stderr, "error: could not open textfile for saving history : history.txt\n");
        exit(EXIT_FAILURE);
    }

    int i;
    for(i = 0; i<history_pointer; i++)
    {
        fprintf(file_history, "%s\n", history[i]);
    }
    fclose(file_history);
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(BUFFER_MAX_LENGTH);
    char *separator = "/";
    strcpy(result, s1);
    strcat(result,separator);
    strcat(result, s2);
    return result;
}
char* check_path(char* arguments[])
{
    if(access(arguments[0],F_OK) == 0)
    {
        return arguments[0];
    }
    int count ;
    for(count = 0 ; count < paths_length; count++)
    {
        char* path = concat(paths[count],arguments[0]);
        if(access(path,F_OK) == 0)
        {
            return path;
        }
    }
    return "not_found";
}

void create_child(char* path ,char*arguments[], int mode)
{
    int status;
    pid_t pid;
    pid = fork();
    if(pid < 0 )
    {
        fprintf(stderr,"error: forking is failed\n");
    }
    else if (pid == 0)   /* child process */
    {
        execv(path, arguments);

    }
    else if (pid > 0)   /* parent process */
    {
        if(mode != background)
        {
            wait(&status);
        }
    }
}

void execute_command(char* command)
{
    char* clone_command = malloc(BUFFER_MAX_LENGTH);
    add_command_to_history(strcpy(clone_command, command));
    char*arguments[BUFFER_MAX_LENGTH];
    int num_element = splite(command," \t",arguments);
    if(num_element == 0)
    {
        fprintf(stdout,"error: empty line\n");
        return;
    }
    else
    {
        int mode = check_mode(num_element,arguments);
        char* path = check_path(arguments);
        if(strcmp(path,"not_found") == 0)
        {
            fprintf(stderr,"error: this command is not valid\n");
        }
        else
        {
            create_child(path,arguments,mode);
        }

    }

}



int main(int argc, char* argv[])
{
    paths_length = splite(getenv("PATH"),":",paths);
    //reading from file
    int reading_file = 0;
    int should_run = 1;
    if(argc > 2 )
    {
        fprintf(stderr, "error: wrong number of argument in command\n");
        exit(EXIT_FAILURE);
    }

    load_history();
    if(argc == 2)
    {
        if(open_file(argv[1]) == empty_file)
        {
            fprintf(stderr, "error: File is empty.\n");
            should_run = 0;
        }
        reading_file = 1;
    }


    while(should_run)
    {
        char *command = malloc(BUFFER_MAX_LENGTH);

        if(reading_file)
        {
            command = read_line_from_file();
            if(command[0] == '\0' )
            {
                should_run = 0;
                save_history();
                continue;
            }
            printf("**************************************\n");
            fprintf(stdout, "%s\n",command);
        }
        else
        {
            printf("**************************************\n");
            printf("shell>>");
            command = read_from_user();
        }

        //check command if(over 80 char , exit ,history,!!,!no)
        char*arguments[BUFFER_MAX_LENGTH];
        char* clone = malloc(BUFFER_MAX_LENGTH);
        int num_element = splite(strcpy(clone,command)," \t",arguments);

        if((num_element == 1) && (strcmp(arguments[0], exit_command) == 0))

        {
            should_run = 0;
            save_history();
            continue;
        }
        else if(command_exceed_max_char(command))
        {
            add_command_to_history(command);
            continue;
        }
        else if(num_element == 1 && strcmp(arguments[0], history_name) == 0)
        {
            display_history();
        }

        else if(num_element == 1 && strcmp(arguments[0], execute_recent_commad) == 0)
        {
            if(history_pointer == 0)
            {
                fprintf(stderr, "error: NO Commands in History\n");
            }
            else
            {
                command = history[history_pointer-1];
                fprintf(stdout, "%s\n",command);
                if(command_exceed_max_char(command))
                {
                    add_command_to_history(command);
                    continue;
                }
                execute_command(command);
            }
        }
        else if(num_element == 1 && arguments[0][0] == '!')
        {
            int length = strlen(arguments[0]);
            int num = 0;
            if(length == 3 && arguments[0][1] == '1' && arguments[0][2] == '0')
            {
                num = 10;
            }
            else if( length == 2 && isdigit(arguments[0][1]))
            {
                num =  arguments[0][1] - '0';
            }

            if(num > history_pointer || num == 0)
            {
                fprintf(stderr, "error: NO such Commands in History\n");
                continue;
            }
            command = history[num-1];
            fprintf(stdout, "%s\n",command);
            if(command_exceed_max_char(command))
            {
                add_command_to_history(command);
                continue;
            }
            execute_command(command);
        }
        else
        {
            execute_command(command);
        }

    }

    return 0;
}
