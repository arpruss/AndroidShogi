#include <string.h>
#include <stdarg.h>
#include "shogi.h"

#if defined(DEKUNOBOU)

int
dek_start( const char *str_addr, int port_dek, int port_bnz )
{
  SOCKADDR_IN service;
  WSADATA wsaData;
  u_short dek_ns_bnz;

  /* initialize winsock */
  if ( WSAStartup( MAKEWORD(1,1), &wsaData ) )
    {
      str_error = "WSAStartup() failed.";
      return -2;
    }

  dek_ul_addr = inet_addr( str_addr );
  if ( dek_ul_addr == INADDR_NONE )
    {
      struct hostent *phe = gethostbyname( str_addr );
      if ( ! phe )
	{
	  str_error = str_WSAError( "gethostbyname() faild." );
	  return -2;
	}
      dek_ul_addr = *( (u_long *)phe->h_addr_list[0] );
    }

  dek_ns      = htons( (u_short)port_dek );
  dek_ns_bnz  = htons( (u_short)port_bnz );

  dek_socket_in = socket( AF_INET, SOCK_STREAM, 0 );
  if ( dek_socket_in == INVALID_SOCKET )
    {
      str_error = str_WSAError( "socket() failed." );
      return -2;
    }
  
  service.sin_family      = AF_INET;
  service.sin_addr.s_addr = dek_ul_addr;
  service.sin_port        = dek_ns_bnz;
  if ( bind( dek_socket_in, (SOCKADDR *)&service, sizeof(service) )
       == SOCKET_ERROR )
    {
      str_error = "bind() failed.";
      return -2;
    }

  if ( listen( dek_socket_in, 1 ) == SOCKET_ERROR )
    {
      str_error = "listen() failed.";
      return -2;
    }

  dek_s_accept = (SOCKET)SOCKET_ERROR;

  return 1;
}


int
dek_next_game( tree_t * restrict ptree )
{
  if ( dek_ngame != 1 && dek_turn )
    {
      Out( "take a nap ..." );
      Sleep( 37000 );
      Out( " done\n" );
    }

  if ( ini_game( ptree, &min_posi_no_handicap, flag_history, NULL, NULL ) < 0
       || get_elapsed( &time_turn_start ) < 0
       || ( dek_turn && com_turn_start( ptree, 0 ) < 0 ) ) { return -1; }

  dek_turn  ^= 1;
  dek_ngame += 1;

  return 1;
}


int
dek_check( void )
{
  struct timeval tv;
  fd_set readfds;
  int iret;
  char ch;

  tv.tv_sec = tv.tv_usec = 0;

  if ( dek_s_accept == SOCKET_ERROR )
    {
      FD_ZERO( &readfds );
#if defined(_MSC_VER)
#  pragma warning(disable:4127)
#endif
      FD_SET( dek_socket_in, &readfds );
#if defined(_MSC_VER)
#  pragma warning(default:4127)
#endif
      iret = select( 1, &readfds, NULL, NULL, &tv );
      if ( iret == SOCKET_ERROR )
	{
	  snprintf( str_message, SIZE_MESSAGE,
		    "select() with a socket listening failed:%d",
		   WSAGetLastError() );
	  str_error = str_message;
	  return -1;
	
	}
      if ( ! iret ) { return 0; } /* no connection is pending. */

      dek_s_accept = accept( dek_socket_in, NULL, NULL );
      if ( dek_s_accept == SOCKET_ERROR )
	{
	  snprintf( str_message, SIZE_MESSAGE,
		    "accept() following select() failed:%d",
		   WSAGetLastError() );
	  str_error = str_message;
	  return -1;
	}
    }

  FD_ZERO( &readfds );
#if defined(_MSC_VER)
#  pragma warning(disable:4127)
#endif
  FD_SET( dek_s_accept, &readfds );
#if defined(_MSC_VER)
#  pragma warning(default:4127)
#endif

  iret = select( 0, &readfds, NULL, NULL, &tv );
  if ( iret == SOCKET_ERROR )
    {
      snprintf( str_message, SIZE_MESSAGE,
		"select() with a socket accepted failed:%d",
		WSAGetLastError() );
      str_error = str_message;
      return -1;
    }
  if ( ! iret ) { return 0; } /* the connection isn't closed,
				 nor has available data. */

  iret = recv( dek_s_accept, &ch, 1, MSG_PEEK );
  if ( iret == SOCKET_ERROR )
    {
      closesocket( dek_s_accept );
      dek_s_accept = (SOCKET)SOCKET_ERROR;
      snprintf( str_message, SIZE_MESSAGE,
		"recv() with flag MSG_PEEK failed:%d",
		WSAGetLastError() );
      str_error = str_message;
      return -1;
    }
  if ( ! iret )
    {
      if ( closesocket( dek_s_accept ) )
	{
	  dek_s_accept = (SOCKET)SOCKET_ERROR;
	  snprintf( str_message, SIZE_MESSAGE,
		    "closesocket() failed:%d", WSAGetLastError() );
	  str_error = str_message;
	  return -1;
	}
      dek_s_accept = (SOCKET)SOCKET_ERROR;
      return 0; /* the connection has been closed. */
    }

  return 1; /* data is available for reading. */
}


int
dek_in( char *str, int n )
{
#if defined(_MSC_VER)
#  pragma warning(disable:4127)
#endif
  int count_byte;

  for (;;) {
    if ( dek_s_accept == SOCKET_ERROR )
      {
	Out( "\nwait for new connection...\n" );
	dek_s_accept = accept( dek_socket_in, NULL, NULL );
	if ( dek_s_accept == SOCKET_ERROR )
	  {
	    str_error = str_WSAError( "accept() failed." );
	    return -1;
	  }
      }
  
    count_byte = recv( dek_s_accept, str, n, 0 );
    if ( count_byte == SOCKET_ERROR )
      {
	closesocket( dek_s_accept );
	dek_s_accept = (SOCKET)SOCKET_ERROR;
	str_error = str_WSAError( "recv() failed." );
	return -1;
      }
    if ( count_byte ) { break; }

    if ( closesocket( dek_s_accept ) )
      {
	dek_s_accept = (SOCKET)SOCKET_ERROR;
	str_error = str_WSAError( "closesocket() failed." );
	return -1;
      }
    dek_s_accept = (SOCKET)SOCKET_ERROR;
  }

  *( str + count_byte ) = '\0';
  Out( "recieved %s", str );

  return count_byte;
#if defined(_MSC_VER)
#  pragma warning(default:4127)
#endif
}


int
dek_out( const char *format, ... )
{
  SOCKADDR_IN service;
  SOCKET socket_out;
  int nch, iret;
  char buf[256];
  va_list arg;

  va_start( arg, format );
  nch = vsnprintf( buf, 256, format, arg );
  va_end( arg );

  Out( "send %s", buf );

  socket_out = socket( AF_INET, SOCK_STREAM, 0 );
  if ( socket_out == INVALID_SOCKET )
    {
      snprintf( str_message, SIZE_MESSAGE,
		"socket() failed:%d", WSAGetLastError() );
      str_error = str_message;
      return -2;
    }

  service.sin_family      = AF_INET;
  service.sin_addr.s_addr = dek_ul_addr;
  service.sin_port        = dek_ns;
  if ( connect( socket_out, (SOCKADDR *)&service, sizeof(service) )
       == SOCKET_ERROR )
    {
      snprintf( str_message, SIZE_MESSAGE,
		"connect() failed:%d", WSAGetLastError() );
      str_error = str_message;
      return -2;
    }

  iret = send( socket_out, buf, nch, 0 );
  if ( iret == SOCKET_ERROR )
    {
      closesocket( socket_out );
      snprintf( str_message, SIZE_MESSAGE,
		"send() failed:%d", WSAGetLastError() );
      str_error = str_message;
      return -2;
    }
  if ( iret != nch )
    {
      closesocket( socket_out );
      str_error = "send() wrote partial number of bytes.";
      return -2;
    }

  if ( closesocket( socket_out ) )
    {
      snprintf( str_message, SIZE_MESSAGE,
		"closesocket() failed:%d", WSAGetLastError() );
      str_error = str_message;
      return -2;
    }

  return 1;
}

int
dek_parse( char *str, int len )
{
  if ( *str == '+' || *str == '-' )
    {
      memmove( str, str+1, 6 );
      str[6] = '\0';
    }
  else if ( ! strcmp( str, str_resign ) )
    {
      strncpy( str, "resign", len-1 );
      str[len-1] = '\0';
      dek_win += 1;
      Out( "Bonanza won against Dekunobou\n" );
    }
  else {
    str_error = "unknown command is recieved from Deknobou.";
    return -2;
  }

  return 1;
}

#  endif /* DEKUNOBOU */
