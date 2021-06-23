#include <assert.h>
#include "shogi.h"


static bitboard_t add_behind_attacks( int idirec, int ik, bitboard_t bb );


unsigned int *
b_gen_checks( tree_t * restrict __ptree__, unsigned int * restrict pmove )
{
  bitboard_t bb_piece, bb_rook_chk, bb_bishop_chk, bb_chk, bb_move_to;
  bitboard_t bb_diag1_chk, bb_diag2_chk, bb_file_chk, bb_drop_to, bb_desti;
  const tree_t * restrict ptree = __ptree__;
  unsigned int u0, u1, u2;
  int from, to, sq_wk, idirec;

  sq_wk = SQ_WKING;
  bb_rook_chk  = bb_file_chk = AttackFile( sq_wk );
  bb_rook_chk.p[aslide[sq_wk].ir0] |= AttackRank( sq_wk );
  bb_diag1_chk = AttackDiag1( sq_wk );
  bb_diag2_chk = AttackDiag2( sq_wk );
  BBOr( bb_bishop_chk, bb_diag1_chk, bb_diag2_chk );
  BBNot( bb_move_to, BB_BOCCUPY );
  BBOr( bb_drop_to, BB_BOCCUPY, BB_WOCCUPY );
  BBNot( bb_drop_to, bb_drop_to );

  from  = SQ_BKING;
  idirec = (int)adirec[sq_wk][from];
  if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
    {
      BBIni( bb_chk );
      bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
      BBAnd( bb_chk, bb_chk, abb_king_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(king)
	    | Cap2Move(-BOARD[to]);
	}
    }


  bb_piece = BB_BDRAGON;
  while( BBToU( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      BBOr( bb_chk, bb_rook_chk, abb_king_attacks[sq_wk] );
      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	}

      AttackDragon( bb_desti, from );
      BBAnd( bb_chk, bb_chk, bb_desti );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(dragon)
	    | Cap2Move(-BOARD[to]);
	}
    }

  bb_piece = BB_BHORSE;
  while( BBToU( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      BBOr( bb_chk, bb_bishop_chk, abb_king_attacks[sq_wk] );
      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	}

      AttackHorse( bb_desti, from );
      BBAnd( bb_chk, bb_chk, bb_desti );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(horse)
	    | Cap2Move(-BOARD[to]);
	}
    }

  u1 = BB_BROOK.p[1];
  u2 = BB_BROOK.p[2];
  while( u1 | u2 )
    {
      from = last_one12( u1, u2 );
      u1   ^= abb_mask[from].p[1];
      u2   ^= abb_mask[from].p[2];

      AttackRook( bb_desti, from );

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	bb_chk       = bb_rook_chk;
	bb_chk.p[0] |= abb_king_attacks[sq_wk].p[0];
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while ( bb_chk.p[0] )
	{
	  to          = last_one0( bb_chk.p[0] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(rook)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}

      while( bb_chk.p[1] | bb_chk.p[2] )
	{
	  to          = last_one12( bb_chk.p[1], bb_chk.p[2] );
	  bb_chk.p[1] ^= abb_mask[to].p[1];
	  bb_chk.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(rook)
	    | Cap2Move(-BOARD[to]);
	}
    }

  u0 = BB_BROOK.p[0];
  while( u0 )
    {
      from = last_one0( u0 );
      u0   ^= abb_mask[from].p[0];
      
      AttackRook( bb_desti, from );

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	BBOr( bb_chk, bb_rook_chk, abb_king_attacks[sq_wk] );
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(rook)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }

  u1 = BB_BBISHOP.p[1];
  u2 = BB_BBISHOP.p[2];
  while( u1 | u2 )
    {
      from = last_one12( u1, u2 );
      u1   ^= abb_mask[from].p[1];
      u2   ^= abb_mask[from].p[2];

      AttackBishop( bb_desti, from );

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	bb_chk       = bb_bishop_chk;
	bb_chk.p[0] |= abb_king_attacks[sq_wk].p[0];
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while ( bb_chk.p[0] )
	{
	  to          = last_one0( bb_chk.p[0] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(bishop)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}

      while( bb_chk.p[1] | bb_chk.p[2] )
	{
	  to          = last_one12( bb_chk.p[1], bb_chk.p[2] );
	  bb_chk.p[1] ^= abb_mask[to].p[1];
	  bb_chk.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(bishop)
	    | Cap2Move(-BOARD[to]);
	}
    }

  u0 = BB_BBISHOP.p[0];
  while( u0 )
    {
      from = last_one0( u0 );
      u0   ^= abb_mask[from].p[0];
      
      AttackBishop( bb_desti, from );

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	BBOr( bb_chk, bb_bishop_chk, abb_king_attacks[sq_wk] );
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(bishop)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }


  bb_piece = BB_BTGOLD;
  while( BBToU( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      bb_chk = abb_w_gold_attacks[sq_wk];

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	}

      BBAnd( bb_chk, bb_chk, abb_b_gold_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = ( To2Move(to) | From2Move(from)
		       | Piece2Move(BOARD[from])
		       | Cap2Move(-BOARD[to]) );
	}
    }
  

  u0 = BB_BSILVER.p[0];
  while( u0 )
    {
      from = last_one0( u0 );
      u0   ^= abb_mask[from].p[0];

      bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
      bb_chk.p[1] = abb_w_gold_attacks[sq_wk].p[1];
      bb_chk.p[2] = 0;

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	}

      bb_chk.p[0] &= bb_move_to.p[0] & abb_b_silver_attacks[from].p[0];
      bb_chk.p[1] &= bb_move_to.p[1] & abb_b_silver_attacks[from].p[1];

      while( bb_chk.p[0] | bb_chk.p[1] )
	{
	  to          = last_one01( bb_chk.p[0], bb_chk.p[1] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  bb_chk.p[1] ^= abb_mask[to].p[1];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(silver)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }
  

  u1 = BB_BSILVER.p[1] & 0x7fc0000U;
  while( u1 )
    {
      from = last_one1( u1 );
      u1   ^= abb_mask[from].p[1];
      
      bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
      bb_chk.p[1] = bb_chk.p[2] = 0;
      
      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	}

      bb_chk.p[0] &= bb_move_to.p[0] & abb_b_silver_attacks[from].p[0];
      while ( bb_chk.p[0] )
	{
	  to          = last_one0( bb_chk.p[0] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(silver)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }
  

  bb_piece = BB_BSILVER;
  while( BBToU( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      bb_chk = abb_w_silver_attacks[sq_wk];

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	}

      BBAnd( bb_chk, bb_chk, abb_b_silver_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(silver)
	    | Cap2Move(-BOARD[to]);
	}
    }
  

  u0 = BB_BKNIGHT.p[0];
  u1 = BB_BKNIGHT.p[1] & 0x7fffe00U;
  while( u0 | u1 )
    {
      from = last_one01( u0, u1 );
      u0   ^= abb_mask[from].p[0];
      u1   ^= abb_mask[from].p[1];

      bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
      bb_chk.p[1] = bb_chk.p[2] = 0;

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	}

      bb_chk.p[0] &= abb_b_knight_attacks[from].p[0] & bb_move_to.p[0];

      while( bb_chk.p[0] )
	{
	  to          = last_one0( bb_chk.p[0] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(knight)
		       | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }
  

  u2 = BB_BKNIGHT.p[2];
  u1 = BB_BKNIGHT.p[1] & 0x3ffffU;
  while( u2 | u1 )
    {
      from = last_one12( u1, u2 );
      u2   ^= abb_mask[from].p[2];
      u1   ^= abb_mask[from].p[1];

      bb_chk = abb_w_knight_attacks[sq_wk];

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	}

      BBAnd( bb_chk, bb_chk, abb_b_knight_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(knight)
	    | Cap2Move(-BOARD[to]);
	}
    }


  bb_piece = BB_BLANCE;
  while( BBToU( bb_piece ) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );

      bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
      bb_chk.p[1] = bb_chk.p[2] = 0;

      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	}

      BBAnd( bb_chk, bb_chk, AttackFile( from ) );
      BBAnd( bb_chk, bb_chk, abb_minus_rays[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(lance)
	    | Cap2Move(-BOARD[to]) | FLAG_PROMO;
	}
    }
  

  u1 = BB_BLANCE.p[1];
  u2 = BB_BLANCE.p[2];
  while( u1| u2 )
    {
      from = last_one12( u1, u2 );
      u1   ^= abb_mask[from].p[1];
      u2   ^= abb_mask[from].p[2];

      bb_chk = bb_file_chk;
      idirec = (int)adirec[sq_wk][from];
      if ( idirec && is_pinned_on_white_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_wk, bb_chk );
	  BBAnd( bb_chk, bb_chk, abb_minus_rays[from] );
	}
      else { BBAnd( bb_chk, bb_file_chk, abb_plus_rays[sq_wk] );}

      BBAnd( bb_chk, bb_chk, AttackFile( from ) );
      BBAnd( bb_chk, bb_chk, bb_move_to );
      bb_chk.p[0] = bb_chk.p[0] & 0x1ffU;

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(lance)
	    | Cap2Move(-BOARD[to]);
	}
    }


  BBAnd( bb_piece, bb_diag1_chk, BB_BPAWN );
  while ( BBToU(bb_piece) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );
      
      to = from - nfile;
      if ( BOARD[to] != empty ) { continue; }

      bb_desti = AttackDiag1( from );
      if ( BBContract( bb_desti, BB_B_BH ) )
	{
	  *pmove = To2Move(to) | From2Move(from)
	    | Piece2Move(pawn) | Cap2Move(-BOARD[to]);
	  if ( from < A5 ) { *pmove |= FLAG_PROMO; }
	  pmove += 1;
	}
    }

  BBAnd( bb_piece, bb_diag2_chk, BB_BPAWN );
  while ( BBToU(bb_piece) )
    {
      from = LastOne( bb_piece );
      Xor( from, bb_piece );
      
      to = from - nfile;
      if ( BOARD[to] != empty ) { continue; }

      bb_desti = AttackDiag2( from );
      if ( BBContract( bb_desti, BB_B_BH ) )
	{
	  *pmove = To2Move(to) | From2Move(from)
	    | Piece2Move(pawn) | Cap2Move(-BOARD[to]);
	  if ( from < A5 ) { *pmove |= FLAG_PROMO; }
	  pmove += 1;
	}
    }

  BBIni( bb_chk );
  bb_chk.p[0] = abb_w_gold_attacks[sq_wk].p[0];
  if ( sq_wk < A2 ) { BBOr( bb_chk, bb_chk, abb_mask[sq_wk+nfile] ); };
  BBAnd( bb_chk, bb_chk, bb_move_to );
  BBAnd( bb_chk, bb_chk, BB_BPAWN_ATK );
  while ( BBToU(bb_chk) )
    {
      to = LastOne( bb_chk );
      Xor( to, bb_chk );

      from = to + nfile;
      *pmove = To2Move(to) | From2Move(from)
	| Piece2Move(pawn) | Cap2Move(-BOARD[to]);
      if ( from < A5 ) { *pmove |= FLAG_PROMO; }
      pmove += 1;
    }


  if ( IsHandGold(HAND_B) )
    {
      BBAnd( bb_chk, bb_drop_to, abb_w_gold_attacks[sq_wk] );
      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | Drop2Move(gold);
	}
    }
  

  if ( IsHandSilver(HAND_B) )
    {
      BBAnd( bb_chk, bb_drop_to, abb_w_silver_attacks[sq_wk] );
      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | Drop2Move(silver);
	}
    }
  

  if ( IsHandKnight(HAND_B) && sq_wk < A2 )
    {
      to = sq_wk + 2*nfile - 1;
      if ( aifile[sq_wk] != file1 && BOARD[to] == empty )
	{
	  *pmove++ = To2Move(to) | Drop2Move(knight);
	}

      to = sq_wk + 2*nfile + 1;
      if ( aifile[sq_wk] != file9 && BOARD[to] == empty )
	{
	  *pmove++ = To2Move(to) | Drop2Move(knight);
	}
    }


  if ( IsHandPawn(HAND_B)
       && sq_wk < A1
       && ! ( BBToU(BB_BPAWN) & ( mask_file1 >> aifile[sq_wk] ) ) )
    {
      to = sq_wk + nfile;
      if ( BOARD[to] == empty && ! is_mate_b_pawn_drop( __ptree__, to ) )
	{
	  *pmove++ = To2Move(to) | Drop2Move(pawn);
	}
    }


  if ( IsHandLance(HAND_B) || IsHandRook(HAND_B) )
    {
      for ( to = sq_wk+nfile; to < nsquare; to += nfile )
	{
	  if ( BOARD[to] != empty ) { break; }
	  if ( IsHandLance(HAND_B) )
	    {
	      *pmove++ = To2Move(to) | Drop2Move(lance);
	    }
	  if ( IsHandRook(HAND_B) )
	    {
	      *pmove++ = To2Move(to) | Drop2Move(rook);
	    }
	}
    }


  if ( IsHandRook(HAND_B) )
    {
      int ifile;

      for ( to = sq_wk-nfile; to >= 0; to -= nfile )
	{
	  if ( BOARD[to] != empty ) { break; }
	  *pmove++ = To2Move(to) | Drop2Move(rook);
	}

      for ( ifile = (int)aifile[sq_wk]-1, to = sq_wk-1;
	    ifile >= file1;
	    ifile -= 1, to -= 1 )
	{
	  if ( BOARD[to] != empty ) { break; }
	  *pmove++ = To2Move(to) | Drop2Move(rook);
	}

      for ( ifile = (int)aifile[sq_wk]+1, to = sq_wk+1;
	    ifile <= file9;
	    ifile += 1, to += 1 )
	{
	  if ( BOARD[to] != empty ) { break; }
	  *pmove++ = To2Move(to) | Drop2Move(rook);
	}
    }


  if ( IsHandBishop(HAND_B) )
    {
      int ifile, irank;

      to   = sq_wk;
      ifile = (int)aifile[sq_wk];
      irank = (int)airank[sq_wk];
      for ( to -= 10, ifile -= 1, irank -= 1;
	    ifile >= 0 && irank >= 0 && BOARD[to] == empty;
	    to -= 10, ifile -= 1, irank -= 1 )
	{
	  *pmove++ = To2Move(to) | Drop2Move(bishop);
	}

      to   = sq_wk;
      ifile = (int)aifile[sq_wk];
      irank = (int)airank[sq_wk];
      for ( to -= 8, ifile += 1, irank -= 1;
	    ifile <= file9 && irank >= 0 && BOARD[to] == empty;
	    to -= 8, ifile += 1, irank -= 1 )
	{
	  *pmove++ = To2Move(to) | Drop2Move(bishop);
	}

      to   = sq_wk;
      ifile = (int)aifile[sq_wk];
      irank = (int)airank[sq_wk];
      for ( to += 8, ifile -= 1, irank += 1;
	    ifile >= 0 && irank <= rank9 && BOARD[to] == empty;
	    to += 8, ifile -= 1, irank += 1 )
	{
	  *pmove++ = To2Move(to) | Drop2Move(bishop);
	}

      to   = sq_wk;
      ifile = (int)aifile[sq_wk];
      irank = (int)airank[sq_wk];
      for ( to += 10, ifile += 1, irank += 1;
	    ifile <= file9 && irank <= rank9 && BOARD[to] == empty;
	    to += 10, ifile += 1, irank += 1 )
	{
	  *pmove++ = To2Move(to) | Drop2Move(bishop);
	}
    }


  return pmove;
}


unsigned int *
w_gen_checks( tree_t * restrict __ptree__, unsigned int * restrict pmove )
{
  bitboard_t bb_piece, bb_rook_chk, bb_bishop_chk, bb_chk, bb_move_to;
  bitboard_t bb_diag1_chk, bb_diag2_chk, bb_file_chk, bb_drop_to, bb_desti;
  const tree_t * restrict ptree = __ptree__;
  unsigned int u0, u1, u2;
  int from, to, sq_bk, idirec;

  sq_bk = SQ_BKING;
  bb_rook_chk = bb_file_chk = AttackFile( sq_bk );
  bb_rook_chk.p[aslide[sq_bk].ir0] |= AttackRank( sq_bk );
  bb_diag1_chk = AttackDiag1( sq_bk );
  bb_diag2_chk = AttackDiag2( sq_bk );
  AttackBishop( bb_bishop_chk, sq_bk );
  BBNot( bb_move_to, BB_WOCCUPY );
  BBOr( bb_drop_to, BB_BOCCUPY, BB_WOCCUPY );
  BBNot( bb_drop_to, bb_drop_to );


  from  = SQ_WKING;
  idirec = (int)adirec[sq_bk][from];
  if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
    {
      BBIni( bb_chk );
      bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
      BBAnd( bb_chk, bb_chk, abb_king_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(king)
	    | Cap2Move(BOARD[to]);
	}
    }


  bb_piece = BB_WDRAGON;
  while( BBToU( bb_piece ) )
    {
      from = FirstOne( bb_piece );
      Xor( from, bb_piece );

      BBOr( bb_chk, bb_rook_chk, abb_king_attacks[sq_bk] );
      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	}

      AttackDragon( bb_desti, from );
      BBAnd( bb_chk, bb_chk, bb_desti );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = LastOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(dragon)
	    | Cap2Move(BOARD[to]);
	}
    }


  bb_piece = BB_WHORSE;
  while( BBToU( bb_piece ) )
    {
      from = FirstOne( bb_piece );
      Xor( from, bb_piece );

      BBOr( bb_chk, bb_bishop_chk, abb_king_attacks[sq_bk] );
      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	}

      AttackHorse( bb_desti, from );
      BBAnd( bb_chk, bb_chk, bb_desti );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(horse)
	    | Cap2Move(BOARD[to]);
	}
    }

  u0 = BB_WROOK.p[0];
  u1 = BB_WROOK.p[1];
  while( u0 | u1 )
    {
      from = first_one01( u0, u1 );
      u0   ^= abb_mask[from].p[0];
      u1   ^= abb_mask[from].p[1];

      AttackRook( bb_desti, from );

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	bb_chk       = bb_rook_chk;
	bb_chk.p[2] |= abb_king_attacks[sq_bk].p[2];
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while ( bb_chk.p[2] )
	{
	  to          = first_one2( bb_chk.p[2] );
	  bb_chk.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(rook)
	    | Cap2Move(BOARD[to]) | FLAG_PROMO;
	}

      while( bb_chk.p[0] | bb_chk.p[1] )
	{
	  to          = first_one01( bb_chk.p[0], bb_chk.p[1] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  bb_chk.p[1] ^= abb_mask[to].p[1];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(rook)
	    | Cap2Move(BOARD[to]);
	}
    }

  u2 = BB_WROOK.p[2];
  while( u2 )
    {
      from = first_one2( u2 );
      u2   ^= abb_mask[from].p[2];
      
      AttackRook( bb_desti, from );

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	BBOr( bb_chk, bb_rook_chk, abb_king_attacks[sq_bk] );
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(rook)
	    | Cap2Move(BOARD[to]) | FLAG_PROMO;
	}
    }

  u0 = BB_WBISHOP.p[0];
  u1 = BB_WBISHOP.p[1];
  while( u0 | u1 )
    {
      from = first_one01( u0, u1 );
      u0   ^= abb_mask[from].p[0];
      u1   ^= abb_mask[from].p[1];

      AttackBishop( bb_desti, from );

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	bb_chk       = bb_bishop_chk;
	bb_chk.p[2] |= abb_king_attacks[sq_bk].p[2];
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while ( bb_chk.p[2] )
	{
	  to          = first_one2( bb_chk.p[2] );
	  bb_chk.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(bishop)
	    | Cap2Move(BOARD[to]) | FLAG_PROMO;
	}

      while( bb_chk.p[0] | bb_chk.p[1] )
	{
	  to          = first_one01( bb_chk.p[0], bb_chk.p[1] );
	  bb_chk.p[0] ^= abb_mask[to].p[0];
	  bb_chk.p[1] ^= abb_mask[to].p[1];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(bishop)
	    | Cap2Move(BOARD[to]);
	}
    }

  u2 = BB_WBISHOP.p[2];
  while( u2 )
    {
      from = first_one2( u2 );
      u2   ^= abb_mask[from].p[2];
      
      AttackBishop( bb_desti, from );

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  BBAnd( bb_chk, bb_desti, bb_move_to );
	}
      else {
	BBOr( bb_chk, bb_bishop_chk, abb_king_attacks[sq_bk] );
	BBAnd( bb_chk, bb_chk, bb_desti );
	BBAnd( bb_chk, bb_chk, bb_move_to );
      }

      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(bishop)
	    | Cap2Move(BOARD[to]) | FLAG_PROMO;
	}
    }


  bb_piece = BB_WTGOLD;
  while( BBToU( bb_piece ) )
    {
      from = FirstOne( bb_piece );
      Xor( from, bb_piece );

      bb_chk = abb_b_gold_attacks[sq_bk];

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	}

      BBAnd( bb_chk, bb_chk, abb_w_gold_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = ( To2Move(to) | From2Move(from)
		       | Piece2Move(-BOARD[from])
		       | Cap2Move(BOARD[to]) );
	}
    }

  
  u2 = BB_WSILVER.p[2];
  while( u2 )
    {
      from = first_one2( u2 );
      u2   ^= abb_mask[from].p[2];

      bb_chk.p[2] = abb_b_gold_attacks[sq_bk].p[2];
      bb_chk.p[1] = abb_b_gold_attacks[sq_bk].p[1];
      bb_chk.p[0] = 0;

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	}

      bb_chk.p[2] &= bb_move_to.p[2] & abb_w_silver_attacks[from].p[2];
      bb_chk.p[1] &= bb_move_to.p[1] & abb_w_silver_attacks[from].p[1];

      while( bb_chk.p[2] | bb_chk.p[1] )
	{
	  to          = first_one12( bb_chk.p[1], bb_chk.p[2] );
	  bb_chk.p[1] ^= abb_mask[to].p[1];
	  bb_chk.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(silver)
	    | Cap2Move(BOARD[to]) | FLAG_PROMO;
	}
    }
  

  u1 = BB_WSILVER.p[1] & 0x1ffU;
  while( u1 )
    {
      from = first_one1( u1 );
      u1   ^= abb_mask[from].p[1];
      
      bb_chk.p[2] = abb_b_gold_attacks[sq_bk].p[2];
      bb_chk.p[1] = bb_chk.p[0] = 0;
      
      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	}

      bb_chk.p[2] &= bb_move_to.p[2] & abb_w_silver_attacks[from].p[2];
      while ( bb_chk.p[2] )
	{
	  to          = first_one2( bb_chk.p[2] );
	  bb_chk.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(silver)
	    | Cap2Move(BOARD[to]) | FLAG_PROMO;
	}
    }
  

  bb_piece = BB_WSILVER;
  while( BBToU( bb_piece ) )
    {
      from = FirstOne( bb_piece );
      Xor( from, bb_piece );

      bb_chk = abb_b_silver_attacks[sq_bk];

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	}

      BBAnd( bb_chk, bb_chk, abb_w_silver_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(silver)
	    | Cap2Move(BOARD[to]);
	}
    }

  
  u2 = BB_WKNIGHT.p[2];
  u1 = BB_WKNIGHT.p[1] & 0x3ffffU;
  while( u2 | u1 )
    {
      from = first_one12( u1, u2 );
      u2   ^= abb_mask[from].p[2];
      u1   ^= abb_mask[from].p[1];

      bb_chk.p[2] = abb_b_gold_attacks[sq_bk].p[2];
      bb_chk.p[1] = bb_chk.p[0] = 0;

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	}

      bb_chk.p[2] &= abb_w_knight_attacks[from].p[2] & bb_move_to.p[2];

      while( bb_chk.p[2] )
	{
	  to          = first_one2( bb_chk.p[2] );
	  bb_chk.p[2] ^= abb_mask[to].p[2];
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(knight)
		       | Cap2Move(BOARD[to]) | FLAG_PROMO;
	}
    }
  

  u0 = BB_WKNIGHT.p[0];
  u1 = BB_WKNIGHT.p[1] & 0x7fffe00U;
  while( u0 | u1 )
    {
      from = first_one01( u0, u1 );
      u0   ^= abb_mask[from].p[0];
      u1   ^= abb_mask[from].p[1];

      bb_chk = abb_b_knight_attacks[sq_bk];

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	}

      BBAnd( bb_chk, bb_chk, abb_w_knight_attacks[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(knight)
	    | Cap2Move(BOARD[to]);
	}
    }


  bb_piece = BB_WLANCE;
  while( BBToU( bb_piece ) )
    {
      from = FirstOne( bb_piece );
      Xor( from, bb_piece );

      bb_chk.p[2] = abb_b_gold_attacks[sq_bk].p[2];
      bb_chk.p[1] = bb_chk.p[0] = 0;

      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	}

      BBAnd( bb_chk, bb_chk, AttackFile( from ) );
      BBAnd( bb_chk, bb_chk, abb_plus_rays[from] );
      BBAnd( bb_chk, bb_chk, bb_move_to );

      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(lance)
	    | Cap2Move(BOARD[to]) | FLAG_PROMO;
	}
    }
  

  u0 = BB_WLANCE.p[0];
  u1 = BB_WLANCE.p[1];
  while( u0 | u1 )
    {
      from = first_one01( u0, u1 );
      u0   ^= abb_mask[from].p[0];
      u1   ^= abb_mask[from].p[1];

      bb_chk = bb_file_chk;
      idirec = (int)adirec[sq_bk][from];
      if ( idirec && is_pinned_on_black_king( ptree, from, idirec ) )
	{
	  bb_chk = add_behind_attacks( idirec, sq_bk, bb_chk );
	  BBAnd( bb_chk, bb_chk, abb_plus_rays[from] );
	}
      else { BBAnd( bb_chk, bb_file_chk, abb_minus_rays[sq_bk] ); }

      BBAnd( bb_chk, bb_chk, AttackFile( from ) );
      BBAnd( bb_chk, bb_chk, bb_move_to );
      bb_chk.p[2] = bb_chk.p[2] & 0x7fc0000U;

      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | From2Move(from) | Piece2Move(lance)
	    | Cap2Move(BOARD[to]);
	}
    }


  BBAnd( bb_piece, bb_diag1_chk, BB_WPAWN );
  while ( BBToU(bb_piece) )
    {
      from = FirstOne( bb_piece );
      Xor( from, bb_piece );
      
      to = from + nfile;
      if ( BOARD[to] != empty ) { continue; }

      bb_desti = AttackDiag1( from );
      if ( BBContract( bb_desti, BB_W_BH ) )
	{
	  *pmove = To2Move(to) | From2Move(from)
	    | Piece2Move(pawn) | Cap2Move(BOARD[to]);
	  if ( from > I5 ) { *pmove |= FLAG_PROMO; }
	  pmove += 1;
	}
    }

  BBAnd( bb_piece, bb_diag2_chk, BB_WPAWN );
  while ( BBToU(bb_piece) )
    {
      from = FirstOne( bb_piece );
      Xor( from, bb_piece );
      
      to = from + nfile;
      if ( BOARD[to] != empty ) { continue; }

      bb_desti = AttackDiag2( from );
      if ( BBContract( bb_desti, BB_W_BH ) )
	{
	  *pmove = To2Move(to) | From2Move(from)
	    | Piece2Move(pawn) | Cap2Move(BOARD[to]);
	  if ( from > I5 ) { *pmove |= FLAG_PROMO; }
	  pmove += 1;
	}
    }

  BBIni( bb_chk );
  bb_chk.p[2] = abb_b_gold_attacks[sq_bk].p[2];
  if ( sq_bk > I8 ) { BBOr( bb_chk, bb_chk, abb_mask[sq_bk-nfile] ); };
  BBAnd( bb_chk, bb_chk, bb_move_to );
  BBAnd( bb_chk, bb_chk, BB_WPAWN_ATK );
  while ( BBToU(bb_chk) )
    {
      to = FirstOne( bb_chk );
      Xor( to, bb_chk );

      from = to - nfile;
      *pmove = To2Move(to) | From2Move(from) | Piece2Move(pawn)
	| Cap2Move(BOARD[to]);
      if ( from > I5 ) { *pmove |= FLAG_PROMO; }
      pmove += 1;
    }


  if ( IsHandGold(HAND_W) )
    {
      BBAnd( bb_chk, bb_drop_to, abb_b_gold_attacks[sq_bk] );
      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | Drop2Move(gold);
	}
    }
  

  if ( IsHandSilver(HAND_W) )
    {
      BBAnd( bb_chk, bb_drop_to, abb_b_silver_attacks[sq_bk] );
      while( BBToU( bb_chk ) )
	{
	  to = FirstOne( bb_chk );
	  Xor( to, bb_chk );
	  *pmove++ = To2Move(to) | Drop2Move(silver);
	}
    }
  

  if ( IsHandKnight(HAND_W) && sq_bk > I8 )
    {
      to = sq_bk - 2*nfile - 1;
      if ( aifile[sq_bk] != file1 && BOARD[to] == empty )
	{
	  *pmove++ = To2Move(to) | Drop2Move(knight);
	}

      to = sq_bk - 2*nfile + 1;
      if ( aifile[sq_bk] != file9 && BOARD[to] == empty )
	{
	  *pmove++ = To2Move(to) | Drop2Move(knight);
	}
    }


  if ( IsHandPawn(HAND_W)
       && sq_bk > I9
       && ! ( BBToU(BB_WPAWN) & ( mask_file1 >> aifile[sq_bk] ) ) )
    {
      to = sq_bk - nfile;
      if ( BOARD[to] == empty && ! is_mate_w_pawn_drop( __ptree__, to ) )
	{
	  *pmove++ = To2Move(to) | Drop2Move(pawn);
	}
    }


  if ( IsHandLance(HAND_W) || IsHandRook(HAND_W) )
    {
      for ( to = sq_bk-nfile; to >= 0; to -= nfile )
	{
	  if ( BOARD[to] != empty ) { break; }
	  if ( IsHandLance(HAND_W) )
	    {
	      *pmove++ = To2Move(to) | Drop2Move(lance);
	    }
	  if ( IsHandRook(HAND_W) )
	    {
	      *pmove++ = To2Move(to) | Drop2Move(rook);
	    }
	}
    }


  if ( IsHandRook(HAND_W) )
    {
      int ifile;

      for ( to = sq_bk+nfile; to < nsquare; to += nfile )
	{
	  if ( BOARD[to] != empty ) { break; }
	  *pmove++ = To2Move(to) | Drop2Move(rook);
	}

      for ( ifile = (int)aifile[sq_bk]+1, to = sq_bk+1;
	    ifile <= file9;
	    ifile += 1, to += 1 )
	{
	  if ( BOARD[to] != empty ) { break; }
	  *pmove++ = To2Move(to) | Drop2Move(rook);
	}

      for ( ifile = (int)aifile[sq_bk]-1, to = sq_bk-1;
	    ifile >= file1;
	    ifile -= 1, to -= 1 )
	{
	  if ( BOARD[to] != empty ) { break; }
	  *pmove++ = To2Move(to) | Drop2Move(rook);
	}
    }


  if ( IsHandBishop(HAND_W) )
    {
      int ifile, irank;

      to   = sq_bk;
      ifile = (int)aifile[sq_bk];
      irank = (int)airank[sq_bk];
      for ( to += 10, ifile += 1, irank += 1;
	    ifile <= file9 && irank <= rank9 && BOARD[to] == empty;
	    to += 10, ifile += 1, irank += 1 )
	{
	  *pmove++ = To2Move(to) | Drop2Move(bishop);
	}

      to   = sq_bk;
      ifile = (int)aifile[sq_bk];
      irank = (int)airank[sq_bk];
      for ( to += 8, ifile -= 1, irank += 1;
	    ifile >= 0 && irank <= rank9 && BOARD[to] == empty;
	    to += 8, ifile -= 1, irank += 1 )
	{
	  *pmove++ = To2Move(to) | Drop2Move(bishop);
	}

      to   = sq_bk;
      ifile = (int)aifile[sq_bk];
      irank = (int)airank[sq_bk];
      for ( to -= 8, ifile += 1, irank -= 1;
	    ifile <= file9 && irank >= 0 && BOARD[to] == empty;
	    to -= 8, ifile += 1, irank -= 1 )
	{
	  *pmove++ = To2Move(to) | Drop2Move(bishop);
	}

      to   = sq_bk;
      ifile = (int)aifile[sq_bk];
      irank = (int)airank[sq_bk];
      for ( to -= 10, ifile -= 1, irank -= 1;
	    ifile >= 0 && irank >= 0 && BOARD[to] == empty;
	    to -= 10, ifile -= 1, irank -= 1 )
	{
	  *pmove++ = To2Move(to) | Drop2Move(bishop);
	}
    }


  return pmove;
}


static bitboard_t
add_behind_attacks( int idirec, int ik, bitboard_t bb )
{
  bitboard_t bb_tmp;

  if ( idirec == direc_diag1 )
    {
      bb_tmp = abb_bishop_attacks_rr45[ik][0];
    }
  else if ( idirec == direc_diag2 )
    {
      bb_tmp = abb_bishop_attacks_rl45[ik][0];
    }
  else if ( idirec == direc_file )
    {
      bb_tmp = abb_file_attacks[ik][0];
    }
  else {
    assert( idirec == direc_rank );
    BBIni( bb_tmp );
    bb_tmp.p[aslide[ik].ir0] = ai_rook_attacks_r0[ik][0];
  }
  BBNot( bb_tmp, bb_tmp );
  BBOr( bb, bb, bb_tmp );

  return bb;
}
