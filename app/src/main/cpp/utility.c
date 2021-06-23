#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shogi.h"
#include "shogi_jni.h"

int
ini_game( tree_t * restrict ptree, const min_posi_t *pmin_posi, int flag,
	  const char *str_name1, const char *str_name2 )
{
  bitboard_t bb;
  int piece;
  int sq, iret;

  if ( flag & flag_history )
    {
      iret = open_history( str_name1, str_name2 );
      if ( iret < 0 ) { return iret; }
    }

  if ( ! ( flag & flag_nofmargin ) )
    {
      fmg_misc      = FMG_MISC;
      fmg_cap       = FMG_CAP;
      fmg_drop      = FMG_DROP;
      fmg_mt        = FMG_MT;
      fmg_misc_king = FMG_MISC_KING;
      fmg_cap_king  = FMG_CAP_KING;
    }

  memcpy( ptree->posi.asquare, pmin_posi->asquare, nsquare );
  ptree->move_last[0]  = ptree->amove;
  ptree->nsuc_check[0] = 0;
  ptree->nsuc_check[1] = 0;
  root_nrep            = 0;
  root_turn            = pmin_posi->turn_to_move;
  HAND_B               = pmin_posi->hand_black;
  HAND_W               = pmin_posi->hand_white;
  MATERIAL             = 0;

  BBIni( BB_BOCCUPY );
  BBIni( BB_BPAWN );
  BBIni( BB_BLANCE );
  BBIni( BB_BKNIGHT );
  BBIni( BB_BSILVER );
  BBIni( BB_BGOLD );
  BBIni( BB_BBISHOP );
  BBIni( BB_BROOK );
  BBIni( BB_BPRO_PAWN );
  BBIni( BB_BPRO_LANCE );
  BBIni( BB_BPRO_KNIGHT );
  BBIni( BB_BPRO_SILVER );
  BBIni( BB_BHORSE );
  BBIni( BB_BDRAGON );
  BBIni( BB_BTGOLD );
  BBIni( BB_WOCCUPY );
  BBIni( BB_WPAWN );
  BBIni( BB_WLANCE );
  BBIni( BB_WKNIGHT );
  BBIni( BB_WSILVER );
  BBIni( BB_WGOLD );
  BBIni( BB_WBISHOP );
  BBIni( BB_WROOK );
  BBIni( BB_WPRO_PAWN );
  BBIni( BB_WPRO_LANCE );
  BBIni( BB_WPRO_KNIGHT );
  BBIni( BB_WPRO_SILVER );
  BBIni( BB_WHORSE );
  BBIni( BB_WDRAGON );
  BBIni( BB_WTGOLD );
  BBIni( OCCUPIED_FILE );
  BBIni( OCCUPIED_DIAG1 );
  BBIni( OCCUPIED_DIAG2 );

  for ( sq = 0; sq < nsquare; sq++ ) {
    piece = BOARD[sq];
    if ( piece > 0 ) {
      Xor( sq, BB_BOCCUPY );
      XorFile( sq, OCCUPIED_FILE );
      XorDiag1( sq, OCCUPIED_DIAG1 );
      XorDiag2( sq, OCCUPIED_DIAG2 );
      switch ( piece )
	{
	case pawn:        Xor( sq, BB_BPAWN );        break;
	case lance:       Xor( sq, BB_BLANCE );       break;
	case knight:      Xor( sq, BB_BKNIGHT );      break;
	case silver:      Xor( sq, BB_BSILVER );      break;
	case rook:        Xor( sq, BB_BROOK );        break;
	case bishop:      Xor( sq, BB_BBISHOP );      break;
	case king:	  SQ_BKING = (char)sq;        break;
	case dragon:      Xor( sq, BB_BDRAGON );      break;
	case horse:       Xor( sq, BB_BHORSE );       break;
	case gold:	  Xor( sq, BB_BGOLD );        break;
	case pro_pawn:	  Xor( sq, BB_BPRO_PAWN );    break;
	case pro_lance:	  Xor( sq, BB_BPRO_LANCE );   break;
	case pro_knight:  Xor( sq, BB_BPRO_KNIGHT );  break;
	case pro_silver:  Xor( sq, BB_BPRO_SILVER );  break;
	}
    }
    else if ( piece < 0 ) {
      Xor( sq, BB_WOCCUPY );
      XorFile( sq, OCCUPIED_FILE );
      XorDiag1( sq, OCCUPIED_DIAG1 );
      XorDiag2( sq, OCCUPIED_DIAG2 );
      switch ( - piece )
	{
	case pawn:        Xor( sq, BB_WPAWN );        break;
	case lance:       Xor( sq, BB_WLANCE );       break;
	case knight:      Xor( sq, BB_WKNIGHT );      break;
	case silver:      Xor( sq, BB_WSILVER );      break;
	case rook:        Xor( sq, BB_WROOK );        break;
	case bishop:      Xor( sq, BB_WBISHOP );      break;
	case king:	  SQ_WKING = (char)sq;        break;
	case dragon:      Xor( sq, BB_WDRAGON );      break;
	case horse:       Xor( sq, BB_WHORSE );       break;
	case gold:        Xor( sq, BB_WGOLD );        break;
	case pro_pawn:    Xor( sq, BB_WPRO_PAWN );    break;
	case pro_lance:   Xor( sq, BB_WPRO_LANCE );   break;
	case pro_knight:  Xor( sq, BB_WPRO_KNIGHT );  break;
	case pro_silver:  Xor( sq, BB_WPRO_SILVER );  break;
	}
    }
  }

  BBOr( BB_BTGOLD, BB_BPRO_PAWN,   BB_BGOLD );
  BBOr( BB_BTGOLD, BB_BPRO_LANCE,  BB_BTGOLD );
  BBOr( BB_BTGOLD, BB_BPRO_KNIGHT, BB_BTGOLD );
  BBOr( BB_BTGOLD, BB_BPRO_SILVER, BB_BTGOLD );
  BBOr( BB_B_HDK,  BB_BHORSE,      BB_BDRAGON );
  BBOr( BB_B_HDK,  BB_BKING,       BB_B_HDK );
  BBOr( BB_B_BH,   BB_BBISHOP,     BB_BHORSE );
  BBOr( BB_B_RD,   BB_BROOK,       BB_BDRAGON );

  BBOr( BB_WTGOLD, BB_WPRO_PAWN,   BB_WGOLD );
  BBOr( BB_WTGOLD, BB_WPRO_LANCE,  BB_WTGOLD );
  BBOr( BB_WTGOLD, BB_WPRO_KNIGHT, BB_WTGOLD );
  BBOr( BB_WTGOLD, BB_WPRO_SILVER, BB_WTGOLD );
  BBOr( BB_W_HDK,  BB_WHORSE,      BB_WDRAGON );
  BBOr( BB_W_HDK,  BB_WKING,       BB_W_HDK );
  BBOr( BB_W_BH,   BB_WBISHOP,     BB_WHORSE );
  BBOr( BB_W_RD,   BB_WROOK,       BB_WDRAGON );

  BB_BPAWN_ATK.p[0]  = ( BB_BPAWN.p[0] <<  9 ) & 0x7ffffffU;
  BB_BPAWN_ATK.p[0] |= ( BB_BPAWN.p[1] >> 18 ) & 0x00001ffU;
  BB_BPAWN_ATK.p[1]  = ( BB_BPAWN.p[1] <<  9 ) & 0x7ffffffU;
  BB_BPAWN_ATK.p[1] |= ( BB_BPAWN.p[2] >> 18 ) & 0x00001ffU;
  BB_BPAWN_ATK.p[2]  = ( BB_BPAWN.p[2] <<  9 ) & 0x7ffffffU;

  BB_WPAWN_ATK.p[2]  = ( BB_WPAWN.p[2] >>  9 );
  BB_WPAWN_ATK.p[2] |= ( BB_WPAWN.p[1] << 18 ) & 0x7fc0000U;
  BB_WPAWN_ATK.p[1]  = ( BB_WPAWN.p[1] >>  9 );
  BB_WPAWN_ATK.p[1] |= ( BB_WPAWN.p[0] << 18 ) & 0x7fc0000U;
  BB_WPAWN_ATK.p[0]  = ( BB_WPAWN.p[0] >>  9 );

  MATERIAL = eval_material( ptree );
  HASH_KEY = hash_func( ptree );

  memset( ptree->history,         0, sizeof(ptree->history) );
  memset( large_object->hash_rejections_parent, 0, sizeof(large_object->hash_rejections_parent) );
  memset( large_object->hash_rejections,        0, sizeof(large_object->hash_rejections) );

  game_status         &= ( flag_quiet | flag_reverse | flag_narrow_book
			   | flag_time_extendable | flag_learning
			   | flag_nobeep | flag_nostress | flag_nopeek
			   | flag_noponder );
  sec_b_total     = 0;
  sec_w_total     = 0;
  sec_elapsed     = 0;
  last_root_value = 0;
  n_nobook_move   = 0;
  last_pv.depth   = 0;
  last_pv.length  = 0;
  last_pv.a[0]    = 0;
  last_pv.a[1]    = 0;

  if ( InCheck( root_turn ) )
    {
      ptree->nsuc_check[1] = 1U;
      if ( is_mate( ptree, 1 ) ) { game_status |= flag_mated; }
    }

  BBOr( bb, BB_BPAWN, BB_WPAWN );
  BBOr( bb, bb, BB_BPRO_PAWN );
  BBOr( bb, bb, BB_WPRO_PAWN );
  npawn_box  = npawn_max;
  npawn_box -= PopuCount( bb );
  npawn_box -= (int)I2HandPawn(HAND_B);
  npawn_box -= (int)I2HandPawn(HAND_W);

  BBOr( bb, BB_BLANCE, BB_WLANCE );
  BBOr( bb, bb, BB_BPRO_LANCE );
  BBOr( bb, bb, BB_WPRO_LANCE );
  nlance_box  = nlance_max;
  nlance_box -= PopuCount( bb );
  nlance_box -= (int)I2HandLance(HAND_B);
  nlance_box -= (int)I2HandLance(HAND_W);

  BBOr( bb, BB_BKNIGHT, BB_WKNIGHT );
  BBOr( bb, bb, BB_BPRO_KNIGHT );
  BBOr( bb, bb, BB_WPRO_KNIGHT );
  nknight_box  = nknight_max;
  nknight_box -= PopuCount( bb );
  nknight_box -= (int)I2HandKnight(HAND_B);
  nknight_box -= (int)I2HandKnight(HAND_W);

  BBOr( bb, BB_BSILVER, BB_WSILVER );
  BBOr( bb, bb, BB_BPRO_SILVER );
  BBOr( bb, bb, BB_WPRO_SILVER );
  nsilver_box  = nsilver_max;
  nsilver_box -= PopuCount( bb );
  nsilver_box -= (int)I2HandSilver(HAND_B);
  nsilver_box -= (int)I2HandSilver(HAND_W);

  BBOr( bb, BB_BGOLD, BB_WGOLD );
  ngold_box  = ngold_max;
  ngold_box -= PopuCount( bb );
  ngold_box -= (int)I2HandGold(HAND_B);
  ngold_box -= (int)I2HandGold(HAND_W);

  BBOr( bb, BB_BBISHOP, BB_WBISHOP );
  BBOr( bb, bb, BB_BHORSE );
  BBOr( bb, bb, BB_WHORSE );
  nbishop_box  = nbishop_max;
  nbishop_box -= PopuCount( bb );
  nbishop_box -= (int)I2HandBishop(HAND_B);
  nbishop_box -= (int)I2HandBishop(HAND_W);

  BBOr( bb, BB_BROOK, BB_WROOK );
  BBOr( bb, bb, BB_BDRAGON );
  BBOr( bb, bb, BB_WDRAGON );
  nrook_box  = nrook_max;
  nrook_box -= PopuCount( bb );
  nrook_box -= (int)I2HandRook(HAND_B);
  nrook_box -= (int)I2HandRook(HAND_W);

  iret = exam_tree( ptree );
  if ( iret < 0 )
    {
      ini_game( ptree, &min_posi_no_handicap, 0, NULL, NULL );
      return iret;
    }

  return 1;
}


int
gen_legal_moves( tree_t * restrict ptree, unsigned int *p0 )
{
  unsigned int *p1;
  int i, j, n;

  p1 = GenCaptures( root_turn, p0 );
  p1 = GenNoCaptures( root_turn, p1 );
  p1 = GenCapNoProEx2( root_turn, p1 );
  p1 = GenNoCapNoProEx2( root_turn, p1 );
  p1 = GenDrop( root_turn, p1 );
  n  = (int)( p1 - p0 );

  for ( i = 0; i < n; i++ )
    {
      MakeMove( root_turn, p0[i], 1 );
      if ( InCheck( root_turn ) )
	{
	  UnMakeMove( root_turn, p0[i], 1 );
	  p0[i] = 0;
	  continue;
	}
      if ( InCheck(Flip(root_turn)) )
	{
	  ptree->nsuc_check[2] = (unsigned char)( ptree->nsuc_check[0] + 1U );
	  if ( ptree->nsuc_check[2] >= 6U
	       && ( detect_repetition( ptree, 2, Flip(root_turn), 3 )
		    == perpetual_check ) )
	    {
	      UnMakeMove( root_turn, p0[i], 1 );
	      p0[i] = 0;
	      continue;
	    }
	}
      UnMakeMove( root_turn, p0[i], 1 );
    }

  for ( i = 0; i < n; )
    {
      if ( ! p0[i] )
	{
	  for ( j = i+1; j < n; j++ ) { p0[j-1] = p0[j]; }
	  n -= 1;
	}
      else { i++; }
    }

  return n;
}


/*
  - detection of perpetual check is omitted.
  - weak moves are omitted.
*/
int
is_mate( tree_t * restrict ptree, int ply )
{
  int iret = 0;

  assert( InCheck(root_turn) );

  ptree->move_last[ply] = GenEvasion( root_turn, ptree->move_last[ply-1] );
  if ( ptree->move_last[ply] == ptree->move_last[ply-1] ) { iret = 1; }

  return iret;
}


int
is_hand_eq_supe( unsigned int u, unsigned int uref )
{
#if 1
/* aggressive superior correspondences are applied, that is:
 *   pawn  <= lance, silver, gold, rook
 *   lance <= rook.
 */
  int nsupe;

  if ( IsHandKnight(u) < IsHandKnight(uref)
       || IsHandSilver(u) < IsHandSilver(uref)
       || IsHandGold(u)   < IsHandGold(uref)
       || IsHandBishop(u) < IsHandBishop(uref)
       || IsHandRook(u)   < IsHandRook(uref) ) { return 0; }

  nsupe  = (int)I2HandRook(u)  - (int)I2HandRook(uref);
  nsupe += (int)I2HandLance(u) - (int)I2HandLance(uref);
  if ( nsupe < 0 ) { return 0; }

  nsupe += (int)I2HandSilver(u) - (int)I2HandSilver(uref);
  nsupe += (int)I2HandGold(u)   - (int)I2HandGold(uref);
  nsupe += (int)I2HandPawn(u)   - (int)I2HandPawn(uref);
  if ( nsupe < 0 ) { return 0; }

  return 1;
#else
  if ( IsHandPawn(u) >= IsHandPawn(uref)
       && IsHandLance(u)  >= IsHandLance(uref)
       && IsHandKnight(u) >= IsHandKnight(uref)
       && IsHandSilver(u) >= IsHandSilver(uref)
       && IsHandGold(u)   >= IsHandGold(uref)
       && IsHandBishop(u) >= IsHandBishop(uref)
       && IsHandRook(u)   >= IsHandRook(uref) ) { return 1; }

  return 0;
#endif
}


/* weak moves are omitted. */
int
detect_repetition( tree_t * restrict ptree, int ply, int turn, int nth )
{
  const unsigned int *p;
  unsigned int hand1, hand2;
  int n, i, imin, counter, irep, ncheck;

  ncheck = (int)ptree->nsuc_check[ply];
  n      = root_nrep + ply - 1;

  /*if ( ncheck >= 6 )*/
  if ( ncheck >= nth * 2 )
    {
      /* imin = n - ncheck*2; */
      imin = n - ncheck*2 + 1;
      if ( imin < 0 ) { imin = 0; }

      ptree->move_last[ply] = GenEvasion( turn, ptree->move_last[ply-1] );
      for ( p = ptree->move_last[ply-1]; p < ptree->move_last[ply]; p++ )
	{
	  MakeMove( turn, *p, ply );

	  /* for ( i = n-1, counter = 0; i >= imin; i -= 2 ) */
	  for ( i = n-3, counter = 0; i >= imin; i -= 2 )
	    {
	      if ( ptree->rep_board_list[i] == HASH_KEY
		   && ptree->rep_hand_list[i] == HAND_B
		   && ++counter == nth )
		   /* && ncheck*2 - 1 >= n - i )*/
		{
		  UnMakeMove( turn, *p, ply );
		  move_evasion_pchk = *p;
		  return perpetual_check;
		}
	    }
	  UnMakeMove( turn, *p, ply );
	}
    }

  irep = no_rep;
  for ( i = n-4, counter = 0; i >= 0; i-- )
    {
      if ( ptree->rep_board_list[i] == HASH_KEY )
	{
	  hand1 = HAND_B;
	  hand2 = ptree->rep_hand_list[i];

	  if ( (n-i) & 1 )
	    {
	      if ( irep == no_rep )
		{
		  if ( turn )
		    {
		      if ( is_hand_eq_supe( hand2, hand1 ) )
			{
			  irep = white_superi_rep;
			}
		    }
		  else if ( is_hand_eq_supe( hand1, hand2 ) )
		    {
		      irep = black_superi_rep;
		    }
		}
	    }
	  else if ( hand1 == hand2 )
	    {
	      if ( ++counter == nth )
		{
		  if ( (ncheck-1)*2 >= n - i ) { return perpetual_check; }
		  else                         { return four_fold_rep; }
		}
	    }
	  else if ( irep == no_rep )
	    {
	      if ( is_hand_eq_supe( hand1, hand2 ) )
		{
		  irep = black_superi_rep;
		}
	      else if ( is_hand_eq_supe( hand2, hand1 ) )
		{
		  irep = white_superi_rep;
		}
	    }
	}
    }

  return irep;
}


int
com_turn_start( tree_t * restrict ptree, int flag )
{
  const char *str_move;
  unsigned int move, sec_total;
  int iret, is_resign, value, ply;

  if ( ! ( flag & flag_from_ponder ) )
    {
      assert( ! ( game_status & mask_game_end ) );

      time_start = time_turn_start;

      game_status |=  flag_thinking;
      iret         = iterate( ptree, flag );
      game_status &= ~flag_thinking;
      if ( iret < 0 ) { return iret; }
    }
  if ( game_status & flag_suspend ) { return 1; }

  move     = last_pv.a[1];
  value    = root_turn ? -last_root_value : last_root_value;
  str_move = str_CSA_move( move );

  // LOG_DEBUG("CompX: %x %s", move, str_move);

  if ( value < -resign_threshold && last_pv.type != pv_fail_high )
    {
#if defined(DEKUNOBOU)
      if ( dek_ngame )
	{
	  dek_lost += 1;
	  Out( "Bonanza lost against Dekunobou\n" );
	}
#endif
      is_resign = 1;
    }
  else {
    is_resign = 0;

#if defined(DEKUNOBOU)
    if ( dek_ngame && ! is_resign
	 && value > ( MT_CAP_DRAGON * 3 ) / 2
	 && value > resign_threshold
	 && value != score_inferior )
      {
	is_resign = 1;
	dek_win  += 1;
	Out( "Bonanza won against Dekunobou.\n" );
      }
    if ( dek_ngame && ! is_resign && value == -score_draw )
      {
	iret = make_move_root( ptree, move, ( flag_rep | flag_nomake_move ) );
	if ( iret < 0 )
	  {
	    Out( "%s\n\n", str_move );
	    return iret;
	  }
	else if ( iret == 2 )
	  {
	    is_resign = 1;
	    Out( "The game with Dekunobou is drawn.\n" );
	  }
      }
    if ( dek_ngame && ! is_resign && record_game.moves > 255 )
      {
	is_resign = 1;
	Out( "The game with Dekunobou is interrupted...\n" );
      }
#endif
  }

#if defined(DBG_EASY)
  if ( easy_move && easy_move != move )
    {
      out_warning( "EASY MOVE DITECTION FAILED." );
    }
#endif

  /* send urgent outputs */
  if ( is_resign )
    {
#if defined(CSA_LAN)
      if ( sckt_csa != SCKT_NULL )
	{
	  iret = sckt_out( sckt_csa, "%%TORYO\n" );
	  if ( iret < 0 ) { return iret; }
	}
#endif
      OutCsaShogi( "resign\n" );
      OutDek( "%%TORYO\n" );
    }
  else {
#if defined(CSA_LAN)
    if ( sckt_csa != SCKT_NULL )
      {
	iret = sckt_out( sckt_csa, "%c%s\n", ach_turn[root_turn], str_move );
	if ( iret < 0 ) { return iret; }
      }
#endif

    OutCsaShogi( "move%s\n", str_move );
    OutDek( "%c%s\n", ach_turn[root_turn], str_move );
  }
  OutBeep();

  /* learning and stuff */;
  ply = record_game.moves;
  if ( ply < HASH_REG_HIST_LEN )
    {
      history_book_learn[ply].data             &= ~( (1U<<31) | 0xffffU );
      history_book_learn[ply].data             |= (unsigned int)(value+32768);
      history_book_learn[ply].move_responsible  = move;
      history_book_learn[ply].key_responsible   = (unsigned int)HASH_KEY;
      history_book_learn[ply].hand_responsible  = (unsigned int)HAND_B;
    }

  iret = hash_learn( ptree, move, value, iteration_depth - 1 );
  if ( iret < 0 ) { return iret; }

  /* show search result and make a move */
  if ( is_resign )
    {
      show_prompt();
      game_status |= flag_resigned;
      renovate_time( root_turn );
      out_CSA( ptree, &record_game, MOVE_RESIGN );
      sec_total = root_turn ? sec_w_total : sec_b_total;
      str_move  = "resign";
    }
  else {
    show_prompt();
    iret = make_move_root( ptree, move,
			   ( flag_rep | flag_time | flag_history
			     | flag_rejections ) );
    if ( iret < 0 )
      {
	Out( "%s\n\n", str_move );
	return iret;
      }
    sec_total = root_turn ? sec_b_total : sec_w_total;
  }

  OutCsaShogi( "info tt %03u:%02u\n", sec_total / 60U, sec_total % 60U );
  Out( "%s '(%d%s) %03u:%02u/%03u:%02u  elapsed: b%u, w%u\n",
       str_move, value,
       ( last_pv.type == pv_fail_high ) ? "!" : "",
       sec_elapsed / 60U, sec_elapsed % 60U,
       sec_total   / 60U, sec_total   % 60U,
       sec_b_total, sec_w_total );

  if ( ! is_resign )
    {
#if ! defined(NO_STDOUT)
      iret = out_board( ptree, stdout, move, 0 );
      if ( iret < 0 ) { return iret; }
#endif
    }

  return 1;
}


void *
memory_alloc( size_t nbytes )
{
#if defined(_WIN32)
  void *p = VirtualAlloc( NULL, nbytes, MEM_COMMIT, PAGE_READWRITE );
  if ( p == NULL ) { str_error = "VirturlAlloc() faild"; }
#else
  void *p = malloc( nbytes );
  if ( p == NULL ) { str_error = "malloc() faild"; }
#endif
  return p;
}


int
memory_free( void *p )
{
#if defined(_WIN32)
  if ( VirtualFree( p, 0, MEM_RELEASE ) ) { return 1; }
  str_error = "VirtualFree() faild";
  return -2;
#else
  free( p );
  return 1;
#endif
}
