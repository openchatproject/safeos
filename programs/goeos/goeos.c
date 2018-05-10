#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "goeos.h"

static char** argv;
static int argc = 0;

extern int eos_main(int argc, char** argv);

void arg_size(int n) {
   argv = (char**)malloc(sizeof(char**)*n);
   argc = 0;
}

void arg_add(char * arg) {
//   printf("%s\n", arg);
   argv[argc] = (char *)malloc(strlen(arg)+1);
   strcpy(argv[argc], arg);
   argc += 1;
}

void arg_show() {
   for (int i=0;i<argc;i++) {
      printf("%s \n", argv[i]);
   }
}

//int call_onApply(int in);



int goeos_main() {
//   call_onApply(123);
   return eos_main(argc, argv);
}





