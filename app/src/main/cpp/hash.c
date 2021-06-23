#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "shogi.h"

static int eval_supe( unsigned int hand_current, unsigned int hand_hash,
		      int turn_current, int turn_hash,
		      int * restrict pvalue_hash, int * restrict ptype_hash );

int
ini_trans_table( void )
{
  size_t size;
  unsigned int ntrans_table;

  ntrans_table = 1U << log2_ntrans_table;
  size         = sizeof( trans_table_t ) * ntrans_table + 15U;
  ptrans_table_orig = memory_alloc( size );
  if ( ptrans_table_orig == NULL ) { return -1; }
  ptrans_table = (trans_table_t *)( ((ptrdiff_t)ptrans_table_orig+15)
				    & ~(ptrdiff_t)15U );
  hash_mask    = ntrans_table - 1;
  Out( "Trans. Table Entries = %dK (%dMB)\n",
       ( ntrans_table * 3U ) / 1024U, size / (1024U * 1024U ) );

  return clear_trans_table();
}


#define Foo( PIECE, piece )  bb = BB_B ## PIECE;                    \
                             while( BBToU(bb) ) {                   \
                               sq = FirstOne( bb );                 \
                               Xor( sq, bb );                       \
                               key ^= ( b_ ## piece ## _rand )[sq]; \
                             }                                      \
                             bb = BB_W ## PIECE;                    \
                             while( BBToU(bb) ) {                   \
                               sq = FirstOne( bb );                 \
                               Xor( sq, bb );                       \
                               key ^= ( w_ ## piece ## _rand )[sq]; \
                             }

uint64_t
hash_func( const tree_t * restrict ptree )
{
  uint64_t key = 0;
  bitboard_t bb;
  int sq;

  key ^= b_king_rand[SQ_BKING];
  key ^= w_king_rand[SQ_WKING];

  Foo( PAWN,       pawn );
  Foo( LANCE,      lance );
  Foo( KNIGHT,     knight );
  Foo( SILVER,     silver );
  Foo( GOLD,       gold );
  Foo( BISHOP,     bishop );
  Foo( ROOK,       rook );
  Foo( PRO_PAWN,   pro_pawn );
  Foo( PRO_LANCE,  pro_lance );
  Foo( PRO_KNIGHT, pro_knight );
  Foo( PRO_SILVER, pro_silver );
  Foo( HORSE,      horse );
  Foo( DRAGON,     dragon );

  return key;
}

#undef Foo


/*
       name    bits  shifts
word1  depth     8     56
       value    16     40
       move     19     21
       hand     21      0
word2  key      57      7 (slot 1 31)
       turn      1      6
       threat    1      5
       type      2      3
       age       3      0
 */

void
hash_store( const tree_t * restrict ptree, int ply, int depth, int turn,
	    int value_type, int value, unsigned int move,
	    unsigned int state_node )
{
  uint64_t word1, word2, hash_word1, hash_word2;
  unsigned int index, slot;
  int depth_hash, age_hash;

#if ! defined(MINIMUM)
  if ( game_status & flag_learning ) { return; }
#endif
  assert( depth <= 0xff );

  if ( depth < 0 ) { depth = 0; }
  if ( abs(value) > score_max_eval )
    {
      if ( abs(value) > score_mate1ply ) { return; }
      if ( value > 0 ) { value += ply-1; }
      else             { value -= ply-1; }
#if ! defined(MINIMUM)
      if ( abs(value) > score_mate1ply )
	{
	  out_warning( "A stored hash value is out of bounce!" );
	}
#endif
    }
  word2 = ( ( HASH_KEY & ~(uint64_t)0x7fU )
	    | (uint64_t)( (turn<<6) | ( state_node & node_mate_threat )
			  | (value_type<<3) | trans_table_age ) );
  word1 = ( ( (uint64_t)( depth<<16 | (value+32768) ) << 40 )
	    | ( (uint64_t)( move & 0x7ffffU ) << 21 )
	    | HAND_B );

  index = (unsigned int)HASH_KEY & hash_mask;
  hash_word1 = ptrans_table[index].prefer.word1;
  hash_word2 = ptrans_table[index].prefer.word2;
  SignKey( hash_word2, hash_word1 );
  age_hash   = (int)((unsigned int)(hash_word2    ) & 0x07U);
  depth_hash = (int)((unsigned int)(hash_word1>>56) & 0xffU);

  if ( age_hash != trans_table_age || depth_hash <= depth )
    {
      unsigned int hand_hash;
      uint64_t keyt_hash;

      hand_hash = (unsigned int)hash_word1 & 0x1fffffU;
      keyt_hash = hash_word2 & ~(uint64_t)0x2fU;

      if ( hand_hash != HAND_B
	   && keyt_hash != ( word2 & ~(uint64_t)0x2fU ) )
	{
	  slot = (unsigned int)hash_word2 >> 31;
	  SignKey( hash_word2, hash_word1 );
	  ptrans_table[index].always[slot].word1 = hash_word1;
	  ptrans_table[index].always[slot].word2 = hash_word2;
	}
      SignKey( word2, word1 );
      ptrans_table[index].prefer.word1 = word1;
      ptrans_table[index].prefer.word2 = word2;
    }
  else {
    slot = (unsigned int)HASH_KEY >> 31;
    SignKey( word2, word1 );
    ptrans_table[index].always[slot].word1 = word1;
    ptrans_table[index].always[slot].word2 = word2;
  }
}


void
hash_store_pv( const tree_t * restrict ptree, unsigned int move, int turn )
{
  uint64_t key_turn_pv, word1, word2;
  unsigned int index;

  key_turn_pv = ( HASH_KEY & ~(uint64_t)0x7fU ) | (unsigned int)( turn << 6 );
  index       = (unsigned int)HASH_KEY & hash_mask;

  word1 = ptrans_table[index].prefer.word1;
  word2 = ptrans_table[index].prefer.word2;
  SignKey( word2, word1 );

  if ( ( (unsigned int)word1 & 0x1fffffU ) == HAND_B
       && ( word2 & ~(uint64_t)0x3fU ) == key_turn_pv )
    {
      if ( ( (unsigned int)(word1>>21) & 0x7ffffU ) != ( move & 0x7ffffU ) )
	{
	  word1 &= ~((uint64_t)0x7ffffU << 21);
	  word1 |= (uint64_t)( move & 0x7ffffU ) << 21;
	  word2 &= ~((uint64_t)0x3U << 3);
	  SignKey( word2, word1 );
	  ptrans_table[index].prefer.word1 = word1;
	  ptrans_table[index].prefer.word2 = word2;
	}
    }
  else {
    unsigned int slot;

    slot = (unsigned int)HASH_KEY >> 31;
    word1 = ptrans_table[index].always[slot].word1;
    word2 = ptrans_table[index].always[slot].word2;
    SignKey( word2, word1 );
    if ( ( (unsigned int)word1 & 0x1fffffU ) == HAND_B
	 && ( word2 & ~(uint64_t)0x3fU ) == key_turn_pv )
      {
	if ( ( (unsigned int)(word1>>21) & 0x7ffffU )
	     != ( move & 0x7ffffU ) )
	  {
	    word1 &= ~((uint64_t)0x7ffffU << 21);
	    word1 |= (uint64_t)( move & 0x7ffffU ) << 21;
	    word2 &= ~((uint64_t)0x3U << 3);
	    SignKey( word2, word1 );
	    ptrans_table[index].always[slot].word1 = word1;
	    ptrans_table[index].always[slot].word2 = word2;
	  }
      }
    else {
      word1  = (uint64_t)32768U << 40;
      word1 |= (uint64_t)( move & 0x7ffffU ) << 21;
      word1 |= HAND_B;
      word2  = key_turn_pv | trans_table_age;
      SignKey( word2, word1 );
      ptrans_table[index].prefer.word1 = word1;
      ptrans_table[index].prefer.word2 = word2;
    }
  }
}


trans_entry_t
hash_learn_store( const tree_t * restrict ptree, int depth, int value,
		  unsigned int move )
{
  trans_entry_t ret;

  assert( 0 <= depth && depth <= 0xff );

  ret.word2 = ( (HASH_KEY&(~(uint64_t)0x7fU))
		| (uint64_t)( (root_turn<<6)
			      | (value_exact<<3) | trans_table_age ) );
  ret.word1 = ( ( (uint64_t)( depth<<16 | (value+32768) ) << 40 )
		| ( (uint64_t)( move & 0x7ffffU ) << 21 )
		| HAND_B );

  return ret;
}


int
all_hash_learn_store( void )
{
  uint64_t aword[2];
  unsigned int u32key, unext, u, index;

  if ( pf_hash == NULL ) { return 0; }

  if ( fseek( pf_hash, sizeof(unsigned int), SEEK_SET ) == EOF
       || fread( &unext, sizeof(unsigned int), 1, pf_hash ) != 1 )
    {
      str_error = "hash1";
      return -2;
    }
  if ( ++unext == 0x10000U ) { unext = 0x1U; }
  if ( fseek( pf_hash, (long)( 20U * unext ), SEEK_SET ) == EOF )
    {
      str_error = "hash2";
      return -2;
    }
  for ( u = 0;; u++ )
    {
      if ( fread( &u32key, sizeof(unsigned int), 1, pf_hash ) != 1
	   || fread( aword, sizeof(uint64_t), 2, pf_hash ) != 2 )
	{
	  str_error = "hash6";
	  return -2;
	}
      index = u32key & hash_mask;
      aword[1] |= (uint64_t)trans_table_age;
      SignKey( aword[1], aword[0] );
      ptrans_table[index].prefer.word1 = aword[0];
      ptrans_table[index].prefer.word2 = aword[1];
      if ( u == 0xfffeU ) { break; }
      if ( ++unext == 0x10000U )
	{
	  unext = 0x1U;
	  if ( fseek( pf_hash, 20, SEEK_SET ) == EOF )
	    {
	      str_error = "hash8";
	      return -2;
	    }
	}
    }

  return 1;
}


unsigned int
hash_probe( tree_t * restrict ptree, int ply, int depth_current,
	    int turn_current, int alpha, int beta, unsigned int state_node )
{
  uint64_t word1, word2, key_current, key_hash;
  unsigned int hand_hash, move_hash, move_infe, move_supe, slot, utemp;
  unsigned int state_node_hash, index;
  int null_depth, value_hash, ifrom;
  int turn_hash, depth_hash, type_hash, is_superior;

  ptree->ntrans_probe++;
  move_supe   = 0;
  move_infe   = 0;

  if ( depth_current < 0 ) { depth_current = 0; }
  null_depth  = NullDepth( depth_current );
  if ( null_depth < PLY_INC ) { null_depth = 0; }

  key_current = HASH_KEY & ~(uint64_t)0x7fU;

  index = (unsigned int)HASH_KEY & hash_mask;
  word1 = ptrans_table[index].prefer.word1;
  word2 = ptrans_table[index].prefer.word2;
  SignKey( word2, word1 );
  key_hash = word2 & ~(uint64_t)0x7fU;

  if ( key_hash == key_current )
    {
      ptree->ntrans_prefer_hit++;

      depth_hash  = (int)((unsigned int)(word1>>56) & 0x00ffU);
      value_hash  = (int)((unsigned int)(word1>>40) & 0xffffU) - 32768;
      move_hash   = (unsigned int)(word1>>21) & 0x7ffffU;
      hand_hash   = (unsigned int)word1 & 0x1fffffU;
      
      utemp           = (unsigned int)word2;
      state_node_hash = utemp & node_mate_threat;
      turn_hash       = (int)((utemp>>6) & 0x1U);
      type_hash       = (int)((utemp>>3) & 0x3U);

      if ( abs(value_hash) > score_max_eval )
	{
	  if ( value_hash > 0 ) { value_hash -= ply-1; }
	  else                  { value_hash += ply-1; }
#if ! defined(MINIMUM)
	  if ( abs(value_hash) > score_mate1ply )
	    {
	      out_warning( "Hash value is out of bounce!!" );
	    }
#endif
	}

      if ( move_hash )
	{
	  move_hash |= turn_current ? Cap2Move( BOARD[I2To(move_hash)])
                                    : Cap2Move(-BOARD[I2To(move_hash)]);
	}

      if ( turn_hash == turn_current && hand_hash == HAND_B )
	{
	  assert( ! move_hash
		  || is_move_valid( ptree, move_hash, turn_current ) );
	  ptree->amove_hash[ply] = move_hash;

	  if ( depth_hash >= depth_current )
	    {
	      switch ( type_hash )
		{
		case value_lower:
		  if ( value_hash >= beta )
		    {
		      HASH_VALUE = value_hash;
		      ptree->ntrans_lower++;
		      return value_lower;
		    }
		  break;
		case value_upper:
		  if ( value_hash <= alpha )
		    {
		      HASH_VALUE = value_hash;
		      ptree->ntrans_upper++;
		      return value_upper;
		    }
		  break;
		case value_exact:
		  HASH_VALUE = value_hash;
		  ptree->ntrans_exact++;
		  return value_exact;
		}
	    }

	  if ( ( type_hash & flag_value_low_exact )
	       && ! ptree->nsuc_check[ply]
	       && ! ptree->nsuc_check[ply-1] )
	    {
	      if ( ( depth_current < 2*PLY_INC
		     && beta+EFUTIL_MG1 <= value_hash )
		   || ( depth_current < 3*PLY_INC
			&& beta+EFUTIL_MG2 <= value_hash ) )
		{
		  HASH_VALUE = beta;
		  ptree->ntrans_lower++;
		  return value_lower;
		}
	    }

	  state_node |= state_node_hash;

	  if ( type_hash & flag_value_up_exact )
	    {
	      if ( value_hash <= score_max_eval )
		{
		  state_node &= ~node_do_mate;
		}
	      if ( value_hash < beta && null_depth <= depth_hash )
		{
		  state_node &= ~node_do_null;
		}
	    }
	}
      else {
	is_superior = eval_supe( HAND_B, hand_hash, turn_current,
				 turn_hash, &value_hash, &type_hash );

	if ( is_superior == 1 )
	  {
	    if ( turn_hash == turn_current )
	      {
		move_supe = move_hash;
		if ( value_hash <= score_max_eval )
		  {
		    state_node &= ~node_do_mate;
		  }
	      }
	    if ( type_hash & flag_value_low_exact )
	      {
		if ( ! ptree->nsuc_check[ply]
		     && ! ptree->nsuc_check[ply-1] )
		  {
		    if ( ( depth_current < 2*PLY_INC
			   && beta+EFUTIL_MG1 <= value_hash )
			 || ( depth_current < 3*PLY_INC
			      && beta+EFUTIL_MG2 <= value_hash ) )
		      {
			HASH_VALUE = beta;
			ptree->ntrans_lower++;
			return value_lower;
		      }
		  }

		if ( beta <= value_hash
		     && ( depth_current <= depth_hash
			  || ( turn_current != turn_hash
			       && depth_hash >= null_depth
			       && ( state_node & node_do_null ) ) ) )
		  {
		    HASH_VALUE = value_hash;
		    ptree->ntrans_superior_hit++;
		    return value_lower;
		  }
	      }
	  }
	else {
	  if ( turn_hash == turn_current ) { move_infe = move_hash; }
	  if ( is_superior == -1 )
	    {
	      state_node |= state_node_hash;
	      if ( type_hash & flag_value_up_exact )
		{
		  if ( depth_hash >= depth_current && value_hash <= alpha )
		    {
		      HASH_VALUE = value_hash;
		      ptree->ntrans_inferior_hit++;
		      return value_upper;
		    }
		  if ( value_hash <= score_max_eval )
		    {
		      state_node &= ~node_do_mate;
		    }
		  if ( value_hash < beta && null_depth <= depth_hash )
		    {
		      state_node &= ~node_do_null;
		    }
		}
	    }
	}
      }
    }
  
  slot  = (unsigned int)HASH_KEY >> 31;
  word1 = ptrans_table[index].always[slot].word1;
  word2 = ptrans_table[index].always[slot].word2;
		       
  SignKey( word2, word1 );
  key_hash = word2 & ~(uint64_t)0x7fU;

  if ( key_hash == key_current )
    {
      ptree->ntrans_always_hit++;

      depth_hash  = (int)((unsigned int)(word1>>56) & 0x00ffU);
      value_hash  = (int)((unsigned int)(word1>>40) & 0xffffU) - 32768;
      move_hash   = (unsigned int)(word1>>21) & 0x7ffffU;
      hand_hash   = (unsigned int)word1 & 0x1fffffU;
  
      utemp           = (unsigned int)word2;
      state_node_hash = utemp & node_mate_threat;
      turn_hash       = (int)((utemp>>6) & 0x1U);
      type_hash       = (int)((utemp>>3) & 0x3U);
      
      if ( abs(value_hash) > score_max_eval )
	{
	  if ( value_hash > 0 ) { value_hash -= ply-1; }
	  else                  { value_hash += ply-1; }
#if ! defined(MINIMUM)
	  if ( abs(value_hash) > score_mate1ply )
	    {
	      out_warning( "Hash value is out of bounce!!" );
	    }
#endif
	}

      if ( move_hash )
	{
	  move_hash |= turn_current ? Cap2Move( BOARD[I2To(move_hash)])
                                    : Cap2Move(-BOARD[I2To(move_hash)]);
	}

      if ( turn_hash == turn_current && hand_hash == HAND_B )
	{
	  if ( ! ptree->amove_hash[ply] )
	    {
	      assert( ! move_hash
		      || is_move_valid( ptree, move_hash, turn_current ) );
	      ptree->amove_hash[ply] = move_hash;
	    }

	  if ( depth_hash >= depth_current )
	    {
	      switch ( type_hash )
		{
		case value_lower:
		  if ( value_hash >= beta )
		    {
		      HASH_VALUE = value_hash;
		      ptree->ntrans_lower++;
		      return value_lower;
		    }
		  break;

		case value_upper:
		  if ( value_hash <= alpha )
		    {
		      HASH_VALUE = value_hash;
		      ptree->ntrans_upper++;
		      return value_upper;
		    }
		  break;

		case value_exact:
		  HASH_VALUE = value_hash;
		  ptree->ntrans_exact++;
		  return value_exact;
		}
	    }

	  if ( ( type_hash & flag_value_low_exact )
	       && ! ptree->nsuc_check[ply]
	       && ! ptree->nsuc_check[ply-1] )
	    {
	      if ( ( depth_current < 2*PLY_INC
		     && beta+EFUTIL_MG1 <= value_hash )
		   || ( depth_current < 3*PLY_INC
			&& beta+EFUTIL_MG2 <= value_hash ) )
		{
		  HASH_VALUE = beta;
		  ptree->ntrans_lower++;
		  return value_lower;
		}
	    }

	  state_node |= state_node_hash;

	  if ( type_hash & flag_value_up_exact )
	    {
	      if ( value_hash <= score_max_eval )
		{
		  state_node &= ~node_do_mate;
		}
	      if ( value_hash < beta && null_depth <= depth_hash )
		{
		  state_node &= ~node_do_null;
		}
	    }
	}
      else {
	is_superior = eval_supe( HAND_B, hand_hash, turn_current,
				 turn_hash, &value_hash, &type_hash );

	if ( is_superior == 1 )
	  {
	    if ( turn_hash == turn_current )
	      {
		if ( ! move_supe ) { move_supe = move_hash; }
		if ( value_hash <= score_max_eval )
		  {
		    state_node &= ~node_do_mate;
		  }
	      }
	    if ( type_hash & flag_value_low_exact )
	      {
		if ( ! ptree->nsuc_check[ply]
		     && ! ptree->nsuc_check[ply-1] )
		  {
		    if ( ( depth_current < 2*PLY_INC
			   && beta+EFUTIL_MG1 <= value_hash )
			 || ( depth_current < 3*PLY_INC
			      && beta+EFUTIL_MG2 <= value_hash ) )
		      {
			HASH_VALUE = beta;
			ptree->ntrans_lower++;
			return value_lower;
		      }
		  }
		if ( value_hash >= beta
		     && ( depth_hash >= depth_current
			  || ( turn_current != turn_hash
			       && depth_hash >= null_depth
			       && ( state_node & node_do_null ) ) ) )
		  {
		    HASH_VALUE = value_hash;
		    ptree->ntrans_superior_hit++;
		    return value_lower;
		  }
	      }
	  }
	else {
	  if ( ! move_infe && turn_hash == turn_current )
	    {
	      move_infe = move_hash;
	    }
	  if ( is_superior == -1 )
	    {
	      state_node |= state_node_hash;
	      if ( type_hash & flag_value_up_exact )
		{
		  if ( depth_hash >= depth_current && value_hash<= alpha )
		    {
		      HASH_VALUE = value_hash;
		      ptree->ntrans_inferior_hit++;
		      return value_upper;
		    }
		  if ( value_hash <= score_max_eval )
		    {
		      state_node &= ~node_do_mate;
		    }
		  if ( value_hash < beta && null_depth <= depth_hash )
		    {
		      state_node &= ~node_do_null;
		    }
		}
	    }
	}
      }
    }

  if ( ! ptree->amove_hash[ply] )
    {
      if ( move_supe )
	{
#if 1
	  ifrom = (int)I2From(move_supe);
	  if ( ifrom >= nsquare )
	    {
	      unsigned int hand = turn_current ? HAND_W : HAND_B;
	      switch( From2Drop(ifrom) )
		{
		case pawn:
		  if ( ! IsHandPawn(hand) ) {
		    move_supe = To2Move(I2To(move_supe));
		    if ( IsHandLance(hand) )
		      {
			move_supe |= Drop2Move(lance);
		      }
		    else if ( IsHandSilver(hand))
		      {
			move_supe |= Drop2Move(silver);
		      }
		    else if ( IsHandGold(hand) )
		      {
			move_supe |= Drop2Move(gold);
		      }
		    else { move_supe |= Drop2Move(rook); }
		  }
		  break;
		
		case lance:
		  if ( ! IsHandLance(hand) )
		    {
		      move_supe = To2Move(I2To(move_supe)) | Drop2Move(rook);
		    }
		  break;
		}
	    }
#endif
	  assert( is_move_valid( ptree, move_supe, turn_current ) );
	  ptree->amove_hash[ply] = move_supe;
	}
      else if ( move_infe )
	{
	  ifrom = (int)I2From(move_infe);
	  if ( ifrom >= nsquare )
	    {
	      unsigned int hand = turn_current ? HAND_W : HAND_B;
	      switch( From2Drop(ifrom) )
		{
		case pawn:   if ( ! IsHandPawn(hand) )   { goto esc; } break;
		case lance:  if ( ! IsHandLance(hand) )  { goto esc; } break;
		case knight: if ( ! IsHandKnight(hand) ) { goto esc; } break;
		case silver: if ( ! IsHandSilver(hand) ) { goto esc; } break;
		case gold:   if ( ! IsHandGold(hand) )   { goto esc; } break;
		case bishop: if ( ! IsHandBishop(hand) ) { goto esc; } break;
		case rook:   if ( ! IsHandRook(hand) )   { goto esc; } break;
		}
	    }
	  assert( is_move_valid( ptree, move_infe, turn_current ) );
	  ptree->amove_hash[ply] = move_infe;
	}
    }
  
 esc:
  return state_node;
}


int
hash_learn_on( void )
{
  int iret = file_close( pf_hash );
  if ( iret < 0 ) { return iret; }

  pf_hash = file_open( str_hash, "rb+" );
  if ( pf_hash == NULL ) { return -2; }

  return 1;
}


int
hash_learn_off( void )
{
  int iret = file_close( pf_hash );
  if ( iret < 0 ) { return iret; }

  pf_hash = NULL;

  return 1;
}

#if !defined(MINIMUM)
int
hash_learn_create( void )
{
  uint64_t au64[2];
  unsigned int u;
  int iret, i;

  iret = hash_learn_off();
  if ( iret < 0 ) { return iret; }

  pf_hash = file_open( str_hash, "wb" );
  if ( pf_hash == NULL ) { return -2; }

  for ( i = 0; i < 5; i++ )
    {
      u = 0;
      if ( fwrite( &u, sizeof(unsigned int), 1, pf_hash ) != 1 )
	{
	  str_error = str_io_error;
	  return -2;
	}
    }

  u = 0;
  au64[0] = au64[1] = 0;
  for ( i = 1; i < 0x10000; i++ )
    if ( fwrite( &u, sizeof(unsigned int), 1, pf_hash ) != 1
	 || fwrite( au64, sizeof(uint64_t), 2, pf_hash ) != 2 )
      {
	str_error = str_io_error;
	return -2;
      }

  return hash_learn_on();
}
#endif

int
hash_learn( const tree_t * restrict ptree, unsigned int move, int value,
	    int depth )
{
  trans_entry_t trans_entry;
  unsigned int unum, unext, u;
  int pre_value, ply;

  ply = record_game.moves;
  if ( ply >= HASH_REG_HIST_LEN )    { return 1; }
  if ( pf_hash == NULL )             { return 1; }
  if ( abs(value) > score_max_eval ) { return 1; }
  if ( ply < 2 )                     { return 1; }
  if ( depth < 2 )                   { return 1; }

  if ( history_book_learn[ply].key_probed == (unsigned int)HASH_KEY
       && history_book_learn[ply].hand_probed == HAND_B
       && history_book_learn[ply].move_probed == move ) { return 1; }

  if ( history_book_learn[ply-2].key_responsible
       != history_book_learn[ply-2].key_played ) { return 1; }
  if ( history_book_learn[ply-2].hand_responsible
       != history_book_learn[ply-2].hand_played ) { return 1; }
  if ( history_book_learn[ply-2].move_responsible
       != history_book_learn[ply-2].move_played ) { return 1; }

  if ( ( history_book_learn[ply-2].key_probed
	 == history_book_learn[ply-2].key_played )
       && ( history_book_learn[ply-2].hand_probed
	    == history_book_learn[ply-2].hand_played )
       && ( history_book_learn[ply-2].move_probed
	    == history_book_learn[ply-2].move_played ) ) { return 1; }
	  
  pre_value = (int)( history_book_learn[ply-2].data & 0xffffU ) - 32768;

  if ( pre_value < value + HASH_REG_MINDIFF ) { return 1; }
  if ( pre_value < -HASH_REG_THRESHOLD )      { return 1; }
  if ( pre_value == score_inferior )          { return 1; }

  Out( "save hash value of the position\n\n" );
  if ( fseek( pf_hash, 0, SEEK_SET ) == EOF
       || fread( &unum,  sizeof(unsigned int), 1, pf_hash ) != 1
       || fread( &unext, sizeof(unsigned int), 1, pf_hash ) != 1 )
    {
      str_error = "hash3";
      return -2;
    }
  if ( ++unum  == 0x10000U ) { unum  = 0xffffU; }
  if ( ++unext == 0x10000U ) { unext = 0x0001U; }

  if ( fseek( pf_hash, 0, SEEK_SET ) == EOF
       || fwrite( &unum,  sizeof(unsigned int), 1, pf_hash ) != 1
       || fwrite( &unext, sizeof(unsigned int), 1, pf_hash ) != 1 )
    {
      str_error = "hash7";
      return -2;
    }
  trans_entry
    = hash_learn_store( ptree, depth * PLY_INC + PLY_INC/2, value, move );
  u = (unsigned int)HASH_KEY;
  if ( fseek( pf_hash, (long)( 20 * unext ), SEEK_SET ) == EOF
       || fwrite( &u, sizeof(unsigned int), 1, pf_hash ) != 1
       || fwrite( &trans_entry.word1, sizeof(uint64_t), 1, pf_hash ) != 1
       || fwrite( &trans_entry.word2, sizeof(uint64_t), 1, pf_hash ) != 1
       || fflush( pf_hash ) == EOF )
    {
      str_error = "hash9";
      return -2;
    }
  
  return 1;
}


static int
eval_supe( unsigned int hand_current, unsigned int hand_hash,
	   int turn_current, int turn_hash,
	   int * restrict pvalue_hash, int * restrict ptype_hash )
{
  int is_superior;

  if ( hand_current == hand_hash ) { is_superior = 0; }
  else if ( is_hand_eq_supe( hand_current, hand_hash ) )
    {
      is_superior = turn_current ? -1 : 1;
    }
  else if ( is_hand_eq_supe( hand_hash, hand_current ) )
    {
      is_superior = turn_current ? 1 : -1;
    }
  else { return 0; }
  
  if ( turn_hash != turn_current )
    {
      if ( is_superior == -1 ) { is_superior = 0; }
      else {
	is_superior   = 1;
	*pvalue_hash *= -1;
	switch ( *ptype_hash )
	  {
	  case value_lower:  *ptype_hash=value_upper;  break;
	  case value_upper:  *ptype_hash=value_lower;  break;
	  }
      }
    }

  return is_superior;
}


int
clear_trans_table( void )
{
  unsigned int elapsed_start, elapsed_end;
  int ntrans_table, i;

  if ( get_elapsed( &elapsed_start ) < 0 ) { return -1; }

  Out( "cleanning the transposition table ..." );
  
  trans_table_age = 1;
  ntrans_table = 1 << log2_ntrans_table;
  for ( i = 0; i < ntrans_table; i++ )
    {
      ptrans_table[i].prefer.word1    = 0;
      ptrans_table[i].prefer.word2    = 0;
      ptrans_table[i].always[0].word1 = 0;
      ptrans_table[i].always[0].word2 = 0;
      ptrans_table[i].always[1].word1 = 0;
      ptrans_table[i].always[1].word2 = 0;
    }

  if ( get_elapsed( &elapsed_end ) < 0 ) { return -1; }
  Out( " done (%ss)\n", str_time_symple( elapsed_end - elapsed_start ) );

  return 1;
}


void
add_rejections_root( tree_t * restrict ptree, unsigned int move_made )
{
  uint64_t hash_value;
  unsigned int * restrict pmove;
  unsigned int *pmove_last;
  unsigned int hash_key, hand_ply_turn;
  int tt;
  unsigned char hash_parent;

  tt = Flip( root_turn );
  UnMakeMove( tt, move_made, 1 );
  hash_parent = (unsigned char)(HASH_KEY >> 32);

  pmove      = ptree->amove;
  pmove_last = GenCaptures( tt, pmove );
  pmove_last = GenNoCaptures( tt, pmove_last );
  pmove_last = GenCapNoProEx2( tt, pmove_last );
  pmove_last = GenNoCapNoProEx2( tt, pmove_last );
  pmove_last = GenDrop( tt, pmove_last );

  while ( pmove != pmove_last )
    {
      if ( *pmove != move_made )
	{
	  MakeMove( tt, *pmove, 1 );
	  if ( ! InCheck( tt ) )
	    {
	      hash_key      = (unsigned int)HASH_KEY & REJEC_MASK;
	      hand_ply_turn = ( HAND_B << 6 ) | 2U | (unsigned int)tt;
	      hash_value    = ( ( HASH_KEY & ~(uint64_t)0x7ffffffU )
				| (uint64_t)hand_ply_turn );
	      large_object->hash_rejections[hash_key].root   = hash_value;
	      large_object->hash_rejections_parent[hash_key] = hash_parent;
	    }      
	  UnMakeMove( tt, *pmove, 1 );
	}
      pmove++;
    }

  MakeMove( tt, move_made, 1 );
}


void
sub_rejections_root( tree_t * restrict ptree, unsigned int move_made )
{
  unsigned int * restrict pmove;
  unsigned int *pmove_last;
  unsigned int hash_key;

  pmove      = ptree->amove;
  pmove_last = GenCaptures( root_turn, pmove );
  pmove_last = GenNoCaptures( root_turn, pmove_last );
  pmove_last = GenCapNoProEx2( root_turn, pmove_last );
  pmove_last = GenNoCapNoProEx2( root_turn, pmove_last );
  pmove_last = GenDrop( root_turn, pmove_last );

  while ( pmove != pmove_last )
    {
      if ( *pmove != move_made )
	{
	  MakeMove( root_turn, *pmove, 1 );
	  if ( ! InCheck( root_turn ) )
	    {
	      hash_key = (unsigned int)HASH_KEY & REJEC_MASK;

	      large_object->hash_rejections[hash_key].root   = 0;
	      large_object->hash_rejections_parent[hash_key] = 0;
	    }      
	  UnMakeMove( root_turn, *pmove, 1 );
	}
      pmove++;
    }
}


void
add_rejections( tree_t * restrict ptree, int turn, int ply )
{
  uint64_t hash_value;
  unsigned int * restrict pmove;
  unsigned int * restrict pmove_last;
  unsigned int hash_key, hand_ply_turn;

#if ! defined(MINIMUM)
  if ( game_status & flag_learning ) { return; }
#endif

  pmove      = ptree->move_last[ply-1];
  pmove_last = GenCaptures( turn, pmove );
  pmove_last = GenNoCaptures( turn, pmove_last );
  pmove_last = GenCapNoProEx2( turn, pmove_last );
  pmove_last = GenNoCapNoProEx2( turn, pmove_last );
  pmove_last = GenDrop( turn, pmove_last );

  while ( pmove != pmove_last )
    {
      MakeMove( turn, *pmove, ply );
      if ( ! InCheck( turn ) )
	{
	  hash_key = (unsigned int)HASH_KEY & REJEC_MASK;
	  if ( ! (unsigned int)large_object->hash_rejections[hash_key].sibling )
	    {
	      hand_ply_turn = ( ( HAND_B << 6 ) | ( (unsigned int)ply << 1 )
				| (unsigned int)turn );
	      hash_value    = ( ( HASH_KEY & ~(uint64_t)0x7ffffffU )
				| (uint64_t)hand_ply_turn );
	      large_object->hash_rejections[hash_key].sibling = hash_value;
#if defined(TLP)
	      tlp_rejections_slot[hash_key] = (unsigned short)
		( ptree->tlp_slot ^ (unsigned short)( hash_value >> 32 ) );
#endif
	    }
	}
      UnMakeMove( turn, *pmove, ply );
      pmove++;
    }
}


void
sub_rejections( tree_t * restrict ptree, int turn, int ply )
{
  uint64_t hash_value;
  unsigned int * restrict pmove;
  unsigned int * restrict pmove_last;
  unsigned int hash_key, hand_ply_turn;

#if ! defined(MINIMUM)
  if ( game_status & flag_learning ) { return; }
#endif

  pmove      = ptree->move_last[ply-1];
  pmove_last = GenCaptures( turn, pmove );
  pmove_last = GenNoCaptures( turn, pmove_last );
  pmove_last = GenCapNoProEx2( turn, pmove_last );
  pmove_last = GenNoCapNoProEx2( turn, pmove_last );
  pmove_last = GenDrop( turn, pmove_last );

  while ( pmove != pmove_last )
    {
      MakeMove( turn, *pmove, ply );
      if ( ! InCheck( turn ) )
	{
	  hash_key      = (unsigned int)HASH_KEY & REJEC_MASK;
	  hand_ply_turn = ( ( HAND_B << 6 )
			    | ( (unsigned int)ply << 1 )
			    | (unsigned int)turn );
	  hash_value    = ( ( HASH_KEY & ~(uint64_t)0x7ffffffU )
			    | (uint64_t)hand_ply_turn );
	  
	  if ( large_object->hash_rejections[hash_key].sibling == hash_value )
	    {
	      large_object->hash_rejections[hash_key].sibling = 0;
	    }
	}
      UnMakeMove( turn, *pmove, ply );
      pmove++;
    }
}


int
rejections_probe( tree_t * restrict ptree, int turn, int ply )
{
  uint64_t value_turn, value_turn_current, value;
  unsigned int hand_hash, hand_current, key_current;
  int nrep, value_ply;
  unsigned char parent_hash, parent_current;

  turn               = Flip(turn);
  hand_current       = HAND_B;
  key_current        = (unsigned int)HASH_KEY & REJEC_MASK;
  value_turn_current = ( HASH_KEY & ~(uint64_t)0x7ffffffU ) | (uint64_t)turn;

  value = large_object->hash_rejections[key_current].root;
  value_turn = value & ~(uint64_t)0x7fffffeU;
  if ( value_turn == value_turn_current )
    {
      hand_hash = ( (unsigned int)value & 0x7ffffffU ) >> 6;
      if ( ( turn && is_hand_eq_supe( hand_current, hand_hash ) )
	   || ( ! turn && is_hand_eq_supe( hand_hash, hand_current ) ) )
	{
	  nrep = root_nrep + ply - 2;
	  parent_current = (unsigned char)(ptree->rep_board_list[nrep] >> 32);
	  parent_hash    = large_object->hash_rejections_parent[key_current];
	  if ( parent_hash != parent_current ) { return 1; }
	}
    }

  value = large_object->hash_rejections[key_current].sibling;
  value_ply = ( (int)value & 0x3eU ) >> 1;
  if ( value_ply + 2 < ply )
    {
      value_turn = value & ~(uint64_t)0x7fffffeU;
      if ( value_turn == value_turn_current )
	{
	  hand_hash = ( (unsigned int)value & 0x7ffffffU ) >> 6;
	  if ( ( turn && is_hand_eq_supe( hand_current, hand_hash ) )
	       || ( ! turn && is_hand_eq_supe( hand_hash, hand_current ) ) )
	    {
#if defined(TLP)
	      int slot_hash;
	      slot_hash = (int)( tlp_rejections_slot[key_current]
				 ^ (unsigned short)( value >> 32 ) );
	      if ( tlp_is_descendant( ptree, slot_hash ) )
		
#endif
		return 1;
	    }
	}
    }

  return 0;
}
