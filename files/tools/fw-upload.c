#include <stdio.h>
#include <stdlib.h>

#define DEBUG
#ifdef DEBUG
#define D(...) fprintf(stderr, __VA_ARGS__)
#else
#define D(...)
#endif


int main (int argc, char *argv[])
{
    char buffer[512] = {0};
    int r;
    char *content_length = getenv("CONTENT_LENGTH");
   // char *filename = getenv("CONTENT_LENGTH");
   D("fw upload...\n");
   while((r = read(0, buffer, sizeof(buffer))) > 0){
    D("r=%d\n", r);
    D("fw upload recv[buffer %s]\n",buffer);
   }
  // r = read(0, buffer, sizeof(buffer));
   
  // D("fw upload recv[%d]\n",r);
   printf("Hello CGI\n");
}
