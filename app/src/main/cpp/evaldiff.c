#include <assert.h>
#include "shogi.h"

void
check_futile_score_quies( const tree_t * restrict ptree, unsigned int move,
			  int old_val, int new_val, int turn )
{
  const int ifrom = I2From(move);
  int fsp, fmt, ipc_cap;

  if ( I2PieceMove(move) == king )
    {
      fmt = new_val;
      fsp = new_val - old_val;

      if ( turn )
	{
	  fmt     += MATERIAL;
	  ipc_cap  = -(int)UToCap(move);
	}
      else {
	fmt     -= MATERIAL;
	ipc_cap  = (int)UToCap(move);
      }

      if ( ipc_cap )
	{
	  fmt -= p_value_ex[15+ipc_cap];
	  fsp -= estimate_score_diff( ptree, move, turn );
	  if ( fsp > fmg_cap_king ) { fmg_cap_king = fsp; }
	}
      else if ( fsp > fmg_misc_king ) { fmg_misc_king = fsp; }

      if ( fmt > fmg_mt ) { fmg_mt = fmt; }
    }
  else {
    fsp = new_val - old_val - estimate_score_diff( ptree, move, turn );
    if ( turn )
      {
	fmt     = new_val + MATERIAL;
	ipc_cap = -(int)UToCap(move);
      }
    else {
      fmt      = new_val - MATERIAL;
      ipc_cap  = (int)UToCap(move);
    }
    if ( ifrom >= nsquare )
      {
	if ( fsp > fmg_drop ) { fmg_drop = fsp; }
      }
    else {
      if ( I2IsPromote(move) )
	{
	  fmt -= benefit2promo[7+I2PieceMove(move)];
	}

      if ( ipc_cap )
	{
	  fmt -= p_value_ex[15+ipc_cap];
	  if ( fsp > fmg_cap ) { fmg_cap = fsp; }
	}
      else if ( fsp > fmg_misc ) { fmg_misc = fsp; }
    }
    
    if ( fmt > fmg_mt )   { fmg_mt = fmt; }
  }
}


int
eval_max_score( const tree_t * restrict ptree, unsigned int move,
		int stand_pat, int turn, int diff )
{
  int score_mt, score_sp, ipc_cap;

  if ( I2From(move) >= nsquare )
    {
      score_sp = stand_pat + diff + fmg_drop + FMG_MG;
      score_mt = ( turn ? -MATERIAL : MATERIAL ) + fmg_mt + FMG_MG_MT;
    }
  else {
    score_sp = diff + stand_pat;
    score_mt = fmg_mt + FMG_MG_MT;
    if ( turn )
      {
	score_mt -= MATERIAL;
	ipc_cap   = -(int)UToCap(move);
      }
    else {
      score_mt += MATERIAL;
      ipc_cap   = (int)UToCap(move);
    }
    if ( I2PieceMove(move) == king )
      {
	if ( ipc_cap )
	  {
	    score_mt += p_value_ex[15+ipc_cap];
	    score_sp += fmg_cap_king;
	  }
	else { score_sp += fmg_misc_king; }
	score_sp += FMG_MG_KING;
      }
    else {
      if ( ipc_cap )
	{
	  score_mt += p_value_ex[15+ipc_cap];
	  score_sp += fmg_cap;
	}
      else { score_sp += fmg_misc; }

      if ( I2IsPromote(move) )
	{
	  score_mt += benefit2promo[7+I2PieceMove(move)];
	}

      score_sp += FMG_MG;
    }
  }
  return score_mt < score_sp ? score_mt : score_sp;
}


int
estimate_score_diff( const tree_t * restrict ptree, unsigned int move,
		     int turn )
{
  const int ibk   = SQ_BKING;
  const int iwk   = Inv(SQ_WKING);
  const int ifrom = I2From(move);
  const int ito   = I2To(move);
  int diff, ipc_move, ipc_cap, ipro_pc_move;

  if ( I2PieceMove(move) == king )
    {
      ipc_cap = (int)UToCap(move);
      if ( ipc_cap )
	{
	  diff  = -(int)PcOnSq( ibk, aipos[15+ipc_cap]+ito );
	  diff +=  (int)PcOnSq( iwk, aipos[15-ipc_cap]+Inv(ito) );
	  diff /= FV_SCALE;
	  if ( turn ) { diff -= p_value_ex[15+ipc_cap]; }
	  else        { diff += p_value_ex[15-ipc_cap]; }
	}
      else { diff = 0; }
    }
  else if ( ifrom >= nsquare )
    {
      ipc_move = turn ? -(int)From2Drop(ifrom) : (int)From2Drop(ifrom);
      diff     = (int)PcOnSq( ibk, aipos[15+ipc_move]+ito );
      diff    -= (int)PcOnSq( iwk, aipos[15-ipc_move]+Inv(ito) );
      diff    /= FV_SCALE;
    }
  else {
    if ( turn )
      {
	ipc_move     = -(int)I2PieceMove(move);
	ipc_cap      =  (int)UToCap(move);
	ipro_pc_move = ipc_move - promote;
      }
    else {
      ipc_move     = (int)I2PieceMove(move);
      ipc_cap      = -(int)UToCap(move);
      ipro_pc_move = ipc_move + promote;
    }
    if ( I2IsPromote(move) && ipc_cap )
      {
	diff  = -(int)PcOnSq( ibk, aipos[15+ipc_move]     + ifrom );
	diff +=  (int)PcOnSq( ibk, aipos[15+ipro_pc_move] + ito );
	diff += -(int)PcOnSq( ibk, aipos[15+ipc_cap]      + ito );
	diff +=  (int)PcOnSq( iwk, aipos[15-ipc_move]     + Inv(ifrom) );
	diff += -(int)PcOnSq( iwk, aipos[15-ipro_pc_move] + Inv(ito) );
	diff +=  (int)PcOnSq( iwk, aipos[15-ipc_cap]      + Inv(ito) );
	diff /= FV_SCALE;
	if ( turn )
	  {
	    diff -= benefit2promo[7+ipc_move];
	    diff -= p_value_ex[15+ipc_cap];
	  }
	else {
	  diff += benefit2promo[7+ipc_move];
	  diff += p_value_ex[15+ipc_cap];
	}
      }
    else if ( ipc_cap )
      {
	diff  = -(int)PcOnSq( ibk, aipos[15+ipc_move] + ifrom );
	diff +=  (int)PcOnSq( ibk, aipos[15+ipc_move] + ito );
	diff += -(int)PcOnSq( ibk, aipos[15+ipc_cap]  + ito );
	diff +=  (int)PcOnSq( iwk, aipos[15-ipc_move] + Inv(ifrom) );
	diff += -(int)PcOnSq( iwk, aipos[15-ipc_move] + Inv(ito) );
	diff +=  (int)PcOnSq( iwk, aipos[15-ipc_cap]  + Inv(ito) );
	diff /= FV_SCALE;
	diff += turn ? -p_value_ex[15+ipc_cap] : p_value_ex[15+ipc_cap];
      }
    else if ( I2IsPromote(move) )
      {
	diff  = -(int)PcOnSq( ibk, aipos[15+ipc_move]     + ifrom );
	diff +=  (int)PcOnSq( ibk, aipos[15+ipro_pc_move] + ito );
	diff +=  (int)PcOnSq( iwk, aipos[15-ipc_move]     + Inv(ifrom) );
	diff += -(int)PcOnSq( iwk, aipos[15-ipro_pc_move] + Inv(ito) );
	diff /= FV_SCALE;
	diff += turn ? -benefit2promo[7+ipc_move] : benefit2promo[7+ipc_move];
      }
    else {
      diff  = -(int)PcOnSq( ibk, aipos[15+ipc_move] + ifrom );
      diff +=  (int)PcOnSq( ibk, aipos[15+ipc_move] + ito );
      diff +=  (int)PcOnSq( iwk, aipos[15-ipc_move] + Inv(ifrom) );
      diff += -(int)PcOnSq( iwk, aipos[15-ipc_move] + Inv(ito) );
      diff /= FV_SCALE;
    }
  }
  
  if ( turn ) { diff = -diff; }

  return diff;
}
