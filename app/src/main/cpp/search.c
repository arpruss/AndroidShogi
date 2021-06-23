#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "shogi.h"

static void history( tree_t * restrict ptree, int ply, int depth, int turn,
		     unsigned int move );
static int detect_signals( tree_t * restrict ptree );
static int detect_rep( tree_t * restrict ptree, int ply, int turn );
static int rep_type( const tree_t * restrict ptree, int n, int i, int ply,
		     int turn );

/* #define DBG_SEARCH */
#if defined(DBG_SEARCH)
#  define DOut( ... )  if ( dbg_flag ) { out( __VA_ARGS__ ); }
#else
#  define DOut( ... )
#endif

int
search( tree_t * restrict ptree, int alpha, int beta, int turn, int depth,
	int ply, unsigned int state_node )
{
  int value, alpha_old;

  if ( ! ptree->nsuc_check[ply]	&& depth < PLY_INC )
    {
      return search_quies( ptree, alpha, beta, turn, ply, 1 );
    }


#if defined(DBG_SEARCH)
  int dbg_flag = 0;
  if ( iteration_depth == 3 && ply == 3
       && ! strcmp( str_CSA_move(ptree->current_move[1]),  "7776FU" )
       && ! strcmp( str_CSA_move(ptree->current_move[2]),  "3334FU" ) )
    {
      dbg_flag = 1;
      Out( "search start (d%.1f %" PRIu64 ")\n",
	   (double)depth / (double)PLY_INC, ptree->node_searched );
    }
#endif

  ptree->node_searched++;

  if ( ply >= PLY_MAX-1 )
    {
      value = evaluate( ptree, ply, turn );
      if ( alpha < value && value < beta ) { pv_close( ptree, ply, no_rep ); }
      MOVE_CURR = MOVE_NA;
      return value;
    }


#if ! defined(MINIMUM)
  if ( ! ( game_status & flag_learning ) )
#endif
    {
      /* check time and input */
#if defined(TLP)
      if ( ! ptree->tlp_id )
#endif
	if ( node_next_signal < ++node_last_check && detect_signals( ptree ) )
	  {
	    root_abort = 1;
	    return 0;
	  }

      /* repetitions */
      ptree->nrep_tried++;
      switch ( detect_rep( ptree, ply, turn ) )
	{
	case black_superi_rep:
	  value = turn ? -score_inferior : score_inferior;
	  if ( alpha < value
	       && value < beta ) { pv_close( ptree, ply, black_superi_rep ); }
	  ptree->nsuperior_rep++;
	  MOVE_CURR = MOVE_NA;
	  return value;
	  
	case white_superi_rep:
	  value = turn ? score_inferior : -score_inferior;
	  if ( alpha < value
	       && value < beta ) { pv_close( ptree, ply, white_superi_rep ); }
	  ptree->nsuperior_rep++;
	  MOVE_CURR = MOVE_NA;
	  return value;
	  
	case four_fold_rep:
	  if ( alpha < score_draw
	       && score_draw < beta ) { pv_close( ptree, ply, four_fold_rep );}
	  ptree->nfour_fold_rep++;
	  MOVE_CURR = MOVE_NA;
	  return score_draw;
	  
	case perpetual_check:
	  ptree->nperpetual_check++;
	  MOVE_CURR = MOVE_NA;
	  return score_foul;
	  
	case perpetual_check2:
	  if ( ply > 4 )
	    {
	      ptree->nperpetual_check++;
	      MOVE_CURR = MOVE_NA;
	      return -score_foul;
	    }
	  break;
	}
      
      ptree->nreject_tried++;
      if ( rejections_probe( ptree, turn, ply ) )
	{
	  ptree->nreject_done++;
	  if ( alpha < score_inferior && score_inferior < beta )
	    {
	      pv_close( ptree, ply, turn ? white_superi_rep:black_superi_rep );
	    }
	  MOVE_CURR = MOVE_NA;
	  return score_inferior;
	}
    }

  /*
    no repetitions.  worst situation of this node is checkmate,
    and the best is to force checkmate within 1-ply.
  */
  alpha_old = alpha;
  value = - ( score_mate1ply + 2 - ply );
  if ( alpha < value )
    {
      if ( beta <= value )
	{
	  MOVE_CURR = MOVE_NA;
	  return value;
	}
      alpha = value;
    }
  else {
    value = score_mate1ply + 1 - ply;
    if ( value <= alpha ) { return value; }
  }

#if defined(NO_NULL_PRUNE)
  state_node &= ~node_do_null;
#endif

  ptree->amove_hash[ply] = 0;
#if ! defined(MINIMUM)
  if ( ! ( game_status & flag_learning ) )
#endif
    {
      /* probe the transposition table */
      int tv = hash_probe( ptree, ply, depth, turn, alpha, beta, state_node );
      switch ( tv )
	{
	case value_exact:
	  if ( alpha < HASH_VALUE )
	    {
	      if ( HASH_VALUE < beta ) { pv_close( ptree, ply, hash_hit ); }
	      else {
		MOVE_CURR = ptree->amove_hash[ply];
		assert( is_move_valid( ptree, MOVE_CURR, turn ) );
	      }
	    }
	  return HASH_VALUE;
	  
	case value_lower:
	  MOVE_CURR = ptree->amove_hash[ply];
	  assert( beta <= HASH_VALUE );
	  assert( ! IsMove(MOVE_CURR)
		  || is_move_valid( ptree, MOVE_CURR, turn ) );
	  if ( beta == alpha_old + 1 ) { return HASH_VALUE; }
	  break;
	  
	case value_upper:
	  assert( HASH_VALUE <= alpha );
	  if ( beta == alpha_old + 1 ) { return HASH_VALUE; }
	  break;
	  
	default:
	  state_node = tv;
	}
    }

  DOut( "\nhash cut passed" );

  if ( depth >= REJEC_MIN_DEPTH ) { add_rejections( ptree, turn, ply ); }
  

  if ( ! ptree->nsuc_check[ply] )
    {
      /* detect a move mates in 1-ply */
      if ( state_node & node_do_mate )
	{
	  MOVE_CURR = IsMateIn1Ply(turn);
	  if ( MOVE_CURR )
	    {
	      value = score_mate1ply + 1 - ply;

	      hash_store( ptree, ply, depth, turn, value_exact, value,
			  MOVE_CURR, 0 );

	      if ( alpha < value
		   && value < beta ) { pv_close( ptree, ply, mate_search ); }

	      if ( depth >= REJEC_MIN_DEPTH )
		{
		  sub_rejections( ptree, turn, ply );
		}
	      assert( is_move_valid( ptree, MOVE_CURR, turn ) );
	      return value;
	    }
	}

      /* null move pruning */
      if ( 2*PLY_INC <= depth
	   && ( state_node & node_do_null )
	   && beta == alpha_old + 1
	   && beta <= evaluate( ptree, ply, turn ) )
	{
	  int null_depth, nrep;
	  
	  null_depth = NullDepth(depth);
	  nrep       = root_nrep + ply - 1;

	  MOVE_CURR                   = MOVE_PASS;
	  ptree->move_last[ply]       = ptree->move_last[ply-1];
	  ptree->stand_pat[ply+1]     = (short)( - ptree->stand_pat[ply] );
	  ptree->nsuc_check[ply+1]    = 0;
	  ptree->rep_board_list[nrep] = HASH_KEY;
	  ptree->rep_hand_list[nrep]  = HAND_B;
	  ptree->null_pruning_tried++;

	  value = -search( ptree, -beta, 1-beta, Flip(turn), null_depth,
			   ply+1, node_do_mate | node_do_recap
			   | node_do_futile );
	  if ( SEARCH_ABORT )
	    {
	      if ( depth >= REJEC_MIN_DEPTH )
		{
		  sub_rejections( ptree, turn, ply );
		}
	      return 0;
	    }
	  
	  if ( beta <= value )
	    {
	      ptree->null_pruning_done++;
	      if ( null_depth < PLY_INC )
		{
		  hash_store( ptree, ply, depth, turn, value_lower,
			      value, MOVE_NA, state_node );
		}
	      if ( depth >= REJEC_MIN_DEPTH )
		{
		  sub_rejections( ptree, turn, ply );
		}
	      
	      DOut( "\nnull move cut!\n" );
	      
	      assert( ! IsMove(MOVE_CURR)
		      || is_move_valid( ptree, MOVE_CURR, turn ) );
	      return value;
	    }
	  
	  DOut( "\nnull passed" );
	  
	  if ( value == - ( score_mate1ply - ply ) )
	    {
	      state_node |= node_mate_threat;
	    }
	}
    }

  /* recursive iterative-deepening for pv nodes */
  if ( ! ptree->amove_hash[ply]
       && depth >= 3*PLY_INC
       && ( ( alpha == root_alpha && beta == root_beta )
	    || ( alpha == -root_beta && beta == -root_alpha ) ) )
    {
      if ( depth >= REJEC_MIN_DEPTH ) { sub_rejections( ptree, turn, ply ); }
      value = search( ptree, -score_bound, beta, turn, depth-2*PLY_INC, ply,
		      node_do_mate | node_do_recap );
      if ( SEARCH_ABORT ) { return 0; }

      if ( beta <= value && IsMove(MOVE_CURR) )
	{
	  ptree->amove_hash[ply] = MOVE_CURR;
	}
      else if ( value < beta && ply <= (int)ptree->pv[ply-1].length )
	{
	  assert( -score_bound < value );
	  ptree->amove_hash[ply] = ptree->pv[ply-1].a[ply];
	}
      assert( ! ptree->amove_hash[ply]
	      || is_move_valid( ptree, ptree->amove_hash[ply], turn ) );

      if ( depth >= REJEC_MIN_DEPTH ) { add_rejections( ptree, turn, ply ); }
    }

  {
    int depth_reduced, first_move_expanded, new_depth, extension;
    int state_node_new;

    ptree->move_last[ply]             = ptree->move_last[ply-1];
    ptree->anext_move[ply].next_phase = next_move_hash;
    first_move_expanded               = 0;
    evaluate( ptree, ply, turn );

    /* expand all of off-springs */
    while ( ptree->nsuc_check[ply]
	    ? gen_next_evasion( ptree, ply, turn )
	    : gen_next_move( ptree, ply, turn ) ) {

      DOut( "\nexpand %s (%" PRIu64 ")",
	    str_CSA_move(MOVE_CURR), ptree->node_searched );

      ptree->nsuc_check[ply+1] = 0U;
      state_node_new           = ( node_do_mate | node_do_recap | node_do_null
				   | node_do_futile );
      extension                = 0;
      depth_reduced            = 0;

      /* decision of extensions */
      if ( turn ? is_move_check_w( ptree, MOVE_CURR )
                : is_move_check_b( ptree, MOVE_CURR ) )
	{
	  ptree->check_extension_done++;
	  ptree->nsuc_check[ply+1]
	    = (unsigned char)( ptree->nsuc_check[ply-1] + 1U );
	  extension = EXT_CHECK;
	}
      else if ( ptree->nsuc_check[ply]
		&& ptree->move_last[ply] - ptree->move_last[ply-1] == 1 )
	{
	  ptree->onerp_extension_done++;
	  extension = EXT_ONEREP;
	}
      else if ( ! ptree->nsuc_check[ply]
		&& ( state_node & node_do_recap )
		&& I2To(MOVE_CURR) == I2To(MOVE_LAST)
		&& ( MOVE_CURR == ptree->anext_move[ply].move_cap1
		     || ( ( ptree->anext_move[ply].value_cap1
			    < ( ptree->anext_move[ply].value_cap2
				+ MT_CAP_PAWN ) )
			  && MOVE_CURR == ptree->anext_move[ply].move_cap2 ))
		&& ( UToCap(MOVE_LAST)
		     || ( I2IsPromote(MOVE_LAST)
			  && I2PieceMove(MOVE_LAST) != silver ) ) )
	{
	  ptree->recap_extension_done++;
	  state_node_new = node_do_null | node_do_mate | node_do_futile;
	  if ( ! I2IsPromote(MOVE_CURR)
	       && I2PieceMove(MOVE_LAST) == UToCap(MOVE_LAST) )
	    {
	      extension = EXT_RECAP2;
	    }
	  else { extension = EXT_RECAP1; }
	}

      LimitExtension( extension, ply );

      new_depth = depth + extension - PLY_INC;

      if ( ! ptree->nsuc_check[ply+1] && ! ptree->nsuc_check[ply] )
	{
	  /* reductions */
	  if ( PLY_INC <= new_depth
	       && first_move_expanded
	       && beta == alpha_old + 1
	       && ! ( state_node & node_mate_threat )
	       && ! UToCap(MOVE_CURR)
	       && ( ! I2IsPromote(MOVE_CURR)
		    || I2PieceMove(MOVE_CURR) == silver ) )
	    {
	      depth_reduced = PLY_INC;
	      new_depth     -= PLY_INC;
	    }

	  /* futility pruning */
	  if ( new_depth < 3*PLY_INC )
	    {
	      int diff  = estimate_score_diff( ptree, MOVE_CURR, turn );
	      int bound = alpha;

	      if      ( 2*PLY_INC <= new_depth ) { bound -= EFUTIL_MG2; }
	      else if ( 1*PLY_INC <= new_depth ) { bound -= EFUTIL_MG1; }
	      
	      if ( eval_max_score( ptree, MOVE_CURR, ptree->stand_pat[ply],
				   turn, diff ) <= bound )
		{
		  first_move_expanded = 1;
		  continue;
		}
	    }
	}

      DOut( ", futil passed" );

      MakeMove( turn, MOVE_CURR, ply );
      if ( I2From(MOVE_CURR) < nsquare
	   && ! ptree->nsuc_check[ply]
	   && InCheck(turn) )
	{
	  UnMakeMove( turn, MOVE_CURR, ply );
	  continue;
	}

      if ( ! ptree->nsuc_check[ply+1] && ! ptree->nsuc_check[ply] )
	{
	  evaluate( ptree, ply+1, Flip(turn) );
	  assert( ptree->stand_pat[ply] != score_bound );

	  /* reductions */
	  if ( PLY_INC <= new_depth
	       && first_move_expanded
	       && ! depth_reduced
	       && ! ( state_node & node_mate_threat )
	       && ptree->stand_pat[ply] > -ptree->stand_pat[ply+1] )
	    {
	      depth_reduced  = PLY_INC;
	      new_depth      -= PLY_INC;
	    }
	  
	  /* futility pruning */
	  if ( ( new_depth < PLY_INC && - ptree->stand_pat[ply+1] <= alpha )
	       || ( new_depth < 2*PLY_INC
		    && - ptree->stand_pat[ply+1] <= alpha - EFUTIL_MG1 )
	       || ( new_depth < 3*PLY_INC
		    && - ptree->stand_pat[ply+1] <= alpha - EFUTIL_MG2 ) )
	    {
	      first_move_expanded = 1;
	      UnMakeMove( turn, MOVE_CURR, ply );
	      continue;
	    }
	}

      if ( ! first_move_expanded )
	{
	  value = -search( ptree, -beta, -alpha, Flip(turn), new_depth, ply+1,
			   state_node_new );
	}
      else if ( ! depth_reduced )
	{
	  value = -search( ptree, -alpha-1, -alpha, Flip(turn), new_depth,
	  		   ply + 1, state_node_new );
	  if ( ! SEARCH_ABORT && alpha < value && beta != alpha+1 )
	    {
	      value = -search( ptree, -beta, -alpha, Flip(turn), new_depth,
			       ply + 1, state_node_new );
	    }
	}
      else {
	value = -search( ptree, -alpha-1, -alpha, Flip(turn), new_depth,
			 ply + 1, state_node_new );
	if ( ! SEARCH_ABORT && alpha < value )
	  {
	    new_depth += depth_reduced;
	    value = -search( ptree, -beta, -alpha, Flip(turn), new_depth,
			     ply+1, state_node_new );
	  }
      }

      if ( SEARCH_ABORT )
	{
	  UnMakeMove( turn, MOVE_CURR, ply );
	  if ( depth >= REJEC_MIN_DEPTH )
	    {
	      sub_rejections( ptree, turn, ply );
	    }
	  return 0;
	}

      UnMakeMove( turn, MOVE_CURR, ply );

      if ( alpha < value )
	{
	  if ( new_depth < PLY_INC
	       && ! ptree->nsuc_check[ply+1]
	       && ptree->stand_pat[ply] != score_bound )
	    {
	      check_futile_score_quies( ptree, MOVE_CURR,
					ptree->stand_pat[ply],
					-ptree->stand_pat[ply+1], turn );
	    }

	  if ( beta <= value )
	    {
	      DOut( ", beta cut (%" PRIu64 ")\n", ptree->node_searched );

	      history( ptree, ply, depth, turn, MOVE_CURR );
	      hash_store( ptree, ply, depth, turn, value_lower, value,
			  MOVE_CURR, state_node );

	      ptree->fail_high++;
	      if ( ! first_move_expanded ) { ptree->fail_high_first++; }
	      
	      if ( depth >= REJEC_MIN_DEPTH )
		{
		  sub_rejections( ptree, turn, ply );
		}

	      assert( is_move_valid( ptree, MOVE_CURR, turn ) );
	      return value;
	    }
	}
      if ( alpha < value ) { alpha = value; }

      first_move_expanded = 1;
#if defined(TLP)
      if ( tlp_idle && depth >= TLP_DEPTH_SPLIT )
	{
	  ptree->tlp_alpha      = (short)alpha;
	  ptree->tlp_beta       = (short)beta;
	  ptree->tlp_value      = (short)alpha;
	  ptree->tlp_depth      = (unsigned char)depth;
	  ptree->tlp_state_node = (unsigned char)state_node;
	  ptree->tlp_turn       = (char)turn;
	  ptree->tlp_ply        = (char)ply;
	  if ( tlp_split( ptree ) )
	    {
	      if ( SEARCH_ABORT ) { return 0; }
	      value = ptree->tlp_best;
	      if ( alpha < value )
		{
		  if ( beta <= value )
		    {
		      if ( depth >= REJEC_MIN_DEPTH )
			{
			  sub_rejections( ptree, turn, ply );
			}
		      assert( is_move_valid( ptree, MOVE_CURR, turn ) );
		      return value;
		    }
		  alpha = value;
		}
	      break;
	    }
	}
#endif
    }

    DOut( "\nall searched (%" PRIu64 ")\n", ptree->node_searched );

    if ( depth >= REJEC_MIN_DEPTH ) { sub_rejections( ptree, turn, ply ); }

    if ( ! first_move_expanded )
      {
#if ! defined(MINIMUM)
	if ( (int)I2From( ptree->current_move[ply-1] ) == Drop2From( pawn ) )
	  {
	    out_warning( "A checkmate by dropping pawn!!" );
	  }
#endif
	if ( alpha != alpha_old ) { pv_close( ptree, ply, 0 ); } 
	return alpha;
      }


    if ( alpha <= - ( score_mate1ply + 2 - ply ) )
      {
#if ! defined(MINIMUM)
	out_warning( "A node returns a value lower than mate." );
#endif
	if ( alpha_old < -score_inferior && -score_inferior < beta )
	  {
	    pv_close( ptree, ply, turn ? black_superi_rep : white_superi_rep );
	  }
	MOVE_CURR = MOVE_NA;
	return -score_inferior;
      }

    if ( alpha != alpha_old )
      {
	history( ptree, ply, depth, turn, ptree->pv[ply].a[ply] );

	pv_copy( ptree, ply );

#if ! defined(MINIMUM)
	if ( ! ( game_status & flag_learning ) )
#endif
	  {
	    hash_store( ptree, ply, depth, turn, value_exact, alpha,
			ptree->pv[ply].a[ply], state_node );
	  }
      }
    else {
      hash_store( ptree, ply, depth, turn, value_upper, alpha, MOVE_NA,
		  state_node );
    }
  }
  
  return alpha;
}


#if defined(TLP)
int
tlp_search( tree_t * restrict ptree, int alpha, int beta, int turn,
	    int depth, int ply, unsigned int state_node )
{
  int value, new_depth, extension, state_node_new, iret, depth_reduced;
  int alpha_old;

  assert( depth >= 3*PLY_INC );

  alpha_old = alpha;

  for ( ;; ) {
    lock( & ptree->tlp_ptree_parent->tlp_lock );
    if ( ptree->nsuc_check[ply] )
      {
	iret = gen_next_evasion( ptree->tlp_ptree_parent, ply, turn );
      }
    else { iret = gen_next_move( ptree->tlp_ptree_parent, ply, turn ); }
    MOVE_CURR = ptree->tlp_ptree_parent->current_move[ply];
    unlock( & ptree->tlp_ptree_parent->tlp_lock );
    if ( ! iret ) { break; }

    ptree->nsuc_check[ply+1] = 0U;
    state_node_new           = ( node_do_mate | node_do_recap | node_do_null
				 | node_do_futile );
    extension                = 0;
    depth_reduced            = 0;

    if ( turn ? is_move_check_w( ptree, MOVE_CURR )
              : is_move_check_b( ptree, MOVE_CURR ) )
      {
	ptree->check_extension_done++;
	ptree->nsuc_check[ply+1]
	  = (unsigned char)( ptree->nsuc_check[ply-1] + 1U );
	extension = EXT_CHECK;
      }
    else if ( ! ptree->nsuc_check[ply] 
	      && ( state_node & node_do_recap )
	      && I2To(MOVE_CURR) == I2To(MOVE_LAST)
	      && ( MOVE_CURR == ptree->anext_move[ply].move_cap1
		   || ( ( ptree->anext_move[ply].value_cap1
			  < ptree->anext_move[ply].value_cap2 + MT_CAP_PAWN )
			&& MOVE_CURR == ptree->anext_move[ply].move_cap2 ) )
	      && ( UToCap(MOVE_LAST)
		   || ( I2IsPromote(MOVE_LAST)
			&& I2PieceMove(MOVE_LAST) != silver ) ) )
      {
	ptree->recap_extension_done++;
	state_node_new = node_do_null | node_do_mate | node_do_futile;
	if ( ! I2IsPromote(MOVE_CURR)
	     && I2PieceMove(MOVE_LAST) == UToCap(MOVE_LAST) )
	  {
	    extension = EXT_RECAP2;
	  }
	else { extension = EXT_RECAP1; }
      }

    LimitExtension( extension, ply );

    new_depth = depth + extension - PLY_INC;
    
    if ( ! ptree->nsuc_check[ply+1] && ! ptree->nsuc_check[ply] )
      {
	if ( beta == alpha_old + 1
	     && ! ( state_node & node_mate_threat )
	     && ( ! UToCap(MOVE_CURR)
		  && ( ! I2IsPromote(MOVE_CURR)
		       || I2PieceMove(MOVE_CURR) == silver ) ) )
	  {
	    depth_reduced  = PLY_INC;
	    new_depth     -= PLY_INC;
	  }

	if ( new_depth < 3*PLY_INC )
	  {
	    int diff  = estimate_score_diff( ptree, MOVE_CURR, turn );
	    int bound = alpha;
	    
	    if      ( 2*PLY_INC <= new_depth ) { bound -= EFUTIL_MG2; }
	    else if ( 1*PLY_INC <= new_depth ) { bound -= EFUTIL_MG1; }
	    
	    if ( eval_max_score( ptree, MOVE_CURR, ptree->stand_pat[ply],
				 turn, diff ) <= bound )
	      {
		continue;
	      }
	  }
      }

    MakeMove( turn, MOVE_CURR, ply );
    if ( I2From(MOVE_CURR) < nsquare
	 && ! ptree->nsuc_check[ply]
	 && InCheck(turn) )
      {
	UnMakeMove( turn, MOVE_CURR, ply );
	continue;
      }

    if ( ! ptree->nsuc_check[ply+1] && ! ptree->nsuc_check[ply] )
      {
	evaluate( ptree, ply+1, Flip(turn) );
	assert( ptree->stand_pat[ply] != score_bound );

	/* reductions */
	if ( PLY_INC <= new_depth
	     && ! depth_reduced
	     && ! ( state_node & node_mate_threat )
	     && ptree->stand_pat[ply] > -ptree->stand_pat[ply+1] )
	  {
	    depth_reduced   = PLY_INC;
	    new_depth      -= PLY_INC;
	  }

	/* futility pruning */
	if ( ( new_depth < PLY_INC && - ptree->stand_pat[ply+1] <= alpha )
	     || ( new_depth < 2*PLY_INC
		  && - ptree->stand_pat[ply+1] <= alpha - EFUTIL_MG1 )
	     || ( new_depth < 3*PLY_INC
		  && - ptree->stand_pat[ply+1] <= alpha - EFUTIL_MG2 ) )
	  {
	    UnMakeMove( turn, MOVE_CURR, ply );
	    continue;
	  }
      }

    value = -search( ptree, -alpha-1, -alpha, Flip(turn), new_depth,
		     ply + 1, state_node_new );
    if ( ! SEARCH_ABORT && alpha < value )
      {
	if ( ! depth_reduced && beta != alpha+1 )
	  {
	    value = -search( ptree, -beta, -alpha, Flip(turn), new_depth,
			     ply + 1, state_node_new );
	  }
	else if ( depth_reduced )
	  {
	    new_depth += depth_reduced;
	    value = -search( ptree, -beta, -alpha, Flip(turn), new_depth,
			     ply + 1, state_node_new );
	  }
      }
    if ( SEARCH_ABORT )
      {
	UnMakeMove( turn, MOVE_CURR, ply );
	return 0;
      }
    
    UnMakeMove( turn, MOVE_CURR, ply );
      
    if ( alpha < value )
      {
	if ( beta <= value )
	  {
	    lock( &tlp_lock );
	    tlp_nabort += 1;
	    if ( ! ptree->tlp_abort )
	      {
		int num;
		for ( num = 0; num < tlp_max; num++ )
		  if ( ptree->tlp_ptree_parent->tlp_ptrees_sibling[num]
		       && num != ptree->tlp_id )
		    tlp_set_abort( ptree->tlp_ptree_parent
				   ->tlp_ptrees_sibling[num] );
	      }
	    unlock( &tlp_lock );

	    hash_store( ptree, ply, depth, turn, value_lower,
			value, MOVE_CURR, state_node );
	    ptree->fail_high++;

	    return value;
	  }
	alpha = value;
      }
  }

  return alpha;
}
#endif /* TLP */


static void
history( tree_t * restrict ptree, int ply, int depth, int turn,
	 unsigned int move )
{
  int value, value_no1, value_no2;

  if ( ptree->nsuc_check[ply] ) { return; }

  if ( ! UToCap(move)
       && ( ! I2IsPromote(move) || I2PieceMove(move) == silver ) )
    {
      int from = (int)I2From(move);
      int to   = (int)I2To(move);

      ptree->history[turn][to][from] += depth;
    }

  value     = p_value_ex[15+UToCap(move)];
  value_no1 = p_value_ex[15+BOARD[I2To(ptree->amove_killer[ply].no1)]];
  value_no2 = p_value_ex[15+BOARD[I2To(ptree->amove_killer[ply].no2)]];
  if ( move == ptree->anext_move[ply].move_cap1 )
    {
      if ( ( ptree->anext_move[ply].phase_done & phase_killer1 )
	   && UToFromToPromo(move) != ptree->amove_killer[ply].no1 )
	{
	  ptree->amove_killer[ply].no1_value
	    = ptree->anext_move[ply].value_cap1 - value - 1;
	}
      if ( ( ptree->anext_move[ply].phase_done & phase_killer2 )
	   && UToFromToPromo(move) != ptree->amove_killer[ply].no2 )
	{
	  ptree->amove_killer[ply].no2_value
	    = ptree->anext_move[ply].value_cap1 - value - 2;
	}
    }
  else if ( UToFromToPromo(move) == ptree->amove_killer[ply].no1 )
    {
      if ( ( ptree->anext_move[ply].phase_done & phase_cap1 )
	   && ( ptree->amove_killer[ply].no1_value + value
		< ptree->anext_move[ply].value_cap1 + 1 ) )
	{
	  ptree->amove_killer[ply].no1_value
	    = ptree->anext_move[ply].value_cap1 - value + 1;
	}
      if ( ( ptree->anext_move[ply].phase_done & phase_killer2 )
	   && ( ptree->amove_killer[ply].no1_value + value
		< ptree->amove_killer[ply].no2_value + value_no2 + 1 ) )
	{
	  ptree->amove_killer[ply].no1_value
	    = ptree->amove_killer[ply].no2_value + value_no2 - value + 1;
	}
    }
  else if ( UToFromToPromo(move) == ptree->amove_killer[ply].no2 )
    {
      unsigned int uswap;
      int iswap;
      
      if ( ( ptree->anext_move[ply].phase_done & phase_cap1 )
	   && ( ptree->amove_killer[ply].no2_value + value
		< ptree->anext_move[ply].value_cap1 + 1 ) )
	{
	  ptree->amove_killer[ply].no2_value
	    = ptree->anext_move[ply].value_cap1 - value + 1;
	}
      if ( ( ptree->anext_move[ply].phase_done & phase_killer1 )
	   && ( ptree->amove_killer[ply].no2_value + value
		< ptree->amove_killer[ply].no1_value + value_no1 + 1 ) )
	{
	  ptree->amove_killer[ply].no2_value
	    = ptree->amove_killer[ply].no1_value + value_no1 - value + 1;
	}

      uswap = ptree->amove_killer[ply].no1;
      ptree->amove_killer[ply].no1 = ptree->amove_killer[ply].no2;
      ptree->amove_killer[ply].no2 = uswap;
	  
      iswap = ptree->amove_killer[ply].no1_value;
      ptree->amove_killer[ply].no1_value = ptree->amove_killer[ply].no2_value;
      ptree->amove_killer[ply].no2_value = iswap;
    }
  else {
    ptree->amove_killer[ply].no2 = ptree->amove_killer[ply].no1;
    ptree->amove_killer[ply].no1 = UToFromToPromo(move);

    if ( ptree->anext_move[ply].phase_done & phase_killer1 )
      {
	int i;
	i  = swap( ptree, move, -MT_CAP_ROOK, MT_CAP_ROOK, turn ) - value - 1;

	if ( ptree->amove_killer[ply].no1_value > i )
	  {
	    ptree->amove_killer[ply].no1_value = i;
	  }
      }
    ptree->amove_killer[ply].no2_value = ptree->amove_killer[ply].no1_value;
    ptree->amove_killer[ply].no1_value
      = ptree->anext_move[ply].value_cap1 - value + 1;
  }
}


static int
detect_signals( tree_t * restrict ptree )
{
  unsigned int tnow, telapsed, tpondered, tsearched, tcount, tlimit, tmax;
  unsigned int tmax_limit_count, tlimit_count, u;
  int iret, easy_time, last_value, stable;

  if ( ! ( game_status & flag_nopeek ) )
    {
      /* peek input-buffer to find a command */
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
      else if ( game_status & flag_quit ) { return 1; } /* EOF */
      else if ( iret )
	{
	  /* a command is found */
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
			       | flag_move_now | flag_suspend ) )
	    {
	      return 1;
	    }
	}
    }

  /* check conditions of search-abortion, and obtain tnow and elapsed */
  if ( node_limit <= ptree->node_searched ) { return 1; }

  if ( get_elapsed( &tnow ) < 0 )
    {
      game_status |= flag_search_error;
      return 1;
    }

  /* keep my connection alive */
#if defined(CSA_LAN)
  if ( sckt_csa != SCKT_NULL
       && SEC_KEEP_ALIVE * 1000U < tnow - time_last_send
       && sckt_out( sckt_csa, "\n" ) < 0 )
    {
      game_status |= flag_search_error;
      return 1;
    }
#endif

  /* shortening the time limit by depth */
  if (  time_limit != UINT_MAX
	&& sec_limit_depth < PLY_MAX
	&& iteration_depth + 10 >= (int)sec_limit_depth )
    {
      if      ( iteration_depth + 0 >= (int)sec_limit_depth ) { u =    1U; }
      else if ( iteration_depth + 1 >= (int)sec_limit_depth ) { u =    3U; }
      else if ( iteration_depth + 2 >= (int)sec_limit_depth ) { u =    7U; }
      else if ( iteration_depth + 3 >= (int)sec_limit_depth ) { u =   15U; }
      else if ( iteration_depth + 4 >= (int)sec_limit_depth ) { u =   31U; }
      else if ( iteration_depth + 5 >= (int)sec_limit_depth ) { u =   63U; }
      else if ( iteration_depth + 6 >= (int)sec_limit_depth ) { u =  127U; }
      else if ( iteration_depth + 7 >= (int)sec_limit_depth ) { u =  255U; }
      else if ( iteration_depth + 8 >= (int)sec_limit_depth ) { u =  511U; }
      else if ( iteration_depth + 9 >= (int)sec_limit_depth ) { u = 1023U; }
      else                                                    { u = 2047U; }
      
      tlimit = u * 1000U + 1000U - time_response;
      tmax   = u * 5000U + 1000U - time_response;
      if ( tlimit > time_limit )     { tlimit = time_limit; }
      if ( tmax   > time_max_limit ) { tmax   = time_max_limit; }
    }
  else {
    tlimit = time_limit;
    tmax   = time_max_limit;
  }

  telapsed   = tnow            - time_turn_start;
  tpondered  = time_turn_start - time_start;
  tsearched  = tnow            - time_start;
  easy_time  = 0;
  last_value = root_turn ? -last_root_value : last_root_value;
  stable     = ( tlimit != UINT_MAX
		 && ( ( root_alpha == root_value && ! root_nfail_low )
		      || last_pv.type == four_fold_rep
		      || ( root_nfail_high
			   && root_value + MT_CAP_DRAGON/8 >= last_value ) ) );

  if ( tlimit != tmax )
    {
      tcount           = tsearched;
      tmax_limit_count = tmax;
      tlimit_count     = tlimit;
    }
  else {
    tcount           = telapsed;
    tmax_limit_count = tmax   + tpondered;
    tlimit_count     = tlimit + tpondered;
  }

  if ( ! ( game_status & flag_pondering )
       && root_nmove == 1
       && telapsed > 2000U - time_response ) { return 1; }

  if ( tcount > tmax ) { return 1; }

  if ( stable && tcount > tlimit ) { return 1; }
  
  if ( stable
       && ( tmax_limit_count * 3U < ( time_last_search - time_start ) * 5U ) )
    {
#if defined(DBG_EASY)
      if ( ! easy_move )
	{
	  Out( "  The search can be terminated "
	       "before it reaches time-limit.\n" );
	  easy_move = ptree->pv[0].a[1];
	}
#else
      Out( "  The search is terminated before it reaches time-limit.\n" );
      return 1;
#endif
    }

  if ( stable
       && easy_abs > abs( last_value )
       && easy_min < last_value - easy_value
       && easy_max > last_value - easy_value )
    {
      u = tlimit_count / 5U;
      if ( u < tpondered ) { ; }
      else if ( u - tpondered < 2000U - time_response )
	{
	  u = tpondered + 2000U - time_response;
	}
      else {
	u = ( ( u - tpondered ) / 1000U + 1U ) * 1000U
	  + tpondered - time_response;
      }

      if ( tsearched > u )
	{
	  Out( "  The root move %s counted as easy!!\n",
	       str_CSA_move(root_move_list[0].move) );
#if defined(DBG_EASY)
	  easy_move = ptree->pv[0].a[1];
	  easy_abs  = 0;
#else
	  return 1;
#endif
	}
      else if ( tsearched + 500U > u ) { easy_time = 1; }
    }

  /* renew node_per_second */
  {
    double dn, dd;

    dn = (double)node_last_check * 1000.0;
    dd = (double)( tnow - time_last_check ) + 0.1;
    u  = (unsigned int)( dn / dd );
    if      ( node_per_second > u * 2U ) { node_per_second /= 2U; }
    else if ( node_per_second < u / 2U ) { node_per_second *= 2U; }
    else                                 { node_per_second  = u; }
  }

  /* renew node_next_signal */
  if ( ! ( game_status & flag_pondering ) && root_nmove == 1 )
    {
      u = 2000U - time_response;
    }
  else { u = tlimit; }

  if ( ! ( game_status & ( flag_pondering | flag_puzzling ) )
       && ! easy_time
       && u > tcount + 500U )
    {
      node_next_signal = node_per_second / 4U;
    }
  else { node_next_signal = node_per_second / 32U; }

  /* renew time_last_check and node_last_check */
  time_last_check = tnow;
  node_last_check = 0;

       
  return 0;
}


int
is_move_check_b( const tree_t * restrict ptree, unsigned int move )
{
  const int from = (int)I2From(move);
  const int to   = (int)I2To(move);
  int is_check, ipiece_move, idirec;
  bitboard_t bb;

  is_check = 0;

  if ( from >= nsquare ) { ipiece_move = From2Drop(from); }
  else {
    ipiece_move = (int)I2PieceMove(move);
    if ( I2IsPromote(move) ) { ipiece_move += promote; }
    
    idirec = (int)adirec[SQ_WKING][from];
    if ( idirec && idirec != (int)adirec[SQ_WKING][to]
	 && is_pinned_on_white_king( ptree, from, idirec ) )
      {
	is_check = 1;
	goto end;
      }
  }
  
  switch ( ipiece_move )
    {
    case pawn:
      if ( BOARD[to-nfile] == -king ) { is_check = 1; }
      break;
	  
    case lance:
      AttackBLance( bb, to );
      if ( BBContract( bb, BB_WKING ) ) { is_check = 1; }
      break;
      
    case knight:
      if ( BBContract(abb_b_knight_attacks[to],BB_WKING) ) { is_check = 1; }
      break;
      
    case silver:
      if ( BBContract(abb_b_silver_attacks[to],BB_WKING) ) { is_check = 1; }
      break;
      
    case gold:	       case pro_pawn:
    case pro_lance:    case pro_knight:
    case pro_silver:
      if ( BBContract(abb_b_gold_attacks[to],BB_WKING) ) { is_check = 1; }
      break;
      
    case bishop:
      AttackBishop( bb, to );
      if ( BBContract( bb, BB_WKING ) ) { is_check = 1; }
      break;
      
    case rook:
      AttackRook( bb, to );
      if ( BBContract( bb, BB_WKING ) ) { is_check = 1; }
      break;
      
    case king:
      break;

    case horse:
      AttackHorse( bb, to );
      if ( BBContract( bb, BB_WKING ) ) { is_check = 1; }
      break;
      
    default:
      assert( ipiece_move == dragon );
      AttackDragon( bb, to );
      if ( BBContract( bb, BB_WKING ) ) { is_check = 1; }
      break;
    }

 end:
  return is_check;
}


int
is_move_check_w( const tree_t * restrict ptree, unsigned int move )
{
  const int from = (int)I2From(move);
  const int to   = (int)I2To(move);
  int is_check, ipiece_move, idirec;
  bitboard_t bb;

  is_check = 0;

  if ( from >= nsquare ) { ipiece_move = From2Drop(from); }
  else {
    ipiece_move = (int)I2PieceMove(move);
    if ( I2IsPromote(move) ) { ipiece_move += promote; }
    
    idirec = (int)adirec[SQ_BKING][from];
    if ( idirec && idirec != (int)adirec[SQ_BKING][to]
	 && is_pinned_on_black_king( ptree, from, idirec ) )
      {
	is_check = 1;
	goto end;
      }
  }
  
  switch ( ipiece_move )
    {
    case pawn:
      if ( BOARD[to+nfile] == king ) { is_check = 1; }
      break;
      
    case lance:
      AttackWLance( bb, to );
      if ( BBContract( bb, BB_BKING ) ) { is_check = 1; }
      break;
      
    case knight:
      if ( BBContract(abb_w_knight_attacks[to],BB_BKING) ) { is_check = 1; }
      break;
      
    case silver:
      if ( BBContract(abb_w_silver_attacks[to],BB_BKING) ) { is_check = 1; }
      break;
      
    case gold:        case pro_pawn:
    case pro_lance:   case pro_knight:
    case pro_silver:
      if ( BBContract(abb_w_gold_attacks[to],BB_BKING) ) { is_check = 1; }
      break;
      
    case bishop:
      AttackBishop( bb, to );
      if ( BBContract( bb, BB_BKING ) ) { is_check = 1; }
      break;
      
    case rook:
      AttackRook( bb, to );
      if ( BBContract( bb, BB_BKING ) ) { is_check = 1; }
      break;

    case king:
      break;

    case horse:
      AttackHorse( bb, to );
      if ( BBContract( bb, BB_BKING ) ) { is_check = 1; }
      break;
      
    default:
      assert( ipiece_move == dragon );
      AttackDragon( bb, to );
      if ( BBContract( bb, BB_BKING ) ) { is_check = 1; }
      break;
    }

 end:
  return is_check;
}


void
pv_close( tree_t * restrict ptree, int ply, int type )
{
  ptree->pv[ply-1].a[ply-1] = (ptree)->current_move[ply-1];
  ptree->pv[ply-1].length   = (unsigned char)(ply-1);
  ptree->pv[ply-1].type     = (unsigned char)type;
  ptree->pv[ply-1].depth    = (unsigned char)iteration_depth;
}


void
pv_copy( tree_t * restrict ptree, int ply )
{
  memcpy( &(ptree->pv[ply-1].a[ply]), &(ptree->pv[ply].a[ply]),
	  ( ptree->pv[ply].length-ply+1 ) * sizeof(unsigned int) );
  ptree->pv[ply-1].type     = ptree->pv[ply].type;
  ptree->pv[ply-1].length   = ptree->pv[ply].length;
  ptree->pv[ply-1].depth    = ptree->pv[ply].depth;
  ptree->pv[ply-1].a[ply-1] = ptree->current_move[ply-1];
}


static int
detect_rep( tree_t * restrict ptree, int ply, int turn )
{
  if ( ply < 4 ) { return detect_repetition( ptree, ply, turn, 2 ); }
  else {
    int n, i, imin, iret;

    n    = root_nrep + ply - 1;
    imin = n - REP_MAX_PLY;
    if ( imin < 0 ) { imin = 0; }

    for ( i = n-2; i >= imin; i-- )
      if ( ptree->rep_board_list[i] == HASH_KEY )
	{
	  iret = rep_type( ptree, n, i, ply, turn );
	  if ( iret ) { return iret; }
	}
  }

  return no_rep;
}


static int
rep_type( const tree_t * restrict ptree, int n, int i, int ply, int turn )
{
  const unsigned int hand1 = HAND_B;
  const unsigned int hand2 = ptree->rep_hand_list[i];
      
  if ( (n-i) & 1 )
    {
      if ( turn )
	{
	  if ( is_hand_eq_supe( hand2, hand1 ) )  { return white_superi_rep; }
	}
      else if ( is_hand_eq_supe( hand1, hand2 ) ) { return black_superi_rep; }
    }
  else if ( hand1 == hand2 )
    {
      const int ncheck   = (int)ptree->nsuc_check[ply];
      const int nchecked = (int)ptree->nsuc_check[ply-1];

      if      ( ncheck   * 2 - 2 >= n-i ) { return perpetual_check; }
      else if ( nchecked * 2     >= n-i ) { return perpetual_check2; }
      else                                { return four_fold_rep; }
    }
  else if ( is_hand_eq_supe( hand1, hand2 ) ) { return black_superi_rep; }
  else if ( is_hand_eq_supe( hand2, hand1 ) ) { return white_superi_rep; }

  return no_rep;
}
