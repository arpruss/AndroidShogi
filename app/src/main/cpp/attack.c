#include <assert.h>
#include <stdlib.h>
#include "shogi.h"


unsigned int
is_pinned_on_white_king( const tree_t * restrict ptree, int isquare,
			int idirec )
{
  unsigned int ubb_attacks;
  bitboard_t bb_attacks, bb_attacker;

  switch ( idirec )
    {
    case direc_rank:
      ubb_attacks = AttackRank( isquare );
      if ( ubb_attacks & (BB_WKING.p[aslide[isquare].ir0]) )
	{
	  return ubb_attacks & BB_B_RD.p[aslide[isquare].ir0];
	}
      break;

    case direc_file:
      bb_attacks = AttackFile( isquare );
      if ( BBContract( bb_attacks, BB_WKING ) )
	{
	  BBAnd( bb_attacker, BB_BLANCE, abb_plus_rays[isquare] );
	  BBOr( bb_attacker, bb_attacker, BB_B_RD );
	  return BBContract( bb_attacks, bb_attacker );  /* return! */
	}
      break;

    case direc_diag1:
      bb_attacks = AttackDiag1( isquare );
      if ( BBContract( bb_attacks, BB_WKING ) )
	{
	  return BBContract( bb_attacks, BB_B_BH );      /* return! */
	}
      break;

    default:
      assert( idirec == direc_diag2 );
      bb_attacks = AttackDiag2( isquare );
      if ( BBContract( bb_attacks, BB_WKING ) )
	{
	  return BBContract( bb_attacks, BB_B_BH );      /* return! */
	}
      break;
    }
  
  return 0;
}


unsigned int
is_pinned_on_black_king( const tree_t * restrict ptree, int isquare,
			int idirec )
{
  unsigned int ubb_attacks;
  bitboard_t bb_attacks, bb_attacker;

  switch ( idirec )
    {
    case direc_rank:
      ubb_attacks = AttackRank( isquare );
      if ( ubb_attacks & (BB_BKING.p[aslide[isquare].ir0]) )
	{
	  return ubb_attacks & BB_W_RD.p[aslide[isquare].ir0];
	}
      break;

    case direc_file:
      bb_attacks = AttackFile( isquare );
      if ( BBContract( bb_attacks, BB_BKING ) )
	{
	  BBAnd( bb_attacker, BB_WLANCE, abb_minus_rays[isquare] );
	  BBOr( bb_attacker, bb_attacker, BB_W_RD );
	  return BBContract( bb_attacks, bb_attacker );      /* return! */
	}
      break;

    case direc_diag1:
      bb_attacks = AttackDiag1( isquare );
      if ( BBContract( bb_attacks, BB_BKING ) )
	{
	  return BBContract( bb_attacks, BB_W_BH );          /* return! */
	}
      break;

    default:
      assert( idirec == direc_diag2 );
      bb_attacks = AttackDiag2( isquare );
      if ( BBContract( bb_attacks, BB_BKING ) )
	{
	  return BBContract( bb_attacks, BB_W_BH );          /* return! */
	}
      break;
    }
  return 0;
}


/* perpetual check detections are omitted. */
int
is_mate_b_pawn_drop( tree_t * restrict ptree, int sq_drop )
{
  bitboard_t bb, bb_sum, bb_move;
  int iwk, ito, iret, ifrom, idirec;

  BBAnd( bb_sum, BB_WKNIGHT, abb_b_knight_attacks[sq_drop] );

  BBAndOr( bb_sum, BB_WSILVER, abb_b_silver_attacks[sq_drop] );
  BBAndOr( bb_sum, BB_WTGOLD, abb_b_gold_attacks[sq_drop] );

  AttackBishop( bb, sq_drop );
  BBAndOr( bb_sum, BB_W_BH, bb );

  AttackRook( bb, sq_drop );
  BBAndOr( bb_sum, BB_W_RD, bb );

  BBOr( bb, BB_WHORSE, BB_WDRAGON );
  BBAndOr( bb_sum, bb, abb_king_attacks[sq_drop] );

  while ( BBToU( bb_sum ) )
    {
      ifrom  = FirstOne( bb_sum );
      Xor( ifrom, bb_sum );

      if ( IsDiscoverWK( ifrom, sq_drop ) ) { continue; }
      return 0;
    }

  iwk  = SQ_WKING;
  iret = 1;
  Xor( sq_drop, BB_BOCCUPY );
  XorFile( sq_drop, OCCUPIED_FILE );
  XorDiag2( sq_drop, OCCUPIED_DIAG2 );
  XorDiag1( sq_drop, OCCUPIED_DIAG1 );
  
  BBNot( bb_move, BB_WOCCUPY );
  BBAnd( bb_move, bb_move, abb_king_attacks[iwk] );
  while ( BBToU( bb_move ) )
    {
      ito = FirstOne( bb_move );
      if ( ! is_white_attacked( ptree, ito ) )
	{
	  iret = 0;
	  break;
	}
      Xor( ito, bb_move );
    }

  Xor( sq_drop, BB_BOCCUPY );
  XorFile( sq_drop, OCCUPIED_FILE );
  XorDiag2( sq_drop, OCCUPIED_DIAG2 );
  XorDiag1( sq_drop, OCCUPIED_DIAG1 );

  return iret;
}


int
is_mate_w_pawn_drop( tree_t * restrict ptree, int sq_drop )
{
  bitboard_t bb, bb_sum, bb_move;
  int ibk, ito, ifrom, iret, idirec;

  BBAnd( bb_sum, BB_BKNIGHT, abb_w_knight_attacks[sq_drop] );

  BBAndOr( bb_sum, BB_BSILVER, abb_w_silver_attacks[sq_drop] );
  BBAndOr( bb_sum, BB_BTGOLD,  abb_w_gold_attacks[sq_drop] );

  AttackBishop( bb, sq_drop );
  BBAndOr( bb_sum, BB_B_BH, bb );

  AttackRook( bb, sq_drop );
  BBAndOr( bb_sum, BB_B_RD, bb );

  BBOr( bb, BB_BHORSE, BB_BDRAGON );
  BBAndOr( bb_sum, bb, abb_king_attacks[sq_drop] );

  while ( BBToU( bb_sum ) )
    {
      ifrom  = FirstOne( bb_sum );
      Xor( ifrom, bb_sum );

      if ( IsDiscoverBK( ifrom, sq_drop ) ) { continue; }
      return 0;
    }

  ibk  = SQ_BKING;
  iret = 1;
  Xor( sq_drop, BB_WOCCUPY );
  XorFile( sq_drop, OCCUPIED_FILE );
  XorDiag2( sq_drop, OCCUPIED_DIAG2 );
  XorDiag1( sq_drop, OCCUPIED_DIAG1 );
  
  BBNot( bb_move, BB_BOCCUPY );
  BBAnd( bb_move, bb_move, abb_king_attacks[ibk] );
  while ( BBToU( bb_move ) )
    {
      ito = FirstOne( bb_move );
      if ( ! is_black_attacked( ptree, ito ) )
	{
	  iret = 0;
	  break;
	}
      Xor( ito, bb_move );
    }

  Xor( sq_drop, BB_WOCCUPY );
  XorFile( sq_drop, OCCUPIED_FILE );
  XorDiag2( sq_drop, OCCUPIED_DIAG2 );
  XorDiag1( sq_drop, OCCUPIED_DIAG1 );

  return iret;
}


bitboard_t
attacks_to_piece( const tree_t * restrict ptree, int sq )
{
  bitboard_t bb_ret, bb_attacks, bb;

  BBIni( bb_ret );
  if ( sq < rank9*nfile && BOARD[sq+nfile] == pawn )
    {
      bb_ret = abb_mask[sq+nfile];
    }
  if ( sq >= nfile && BOARD[sq-nfile] == -pawn )
    {
      BBOr( bb_ret, bb_ret, abb_mask[sq-nfile] );
    }

  BBAndOr( bb_ret, BB_BKNIGHT, abb_w_knight_attacks[sq] );
  BBAndOr( bb_ret, BB_WKNIGHT, abb_b_knight_attacks[sq] );

  BBAndOr( bb_ret, BB_BSILVER, abb_w_silver_attacks[sq] );
  BBAndOr( bb_ret, BB_WSILVER, abb_b_silver_attacks[sq] );

  BBAndOr( bb_ret, BB_BTGOLD,  abb_w_gold_attacks[sq] );
  BBAndOr( bb_ret, BB_WTGOLD,  abb_b_gold_attacks[sq] );

  BBOr( bb, BB_B_HDK, BB_W_HDK );
  BBAndOr( bb_ret, bb, abb_king_attacks[sq] );

  BBOr( bb, BB_B_BH, BB_W_BH );
  AttackBishop( bb_attacks, sq );
  BBAndOr( bb_ret, bb, bb_attacks );

  BBOr( bb, BB_B_RD, BB_W_RD );
  bb_ret.p[aslide[sq].ir0]
    |= bb.p[aslide[sq].ir0] & AttackRank( sq );
  
  BBAndOr( bb, BB_BLANCE, abb_plus_rays[sq] );
  BBAndOr( bb, BB_WLANCE, abb_minus_rays[sq] );
  bb_attacks = AttackFile( sq );
  BBAndOr( bb_ret, bb, bb_attacks );
  
  return bb_ret;
}


unsigned int
is_white_attacked( const tree_t * restrict ptree, int sq )
{
  bitboard_t bb;
  unsigned int u;

  u  = BBContract( BB_BPAWN_ATK, abb_mask[sq] );
  u |= BBContract( BB_BKNIGHT,   abb_w_knight_attacks[sq] );
  u |= BBContract( BB_BSILVER,   abb_w_silver_attacks[sq] );
  u |= BBContract( BB_BTGOLD,    abb_w_gold_attacks[sq] );
  u |= BBContract( BB_B_HDK,     abb_king_attacks[sq] );

  AttackBishop( bb, sq );
  u |= BBContract( BB_B_BH, bb );

  u |= BB_B_RD.p[aslide[sq].ir0] & AttackRank( sq );

  bb = AttackFile( sq );
  u |= ( ( BB_BLANCE.p[0] & abb_plus_rays[sq].p[0] )
	 | BB_B_RD.p[0] ) & bb.p[0];
  u |= ( ( BB_BLANCE.p[1] & abb_plus_rays[sq].p[1] )
	       | BB_B_RD.p[1] ) & bb.p[1];
  u |= ( ( BB_BLANCE.p[2] & abb_plus_rays[sq].p[2] )
	       | BB_B_RD.p[2] ) & bb.p[2];

  return u;
}


unsigned int
is_black_attacked( const tree_t * restrict ptree, int sq )
{
  bitboard_t bb;
  unsigned int u;

  u  = BBContract( BB_WPAWN_ATK, abb_mask[sq] );
  u |= BBContract( BB_WKNIGHT,   abb_b_knight_attacks[sq] );
  u |= BBContract( BB_WSILVER,   abb_b_silver_attacks[sq] );
  u |= BBContract( BB_WTGOLD,    abb_b_gold_attacks[sq] );
  u |= BBContract( BB_W_HDK,     abb_king_attacks[sq] );

  AttackBishop( bb, sq );
  u |= BBContract( BB_W_BH, bb );

  u |= BB_W_RD.p[aslide[sq].ir0] & AttackRank( sq );

  bb = AttackFile( sq );
  u |= ( ( BB_WLANCE.p[0] & abb_minus_rays[sq].p[0] )
	 | BB_W_RD.p[0] ) & bb.p[0];
  u |= ( ( BB_WLANCE.p[1] & abb_minus_rays[sq].p[1] )
	       | BB_W_RD.p[1] ) & bb.p[1];
  u |= ( ( BB_WLANCE.p[2] & abb_minus_rays[sq].p[2] )
	       | BB_W_RD.p[2] ) & bb.p[2];

  return u;
}
