#include <stdlib.h>
#include <assert.h>
#include "shogi.h"

static int ehash_probe( uint64_t current_key, unsigned int hand_b,
			int *pscore );
static void ehash_store( uint64_t key, unsigned int hand_b, int score );
static int make_list( const tree_t * restrict ptree, int * restrict pscore,
		      int list0[52], int list1[52] );

int
eval_material( const tree_t * restrict ptree )
{
  int material, itemp;

  itemp     = PopuCount( BB_BPAWN )   + (int)I2HandPawn( HAND_B );
  itemp    -= PopuCount( BB_WPAWN )   + (int)I2HandPawn( HAND_W );
  material  = itemp * p_value[15+pawn];

  itemp     = PopuCount( BB_BLANCE )  + (int)I2HandLance( HAND_B );
  itemp    -= PopuCount( BB_WLANCE )  + (int)I2HandLance( HAND_W );
  material += itemp * p_value[15+lance];

  itemp     = PopuCount( BB_BKNIGHT ) + (int)I2HandKnight( HAND_B );
  itemp    -= PopuCount( BB_WKNIGHT ) + (int)I2HandKnight( HAND_W );
  material += itemp * p_value[15+knight];

  itemp     = PopuCount( BB_BSILVER ) + (int)I2HandSilver( HAND_B );
  itemp    -= PopuCount( BB_WSILVER ) + (int)I2HandSilver( HAND_W );
  material += itemp * p_value[15+silver];

  itemp     = PopuCount( BB_BGOLD )   + (int)I2HandGold( HAND_B );
  itemp    -= PopuCount( BB_WGOLD )   + (int)I2HandGold( HAND_W );
  material += itemp * p_value[15+gold];

  itemp     = PopuCount( BB_BBISHOP ) + (int)I2HandBishop( HAND_B );
  itemp    -= PopuCount( BB_WBISHOP ) + (int)I2HandBishop( HAND_W );
  material += itemp * p_value[15+bishop];

  itemp     = PopuCount( BB_BROOK )   + (int)I2HandRook( HAND_B );
  itemp    -= PopuCount( BB_WROOK )   + (int)I2HandRook( HAND_W );
  material += itemp * p_value[15+rook];

  itemp     = PopuCount( BB_BPRO_PAWN );
  itemp    -= PopuCount( BB_WPRO_PAWN );
  material += itemp * p_value[15+pro_pawn];

  itemp     = PopuCount( BB_BPRO_LANCE );
  itemp    -= PopuCount( BB_WPRO_LANCE );
  material += itemp * p_value[15+pro_lance];

  itemp     = PopuCount( BB_BPRO_KNIGHT );
  itemp    -= PopuCount( BB_WPRO_KNIGHT );
  material += itemp * p_value[15+pro_knight];

  itemp     = PopuCount( BB_BPRO_SILVER );
  itemp    -= PopuCount( BB_WPRO_SILVER );
  material += itemp * p_value[15+pro_silver];

  itemp     = PopuCount( BB_BHORSE );
  itemp    -= PopuCount( BB_WHORSE );
  material += itemp * p_value[15+horse];

  itemp     = PopuCount( BB_BDRAGON );
  itemp    -= PopuCount( BB_WDRAGON );
  material += itemp * p_value[15+dragon];

  return material;
}


int
evaluate( tree_t * restrict ptree, int ply, int turn )
{
  int list0[52], list1[52];
  int nlist, score, sq_bk, sq_wk, k0, k1, l0, l1, i, j, sum;

  ptree->neval_called++;

  if ( ptree->stand_pat[ply] != score_bound )
    {
      return (int)ptree->stand_pat[ply];
    }

  if ( ehash_probe( HASH_KEY, HAND_B, &score ) )
    {
      score                 = turn ? -score : score;
      ptree->stand_pat[ply] = (short)score;

      return score;
    }


  score = 0;
  nlist = make_list( ptree, &score, list0, list1 );
  sq_bk = SQ_BKING;
  sq_wk = Inv( SQ_WKING );

  sum = 0;
  for ( i = 0; i < nlist; i++ )
    {
      k0 = list0[i];
      k1 = list1[i];
      for ( j = 0; j <= i; j++ )
	{
	  l0 = list0[j];
	  l1 = list1[j];
	  assert( k0 >= l0 && k1 >= l1 );
	  sum += PcPcOnSq( sq_bk, k0, l0 );
	  sum -= PcPcOnSq( sq_wk, k1, l1 );
	}
    }

  score += sum;
  score /= FV_SCALE;

  score += MATERIAL;

#if ! defined(MINIMUM)
  if ( abs(score) > score_max_eval )
    {
      out_warning( "A score at evaluate() is out of bounce." );
    }
#endif

  ehash_store( HASH_KEY, HAND_B, score );

  score = turn ? -score : score;
  ptree->stand_pat[ply] = (short)score;

  return score;

}


void ehash_clear( void )
{
  int i;

  for ( i = 0; i < EHASH_MASK + 1; i++ ) { large_object->ehash_tbl[i] = UINT64_C(0); }
}


static int ehash_probe( uint64_t current_key, unsigned int hand_b,
			int *pscore )
{
  uint64_t hash_word, hash_key;

  hash_word = large_object->ehash_tbl[ (unsigned int)current_key & EHASH_MASK ];

  current_key ^= (uint64_t)hand_b << 16;
  current_key &= ~(uint64_t)0xffffU;

  hash_key  = hash_word;
  hash_key &= ~(uint64_t)0xffffU;

  if ( hash_key != current_key ) { return 0; }

  *pscore = (int)( (unsigned int)hash_word & 0xffffU ) - 32768;

  return 1;
}


static void ehash_store( uint64_t key, unsigned int hand_b, int score )
{
  uint64_t hash_word;

  hash_word  = key;
  hash_word ^= (uint64_t)hand_b << 16;
  hash_word &= ~(uint64_t)0xffffU;
  hash_word |= (uint64_t)( score + 32768 );

  large_object->ehash_tbl[ (unsigned int)key & EHASH_MASK ] = hash_word;
}


static int
make_list( const tree_t * restrict ptree, int * restrict pscore,
	   int list0[52], int list1[52] )
{
  bitboard_t bb;
  int list2[34];
  int nlist, sq, n2, i, score, sq_bk0, sq_wk0, sq_bk1, sq_wk1;

  nlist  = 14;
  score  = 0;
  sq_bk0 = SQ_BKING;
  sq_wk0 = SQ_WKING;
  sq_bk1 = Inv(SQ_WKING);
  sq_wk1 = Inv(SQ_BKING);

  list0[ 0] = f_hand_pawn   + I2HandPawn(HAND_B);
  list0[ 1] = e_hand_pawn   + I2HandPawn(HAND_W);
  list0[ 2] = f_hand_lance  + I2HandLance(HAND_B);
  list0[ 3] = e_hand_lance  + I2HandLance(HAND_W);
  list0[ 4] = f_hand_knight + I2HandKnight(HAND_B);
  list0[ 5] = e_hand_knight + I2HandKnight(HAND_W);
  list0[ 6] = f_hand_silver + I2HandSilver(HAND_B);
  list0[ 7] = e_hand_silver + I2HandSilver(HAND_W);
  list0[ 8] = f_hand_gold   + I2HandGold(HAND_B);
  list0[ 9] = e_hand_gold   + I2HandGold(HAND_W);
  list0[10] = f_hand_bishop + I2HandBishop(HAND_B);
  list0[11] = e_hand_bishop + I2HandBishop(HAND_W);
  list0[12] = f_hand_rook   + I2HandRook(HAND_B);
  list0[13] = e_hand_rook   + I2HandRook(HAND_W);

  list1[ 0] = f_hand_pawn   + I2HandPawn(HAND_W);
  list1[ 1] = e_hand_pawn   + I2HandPawn(HAND_B);
  list1[ 2] = f_hand_lance  + I2HandLance(HAND_W);
  list1[ 3] = e_hand_lance  + I2HandLance(HAND_B);
  list1[ 4] = f_hand_knight + I2HandKnight(HAND_W);
  list1[ 5] = e_hand_knight + I2HandKnight(HAND_B);
  list1[ 6] = f_hand_silver + I2HandSilver(HAND_W);
  list1[ 7] = e_hand_silver + I2HandSilver(HAND_B);
  list1[ 8] = f_hand_gold   + I2HandGold(HAND_W);
  list1[ 9] = e_hand_gold   + I2HandGold(HAND_B);
  list1[10] = f_hand_bishop + I2HandBishop(HAND_W);
  list1[11] = e_hand_bishop + I2HandBishop(HAND_B);
  list1[12] = f_hand_rook   + I2HandRook(HAND_W);
  list1[13] = e_hand_rook   + I2HandRook(HAND_B);

#define KKP (*p_kkp)
  score += KKP[sq_bk0][sq_wk0][ kkp_hand_pawn   + I2HandPawn(HAND_B) ];
  score += KKP[sq_bk0][sq_wk0][ kkp_hand_lance  + I2HandLance(HAND_B) ];
  score += KKP[sq_bk0][sq_wk0][ kkp_hand_knight + I2HandKnight(HAND_B) ];
  score += KKP[sq_bk0][sq_wk0][ kkp_hand_silver + I2HandSilver(HAND_B) ];
  score += KKP[sq_bk0][sq_wk0][ kkp_hand_gold   + I2HandGold(HAND_B) ];
  score += KKP[sq_bk0][sq_wk0][ kkp_hand_bishop + I2HandBishop(HAND_B) ];
  score += KKP[sq_bk0][sq_wk0][ kkp_hand_rook   + I2HandRook(HAND_B) ];

  score -= KKP[sq_bk1][sq_wk1][ kkp_hand_pawn   + I2HandPawn(HAND_W) ];
  score -= KKP[sq_bk1][sq_wk1][ kkp_hand_lance  + I2HandLance(HAND_W) ];
  score -= KKP[sq_bk1][sq_wk1][ kkp_hand_knight + I2HandKnight(HAND_W) ];
  score -= KKP[sq_bk1][sq_wk1][ kkp_hand_silver + I2HandSilver(HAND_W) ];
  score -= KKP[sq_bk1][sq_wk1][ kkp_hand_gold   + I2HandGold(HAND_W) ];
  score -= KKP[sq_bk1][sq_wk1][ kkp_hand_bishop + I2HandBishop(HAND_W) ];
  score -= KKP[sq_bk1][sq_wk1][ kkp_hand_rook   + I2HandRook(HAND_W) ];

  n2 = 0;
  bb = BB_BPAWN;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_pawn + sq;
    list2[n2]    = e_pawn + Inv(sq);
    score += KKP[sq_bk0][sq_wk0][ kkp_pawn + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WPAWN;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_pawn + sq;
    list2[n2]    = f_pawn + Inv(sq);
    score -= KKP[sq_bk1][sq_wk1][ kkp_pawn + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }

  n2 = 0;
  bb = BB_BLANCE;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_lance + sq;
    list2[n2]    = e_lance + Inv(sq);
    score += KKP[sq_bk0][sq_wk0][ kkp_lance + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WLANCE;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_lance + sq;
    list2[n2]    = f_lance + Inv(sq);
    score -= KKP[sq_bk1][sq_wk1][ kkp_lance + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BKNIGHT;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_knight + sq;
    list2[n2]    = e_knight + Inv(sq);
    score += KKP[sq_bk0][sq_wk0][ kkp_knight + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WKNIGHT;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_knight + sq;
    list2[n2]    = f_knight + Inv(sq);
    score -= KKP[sq_bk1][sq_wk1][ kkp_knight + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BSILVER;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_silver + sq;
    list2[n2]    = e_silver + Inv(sq);
    score += KKP[sq_bk0][sq_wk0][ kkp_silver + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WSILVER;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_silver + sq;
    list2[n2]    = f_silver + Inv(sq);
    score -= KKP[sq_bk1][sq_wk1][ kkp_silver + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BTGOLD;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_gold + sq;
    list2[n2]    = e_gold + Inv(sq);
    score += KKP[sq_bk0][sq_wk0][ kkp_gold + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WTGOLD;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_gold + sq;
    list2[n2]    = f_gold + Inv(sq);
    score -= KKP[sq_bk1][sq_wk1][ kkp_gold + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BBISHOP;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_bishop + sq;
    list2[n2]    = e_bishop + Inv(sq);
    score += KKP[sq_bk0][sq_wk0][ kkp_bishop + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WBISHOP;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_bishop + sq;
    list2[n2]    = f_bishop + Inv(sq);
    score -= KKP[sq_bk1][sq_wk1][ kkp_bishop + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BHORSE;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_horse + sq;
    list2[n2]    = e_horse + Inv(sq);
    score += KKP[sq_bk0][sq_wk0][ kkp_horse + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WHORSE;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_horse + sq;
    list2[n2]    = f_horse + Inv(sq);
    score -= KKP[sq_bk1][sq_wk1][ kkp_horse + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BROOK;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_rook + sq;
    list2[n2]    = e_rook + Inv(sq);
    score += KKP[sq_bk0][sq_wk0][ kkp_rook + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WROOK;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_rook + sq;
    list2[n2]    = f_rook + Inv(sq);
    score -= KKP[sq_bk1][sq_wk1][ kkp_rook + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }


  n2 = 0;
  bb = BB_BDRAGON;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = f_dragon + sq;
    list2[n2]    = e_dragon + Inv(sq);
    score += KKP[sq_bk0][sq_wk0][ kkp_dragon + sq ];
    nlist += 1;
    n2    += 1;
  }

  bb = BB_WDRAGON;
  while ( BBToU(bb) ) {
    sq = FirstOne( bb );
    Xor( sq, bb );

    list0[nlist] = e_dragon + sq;
    list2[n2]    = f_dragon + Inv(sq);
    score -= KKP[sq_bk1][sq_wk1][ kkp_dragon + Inv(sq) ];
    nlist += 1;
    n2    += 1;
  }
  for ( i = 0; i < n2; i++ ) { list1[nlist-i-1] = list2[i]; }

  assert( nlist <= 52 );
  *pscore += score;
  return nlist;
}
