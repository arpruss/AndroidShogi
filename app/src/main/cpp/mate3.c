#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "shogi.h"

enum { mate_king_cap_checker = 0,
       mate_cap_checker_gen,
       mate_cap_checker,
       mate_king_cap_gen,
       mate_king_cap,
       mate_king_move_gen,
       mate_king_move,
       mate_intercept_move_gen,
       mate_intercept_move,
       mate_intercept_weak_move };

static int mate3_and( tree_t * restrict ptree, int turn, int ply );
static void checker( const tree_t * restrict ptree, char *psq, int turn );
static unsigned int gen_king_cap_checker( const tree_t * restrict ptree,
					  int to, int turn );
static int mate_weak_or( tree_t * restrict ptree, int turn, int ply,
			 int from, int to );
static unsigned int *gen_move_to( const tree_t * restrict ptree, int sq,
				  int turn, unsigned int * restrict pmove );
static unsigned int *gen_king_move( const tree_t * restrict ptree,
				    const char *psq, int turn, int is_capture,
				    unsigned int * restrict pmove );
static unsigned int *gen_intercept( tree_t * restrict __ptree__,
				    int sq_checker, int turn,
				    int * restrict premaining,
				    unsigned int * restrict pmove );
static int gen_next_evasion_mate( tree_t * restrict ptree, const char *psq,
				  int ply, int turn );

unsigned int
is_mate_in3ply( tree_t * restrict ptree, int turn, int ply )
{
  int value;

  if ( ply >= PLY_MAX-2 ) { return 0; }

  ptree->anext_move[ply].move_last = ptree->move_last[ply-1];
  ptree->move_last[ply] = GenCheck( turn, ptree->move_last[ply-1] );

  while ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
    {
      MOVE_CURR = *ptree->anext_move[ply].move_last++;

      assert( is_move_valid( ptree, MOVE_CURR, turn ) );
      MakeMove( turn, MOVE_CURR, ply );
      if ( InCheck(turn) )
	{
	  UnMakeMove( turn, MOVE_CURR, ply );
	  continue;
	}

      value = mate3_and( ptree, Flip(turn), ply+1 );
      
      UnMakeMove( turn, MOVE_CURR, ply );

      if ( value ) { return 1; }
    }

  return 0;
}


static int
mate3_and( tree_t * restrict ptree, int turn, int ply )
{
  unsigned int move;
  char asq[2];

  assert( InCheck(turn) );

  ptree->anext_move[ply].next_phase = mate_king_cap_checker;
  checker( ptree, asq, turn );

  while ( gen_next_evasion_mate( ptree, asq, ply, turn ) )
    {
      MakeMove( turn, MOVE_CURR, ply );
      assert( ! InCheck(turn) );

      if ( ptree->anext_move[ply].next_phase == mate_intercept_weak_move )
	{
	  assert( asq[1] == nsquare );
	  move = (unsigned int)mate_weak_or( ptree, Flip(turn), ply+1, asq[0],
					     I2To(MOVE_CURR) );
	}
      else { move = InCheck( Flip(turn) ) ? 0 : IsMateIn1Ply( Flip(turn) ); }

      UnMakeMove( turn, MOVE_CURR, ply );
      
      if ( ! move ) { return 0; }
    }
  
  return 1;
}


static int
mate_weak_or( tree_t * restrict ptree, int turn, int ply, int from,
	      int to )
{
  int direc, pc, pc_cap, value;

  if ( ply >= PLY_MAX-2 ) { return 0; }
  
  if ( turn )
    {
      direc = (int)adirec[SQ_WKING][from];
      if ( direc
	   && is_pinned_on_white_king( ptree, from, direc ) ) { return 0; }

      pc     = -BOARD[from];
      pc_cap =  BOARD[to];
      MOVE_CURR = ( To2Move(to) | From2Move(from)
		      | Piece2Move(pc) | Cap2Move(pc_cap) );
      if ( ( pc == bishop || pc == rook )
	   && ( to > I4 || from > I4 ) ) { MOVE_CURR |= FLAG_PROMO; }
    }
  else {
    direc = (int)adirec[SQ_BKING][from];
    if ( direc
	 && is_pinned_on_black_king( ptree, from, direc ) ) { return 0; }
    
    pc     =  BOARD[from];
    pc_cap = -BOARD[to];
    MOVE_CURR = ( To2Move(to) | From2Move(from) | Piece2Move(pc)
		    | Cap2Move(pc_cap) );
    if ( ( pc == bishop || pc == rook )
	 && ( to < A6 || from < A6 ) ) { MOVE_CURR |= FLAG_PROMO; }
  }

  MakeMove( turn, MOVE_CURR, ply );
  if ( InCheck(turn) )
    {
      UnMakeMove( turn, MOVE_CURR, ply );
      return 0;
    }
  
  ptree->move_last[ply] = ptree->move_last[ply-1];
  value = mate3_and( ptree, Flip(turn), ply+1 );
  
  UnMakeMove( turn, MOVE_CURR, ply );

  return value;
}


static int
gen_next_evasion_mate( tree_t * restrict ptree, const char *psq, int ply,
		       int turn )
{
  switch ( ptree->anext_move[ply].next_phase )
    {
    case mate_king_cap_checker:
      ptree->anext_move[ply].next_phase = mate_cap_checker_gen;
      MOVE_CURR = gen_king_cap_checker( ptree, psq[0], turn );
      if ( MOVE_CURR ) { return 1; }

    case mate_cap_checker_gen:
      ptree->anext_move[ply].next_phase = mate_cap_checker;
      ptree->anext_move[ply].move_last	= ptree->move_last[ply-1];
      ptree->move_last[ply]             = ptree->move_last[ply-1];
      if ( psq[1] == nsquare )
	{
	  ptree->move_last[ply]
	    = gen_move_to( ptree, psq[0], turn, ptree->move_last[ply-1] );
	}

    case mate_cap_checker:
      if ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
	{
	  MOVE_CURR = *(ptree->anext_move[ply].move_last++);
	  return 1;
	}

    case mate_king_cap_gen:
      ptree->anext_move[ply].next_phase = mate_king_cap;
      ptree->anext_move[ply].move_last  = ptree->move_last[ply-1];
      ptree->move_last[ply]
	= gen_king_move( ptree, psq, turn, 1, ptree->move_last[ply-1] );

    case mate_king_cap:
      if ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
	{
	  MOVE_CURR = *(ptree->anext_move[ply].move_last++);
	  return 1;
	}

    case mate_king_move_gen:
      ptree->anext_move[ply].next_phase = mate_king_move;
      ptree->anext_move[ply].move_last  = ptree->move_last[ply-1];
      ptree->move_last[ply]
	= gen_king_move( ptree, psq, turn, 0, ptree->move_last[ply-1] );

    case mate_king_move:
      if ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
	{
	  MOVE_CURR = *(ptree->anext_move[ply].move_last++);
	  return 1;
	}

    case mate_intercept_move_gen:
      ptree->anext_move[ply].remaining  = 0;
      ptree->anext_move[ply].next_phase = mate_intercept_move;
      ptree->anext_move[ply].move_last  = ptree->move_last[ply-1];
      ptree->move_last[ply]             = ptree->move_last[ply-1];
      if ( psq[1] == nsquare && abs(BOARD[(int)psq[0]]) != knight  )
	{
	  int n;
	  ptree->move_last[ply] = gen_intercept( ptree, psq[0], turn, &n,
						 ptree->move_last[ply-1] );
	  ptree->anext_move[ply].remaining = n;
	}

    case mate_intercept_move:
      if ( ptree->anext_move[ply].remaining-- )
	{
	  MOVE_CURR = *(ptree->anext_move[ply].move_last++);
	  return 1;
	}
      ptree->anext_move[ply].next_phase = mate_intercept_weak_move;

    case mate_intercept_weak_move:
      if ( ptree->anext_move[ply].move_last != ptree->move_last[ply] )
	{
	  MOVE_CURR = *(ptree->anext_move[ply].move_last++);
	  return 1;
	}
      break;

    default:
      assert( 0 );
    }

  return 0;
}


static void
checker( const tree_t * restrict ptree, char *psq, int turn )
{
  bitboard_t bb, bb_checkers;
  int n, sq0, sq1, sq_king;

  if ( turn )
    {
      sq_king     = SQ_WKING;
      bb_checkers = BB_BOCCUPY;
    }
  else {
    sq_king     = SQ_BKING;
    bb_checkers = BB_WOCCUPY;
  }
  bb = attacks_to_piece( ptree, sq_king );
  BBAnd( bb, bb, bb_checkers );

  assert( BBToU(bb) );
  sq0 = LastOne( bb );
  sq1 = nsquare;

  Xor( sq0, bb );
  if ( BBToU( bb ) )
    {
      sq1 = LastOne( bb );
      if ( BBContract( abb_king_attacks[sq_king], abb_mask[sq1] ) )
	{
	  n   = sq0;
	  sq0 = sq1;
	  sq1 = n;
	}
    }

  psq[0] = (char)sq0;
  psq[1] = (char)sq1;
}


static unsigned int
gen_king_cap_checker( const tree_t * restrict ptree, int to, int turn )
{
  unsigned int move;
  int from;

  if ( turn )
    {
      from = SQ_WKING;
      if ( ! BBContract( abb_king_attacks[from],
			 abb_mask[to] ) )   { return 0;}
      if ( is_white_attacked( ptree, to ) ) { return 0; }
      move = Cap2Move(BOARD[to]);
    }
  else {
    from = SQ_BKING;
    if ( ! BBContract( abb_king_attacks[from],
		       abb_mask[to] ) )   { return 0;}
    if ( is_black_attacked( ptree, to ) ) { return 0; }
    move = Cap2Move(-BOARD[to]);
  }
  move |= To2Move(to) | From2Move(from) | Piece2Move(king);

  return move;
}


static unsigned int *
gen_move_to( const tree_t * restrict ptree, int to, int turn,
	     unsigned int * restrict pmove )
{
  bitboard_t bb;
  int direc, from, pc, flag_promo, flag_unpromo;

  bb = attacks_to_piece( ptree, to );
  if ( turn )
    {
      BBAnd( bb, bb, BB_WOCCUPY );
      BBNotAnd( bb, abb_mask[SQ_WKING] );
      while ( BBToU(bb) )
	{
	  from = LastOne( bb );
	  Xor( from, bb );

	  direc = (int)adirec[SQ_WKING][from];
	  if ( direc && is_pinned_on_white_king( ptree, from, direc ) )
	    {
	      continue;
	    }

	  flag_promo   = 0;
	  flag_unpromo = 1;
	  pc           = -BOARD[from];
	  switch ( pc )
	    {
	    case pawn:
	      if ( to > I4 ) { flag_promo = 1;  flag_unpromo = 0; }
	      break;

	    case lance:	 case knight:
	      if      ( to > I3 ) { flag_promo = 1;  flag_unpromo = 0; }
	      else if ( to > I4 ) { flag_promo = 1; }
	      break;

	    case silver:
	      if ( to > I4 || from > I4 ) { flag_promo = 1; }
	      break;

	    case bishop:  case rook:
	      if ( to > I4
		   || from > I4 ) { flag_promo = 1;  flag_unpromo = 0; }
	      break;

	    default:
	      break;
	    }
	  assert( flag_promo || flag_unpromo );
	  if ( flag_promo )
	    {
	      *pmove++ = ( From2Move(from) | To2Move(to) | FLAG_PROMO
			   | Piece2Move(pc) | Cap2Move(BOARD[to]) );
	    }
	  if ( flag_unpromo )
	    {
	      *pmove++ = ( From2Move(from) | To2Move(to)
			   | Piece2Move(pc) | Cap2Move(BOARD[to]) );
	    }
	}
    }
  else {
    BBAnd( bb, bb, BB_BOCCUPY );
    BBNotAnd( bb, abb_mask[SQ_BKING] );
    while ( BBToU(bb) )
      {
	from = FirstOne( bb );
	Xor( from, bb );
	
	direc = (int)adirec[SQ_BKING][from];
	if ( direc && is_pinned_on_black_king( ptree, from, direc ) )
	  {
	    continue;
	  }

	flag_promo   = 0;
	flag_unpromo = 1;
	pc           = BOARD[from];
	switch ( pc )
	  {
	  case pawn:
	    if ( to < A6 ) { flag_promo = 1;  flag_unpromo = 0; }
	    break;
	    
	  case lance:  case knight:
	    if      ( to < A7 ) { flag_promo = 1;  flag_unpromo = 0; }
	    else if ( to < A6 ) { flag_promo = 1; }
	    break;
	    
	  case silver:
	    if ( to < A6 || from < A6 ) { flag_promo = 1; }
	    break;
	    
	  case bishop:  case rook:
	    if ( to < A6
		 || from < A6 ) { flag_promo = 1;  flag_unpromo = 0; }
	    break;
	    
	  default:
	    break;
	  }
	assert( flag_promo || flag_unpromo );
	if ( flag_promo )
	  {
	    *pmove++ = ( From2Move(from) | To2Move(to) | FLAG_PROMO
			 | Piece2Move(pc) | Cap2Move(-BOARD[to]) );
	  }
	if ( flag_unpromo )
	  {
	    *pmove++ = ( From2Move(from) | To2Move(to)
			 | Piece2Move(pc) | Cap2Move(-BOARD[to]) );
	  }
      }
  }

  return pmove;
}


static unsigned int *
gen_king_move( const tree_t * restrict ptree, const char *psq, int turn,
	       int is_capture, unsigned int * restrict pmove )
{
  bitboard_t bb;
  int to, from;

  if ( turn )
    {
      from = SQ_WKING;
      bb      = abb_king_attacks[from];
      if ( is_capture )
	{
	  BBAnd( bb, bb, BB_BOCCUPY );
	  BBNotAnd( bb, abb_mask[(int)psq[0]] );
	}
      else { BBNotAnd( bb, BB_BOCCUPY ); }
      BBNotAnd( bb, BB_WOCCUPY );
    }
  else {
    from = SQ_BKING;
    bb      = abb_king_attacks[from];
    if ( is_capture )
      {
	BBAnd( bb, bb, BB_WOCCUPY );
	BBNotAnd( bb, abb_mask[(int)psq[0]] );
      }
    else { BBNotAnd( bb, BB_WOCCUPY ); }
    BBNotAnd( bb, BB_BOCCUPY );
  }
  
  while ( BBToU(bb) )
    {
      to = LastOne( bb );
      Xor( to, bb );

      if ( psq[1] != nsquare
	   && ( adirec[from][(int)psq[1]]
		== adirec[from][to] ) ) { continue; }

      if ( psq[0] != to
	   && adirec[from][(int)psq[0]] == adirec[from][to] ) {
	  if ( adirec[from][(int)psq[0]] & flag_cross )
	    {
	      if ( abs(BOARD[(int)psq[0]]) == lance
		   || abs(BOARD[(int)psq[0]]) == rook
		   || abs(BOARD[(int)psq[0]]) == dragon ) { continue; }
	    }
	  else if ( ( adirec[from][(int)psq[0]] & flag_diag )
		    && ( abs(BOARD[(int)psq[0]]) == bishop
			 || abs(BOARD[(int)psq[0]]) == horse ) ){ continue; }
	}

      if ( turn )
	{
	  if ( is_white_attacked( ptree, to ) ) { continue; }

	  *pmove++ = ( From2Move(from) | To2Move(to)
		       | Piece2Move(king) | Cap2Move(BOARD[to]) );
	}
      else {
	if ( is_black_attacked( ptree, to ) ) { continue; }

	*pmove++ = ( From2Move(from) | To2Move(to)
		     | Piece2Move(king) | Cap2Move(-BOARD[to]) );
      }
    }

  return pmove;
}


static unsigned int *
gen_intercept( tree_t * restrict __ptree__, int sq_checker, int turn,
	       int * restrict premaining, unsigned int * restrict pmove )
{
#define Drop(pc) ( To2Move(to) | Drop2Move(pc) )

  const tree_t * restrict ptree = __ptree__;
  bitboard_t bb_atk, bb_defender, bb;
  unsigned int amove[16];
  unsigned int hand;
  int n0, n1, inc, pc, sq_k, to, from, direc, nmove, nsup, i;
  int flag_promo, flag_unpromo;

  n0 = n1 = 0;
  if ( turn )
    {
      sq_k        = SQ_WKING;
      bb_defender = BB_WOCCUPY;
      BBNotAnd( bb_defender, abb_mask[sq_k] );
    }
  else {
    sq_k        = SQ_BKING;
    bb_defender = BB_BOCCUPY;
    BBNotAnd( bb_defender, abb_mask[sq_k] );
  }

  switch ( adirec[sq_k][sq_checker] )
    {
    case direc_rank:   inc = 1;  break;
    case direc_diag1:  inc = 8;  break;
    case direc_file:   inc = 9;  break;
    default:
      assert( (int)adirec[sq_k][sq_checker] == direc_diag2 );
      inc = 10;
    }
  if ( sq_k > sq_checker ) { inc = -inc; }
  
  for ( to = sq_k + inc; to != sq_checker; to += inc )
    {
      assert( 0 <= to && to < nsquare && BOARD[to] == empty );

      nmove  = 0;
      bb_atk = attacks_to_piece( ptree, to );
      BBAnd( bb, bb_defender, bb_atk );
      while ( BBToU(bb) )
	{
	  from = LastOne( bb );
	  Xor( from, bb );
	  
	  direc        = (int)adirec[sq_k][from];
	  flag_promo   = 0;
	  flag_unpromo = 1;
	  if ( turn )
	    {
	      if ( direc && is_pinned_on_white_king( ptree, from, direc ) )
		{
		  continue;
		}
	      pc = -BOARD[from];
	      switch ( pc )
		{
		case pawn:
		  if ( to > I4 ) { flag_promo = 1;  flag_unpromo = 0; }
		  break;
		  
		case lance:  case knight:
		  if      ( to > I3 ) { flag_promo = 1;  flag_unpromo = 0; }
		  else if ( to > I4 ) { flag_promo = 1; }
		  break;
		  
		case silver:
		  if ( to > I4 || from > I4 ) { flag_promo = 1; }
		  break;
		  
		case bishop:  case rook:
		  if ( to > I4
		       || from > I4 ) { flag_promo = 1;  flag_unpromo = 0; }
		  break;
		  
		default:
		  break;
		}
	    }
	  else {
	    if ( direc && is_pinned_on_black_king( ptree, from, direc ) )
	      {
		continue;
	      }
	    pc = BOARD[from];
	    switch ( pc )
	      {
	      case pawn:
		if ( to < A6 ) { flag_promo = 1;  flag_unpromo = 0; }
		break;
		
	      case lance:  case knight:
		if      ( to < A7 ) { flag_promo = 1;  flag_unpromo = 0; }
		else if ( to < A6 ) { flag_promo = 1; }
		break;
		
	      case silver:
		if ( to < A6 || from < A6 ) { flag_promo = 1; }
		break;
		
	      case bishop:  case rook:
		if ( to < A6
		     || from < A6 ) { flag_promo = 1;  flag_unpromo = 0; }
		break;
		
	      default:
		break;
	      }
	  }
	  assert( flag_promo || flag_unpromo );
	  if ( flag_promo )
	    {
	      amove[nmove++] = ( From2Move(from) | To2Move(to)
				 | FLAG_PROMO | Piece2Move(pc) );
	    }
	  if ( flag_unpromo )
	    {
	      amove[nmove++] = ( From2Move(from) | To2Move(to)
				 | Piece2Move(pc) );
	    }
	}
      
      nsup = ( to == sq_k + inc ) ? nmove + 1 : nmove;
      if ( nsup > 1 )
	{
	  for ( i = n0 + n1 - 1; i >= n0; i-- ) { pmove[i+nmove] = pmove[i]; }
	  for ( i = 0; i < nmove; i++ ) { pmove[n0++] = amove[i]; }
	}
      else for ( i = 0; i < nmove; i++ ) { pmove[n0 + n1++] = amove[i]; }

      nmove = 0;

      if ( turn )
	{
	  hand = HAND_W;
	  if ( IsHandRook(hand) ) { amove[nmove++] = Drop(rook); }
	  else if ( IsHandLance(hand) && to < A1 )
	    {
	      amove[nmove++] = Drop(lance);
	    }
	  else if ( IsHandPawn(hand)
		    && to < A1
		    && ! ( BBToU(BB_WPAWN) & ( mask_file1 >> aifile[to] ) )
		    && ! IsMateWPawnDrop( __ptree__, to ) )
	    {
	      amove[nmove++] = Drop(pawn);
	    }
	  if ( IsHandKnight(hand) && to < A2 )
	    {
	      amove[nmove++] = Drop(knight);
	    }
	}
      else {
	hand = HAND_B;
	if ( IsHandRook(hand) ) { amove[nmove++] = Drop(rook); }
	else if ( IsHandLance(hand) && to > I9 )
	  {
	    amove[nmove++] = Drop(lance);
	  }
	else if ( IsHandPawn(hand)
		  && to > I9
		  && ! ( BBToU(BB_BPAWN) & ( mask_file1 >> aifile[to] ) )
		  && ! IsMateBPawnDrop( __ptree__, to ) )
	  {
	    amove[nmove++] = Drop(pawn);
	  }
	if ( IsHandKnight(hand) && to > I8 ) { amove[nmove++] = Drop(knight); }
      }

      if ( IsHandSilver(hand) ) { amove[nmove++] = Drop(silver); }
      if ( IsHandGold(hand) )   { amove[nmove++] = Drop(gold); }
      if ( IsHandBishop(hand) ) { amove[nmove++] = Drop(bishop); }

      if ( nsup )
	{
	  for ( i = n0 + n1 - 1; i >= n0; i-- ) { pmove[i+nmove] = pmove[i]; }
	  for ( i = 0; i < nmove; i++ ) { pmove[n0++] = amove[i]; }
	}
      else for ( i = 0; i < nmove; i++ ) { pmove[n0 + n1++] = amove[i]; }
    }
  
  *premaining = n0;
  return pmove + n0 + n1;

#undef Drop
}
