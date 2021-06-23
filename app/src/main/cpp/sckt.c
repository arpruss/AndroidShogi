#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#if ! defined(_WIN32)
#  include <arpa/inet.h>
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netdb.h>
#  include <unistd.h>
#endif
#include "shogi.h"

#if defined(CSA_LAN) || defined(DEKUNOBOU)
const char *
str_WSAError( const char *str )
{
#  if defined(_WIN32)
  snprintf( str_message, SIZE_MESSAGE, "%s:%d", str,  WSAGetLastError() );
  str_message[SIZE_MESSAGE-1] = '\0';
  return str_message;
#  else
  return str;
#  endif
}
#endif

#if defined(CSA_LAN)

int
client_next_game( tree_t * restrict ptree )
{
  int iret;
  int my_turn;
  const char *str_name1, *str_name2;
  char buf1[SIZE_PLAYERNAME], buf2[SIZE_PLAYERNAME];

  str_name1 = str_name2 = NULL;
  iret = sckt_out( sckt_csa, "LOGIN %s %s\n", client_str_id, client_str_pwd );
  if ( iret < 0 ) { return iret; }

  for ( ;; ) {
    Out( "wait for next game-conditions...\n" );

    my_turn = black;
    for ( ;; ) {
      iret = next_cmdline( 1 );
      if ( iret < 0 ) { return iret; }
      
      if      ( ! strcmp( str_cmdline, "END Game_Summary" ) ) { break; }
      else if ( ! strcmp( str_cmdline, "Your_Turn:-" ) ) { my_turn = white; }
      else if ( ! memcmp( str_cmdline, "Name+:", 6 ) )
	{
	  strncpy( buf1, str_cmdline+6, SIZE_PLAYERNAME-1 );
	  buf1[SIZE_PLAYERNAME-1] = '\0';
	  str_name1 = buf1;
	}
      else if ( ! memcmp( str_cmdline, "Name-:", 6 ) )
	{
	  strncpy( buf2, str_cmdline+6, SIZE_PLAYERNAME-1 );
	  buf2[SIZE_PLAYERNAME-1] = '\0';
	  str_name2 = buf2;
	}
    }
    
    iret = sckt_out( sckt_csa, "AGREE\n" );
    if ( iret < 0 ) { return -2; }

    iret = next_cmdline( 1 );
    if ( iret < 0 ) { return iret; }

    if      ( ! memcmp( str_cmdline, "REJECT:", 7 ) )  { continue; }
    else if ( ! memcmp( str_cmdline, "START:", 6 ) )   { break; }

    str_error = str_server_err;
    return -2;
  }

  if ( ini_game( ptree, &min_posi_no_handicap, flag_history,
		 str_name1, str_name2 ) < 0 )
    {
      return -1;
    }

  if ( get_elapsed( &time_turn_start ) < 0 ) { return -1; }

  client_turn   = my_turn;
  client_ngame += 1;

  Out( "Game Conditions (%dth):\n", client_ngame );
  Out( "  my turn:%c\n", ach_turn[my_turn] );

  if ( my_turn == root_turn )
    {
      iret = com_turn_start( ptree, 0 );
      if ( iret < 0 ) { return iret; }
    }

  return 1;
}


sckt_t
sckt_connect( const char *str_addr, int iport )
{
  struct hostent *phe;
  struct sockaddr_in sin;
  sckt_t sd;
  u_long ul_addr;

#if defined(_WIN32)
  {
    WSADATA wsaData;
    if ( WSAStartup( MAKEWORD(1,1), &wsaData ) )
      {
	str_error = str_WSAError( "WSAStartup() failed." );
	return SCKT_NULL;
      }
  }
#endif

  ul_addr = inet_addr( str_addr );
  if ( ul_addr == INADDR_NONE )
    {
      phe = gethostbyname( str_addr );
      if ( ! phe )
	{
	  str_error = str_WSAError( "gethostbyname() faild." );
#if defined(_WIN32)
	  WSACleanup();
#endif
	  return SCKT_NULL;
	}
      ul_addr = *( (u_long *)phe->h_addr_list[0] );
    }

  sd = socket( AF_INET, SOCK_STREAM, 0 );
  if ( sd == SCKT_NULL )
    {
      str_error = str_WSAError( "socket() faild." );
#if defined(_WIN32)
      WSACleanup();
#endif
      return SCKT_NULL;
    }

  sin.sin_family      = AF_INET;
  sin.sin_addr.s_addr = ul_addr;
  sin.sin_port        = htons( (u_short)iport );
  if ( connect( sd, (struct sockaddr *)&sin, sizeof(sin) ) == SOCKET_ERROR )
    {
      str_error = str_WSAError( "connect() faild." );
#if defined(_WIN32)
      WSACleanup();
#endif
      return SCKT_NULL;
    }

  return sd;
}


int
sckt_shutdown( sckt_t sd )
{
  int iret;

  if ( sd == SCKT_NULL ) { return 1; }
  Out( "shut down connection\n" );

#  if defined(_WIN32)
  if ( shutdown( sd, SD_SEND ) == SOCKET_ERROR )
    {
      str_error = str_WSAError( "shutdown() faild." );
      WSACleanup();
      return -2;
    }

  for ( ;; ) {
    iret = recv( sd, str_message, SIZE_MESSAGE, 0 );
    if ( iret == SOCKET_ERROR )
      {
	str_error = str_WSAError( "recv() failed." );
	WSACleanup();
	return -2;
      }
    else if ( ! iret ) { break; }
  }
  
  if ( closesocket( sd ) == SOCKET_ERROR )
    {
      str_error = str_WSAError( "closesocket() failed." );
      WSACleanup();
      return -2;
    }

  if ( WSACleanup() == SOCKET_ERROR )
    {
      str_error = str_WSAError( "WSACleanup() faild." );
      return -2;
    }

  return 1;

#  else
  if ( shutdown( sd, SHUT_RD ) == -1 )
    {
      str_error = "shutdown() faild.";
      return -2;
    }

  for ( ;; ) {
    iret = (int)recv( sd, str_message, SIZE_MESSAGE, 0 );
    if ( iret == -1 )
      {
	str_error = "recv() failed.";
	return -2;
      }
    else if ( ! iret ) { break; }
  }
  
  if ( close( sd ) == -1 )
    {
      str_error = "close() failed.";
      return -2;
    }

  return 1;
#  endif
}


int
sckt_check( sckt_t sd )
{
  struct timeval tv;
  fd_set readfds;
  int iret;

  tv.tv_sec = tv.tv_usec = 0;

  FD_ZERO( &readfds );
#  if defined(_MSC_VER)
#    pragma warning(disable:4127)
#  endif
  FD_SET( sd, &readfds );
#  if defined(_MSC_VER)
#    pragma warning(default:4127)
#  endif

  iret = select( (int)sd+1, &readfds, NULL, NULL, &tv );
  if ( iret == SOCKET_ERROR )
    {
      str_error = str_WSAError( "select() with a socket connected failed." );
      return -2;
    }

  return iret;
}


int
sckt_in( sckt_t sd, char *str, int n )
{
  struct timeval tv;
  fd_set readfds;
  int iret;

  for ( ;; ) {
    Out( "wait for a message ... " );

    tv.tv_sec  = SEC_KEEP_ALIVE;
    tv.tv_usec = 0;
    FD_ZERO( &readfds );
#  if defined(_MSC_VER)
#    pragma warning(disable:4127)
#  endif
    FD_SET( sd, &readfds );
#  if defined(_MSC_VER)
#    pragma warning(default:4127)
#  endif

    iret = select( (int)sd+1, &readfds, NULL, NULL, &tv );
    if ( iret == SOCKET_ERROR )
      {
	str_error = str_WSAError( "select() with a socket connected failed." );
	return -2;
      }
    if ( iret )
      {
	Out( "done.\n" );
	break;
      }
    Out( "time out.\n" );

    if ( sckt_out( sd, "\n" ) == SOCKET_ERROR )
      {
	str_error = str_WSAError( "send() failed." );
	return -2;
      }
  }

  iret = recv( sd, str, n-1, 0 );
  if ( iret == SOCKET_ERROR )
    {
      str_error = str_WSAError( "recv() failed." );
      return -2;
    }
  if ( ! iret )
    {
      str_error = "connection closed.";
      return -2;
    }
  *( str + iret ) = '\0';

  Out( "%s[END]\n", str );
  
  return iret;
}


int
sckt_out( sckt_t sd, const char *fmt, ... )
{
  int nch;
  char buf[256];
  va_list arg;

  va_start( arg, fmt );
  nch = vsnprintf( buf, 256, fmt, arg );
  va_end( arg );
  if ( nch >= 256 || nch < 0 )
    {
      str_error = "buffer overflow at sckt_out()";
      return -2;
    }

  Out( "\n[now sending message]\n%s[END]\n", buf );

  if ( send( sd, buf, nch, 0 ) == SOCKET_ERROR )
    {
      str_error = str_WSAError( "send() failed." );
      return -2;
    }

  return get_elapsed( &time_last_send );
}

#endif /* CSA_LAN */

