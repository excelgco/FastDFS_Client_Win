#include "Winsock2.h"
#include "Sockopt.h"

int is_socket_closed(int sock)
{
	int nRet;
	HANDLE hCloseEvent = WSACreateEvent();
	WSAEventSelect(sock, hCloseEvent, FD_CLOSE);

	DWORD dwRet = WaitForSingleObject(hCloseEvent, 1);

	if(dwRet == WSA_WAIT_EVENT_0)
		nRet = 1;
	else if(dwRet == WSA_WAIT_TIMEOUT)
		nRet = 0;

	WSACloseEvent(hCloseEvent);
	return nRet;
}

int tcprecvdata_nb(int sock, void *data, const int size, const int timeout_ms, int *count)
{
	int left_bytes;
	int read_bytes;
	int res;
	int ret_code;
	char* p;
	fd_set read_set;
	struct timeval t;

	FD_ZERO(&read_set);
	FD_SET(sock, &read_set);

	read_bytes = 0;
	ret_code = 0;
	p = (char*)data;
	left_bytes = size;
	while (left_bytes > 0)
	{
		read_bytes = recv(sock, p, left_bytes, 0);
		if (read_bytes > 0)
		{
			left_bytes -= read_bytes;
			p += read_bytes;
			continue;
		}

		if (read_bytes < 0)
		{
			int nErrno = WSAGetLastError();
			if (!(nErrno == WSAEWOULDBLOCK || nErrno == WSAEINTR))
			{
				ret_code = nErrno != 0 ? nErrno : WSAEINTR;
				break;
			}
		}
		else
		{
			ret_code = WSAENOTCONN;
			break;
		}

		if (timeout_ms <= 0)
		{
			res = select(sock+1, &read_set, NULL, NULL, NULL);
		}
		else
		{
			t.tv_usec = 0;
			t.tv_sec = timeout_ms / 1000;
			res = select(sock+1, &read_set, NULL, NULL, &t);
		}

		if (res < 0)
		{
			int nErrno = WSAGetLastError();
            if (nErrno == WSAEINTR)
            {
                continue;
            }
			ret_code = nErrno != 0 ? nErrno : WSAEINTR;
			break;
		}
		else if (res == 0)
		{
			ret_code = WSAETIMEDOUT;
			break;
		}
	}

	if (count != NULL)
	{
		*count = size - left_bytes;
	}

	return ret_code;
}

int tcpsenddata_nb(int sock, void* data, const int size, const int timeout_ms)
{
	int left_bytes;
	int write_bytes;
	int result;
	char* p;
	fd_set write_set;
	struct timeval t;

	FD_ZERO(&write_set);
	FD_SET(sock, &write_set);

	p = (char*)data;
	left_bytes = size;
	while (left_bytes > 0)
	{
		write_bytes = send(sock, p, left_bytes, 0);
		if (write_bytes < 0)
		{
			int nErrno = WSAGetLastError();
			if (!(nErrno == WSAEWOULDBLOCK || nErrno == WSAEINTR))
			{
				return nErrno != 0 ? nErrno : WSAEINTR;
			}
		}
		else
		{
			left_bytes -= write_bytes;
			p += write_bytes;
			continue;
		}

		if (timeout_ms <= 0)
		{
			result = select(sock+1, NULL, &write_set, NULL, NULL);
		}
		else
		{
			t.tv_usec = 0;
			t.tv_sec = timeout_ms / 1000;
			result = select(sock+1, NULL, &write_set, NULL, &t);
		}

		if (result < 0)
		{
			int nErrno = WSAGetLastError();
            if (nErrno == WSAEINTR)
            {
                continue;
            }
			return nErrno != 0 ? nErrno : WSAEINTR;
		}
		else if (result == 0)
		{
			return WSAETIMEDOUT;
		}
	}

	return 0;
}
