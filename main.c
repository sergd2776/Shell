#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

typedef struct word_s{
    char* word_data;
    int Size_word;
    int Memory_word;
} word_s;

typedef struct command_s{
    struct word_s* command_data;
    struct word_s append_filename;
    struct word_s rewrite_filename;
    struct word_s read_filename;
    int Size_command;
    int Memory_command;
} command_s;

typedef struct pipeline_s{
    struct command_s* pipeline_data;
    int Size_pipeline;
    int Memory_pipeline;
} pipeline_s;

typedef struct list_s{
    struct pipeline_s* list_data;
    int Size_list;
    int Memory_list;
} list_s;

typedef struct Pipe{
    int fd[2];
} Pipe;

int expand_word(int N, word_s** a){
    word_s b;
    b.word_data = malloc(2*N*sizeof(char));
    for (int i = 0; i < N; i++){
        b.word_data[i] = (*a)->word_data[i];
    }
    free((*a)->word_data);
    **a = b;
    return 2*N;
}

int expand_command(int N, command_s** a){
    command_s b;
    b.command_data = malloc(2*N*sizeof(word_s));
    for (int i = 0; i < N; i++){
        b.command_data[i] = (*a)->command_data[i];
    }
    b.append_filename.word_data = malloc((*a)->append_filename.Size_word * sizeof(char));
    b.append_filename = (*a)->append_filename;
    b.read_filename.word_data = malloc((*a)->read_filename.Size_word * sizeof(char));
    b.read_filename = (*a)->read_filename;
    b.rewrite_filename.word_data = malloc((*a)->rewrite_filename.Size_word * sizeof(char));
    b.rewrite_filename = (*a)->rewrite_filename;
    free((*a)->command_data);
    free((*a)->rewrite_filename.word_data);
    free((*a)->read_filename.word_data);
    free((*a)->append_filename.word_data);
    *(*a) = b;
    return 2*N;
}

int expand_pipeline(int N, pipeline_s** a){
    pipeline_s b;
    b.pipeline_data = malloc(2*N*sizeof(command_s));
    for (int i = 0; i < N; i++){
        b.pipeline_data[i] = (*a)->pipeline_data[i];
    }
    free((*a)->pipeline_data);
    **a = b;
    return 2*N;
}

int SetFileFlag(int c1, int c2, int c3){
    if (c1){
        return 1;
    }
    else if (c2){
        return 2;
    }
    else if (c3){
        return 3;
    }
    else {
        return 0;
    }
}

int expand_list(int N, list_s** a){
    list_s b;
    b.list_data = malloc(2*N*sizeof(pipeline_s));
    for (int i = 0; i < N; i++){
        b.list_data[i] = (*a)->list_data[i];
    }
    free((*a)->list_data);
    **a = b;
    return 2*N;
}


word_s* Enter_word(int size, word_s* a, int* c, int* flag_file){
    int i = 0;
    unsigned int flag_k = 0;
    int count_k_double = 0;
    int count_k_single = 0;
    int c_special_slash = 0;
    int flag_append = 0;
    int flag_rewrite = 0;
    int flag_read = 0;
    int d = getchar();
    int flag_cont_word;
    a->word_data = malloc(size*sizeof(char));
    flag_cont_word = 1;
    while (d == ' '){
        d = getchar();
    }
    if ((*c == ' ') && ((d == '\n') || (d == '|') || (d == '&'))){
        *c = d;
        a->word_data = NULL;
        a->Size_word = 0;
        return a;
    }
    if (*c == '>'){
        if (d == '>'){
            flag_append = 1;
            d = getchar();
        }
        else {
            flag_rewrite = 1;
        }
    }
    else if (*c == '<') {
        flag_read = 1;
    }
    else if (d == '>'){
        *c = d;
        d = getchar();
        if (d == '>'){
            *c = d;
            d = getchar();
            flag_append = 1;
        }
        else {
            flag_rewrite = 1;
        }
    }
    else if (d == '<') {
        *c = d;
        d = getchar();
        flag_read = 1;
    }
    while (d == ' '){
        d = getchar();
    }
    while ((*c == '|' ) && (d == '\n')){
        printf("> ");
        d = getchar();
    }
    *c = d;
    while (flag_cont_word != 0) {
        flag_k = count_k_double | count_k_single;
        if ((c_special_slash != 0) && (*c == '\n')){
            c_special_slash = 0;
            printf("> ");
            *c = getchar();
            continue;
        }
        else if ((c_special_slash != 0) && (count_k_double != 0)) {
            if ((*c != '\"') && (*c != '\\')) {
                a->word_data[i] = (char) '\\';
                c_special_slash = 0;
                i++;
                if (i == size) {
                    size = expand_word(size, &a);
                }
                a->word_data[i] = (char) *c;
            } else {
                a->word_data[i] = (char) *c;
                c_special_slash = 0;
            }
        }
        else if (c_special_slash != 0){
            a->word_data[i] = (char) *c;
            c_special_slash = 0;
        }
        else {
            if ((*c == '\\') && (count_k_single == 0)) {
                c_special_slash = 1;
                *c = getchar();
                continue;
            }
            if (((*c == '\n') || (*c == ' ') || (*c == '|') || (*c == '&') || (*c == '>') || (*c == '<')) && (flag_k == 0)) {
                flag_cont_word = 0;
                continue;
            }
            if ((*c == '\"') && (count_k_single == 0)) {
                count_k_double ^= 1;
                *c = getchar();
                continue;
            }
            if ((*c == '\'') && (count_k_double == 0)) {
                count_k_single ^= 1;
                *c = getchar();
                continue;
            }
            if (((count_k_double != 0) || (count_k_single != 0)) && (*c == '\n')){
                printf("> ");
            }
            a->word_data[i] = (char) *c;
        }
        i++;
        if (i == size) {
            size = expand_word(size, &a);
        }
        *c = getchar();
    }
    *flag_file = SetFileFlag(flag_rewrite, flag_append, flag_read);
    a->word_data[i] = '\0';
    a->Size_word = i;
    a->Memory_word = size;
    return a;
}

command_s* Enter_command(int size, command_s* a, int* c, int* flag_pipeline, int* flag_syntax_error){
    int i = 0;
    int flag_file = 0;
    a->command_data = malloc(size*sizeof(word_s));
    word_s buf = *Enter_word(1, &buf, c, &flag_file);
    a->read_filename.word_data = NULL;
    a->rewrite_filename.word_data = NULL;
    a->append_filename.word_data = NULL;
    if (!flag_file) {
        a->command_data[i] = buf;
        i++;
    }
    else if (flag_file == 1){
        //a->rewrite_filename.word_data = malloc(buf.Size_word*sizeof(char));
        a->rewrite_filename = buf;
        if (a->rewrite_filename.word_data[0] == '\0'){
            *flag_syntax_error = 1;
        }
    }
    else if (flag_file == 2){
        //a->append_filename.word_data = malloc(buf.Size_word*sizeof(char));
        a->append_filename = buf;
        if (a->append_filename.word_data[0] == '\0'){
            *flag_syntax_error = 1;
        }
    }
    else {
        //a->read_filename.word_data = malloc(buf.Size_word*sizeof(char));
        a->read_filename = buf;
        if (a->read_filename.word_data[0] == '\0'){
            *flag_syntax_error = 1;
        }
    }
    while ((*c != '\n') && (*c != '&') && (*c != '|')) {
        if (i == size) {
            size = expand_command(size, &a);
        }
        buf = *Enter_word(1, &buf, c, &flag_file);
        if (!flag_file) {
            a->command_data[i] = buf;
            i++;
        }
        else if (flag_file == 1){
            //a->rewrite_filename.word_data = malloc(buf.Size_word*sizeof(char));
            a->rewrite_filename = buf;
            if (a->rewrite_filename.word_data[0] == '\0'){
                *flag_syntax_error = 1;
            }
        }
        else if (flag_file == 2){
            //a->append_filename.word_data = malloc(buf.Size_word*sizeof(char));
            a->append_filename = buf;
            if (a->append_filename.word_data[0] == '\0'){
                *flag_syntax_error = 1;
            }
        }
        else {
            //a->read_filename.word_data = malloc(buf.Size_word*sizeof(char));
            a->read_filename = buf;
            if (a->read_filename.word_data[0] == '\0'){
                *flag_syntax_error = 1;
            }
        }
    }
    if (a->command_data[i-1].word_data != NULL) {
        if (i == size) {
            size = expand_command(size, &a);
        }
        a->command_data[i].word_data = NULL;
    }
    a->Size_command = i;
    a->Memory_command = size;
    if (*c == '|'){
        *flag_pipeline = 1;
    }
    else{
        *flag_pipeline = 0;
    }
    return a;
}

pipeline_s* Enter_pipeline(int size, pipeline_s* a, int* last_symb_pipeline, int* flag_list, int* flag_syntax_error){
    int i = 0;
    int c = '\0';
    int flag_pipeline = 0;
    a->pipeline_data = malloc(size*sizeof(command_s));
    a->pipeline_data[i] = *Enter_command(1, &a->pipeline_data[i], &c, &flag_pipeline, flag_syntax_error);
    while ((c != '\n') && (c != '&') && (flag_pipeline != 0)){
        if (a->pipeline_data[i].command_data[0].word_data[0] == '\0'){
            *flag_syntax_error = 1;
        }
        i++;
        if (i == size){
            size = expand_pipeline(size, &a);
        }
        a->pipeline_data[i] = *Enter_command(1, &a->pipeline_data[i], &c, &flag_pipeline, flag_syntax_error);
    }
    a->Size_pipeline = i + 1;
    a->Memory_pipeline = size;
    *last_symb_pipeline = c;
    if (*last_symb_pipeline == '&'){
        *flag_list = 1;
    }
    else{
        *flag_list = 0;
    }
    return a;
}

list_s* Enter_list(int size, list_s* a, int* last_symb_list, int* flag_syntax_error){
    int i = 0;
    int c = '\0';
    int flag_list = 0;
    *flag_syntax_error = 0;
    a->list_data = malloc(size*sizeof(pipeline_s));
    a->list_data[i] = *Enter_pipeline(1, &a->list_data[i], &c, &flag_list, flag_syntax_error);
    while ((c != '\n') && (flag_list != 0)){
        if (a->list_data[i].pipeline_data[0].command_data[0].word_data[0] == '\0'){
            *flag_syntax_error = 1;
        }
        i++;
        if (i == size){
            size = expand_list(size, &a);
        }
        a->list_data[i] = *Enter_pipeline(1, &a->list_data[i], &c, &flag_list, flag_syntax_error);
    }
    *last_symb_list = c;
    a->Size_list = i + 1;
    a->Memory_list = size;
    return a;
}

int Check_cd(const char* b){
    if ((b[0] == 'c') && (b[1] == 'd') && (b[2] == '\0')){
        return 1;
    }
    else{
        return 0;
    }
}

int Check_exit(const char* b){
    if ((b[0] == 'e') && (b[1] == 'x') && (b[2] == 'i') && (b[3] == 't') && (b[4] == '\0')){
        return 1;
    }
    else{
        return 0;
    }
}

int Proceed_Command(command_s* a){
    char** b = malloc(sizeof(char*)*a->Size_command);
    int error_cd;
    for (int i = 0; i < a->Size_command; i++){
        b[i] = a->command_data[i].word_data;
    }
    b[a->Size_command] = NULL;
    if (Check_exit(b[0]) != 0){
        return 0;
    }
    if (Check_cd(b[0]) != 0) {
        if (b[2] != NULL){
            printf("bash: cd: слишком много аргументов\n");
        }
        else{
            error_cd = chdir(b[1]);
            if (error_cd == -1){
                printf("bash: cd: %s: Нет такого файла или каталога\n", b[1]);
            }
        }
    }

    else{
        pid_t child_proc = fork();
        if (child_proc < 0){
            printf("Process start error 0x1\n");
        }
        else if (child_proc == 0){
            execvp(b[0], b);
            printf("Execvp work error 0x2\n");
            exit(EXIT_FAILURE);
        }
        else{
            wait(NULL);
        }
    }
    free(*b);
    return 1;
}

void Close_all_holes(Pipe* a, int size){
    for (int i = 0; i < size; i++){
        close((a)[i].fd[0]);
        close((a)[i].fd[1]);
    }
}

void Clear_Memory(list_s* a){
    for (int i1 = 0; i1 < a->Size_list; i1++){
        for (int i2 = 0; i2 < a->list_data[i1].Size_pipeline; i2++) {
            for (int i3 = 0; i3 < a->list_data[i1].pipeline_data[i2].Size_command; i3++) {
                free(a->list_data[i1].pipeline_data[i2].command_data[i3].word_data);
            }
            free(a->list_data[i1].pipeline_data[i2].command_data);
            free(a->list_data[i1].pipeline_data[i2].rewrite_filename.word_data);
            free(a->list_data[i1].pipeline_data[i2].append_filename.word_data);
            free(a->list_data[i1].pipeline_data[i2].read_filename.word_data);
        }
        free(a->list_data[i1].pipeline_data);
    }
    free(a->list_data);
}

int Proceed_Pipe(pipeline_s* a, int flag_background){
    int error_cd = 0;
    int error_pipes = 0;
    int i = 0;
    int fd_rewrite;
    int fd_append;
    int fd_read;
    int flag_read;
    int flag_write;
    Pipe* pipes = NULL;
    int* PIDs = malloc((a->Size_pipeline) * sizeof(pid_t));
    if (a->Size_pipeline > 1) {
        pipes = malloc((a->Size_pipeline - 1) * sizeof(Pipe));
        while ((i < (a->Size_pipeline - 1)) && (error_pipes == 0)) {
            error_pipes = pipe(pipes[i].fd);
            i++;
        }
    }
    for (i = 0; i < a->Size_pipeline; i++){
        flag_read = 0;
        flag_write = 0;
        char** b = malloc(sizeof(char*) * (a->pipeline_data[i].Size_command + 1));
        for (int j = 0; j <= a->pipeline_data[i].Size_command; j++){
            b[j] = a->pipeline_data[i].command_data[j].word_data;
        }
        //b[a->pipeline_data[i].Size_command] = NULL; Оставлю это здесь на память :) Величайшая ошибка всего человечества!!!!!!!!!!!!!!
        if (Check_exit(b[0]) != 0){
            if ((a->Size_pipeline == 1) && (flag_background == 0)) {
                free(PIDs);
                free(b);
                return 0;
            }
        }
        else if (Check_cd(b[0]) != 0){
            if ((a->Size_pipeline == 1) && (flag_background == 0)){
                if (b[2] != NULL) {
                    printf("bash: cd: слишком много аргументов\n");
                } else {
                    error_cd = chdir(b[1]);
                    if (error_cd == -1) {
                        printf("bash: cd: %s: Нет такого файла или каталога\n", b[1]);
                    }
                }
            }
        }
        else {
            pid_t child_proc = fork();
            if (child_proc < 0) {
                printf("Process start error. For further information check logfile or contact technical support\n");
            } else if (child_proc == 0) {
                if (a->pipeline_data[i].rewrite_filename.Size_word != 0){
                    fd_rewrite = open(a->pipeline_data[i].rewrite_filename.word_data, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    dup2(fd_rewrite, 1);
                    close(fd_rewrite);
                    flag_write = 1;
                }
                if (a->pipeline_data[i].append_filename.Size_word != 0){
                    fd_append = open(a->pipeline_data[i].append_filename.word_data, O_WRONLY | O_APPEND | O_CREAT, 0666);
                    dup2(fd_append, 1);
                    close(fd_append);
                    flag_write = 1;
                }
                if (a->pipeline_data[i].read_filename.Size_word != 0){
                    fd_read = open(a->pipeline_data[i].read_filename.word_data, O_RDONLY);
                    dup2(fd_read, 0);
                    close(fd_read);
                    flag_read = 1;
                }
                if ((i != 0) && (!flag_read)){
                    dup2(pipes[i-1].fd[0],0);
                }
                if ((i != a->Size_pipeline - 1) && (!flag_write)) {
                    dup2(pipes[i].fd[1],1);
                }
                if (a->Size_pipeline > 1) {
                    Close_all_holes(pipes, a->Size_pipeline - 1);
                }

                execvp(b[0], b);
                if ((b[0][0] == '.') && (b[0][1] == '/')) {
                    printf("bash: %s: Нет такого файла или каталога\n", b[0]);
                }
                else {
                    printf("%s: команда не найдена\n", b[0]);
                }
                exit(EXIT_FAILURE);
            } else {
                PIDs[i] = child_proc;
            }
        }
        free(b);
    }
    if (a->Size_pipeline > 1) {
        Close_all_holes(pipes, a->Size_pipeline - 1);
        free(pipes);
    }
    if (flag_background == 0) {
        for (i = 0; i < a->Size_pipeline; i++) {
            waitpid(PIDs[i], NULL, 0);
        }
    }
    free(PIDs);
    return 1;
}

int Proceed_List(list_s* a){
    int flag_background = 1;
    int Working_process = 1;
    for (int i = 0; i < a->Size_list; i++){
        if (i == (a->Size_list - 1)){
            flag_background = 0;
        }
        if (a->list_data[i].pipeline_data[0].command_data[0].word_data[0] == '\0'){
            continue;
        }
        Working_process = Proceed_Pipe(&a->list_data[i], flag_background);
    }
    return Working_process;
}

int main() {
    int Working_process = 1;
    int c;
    int flag_syntax_error = 0;
    char* buf_getcwd;
    list_s list_p;
    while (Working_process != 0){
        //printf("[]~$");
        printf("[%s]~$ ", buf_getcwd = getcwd(NULL, 128));
        free(buf_getcwd);
        list_p = *Enter_list(1, &list_p, &c, &flag_syntax_error);

        if (flag_syntax_error == 0) {
            Working_process = Proceed_List(&list_p);
        }
        else{
            printf("bash: синтаксическая ошибка\n");
        }
        Clear_Memory(&list_p);
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }
}
