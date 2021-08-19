CC = $(CC)



all:dynamic_lib compile 

dynamic_lib:
	@$(CC) -shared -fPIC -o libftp_client.so creat_socket.c accept_client.c connect_server.c ftp_list.c ftp_pasv.c ftp_put.c ftp_get.c ftp_login.c ftp_usage_print.c ftp_port.c ftp_cwd.c


compile:
	@$(CC) -Wall ftp_client.c -o ftp_client -L `pwd`  -lftp_client

clean:
	@rm ftp_client libftp_client.so
