#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <signal.h>

//#define struct pid *pid_struct;

char check_symbols_one[10] = {'.', ' ', '/', '_', '$', '\0', '"', '#', '-', ':'};
char check_symbols_two[9] = {'\n', '>', '\t', '<', '|', '&', ';', '(', ')'};
char spec_symbols[7] = {'>', '|', '&', '<', ')', '(', ';'};
char double_spec_symbols[3] = {'>', '|', '&'};

enum 
{
    SIZE = 16,
    N = 5
};

struct List {

    char ** lst;
    int sizelist;
    int curlist;
} list;

struct Buf {

    char * buf; /* string */
    int sizebuf; /* mas length of words */
    int curbuf; /* current lenght of words */
} buff;

struct Help {

    char fl;
    char c; /* use for entered symbols */
    char cc; /*for errors*/
    int ignore_flag;
    int add_word_flag;
} help;

int null_flag = 0;

typedef enum {
    Start,
    Word,
    Greater,
    Greater2,
    Wrong,
    Ignore,
    Quote,
    Error
  } vertex;

vertex V;

typedef enum {
    Conv,
    In_File,
    Out_File,
    Pipe,
    Subshell_start,
    Subshell_end,
    Backgrnd,
    Type
} vertex2;

vertex2 V2;

void clearlist() 
{
  /* clear list */

  int i;
  list.sizelist = 0;
  list.curlist = 0;
  if (list.lst == NULL) return;
  for (i = 0; list.lst[i] != NULL; i++)
    free(list.lst[i]);
  free(list.lst);
  list.lst = NULL; /* empty list when we press "enter" */
}

void clearbuf() 
{

  buff.buf = NULL;
  buff.sizebuf = 0;
  buff.curbuf = 0;
  //for (int i = 0; i < (int)buff.sizebuf; i++)
    //free( & buff.buf[i]);
  free(buff.buf);
  
  //buff.buf = NULL;
}

void null_list() 
{
  /* use for create the list */

  list.sizelist = 0;
  list.curlist = 0;
  list.lst = NULL;
}

void termlist() 
{
  /* if our memory is full, we create new space for next words(special or not) */

  if (list.lst == NULL) return;
  if (list.curlist > list.sizelist - 1)
    list.lst = realloc(list.lst, (list.sizelist + 1) * sizeof( * list.lst));

  list.lst = realloc(list.lst, (list.sizelist = (list.curlist + 1)) * sizeof( * list.lst)); /* if empty memory, then delete it (fill to number of words) */
  list.lst[list.curlist] = NULL;
}

void nullbuf() 
{
  /* use for empty words in start of line */

  buff.buf = NULL;
  buff.sizebuf = 0;
  buff.curbuf = 0;
}

void addsym() 
{
  /* add symbol into our word */

  if (buff.curbuf > buff.sizebuf - 1) /* if lenght of our words more. then maxsize, create new space */
    buff.buf = realloc(buff.buf, buff.sizebuf += SIZE);
  buff.buf[buff.curbuf++] = help.c;
}

void addword() 
{
  /* add new word into our list with create new list node */

  if (buff.curbuf > buff.sizebuf - 1)
    buff.buf = realloc(buff.buf, buff.sizebuf += 1);
  buff.buf[buff.curbuf++] = '\0';
  buff.buf = realloc(buff.buf, buff.sizebuf = buff.curbuf);
  if (list.curlist > list.sizelist - 1)
    list.lst = realloc(list.lst, (list.sizelist += SIZE) * sizeof( * list.lst));
  list.lst[list.curlist++] = buff.buf;
}

void printlist() 
{
  /* primitive function for print out list of words */

  int i;
  if (list.lst == NULL) return;
  for (i = 0; i < (int)list.sizelist - 1; i++)
    if (list.lst[i] != NULL)
      printf("%s\n", list.lst[i]);  
}



int in(char c, int casse)
{
  int i, flag = 1;
  if (casse == 2)
    for (i = 0; flag && i < 9; i++) 
        flag = (c != check_symbols_two[i]);
  else if (casse == 1)
    for (i = 0; flag && i < 10; i++) 
        flag = (c != check_symbols_one[i]);
  else if (casse == 3)
    for (i = 0; flag && i < 7; i++)
        flag = (c != spec_symbols[i]);
  else if (casse == 4)
    for (i = 0; flag && i < 3; i++)
        flag = (c != double_spec_symbols[i]);
  return (flag) ? 0 : 1;
}

int check_before(char c) 
{
  if (   in(c, 1) || in(c, 2) ||
     ( (c >= 'a') && (c <= 'z') ) ||
     ( (c >= 'A') && (c <= 'Z') ) ||
     ( (c >= '0') && (c <= '9') ) )
      return 1;
      else return 0;
}

char check() 
{

  help.c = getchar();
  if (help.c == '\\') {
    help.c = getchar();
    if (in(help.c, 4) || help.c == '#' || help.c == '\\' || help.c == '"') {
      addsym();
      help.c = getchar();
    } else 
      return help.c;  
  }
  if (check_before(help.c))  {
    /*if normal symbol*/
  } else {
    help.cc = help.c; /* remember the wrong symbol*/
    V = Wrong;//help.c = '^';
  }
  if (help.c == '#') V = Ignore;
  else if (help.c == '"') V = Quote;
  else if (help.c == ' ') V = Start;
  return help.c;
}

void from_un_to_str (unsigned un, char *str)
{
  int cnt = 1;
  unsigned help_un = un; 
  while (help_un /= 10)
    ++cnt;
  for (int i = 0; i < cnt; ++i) {
    str[cnt - i - 1] = (un % 10) + '0';
    un /= 10;
  }
  //str[cnt+1] = '\0';
}

void search_and_change()
{
  int i = 0;
  if (list.lst == NULL) return;
  //for (i = 0; i < (int)list.sizelist - 1; i++)
  while (list.lst[i] != NULL) {  
    
      if (!strcmp(list.lst[i], "$HOME")) {
        list.lst[i] = (char *)realloc(list.lst[i], PATH_MAX);
        strcpy(list.lst[i], getenv("HOME"));
        //printf("IM HERE\n");
      }
      else if (!strcmp(list.lst[i], "$SHELL")) {
        list.lst[i] = (char *)realloc(list.lst[i], PATH_MAX);
        strcpy(list.lst[i], getenv("SHELL"));
      }
      else if (!strcmp(list.lst[i], "$USER")) {
        list.lst[i] = (char *)realloc(list.lst[i], PATH_MAX);
        strcpy(list.lst[i], getlogin());
      }
      else if (!strcmp(list.lst[i], "$EUID")) {
        char euid[PATH_MAX];
        list.lst[i] = (char *)realloc(list.lst[i], PATH_MAX);
        from_un_to_str(geteuid(), euid);
        strcpy(list.lst[i], euid);
      }
      //fprintf(stderr, "HAIL == %s\n", list.lst[i]);
    
  ++i;
  }    
}

void task3() 
{
  

  V = Start;
  help.ignore_flag = 0;
  help.add_word_flag = 1;
  
  help.c = check();
  null_list();
  nullbuf();
  while (!feof(stdin)) switch (V) {
       

  case Start:
    if (help.c == ' ' || help.c == '\t') {
      
      help.ignore_flag = 1;
      if (help.add_word_flag)
        addword();
      help.add_word_flag = 1;
      nullbuf();
      V = Start;
      //help.c = check();
      while ((help.c = check()) == ' ') ;
      if (help.c == '^') {
        //help.ignore_flag = 1;
        V = Wrong;
      }
      else if (help.c == '#') {
        V = Ignore;
        help.ignore_flag = 0;
      }
      else if (help.c == '"') {
        V = Quote;
        help.ignore_flag = 0;
      }
    } else if (help.c == '\n') {

      if (list.lst == NULL) {
        null_flag = 1;
        return;
      }
      help.ignore_flag = 1;
      
      termlist();
      search_and_change();
      //printf("\n-----------------");
      //printf("\nEntered list: \n");
      //printf("-----------------\n");
      //printlist();
      return;
      //printf("lenght == %d\n", sum_len_word());
      //printf("\n\n==> ");
      //clearlist();
      //null_list();
      //nullbuf();

      V = Start;
      help.c = check();
    } 
    
    else {
      help.ignore_flag = 1;
      nullbuf();
      addsym();
      V = (in(help.c, 3)) ? Greater : Word;
      if (in(help.c, 4)) help.fl = help.c;
      else if (help.c == '!') { 
        help.cc = help.c; 
        V = Wrong;
        break;
      } else help.fl = '!';
      help.c = check();
    }
    break;

  case Word:
    if (!in(help.c, 2)) {
      addsym();
      help.c = check();
      
    } else {

      V = Start;
      addword();
      help.ignore_flag = 0;
    }
    break;

  case Greater:
    if (help.c == help.fl) {

      addsym();
      help.ignore_flag = 1;
      V = Greater2;
      help.c = check();
      
      
    } else {

      if (help.c == '^')
        V = Wrong;
      else if (help.c == '#')
        V = Ignore;
      else if (help.c == '"')
        V = Quote;
      else 
        V = Start;
      addword();
      help.ignore_flag = 0;
    }
    break;

  case Greater2:
    V = Start;
    help.fl = 0;
    addword();
    help.ignore_flag = 0;
    break;

  case Ignore:
  //addword();
    if (help.ignore_flag) addword();
    while (((help.c = getchar()) != '\n') && !feof(stdin));
    V = Start;
    break;

  case Quote:
  
    if (help.ignore_flag) {
       addword();
       
    }
    nullbuf();
    if ((help.c = getchar()) == '"') {
      V = Start;
      help.c = check();
      break;
    } else if (help.c == '\n') {
      V = Error;
      break;
    } else 
      addsym();
    while ((help.c = getchar()) != '"') {
        if (help.c == '\n') {
          V = Error;
          break;
        } else addsym();
    }
        
    if (V == Error) break;
    addword();
    nullbuf();
    if (help.c == '\n') V = Start;
    else {
      V = Start;  
      help.c = check();
      if (help.c == ' ') help.add_word_flag = 0;
    }
    break;

  case Wrong:
    help.c = getchar();
    while ((help.c != '\n') && !feof(stdin)) {

      help.c = getchar();
      if (help.c == '\\') help.c = getchar(); 
    }
    termlist();
    printf("\nWrong symbol: %c\n", help.cc);
    printf("\nuser@this-shell task5 %% ");
    clearlist();
    nullbuf();
    help.c = check();
    V = Start;
    break;

  case Error:
  
    printf("\nERROR!!!\n");
    termlist();
    printf("\nuser@this-shell task5 %% ");
    clearlist();
    nullbuf();
    help.c = check();
    V = Start;
  }
  
  if (feof(stdin)) {
    clearlist();
    printf("\nGoodbye! \n");
    exit(0);
  }
}



typedef struct Cmd_inf *cmd_inf;

typedef struct Cmd_inf {
    char **argv; // список аргумнтов 
    char *in_file; // имя входного файла
    char *out_file; // имя выходного файла
    int write_type; // 0 - < 1 - > 2 - >> 
    int backgrnd; // 1 - фоновый режим; 0 - обычный 
    cmd_inf psubcmd; // указатель на саб шелл
    cmd_inf pipe; // указатель на pipe
    cmd_inf next; // указатель на следующую струкруту
    int type; //0 - ; 1 - && 2 - ||

} cmd_inf_struct;

cmd_inf shell_tree;

struct tree_atributes {
    int next_mode;
    char in_file[PATH_MAX];
    char out_file[PATH_MAX];
    int type_of_write;
    cmd_inf sub_shell;
    int type;
} tr_atr;

struct tree_atributes tree_atr;

int i = 0;
int flag_error;
int flag_out;

void make_shift(int n){
    while(n--)
        putc(' ', stderr);
}

void print_argv(char **p, int shift){
    char **q=p;
    if(p!=NULL){
        while(*p!=NULL){
             make_shift(shift);
             fprintf(stderr, "argv[%d]=%s\n",(int) (p-q), *p);
             p++;
        }
    }
}

void print_tree(cmd_inf t, int shift){
    char **p;
    if(t==NULL)
        return;
    p=t->argv;
    if(p!=NULL)
        print_argv(p, shift);
    else{
        make_shift(shift);
        fprintf(stderr, "psubshell\n");
    }
    make_shift(shift);
    if(t->in_file==NULL)
        fprintf(stderr, "infile=NULL\n");
    else
        fprintf(stderr, "infile=%s\n", t->in_file);
    make_shift(shift);
    if(t->out_file==NULL)
        fprintf(stderr, "outfile=NULL\n");
    else
        fprintf(stderr, "outfile=%s\n", t->out_file);
    make_shift(shift);
    fprintf(stderr, "append=%d\n", t->write_type);
    make_shift(shift);
    fprintf(stderr, "background=%d\n", t->backgrnd);
    make_shift(shift);
    fprintf(stderr, "type=%d\n", t->type);
    make_shift(shift);
    if(t->psubcmd==NULL)
        fprintf(stderr, "psubcmd=NULL \n");
    else{
        fprintf(stderr, "psubcmd---> \n");
        print_tree(t->psubcmd, shift+5);
    }
    make_shift(shift);
    if(t->pipe==NULL)
        fprintf(stderr, "pipe=NULL \n");
    else{
        fprintf(stderr, "pipe---> \n");
        print_tree(t->pipe, shift+5);
    }
    make_shift(shift);
    if(t->next==NULL)
        fprintf(stderr, "next=NULL \n");
    else{
        fprintf(stderr, "next---> \n");
        print_tree(t->next, shift+5);
    }
}
                                                                                                                                                                                                                              



int count_of_word(char **list)
{
    int j = 0;
    int cnt = 0;
    while (list[j++] != NULL && ++cnt) ;
    return cnt;
}

void add_cmd(cmd_inf *tr, char **argum, cmd_inf *sub_sh, int next_mode, int wr_type, cmd_inf *backgrndd, char *in_file, char *out_file, int type) //recursive 
{
    if (*tr == NULL) {
      // создаем список и заполняем его известными переменными 
      *tr = (cmd_inf)malloc(sizeof(cmd_inf_struct));
      int j;
      (*tr)->argv = (char **)malloc((count_of_word(argum)+1)*sizeof(argum));
      for (j = 0; argum[j] != NULL; ++j) {
          (*tr)->argv[j] = (char *)malloc(strlen(argum[j]) * sizeof(argum[j]));
          strcpy((*tr)->argv[j], argum[j]);
      }
      (*tr)->argv[j] = NULL;
      (*tr)->in_file = (char *)malloc(PATH_MAX);
      strcpy((*tr)->in_file, in_file);
      (*tr)->out_file = (char *)malloc(PATH_MAX);
      strcpy((*tr)->out_file, out_file);
      (*tr)->write_type = wr_type;
      (*tr)->backgrnd = 0;
      (*tr)->psubcmd = *sub_sh;
      (*tr)->pipe = NULL;
      (*tr)->next = NULL;
      (*tr)->type = type;
      if (next_mode == 0)
        *backgrndd = *tr;    
    }  
    else {
        if (next_mode == 0) 
            //если у нас аргумент идет после ; то заполняем вспомогательную структуру 
            add_cmd(&(*backgrndd)->next, argum, sub_sh, next_mode, wr_type, backgrndd, in_file, out_file, type);
        else if ((*tr)->next != NULL) 
            // если дерево не конечно, идем дальше
            add_cmd(&(*tr)->next, argum, sub_sh, next_mode, wr_type, backgrndd, in_file, out_file, type);
        else if((*tr)->pipe != NULL)
            add_cmd(&(*tr)->pipe, argum, sub_sh, next_mode, wr_type, backgrndd, in_file, out_file, type);
        else  
          add_cmd(&(*tr)->pipe, argum, sub_sh, next_mode, wr_type, backgrndd, in_file, out_file, type);
    }
}

void delete_argument(char **arg)
{
  int i = 0;
  while (arg[i] != NULL) {
    free(arg[i++]);
  }
}

void to_backgrnd(cmd_inf tree)
{
    while(tree->pipe != NULL){
        tree->backgrnd = 1;
        tree = tree->pipe;
    }
    tree->backgrnd = 1;
}

int if_command(char *q)
{
  if (!strcmp(q, "|") || !strcmp(q, "||") || !strcmp(q, "&") || !strcmp(q, "&&") || !strcmp(q, ";") || !strcmp(q, "(") || !strcmp(q, ")") || !strcmp(q, ">") || !strcmp(q, ">>") || !strcmp(q, "<")) return 1;
  return 0;
}

void add_type(cmd_inf p, int type)
{
    while(p->pipe != NULL){
        p->type = type;
        p = p->pipe;
    }
    p->type = type;
}

void clear_tree_rec(cmd_inf *tree)
{
    if (*tree != NULL){
        cmd_inf q = *tree;
        delete_argument(q->argv);
        free(q->argv);
        free(q->in_file);
        free(q->out_file);
        clear_tree_rec(&(*tree)->psubcmd);
        clear_tree_rec(&(*tree)->pipe);
        clear_tree_rec(&(*tree)->next);
        free(q);
    }
    return;
}

void clear_tree(cmd_inf *tree)
{
    clear_tree_rec(tree);
    *tree = NULL;
    return;
}

void delete_all(cmd_inf *tree1, cmd_inf *tree2, char ***arg)
{
  clear_tree(tree1);
  clear_tree(tree2);
  clearlist();
  //if (*arg != NULL)
    //delete_argument(*arg);
}

int creating_tree(cmd_inf *shell, int *brakets) 
{
    clear_tree(shell);
    cmd_inf sub_shell = NULL;
    cmd_inf backgrnd = NULL;
    int argv_counter = 0;
    char **argument;
    argument = (char **)malloc(list.sizelist * sizeof(list.lst));
    tree_atr.next_mode = 0;
    tree_atr.type_of_write = 0;
    tree_atr.type = 0;
    flag_error = 1;
    strcpy(tree_atr.in_file, "EMPTY");
    strcpy(tree_atr.out_file, "EMPTY");
    clear_tree(shell);
    V2 = Conv;
    while ((list.lst[i] != NULL) && flag_error) switch (V2) {

      case Conv:
        if (!strcmp(list.lst[i], "(")) {
            delete_all(shell, &sub_shell, &argument);
            return 2;
        }
        else if (!strcmp(list.lst[i], ")")) {
          delete_all(shell, &sub_shell, &argument);
          return 2;
        }
        else if (!strcmp(list.lst[i], "&"))
          V2 = Backgrnd;
        else if (!strcmp(list.lst[i], "|") || (!strcmp(list.lst[i], ";"))) 
          V2 = Pipe;
        else if (!strcmp(list.lst[i], ">") || (!strcmp(list.lst[i], ">>"))) 
          V2 = Out_File; 
        else if (!strcmp(list.lst[i], "<"))
          V2 = In_File;
        else if (!strcmp(list.lst[i], "&&") || (!strcmp(list.lst[i], "||")))
          V2 = Type;
        else {
          if (list.lst[i + 1] != NULL) {
            if (!strcmp(list.lst[i + 1], "(")) {
                delete_all(shell, &sub_shell, &argument);
                return 2;
            }
          }
          argument[argv_counter] = (char *)malloc(strlen(list.lst[i]) * sizeof(list.lst[i]));
          strcpy(argument[argv_counter++], list.lst[i++]);  
        }
        break;

      case Subshell_start:

        ++i;
        ++(*brakets);
        int save_next_mode = tree_atr.next_mode;
        char save_in_file[PATH_MAX];
        strcpy(save_in_file, tree_atr.in_file);
        char save_out_file[PATH_MAX];
        strcpy(save_out_file, tree_atr.out_file);
        int save_type_of_write = tree_atr.type_of_write;
        int save_type = tree_atr.type;
        int check = creating_tree(&sub_shell, brakets);
        if (check == 0) flag_error = 1;
        tree_atr.next_mode = save_next_mode;
        strcpy(tree_atr.in_file, save_in_file);
        strcpy(tree_atr.out_file, save_out_file);
        tree_atr.type_of_write = save_type_of_write;
        tree_atr.type = save_type;
        if (list.lst[i] == NULL) 
          argument[argv_counter++] = NULL;
        else
          V2 = Conv;
        break;

      case Subshell_end:
          
        --(*brakets);
        if (argv_counter == 0)
          argument[argv_counter++] = NULL;
        flag_error = 0;
        break;

      case Backgrnd:

        argument[argv_counter] = NULL;
        if (i == 0) {
          delete_all(shell, &sub_shell, &argument);
          return 2;
        }
        if (argument[0] != NULL || sub_shell != NULL) {
          add_cmd(shell, argument, &sub_shell, tree_atr.next_mode, tree_atr.type_of_write, &backgrnd, tree_atr.in_file, tree_atr.out_file, tree_atr.type);
          tree_atr.type_of_write = 0;
          argument[argv_counter] = NULL;
          delete_argument(argument);
          sub_shell = NULL;
        }
        to_backgrnd(backgrnd);
        argv_counter = 0;
        argument[argv_counter] = NULL;
        tree_atr.next_mode = 0;
        strcpy(tree_atr.in_file, "EMPTY");
        strcpy(tree_atr.out_file, "EMPTY");
        ++i;
        if (list.lst[i] != NULL && if_command(list.lst[i])) {
          delete_all(shell, &sub_shell, &argument);
          return 2;
        }
        if (list.lst[i] == NULL)
          break;
        V2 = Conv;
        break;
          
      case Pipe:

        if (i == 0) {
          delete_all(shell, &sub_shell, &argument);
          return 2;
        }
        argument[argv_counter] = NULL;
        if (argument[0] != NULL || sub_shell != NULL) {
          add_cmd(shell, argument, &sub_shell, tree_atr.next_mode, tree_atr.type_of_write, &backgrnd, tree_atr.in_file, tree_atr.out_file, tree_atr.type);
          tree_atr.type_of_write = 0;
          delete_argument(argument);
          sub_shell = NULL;
        }
        argv_counter = 0;
        argument[argv_counter] = NULL;
        strcpy(tree_atr.in_file, "EMPTY");
        strcpy(tree_atr.out_file, "EMPTY");
        tree_atr.type = 0;
        if (!strcmp(list.lst[i], ";")) 
          tree_atr.next_mode = 0;
        else
          tree_atr.next_mode = 1;
        ++i;
        if (((list.lst[i] != NULL) && (if_command(list.lst[i]))) || (list.lst[i] == NULL)) { 
          delete_all(shell, &sub_shell, &argument);
          return 2;
        }
        V2 = Conv;
        break;
    
      
      case In_File:

        if (((list.lst[i + 1] == NULL) || (if_command(list.lst[i + 1]))) || i == 0) { 
          delete_all(shell, &sub_shell, &argument);
          return 2;
        }
        strcpy(tree_atr.in_file, list.lst[++i]);
        ++i;
        if ((sub_shell != NULL) && list.lst[i] == NULL) 
            argument[argv_counter++] = NULL;
        V2 = Conv;
        break;

      case Out_File:

        if(((list.lst[i + 1] == NULL) || (if_command(list.lst[i + 1]))) || i == 0) { 
          delete_all(shell, &sub_shell, &argument);
          return 2;
        }
        if (!strcmp(list.lst[i], ">"))
          tree_atr.type_of_write = 1;
        else
          tree_atr.type_of_write = 2;
        ++i;
        strcpy(tree_atr.out_file, list.lst[i]);
        ++i;
        if ((sub_shell != NULL) && list.lst[i] == NULL) 
          argument[argv_counter++] = NULL;
        V2 = Conv;

        break;

      case Type:

        if (i == 0) {
          delete_all(shell, &sub_shell, &argument);
          return 2;
        }
        if (!strcmp(list.lst[i], "&&"))
          tree_atr.type = 1;
        else
          tree_atr.type = 2;
        argument[argv_counter] = NULL;
        if (argument[0] != NULL || sub_shell != NULL) {
          add_cmd(shell, argument, &sub_shell, tree_atr.next_mode, tree_atr.type_of_write, &backgrnd, tree_atr.in_file, tree_atr.out_file, tree_atr.type);
          delete_argument(argument);
          tree_atr.type_of_write = 0; 
          sub_shell = NULL;
        }
        add_type(backgrnd, tree_atr.type);
        argv_counter = 0;
        argument[argv_counter] = NULL;
        strcpy(tree_atr.in_file, "EMPTY");
        strcpy(tree_atr.out_file, "EMPTY");
        tree_atr.type = 0;
        tree_atr.next_mode = 0;
        ++i;
        if ((list.lst[i] != NULL) && (if_command(list.lst[i]))) {
          delete_all(shell, &sub_shell, &argument);
          return 2;
        }
        V2 = Conv;
        break;
    }

    argument[argv_counter] = NULL;
    if (argv_counter >= 1)
      add_cmd(shell, argument, &sub_shell, tree_atr.next_mode, tree_atr.type_of_write, &backgrnd, tree_atr.in_file, tree_atr.out_file, tree_atr.type);
    delete_argument(argument);
    ++i;
    return 0;
    clear_tree(&backgrnd);
    clear_tree(&sub_shell);
}


typedef struct pids *pids_struct;

typedef struct pids {
  pid_t pid;
  pids_struct next;
} piddds;

typedef pids_struct pid_list;

pid_list pidds = NULL;
pid_list pides = NULL;

void add_background_pids(pid_list *pid_lst, pid_t p)
{ 
    if (*pid_lst == NULL){
        *pid_lst = (pid_list) malloc(sizeof(piddds));
        (*pid_lst)->pid = p;
        (*pid_lst)->next = NULL;
        return;
    }
    add_background_pids(&(*pid_lst)->next, p);
}

void add_pids(pid_list *pid_lst, pid_t p)
{
  if (*pid_lst == NULL){
      *pid_lst = (pid_list) malloc(sizeof(piddds));
      (*pid_lst)->pid = p;
      (*pid_lst)->next = NULL;
      return;
    }
    add_pids(&(*pid_lst)->next, p);
}

void delete_pid(pid_list *p, pid_t pid) 
{ 
    if ((*p)->next != NULL) {
        if ((*p)->pid == pid) {
            pid_list p_help = *p; 
            (*p) = (*p)->next; 
            free(p_help); 
        } else
            delete_pid(&(*p)->next, pid);
    } else {
        if ((*p)->pid == pid) {
            pid_list p_help = *p; 
            (*p) = (*p)->next; 
            free(p_help); 
        }
    }
}

void wait_background()
{
  int status = 0;
  while (pidds != NULL) {
    waitpid(pidds->pid, &status, WNOHANG);
    //pid_list p = pidds;
    pidds = pidds->next;
    //if (WIFEXITED(status))
      //delete_pid(&p, p->pid);
  }
}

void wait_pids()
{
  int status = 0;
  while (pides != NULL) {
    //printf("WAIT PID == %d\n", pides->pid);
    waitpid(pides->pid, &status, 0);
    pid_list p = pides;
    pides = pides->next;
    delete_pid(&p, p->pid);
  }
}

void wait_until()
{
  int status = 0;
  while (pides != NULL) {
    //printf("WAIT PID == %d\n", pides->pid);
    waitpid(pides->pid, &status, 0);
    pid_list p = pides;
    pides = pides->next;
    delete_pid(&p, p->pid);
  }
}

void main_exec(cmd_inf tree)
{
  int in_file, out_file;
  
  if (strcmp(tree->in_file, "EMPTY")) {
    in_file = open(tree->in_file, O_RDONLY, 0666);
    dup2(in_file, 0);
    close(in_file);
  } 
  if (strcmp(tree->out_file, "EMPTY")) {
    out_file = (tree->write_type == 1) ? open(tree->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666) : open(tree->out_file, O_APPEND | O_WRONLY | O_CREAT, 0666);
    dup2(out_file, 1);
    close(out_file);
  }
  if (tree->backgrnd == 1)
    signal(SIGINT, SIG_IGN);
  //if (tree->pipe != NULL)
    //exec_pipe(tree);
  execvp(tree->argv[0], tree->argv);
  fprintf(stderr, "zsh: command not found: %s\n", tree->argv[0]);
  exit(1);
  //while (wait() == -1);
  //if (tree->next != NULL)
    //main_exec(tree->next);
}

void exec_pipe(cmd_inf tree)
{
  int p[2], p_help, pid, status = 0;

  cmd_inf tree_help = tree;
  if (!strcmp(tree->argv[0], "cd")) {
    if (tree_help->argv[1] != NULL && tree_help->argv[2] == NULL)
      chdir(tree_help->argv[1]);
    else if (tree_help->argv[1] == NULL)
      chdir(getenv("HOME"));
    else if (tree_help->argv[2] != NULL)
      fprintf(stderr, "cd: string not in pwd: %s\n", tree_help->argv[2]);
  }

  if (tree_help->pipe == NULL) {
    if ((pid = fork()) == 0) 
      main_exec(tree_help);
    if (tree_help->backgrnd == 1)
      add_background_pids(&pidds, pid);
    else
      waitpid(pid, &status, 0);
  } else {
    pipe(p);
    if ((pid = fork()) == 0) {
      dup2(p[1], 1);
      close(p[1]);
      close(p[0]);
      //++pid_cnt;
      main_exec(tree_help);
    }
    if (tree_help->backgrnd == 1)
      add_background_pids(&pidds, pid);
    else 
      add_pids(&pides, pid);
    p_help = p[0];
    tree_help = tree_help->pipe;
    while(tree_help->pipe != NULL) {
      close(p[1]);
      pipe(p);
      if ((pid = fork()) == 0) {
        dup2(p_help, 0);
        close(p_help);
        dup2(p[1], 1); 
        close(p[1]);
        close(p[0]);
        //++pid_cnt;
        main_exec(tree_help);
      }
      if (tree_help->backgrnd == 1)
        add_background_pids(&pidds, pid);
      else
        add_pids(&pides, pid);
      close(p_help);
      p_help = p[0];
      tree_help = tree_help->pipe;
    }
    close(p[1]);
    if ((pid = fork()) == 0) {
      dup2(p_help, 0);
      close(p_help);
      //++pid_cnt;
      main_exec(tree_help);
    }
    if (tree_help->backgrnd == 1)
      add_background_pids(&pidds, pid);
    else 
      add_pids(&pides, pid);
    close(p_help);
    wait_pids();
    //printf("IM HERE\n");
    //for (int j = 0; j < pid_cnt; ++j) 
      //wait(NULL);
  }
  if (tree_help->next != NULL) {
    exec_pipe(tree_help->next);
    wait(NULL);
  }
  //wait(NULL);
  wait_background();
}


void sighandlr(int s)
{
    wait_until();
}

int main(int argc, char **argv)
{
    while (1) {
    signal(SIGINT, sighandlr);
    printf("user@this-shell task5 %% ");
    task3();
    if (null_flag) {
      null_flag = 0;
      continue;
    }
    //printlist();
    int brakets = 0;
    i = 0;
    int ret = creating_tree(&shell_tree, &brakets);
    //print_tree(shell_tree, 5);
    if (ret == 2) {
      printf("Syntax error\n");
      continue;
    }
    clearlist();
    exec_pipe(shell_tree);
    }
    return 0;
}
