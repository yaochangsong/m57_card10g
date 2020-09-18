#include <stdio.h>
#include <stdlib.h>


int main (int argc, char *argv[])
{
    char buffer[512] = {0};
    char *content_length = getenv("CONTENT_LENGTH");
   // char *filename = getenv("CONTENT_LENGTH");
   //sscanf();
   snprintf(buffer,sizeof(buffer), "echo 111 > cgi.txt");
   system(buffer);
  // snprintf(buffer,sizeof(buffer), "echo %s >> cgi.txt", content_length);
  // system(buffer);
   printf("Hello CGI\n");
}
