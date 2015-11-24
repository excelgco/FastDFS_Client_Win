#ifndef _SOCKETOPT_H_
#define _SOCKETOPT_H_


#define FDFS_WRITE_BUFF_SIZE  256 * 1024

#ifdef __cplusplus
extern "C" {
#endif

int is_socket_closed(int sock);

/** recv data (non-block mode)
 *  parameters:
 *          sock: the socket
 *          data: the buffer
 *          size: buffer size (max bytes can receive)
 *          timeout: read timeout in seconds
 *          count: store the bytes recveived
 *  return: error no, 0 success, != 0 fail
*/
int tcprecvdata_nb(int sock, void *data, const int size, \
		const int timeout, int *count);

/** send data (non-block mode)
 *  parameters:
 *          sock: the socket
 *          data: the buffer to send
 *          size: buffer size
 *          timeout: write timeout
 *  return: error no, 0 success, != 0 fail
*/
int tcpsenddata_nb(int sock, void* data, const int size, const int timeout);

/** connect to server by block mode
 *  parameters:
 *          sock: the socket
 *          server_ip: ip address of the server
 *          server_port: port of the server
 *  return: error no, 0 success, != 0 fail
*/
int connectserverbyip(int sock, const char *server_ip, const short server_port);

/** set socket non-block options
 *  parameters:
 *          sock: the socket
 *  return: error no, 0 success, != 0 fail
*/
int tcpsetnonblockopt(int fd);

#ifdef __cplusplus
}
#endif

#endif

