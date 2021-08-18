/*
 * uhttpd - Tiny single-threaded httpd - CGI header
 *
 *   Copyright (C) 2010-2012 Jo-Philipp Wich <xm@subsignal.org>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _UHTTPD_CGI_ 
#define _UHTTPD_CGI_ 


#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <time.h>
#include "client.h"

#define ensure_out(x)                                                          \
  do {                                                                         \
    if ((x) < 0)                                                               \
      goto out;                                                                \
  } while (0) 
  
#define foreach_header(i, h)                                                   \
          for (i = 0; (i + 1) < (sizeof(h) / sizeof(h[0])) && h[i]; i += 2) 


struct uh_cgi_state {
	struct {
		char buf[UH_LIMIT_MSGHEAD];
		char *ptr;
		int len;
	} httpbuf;
	int content_length;
	bool header_sent;
};

struct client_light {
  struct uloop_fd fd;
};



struct interpreter {
  char path[PATH_MAX];
  char extn[32];
  struct interpreter *next;
};


//extern bool uh_cgi_request(struct uh_client *cl, struct path_info *pi, struct interpreter *ip);
//extern bool uh_cgi_request(struct uh_client *cl, struct path_info *pi, struct interpreter *ip);


#endif
