#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_VECTOR_SIZE 3000
#define MAX_THREADS 9


typedef struct {
    double *vector1;
    double *vector2;
    double *result;
    int n; // Length of vectors
} ThreadData;

int parseAndLoadVector(char *line, double *vector, int maxVectorSize) {
    int count = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && count < maxVectorSize) {
        vector[count++] = atof(token);
        token = strtok(NULL, " \t\n");
    }
    return count;
}

void display_result(double* result, int n)
{
    for (int i = 0; i < n; i++) {
        printf("%.2f ", result[i]);
    }
}

void *vectorSum(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int i = 0; i < data->n; i++) {
        data->result[i] = data->vector1[i] + data->vector2[i];
    }
    pthread_exit(NULL);
}

void *calculateInnerProduct(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    double localResult = 0.0;
    for (int i = 0; i < data->n; i++) {
        localResult += data->vector1[i] * data->vector2[i];
    }
    *(data->result) = localResult;
    pthread_exit(NULL);
}

void *vectorDifference(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    for (int i = 0; i < data->n; i++) {
        data->result[i] = data->vector1[i] - data->vector2[i];
    }
    pthread_exit(NULL);
}


typedef struct VIinfo
{
    int num_lines;
    int num_words;
    int num_characters;
} myVI;


int max(int a, int b)
{
    if(a>b)
    return a;
    return b;
}

void execute_command(char* args[], int n)
{
    int execvp_code = execvp(args[0], args);
    if(execvp_code == -1)
    {
        perror("Execvp() failed");
        exit(EXIT_FAILURE);
    }
}

int count_words(char* line)
{
    int word_count = 0;
    char * token = strtok(line, " ");
    while( token != NULL ) {
        token = strtok(NULL, " ");
        word_count++;
    }
    return word_count;
}

myVI* my_vi_editor(char *File_Name)
{
    myVI* vi_info = (myVI*)malloc(sizeof(myVI));
    vi_info->num_characters = vi_info->num_lines = vi_info->num_words = 0;
    FILE* fptr = fopen(File_Name, "a+");
    if(fptr == NULL)
    {
        printf("File:'%s' Opening Error", File_Name);
        return vi_info;
    }
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    int line_chars[LINES];
    for(int i=0;i<LINES;i++)
    line_chars[i] = 0;
    char line[500];
    while(fgets(line, 500, fptr))
    {
        printw("%s\n", line);
        line_chars[vi_info->num_lines] = strlen(line);
        vi_info->num_characters += line_chars[vi_info->num_lines];
        vi_info->num_words = count_words(line);
        (vi_info->num_lines)++;
    }
    fclose(fptr);
    refresh();
    short current_char;
    int x_cursor = 0, y_cursor = 0;
    move(y_cursor, x_cursor);
    while((current_char = getch()) != 27) // Loop untill 'ESC' is pressed
    {
        switch (current_char)
        {
        case 19: // Ctrl + S [Save this content to file]
        {
            char str[1024];
            FILE* fptr2 = fopen(File_Name, "w");
            if(fptr2 == NULL)
            {
                endwin();
                printf("File:'%s' Saving Error", File_Name);
                return vi_info;
            }
            for (int i = 0; i < vi_info->num_lines; i++)
            {
                mvinnstr(i, 0, str, line_chars[i]);
                fprintf(fptr2, "%s\n", str);
            }
            fclose(fptr2);
        }
    break;
        
        case KEY_LEFT:
            if(x_cursor > 0)
            x_cursor--;
            break;
        
        case KEY_RIGHT:
            if(x_cursor < line_chars[y_cursor])
            x_cursor++;
            break;
        
        case KEY_UP:
            if(y_cursor > 0)
            y_cursor--;
            break;
        
        case KEY_DOWN:
            if(y_cursor < vi_info->num_lines)
            y_cursor++;
            break;
        
        case KEY_DC: // Delete Key
            if(y_cursor != line_chars[y_cursor])
            {
                line_chars[y_cursor] = max(line_chars[y_cursor]-1, 0);
                vi_info->num_characters = max(vi_info->num_characters-1, 0);
                if(mvinch(y_cursor, x_cursor) == ' ')
                vi_info->num_words--;
            }
            mvdelch(y_cursor, x_cursor);
            break;

        case 10: //Press Enter to get to next line
            y_cursor++;
            x_cursor = 0;
            vi_info->num_lines = max(vi_info->num_lines, y_cursor+1);
            break;
        
        case 24: // Ctrl+X key
            endwin();
            return vi_info;

        default:
            insch(current_char);
            vi_info->num_lines = max(vi_info->num_lines, 1);
            if(x_cursor==0 || mvinch(y_cursor, x_cursor-1) == ' ')
            vi_info->num_words++;
            x_cursor++;
            line_chars[y_cursor]++;
            vi_info->num_characters++;
            break;
        }
        move(y_cursor, x_cursor);
    }
    endwin();
    return vi_info;
}

int main()
{
    using_history(); //Intializing History for readline utility
    while (1) // New Command Prompt
    {
        char piped_command[1024] = "";
        short isAmps = 0;

        /*Reading multilines*/
        strcat(piped_command, readline("\nShell>"));
        while (piped_command[strlen(piped_command)-1] == '\\')
        {
            piped_command[strlen(piped_command)-1] = '\0';
            strcat(piped_command, readline("\n>"));
        }
        
        if(!strlen(piped_command))
        continue;
        add_history(piped_command);

        /*Piped Tokenized*/
        char* piped_comm_args[100];
        int piped_arg_count = 0;
        char * Ptoken = strtok(piped_command, "|");
        while( Ptoken != NULL ) {
            piped_comm_args[piped_arg_count] = Ptoken;
            Ptoken = strtok(NULL, "|");
            piped_arg_count++;
        }

        int file_des[2];
        int inp_init = 0;

        for(int k=0;k<piped_arg_count;k++)
        {
            /*Space Tokenized*/
            char command[1024] = "";
            strcat(command, piped_comm_args[k]);
            if(!strlen(command))
            continue;
            char* comm_args[100];
            int arg_count = 0;
            char * token = strtok(command, " ");
            while( token != NULL ) {
                comm_args[arg_count] = token;
                token = strtok(NULL, " ");
                arg_count++;
            }

            /*Checking last token for '&'*/
            if(comm_args[arg_count-1][strlen(comm_args[arg_count-1])-1] == '&')
            {
                isAmps = 1;
                if(strlen(comm_args[arg_count-1]) == 1)
                arg_count--;
                else
                comm_args[arg_count-1][strlen(comm_args[arg_count-1])-1] = '\0';
            }

            /*Making Last token as NULL*/
            comm_args[arg_count] = NULL;
            arg_count++;

            if(k < piped_arg_count-1)
            {
                int pipe_status_code = pipe(file_des);
                if(pipe_status_code == -1)
                {
                    perror("Pipe Error");
                    exit(EXIT_FAILURE);
                }
            }

            // printf("Command Given: %s\n", command);
            if(strcmp(comm_args[0], "exit") == 0)
            exit(0);
            if(strcmp(comm_args[0], "help") == 0)
            {
                printf("\nCommands Availaible:-\n");
                printf("\t1. pwd\n");
                printf("\t2. cd <directory_name>\n");
                printf("\t3. mkdir <directory_name>\n");
                printf("\t4. ls <flag>\n");
                printf("\t5. exit\n");
                printf("\t6. help\n");
                printf("\t7. vi <file_name>\n");
                printf("\t8. addvec <file_1> <file_2> -<no_thread>\n");
                printf("\t9. subvec <file_1> <file_2> -<no_thread>\n");
                printf("\t10. dotprod <file_1> <file_2> -<no_thread>\n");
                printf("\t11. Execute any other command\n");
                continue;
            }
            if(strcmp(comm_args[0], "cd") == 0)
            {
                char s1[100], s2[100], temp[1024] = "";
                getcwd(s1,100);
                strcat(temp, comm_args[1]);
                for(int i=2;i<arg_count-1;i++)
                {
                    strcat(temp, " ");
                    strcat(temp, comm_args[i]);
                }
                int cd_status_code = chdir(temp);
                if(cd_status_code == 0)
                {
                    printf("\nPresent Working Directory changed:-\nFrom: %s\t", s1);
                    printf("To: %s\n", getcwd(s2,100));
                }
                else
                {
                    perror("cd command failed");
                }
                continue;
            }

            if(strcmp(comm_args[0], "vi") == 0)
            {
                char fname[1024] = "";
                strcat(fname, comm_args[1]);
                for(int i=2;i<arg_count-1;i++)
                {
                    strcat(fname, " ");
                    strcat(fname, comm_args[i]);
                }
                myVI* vi = my_vi_editor(fname);
                printf("\nNumber of lines in Editor: %d\n", vi->num_lines);
                printf("Number of words in Editor: %d\n", vi->num_words);
                printf("Number of characters in Editor: %d\n", vi->num_characters);
                continue;
            }

            if (strcmp(comm_args[0], "addvec") == 0 || strcmp(comm_args[0], "subvec") == 0 || strcmp(comm_args[0], "dotprod") == 0) {
            char *inputFile1 = comm_args[1];
            char *inputFile2 = comm_args[2];

            // Read vectors from input files
            double vector1[MAX_VECTOR_SIZE], vector2[MAX_VECTOR_SIZE], result[MAX_VECTOR_SIZE];
            int n;

            FILE *file1 = fopen(inputFile1, "r");
            FILE *file2 = fopen(inputFile2, "r");

            if (file1 == NULL || file2 == NULL) {
                perror("File open failed");
                continue;
            }

            // Read vector 1 from file1
            char line[1024];
            if (fgets(line, sizeof(line), file1) == NULL) {
                printf("Error reading vector 1 from file\n");
                continue;
            }
            n = parseAndLoadVector(line, vector1, MAX_VECTOR_SIZE);

            // Read vector 2 from file2
            if (fgets(line, sizeof(line), file2) == NULL) {
                printf("Error reading vector 2 from file\n");
                continue;
            }
            if (parseAndLoadVector(line, vector2, MAX_VECTOR_SIZE) != n) {
                printf("Vector sizes do not match\n");
                continue;
            }

            int numThreads = 3; // Default number of threads
            if (comm_args[3]) {
                numThreads = atoi(comm_args[3]+1);
            }

            if (numThreads <= 0 || numThreads > MAX_THREADS) {
                printf("Invalid number of threads\n");
                continue;
            }

            pthread_t threads[MAX_THREADS];
            ThreadData threadData[MAX_THREADS];
            int chunkSize = n / numThreads;
            int remainder = n % numThreads;

            int currentIndex = 0;
            for (int i = 0; i < numThreads; i++) {
                threadData[i].n = chunkSize + (i < remainder ? 1 : 0);
                threadData[i].vector1 = &vector1[currentIndex];
                threadData[i].vector2 = &vector2[currentIndex];
                threadData[i].result = &result[currentIndex];

                if (strcmp(comm_args[0], "addvec") == 0) {
                    pthread_create(&threads[i], NULL, vectorSum, &threadData[i]);
                } else if (strcmp(comm_args[0], "subvec") == 0) {
                    pthread_create(&threads[i], NULL, vectorDifference, &threadData[i]);
                } else if (strcmp(comm_args[0], "dotprod") == 0) {
                    threadData[i].result = (double *)malloc(sizeof(double)); // Allocate memory
                    pthread_create(&threads[i], NULL, calculateInnerProduct, &threadData[i]);
                }

                currentIndex += threadData[i].n;
            }

            // Wait for all threads to complete
            for (int i = 0; i < numThreads; i++) {
                pthread_join(threads[i], NULL);
            }

            // Output the result to the console
            if (strcmp(comm_args[0], "addvec") == 0) {
                printf("Resultant Vector (Addition):\n");
                display_result(result, n);
            } else if (strcmp(comm_args[0], "subvec") == 0) {
                printf("Resultant Vector (Subtraction):\n");
                display_result(result, n);
            } else if (strcmp(comm_args[0], "dotprod") == 0) {
                double dotProduct = 0.0;
                for (int i = 0; i < numThreads; i++) {
                    dotProduct += *(threadData[i].result);
                    free(threadData[i].result); // Free allocated memory
                }
                printf("Dot Product: %.2f\n", dotProduct);
            }

            printf("\n");

            fclose(file1);
            fclose(file2);
            continue;
        }
            
            pid_t comm_child_pid = fork();
            if(comm_child_pid == -1)
            {
                perror("Fork() failed to create Child Process...\n");
                exit(1);
            }
            else if(comm_child_pid == 0)
            {
                printf("\nInside Child Process with PID:%d\n", getpid());
                printf("Executing Command: %s\n\n", comm_args[0]);
                if(k > 0)
                {
                    dup2(inp_init, STDIN_FILENO);
                    close(inp_init);
                }
                if(k < piped_arg_count-1)
                {
                    dup2(file_des[1], STDOUT_FILENO);
                    close(file_des[0]);
                    close(file_des[1]);
                }
                execute_command(comm_args, arg_count);
                return 0;
            }
            else
            {
                if(k > 0)
                {
                    close(inp_init);
                }
                if(k < piped_arg_count-1)
                {
                    close(file_des[1]);
                    inp_init = file_des[0];
                }
                if(!isAmps)
                {
                    int CStatus;
                    wait(&CStatus);

                    if (WIFEXITED(CStatus)) {
                        printf("\nChild Process exited with status: %d\n", WEXITSTATUS(CStatus));
                    }
                }
            }
        }
    }
    return 0;
}