#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "message.h"
static const char* messages[] = {
"Welcome to CprE 308!",
"Operating Systems are everywhere.",
"The kernel is always watching.",
"Process management is fun!"
};
void print_message() {
srand(time(NULL));
int index = rand() % 4;
printf("%s\n", messages[index]);
}
