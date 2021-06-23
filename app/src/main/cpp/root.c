#include <stdlib.h>
#include <limits.h>
#include "shogi.h"

static int read_rest_list( tree_t * restrict ptree,
			   unsigned int * restrict pmove_list );

static int is_move_rest( unsigned int move,
			 const unsigned int * restrict pmove_restraint );

int
make_root_move_list( tree_t * restrict ptree, int flag )
{
  unsigned int * restrict pmove;
  unsigned int arestraint_list[ MAX_LEGAL_MOVES ];
  int asort[ MAX_LEGAL_MOVES ];
  unsigned int move, move_best;
  int i, j, k, h, value, num_root_move, iret, value_pre_pv;
  int value_best;

  if ( flag & flag_refer_rest ) { read_rest_list( ptree, arestraint_list ); }
  else                          { arestraint_list[0] = 0; }

  pmove = ptree->move_last[0];
  ptree->move_last[1] = GenCaptures( root_turn, pmove );
  ptree->move_last[1] = GenNoCaptures( root_turn, ptree->move_last[1] );
  ptree->move_last[1] = GenDrop( root_turn, ptree->move_last[1] );
  num_root_move = (int)( ptree->move_last[1] - pmove );

  value_pre_pv = INT_MIN;
  move_best    = 0;
  value_best   = 0;
  for ( i = 0; i < num_root_move; i++ )
    {
      if ( ! ( game_status & flag_nopeek )
	   && ( game_status & ( flag_puzzling | flag_pondering ) )
	   && i != 0
	   && ( i % 8 ) == 0 )
	{
	  iret = next_cmdline( 0 );
	  if ( iret == -1 )
	    {
	      game_status |= flag_search_error;
	      return 1;
	    }
	  else if ( iret == -2 )
	    {
	      out_warning( "%s", str_error );
	      ShutdownClient;
	    }
	  else if ( game_status & flag_quit ) { return 1; }
	  else if ( iret )
	    {
	      iret = procedure( ptree );
	      if ( iret == -1 )
		{
		  game_status |= flag_search_error;
		  next_cmdline( 1 );
		  return 1;
		}
	      else if ( iret == -2 )
		{
		  out_warning( "%s", str_error );
		  next_cmdline( 1 );
		  ShutdownClient;
		}
	      else if ( iret == 1 ) { next_cmdline( 1 ); }

	      if ( game_status & ( flag_quit | flag_quit_ponder
				   | flag_suspend ) )
		{
		  return 1;
		}
	    }
	}

      value = INT_MIN;
      move  = pmove[i];

      MakeMove( root_turn, move, 1 );
      if ( ! InCheck( root_turn )
	   && ! is_move_rest( move, arestraint_list ) )
	{
	  iret = no_rep;
	  if ( InCheck(Flip(root_turn)) )
	    {
	      ptree->nsuc_check[2]
		= (unsigned char)( ptree->nsuc_check[0] + 1U );
	      if ( ptree->nsuc_check[2] >= 2 * 2 )
		{
		  iret = detect_repetition( ptree, 2, Flip(root_turn), 2 );
		}
	    }

	  if ( iret == perpetual_check ) { value = INT_MIN; }
	  else {
	    ptree->current_move[1] = move;
	    value = -search_quies( ptree, -score_bound, score_bound,
				   Flip(root_turn), 2, 1 );
	    if ( value > value_best )
	      {
		value_best = value;
		move_best  = move;
	      }
	    if ( I2IsPromote(move) ) { value++; }
	    if ( move == ptree->pv[0].a[1] )
	      {
		value_pre_pv = value;
		value        = INT_MAX;
	      }
	  }
	}
      UnMakeMove( root_turn, move, 1 );
      asort[i] = value;
    }
  if ( UToCap(move_best) ) { root_move_cap = 1; }

  /* shell sort */
  for ( k = SHELL_H_LEN - 1; k >= 0; k-- )
    {
      h = ashell_h[k];
      for ( i = num_root_move-h-1; i >= 0; i-- )
	{
	  value = asort[i];
	  move  = pmove[i];
	  for ( j = i+h; j < num_root_move && asort[j] > value; j += h )
	    {
	      asort[j-h] = asort[j];
	      pmove[j-h] = pmove[j];
	    }
	  asort[j-h] = value;
	  pmove[j-h] = move;
	}
    }

  /* discard all of moves cause mate or perpetual check */
  if ( asort[0] >= -score_max_eval )
    {
      for ( ; num_root_move; num_root_move-- )
	{
	  if ( asort[num_root_move-1] >= -score_max_eval ) { break; }
	}
    }

  /* discard perpetual checks */
  else for ( ; num_root_move; num_root_move-- )
    {
      if ( asort[num_root_move-1] != INT_MIN ) { break; }
    }

  for ( i = 0; i < num_root_move; i++ )
    {
      root_move_list[i].move   = pmove[i];
      root_move_list[i].nodes  = 0;
      root_move_list[i].status = 0;
    }
  if ( value_pre_pv != INT_MIN ) { asort[0] = value_pre_pv; }
  root_nmove = num_root_move;

  if ( num_root_move > 1 && ! ( game_status & flag_puzzling ) )
    {
      int id_easy_move = 0;

      if ( asort[0] > asort[1] + ( MT_CAP_DRAGON * 3 ) / 8 )
	{
	  id_easy_move = 3;
	  easy_min     = - ( MT_CAP_DRAGON *  4 ) / 16;
	  easy_max     =   ( MT_CAP_DRAGON * 32 ) / 16;
	  easy_abs     =   ( MT_CAP_DRAGON * 19 ) / 16;
	}
      else if ( asort[0] > asort[1] + ( MT_CAP_DRAGON * 2 ) / 8 )
	{
	  id_easy_move = 2;
	  easy_min     = - ( MT_CAP_DRAGON *  3 ) / 16;
	  easy_max     =   ( MT_CAP_DRAGON *  6 ) / 16;
	  easy_abs     =   ( MT_CAP_DRAGON *  9 ) / 16;
	}
      else if ( asort[0] > asort[1] + MT_CAP_DRAGON / 8
		&& asort[0] > - MT_CAP_DRAGON / 8
		&& I2From(pmove[0]) < nsquare )
	{
	  id_easy_move = 1;
	  easy_min     = - ( MT_CAP_DRAGON *  2 ) / 16;
	  easy_max     =   ( MT_CAP_DRAGON *  4 ) / 16;
	  easy_abs     =   ( MT_CAP_DRAGON *  6 ) / 16;
	}

      if ( easy_abs )
	{
	  Out( "\n    the root move %s looks easy (type %d).\n",
	       str_CSA_move(pmove[0]), id_easy_move );
	  Out( "    evasion:%d, capture:%d, promotion:%d, drop:%d, "
	       "value:%5d - %5d\n",
	       ptree->nsuc_check[1]        ? 1 : 0,
	       UToCap(pmove[0])            ? 1 : 0,
	       I2IsPromote(pmove[0])       ? 1 : 0,
	       I2From(pmove[0]) >= nsquare ? 1 : 0,
	       asort[0], asort[1] );
	  easy_value = asort[0];
	}
    }

  return asort[0];
}


static int
read_rest_list( tree_t * restrict ptree, unsigned int * restrict pmove_list )
{
  FILE *pf;
  int iret, imove;
  char a[65536];

  pf = file_open( "restraint.dat", "r" );
  if ( pf == NULL ) { return -2; }

  for ( imove = 0; imove < MAX_LEGAL_MOVES; imove++ )
    {
#if defined(_MSC_VER)
      iret = fscanf_s( pf, "%s\n", a, 65536 );
#else
      iret = fscanf( pf, "%s\n", a );
#endif
      if ( iret != 1 ) { break; }
      iret = interpret_CSA_move( ptree, pmove_list+imove, a );
      if ( iret < 0 )
	{
	  file_close( pf );
	  return iret;
	}
    }

  pmove_list[imove] = 0;

  return file_close( pf );
}


static int
is_move_rest( unsigned int move,
	      const unsigned int * restrict pmove_restraint )
{
  while ( *pmove_restraint )
    {
      if ( move == *pmove_restraint++ ) { return 1; }
    }

  return 0;
}
