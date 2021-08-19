# ftp_client

#### 介绍
ftp客户端，可用于没安装有ftp的linux主机，连接ftp服务器登录上传下载等

#### 安装教程

1.cd ftp-client/

2. make

3. export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd` （在pwd前后加~键对应的`）


#### 使用说明

 ./ftp_client 

ftp_client -- (2019.3.16)
 Usage: ftp_client -i <server_ip>/<ftp_server_hostname> -p <ftp_server_port>  [-h <use_help>]

        -p --port       the port of the ftp server you want to connect

        -i --ip         the ip address or hostname of the ftp server you want to connect

        -h --help       the ftp_client how to use

After Loin OK:

ftp_client version 1.0.0:

help:        show the help fro use ftp_client

ls:          to list ftp server file

put:         upload file to ftp server

get:         download file from ftp server

cd:          change work path

quit:        to exit ftp_client


