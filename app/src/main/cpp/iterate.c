#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include "shogi.h"

static void adjust_fmg( void );
static int ini_hash( void );
static int set_root_alpha( int nfail_low, int root_alpha_old );
static int set_root_beta( int nfail_high, int root_beta_old );
static int is_answer_right( unsigned int move );
static const char *str_fail_high( int turn, int nfail_high );


static int rep_book_prob( tree_t * restrict ptree )
{
  int i;

  for ( i = root_nrep - 2; i >= 0; i -= 2 )
    if ( ptree->rep_board_list[i] == HASH_KEY
	 && ptree->rep_hand_list[i] == HAND_B )
      {
	Out( "- book is ignored due to a repetition.\n" );
	return 1;
      }

  return 0;
}


int
iterate( tree_t * restrict ptree, int flag )
{
  int value, iret, ply, is_hash_learn_stored;
  unsigned int cpu_start;
  int right_answer_made;

  /* probe the opening book */
  if ( pf_book != NULL && n_nobook_move < 7 && ! rep_book_prob( ptree  ) )
    {
      int is_book_hit, i;
      unsigned int elapsed;

      is_book_hit = book_probe( ptree );
      if ( is_book_hit < 0 ) { return is_book_hit; }

      iret = get_elapsed( &elapsed );
      if ( iret < 0 ) { return iret; }

      Out( "- opening book is probed. (%ss)\n",
	   str_time_symple( elapsed - time_start ) );
      if ( is_book_hit )
	{
	  pv_close( ptree, 2, book_hit );
	  last_pv         = ptree->pv[1];
	  last_root_value = 0;
	  n_nobook_move   = 0;
	  if ( ! ( game_status & flag_puzzling ) )
	    for ( i = 0; i < 2*nsquare*(nsquare+7); i++ )
	      {
		ptree->history[0][0][i] /= 256U;
	      }
	  return 1;
	}
      if ( ! ( game_status & ( flag_puzzling | flag_pondering ) ) )
	{
	  n_nobook_move += 1;
	}
    }

  /* initialize variables */
  if ( get_cputime( &cpu_start ) < 0 ) { return -1; }

  ptree->node_searched         =  0;
  ptree->nreject_done          =  0;
  ptree->nreject_tried         =  0;
  ptree->null_pruning_done     =  0;
  ptree->null_pruning_tried    =  0;
  ptree->check_extension_done  =  0;
  ptree->recap_extension_done  =  0;
  ptree->onerp_extension_done  =  0;
  ptree->nfour_fold_rep        =  0;
  ptree->nperpetual_check      =  0;
  ptree->nsuperior_rep         =  0;
  ptree->nrep_tried            =  0;
  ptree->neval_called          =  0;
  ptree->nquies_called         =  0;
  ptree->ntrans_always_hit     =  0;
  ptree->ntrans_prefer_hit     =  0;
  ptree->ntrans_probe          =  0;
  ptree->ntrans_exact          =  0;
  ptree->ntrans_lower          =  0;
  ptree->ntrans_upper          =  0;
  ptree->ntrans_superior_hit   =  0;
  ptree->ntrans_inferior_hit   =  0;
  ptree->fail_high             =  0;
  ptree->fail_high_first       =  0;
  ptree->current_move[0]       =  0;
  ptree->pv[0].a[0]            =  0;
  ptree->pv[0].a[1]            =  0;
  ptree->pv[0].depth           =  0;
  ptree->pv[0].length          =  0;
  iteration_depth              =  0;
  easy_value                   =  0;
  easy_abs                     =  0;
  right_answer_made            =  0;
  is_hash_learn_stored         =  0;
  root_abort                   =  0;
  root_nmove                   =  0;
  root_value                   = -score_bound;
  root_alpha                   = -score_bound;
  root_beta                    =  score_bound;
  root_move_cap                =  0;
  node_last_check              =  0;
  time_last_eff_search         =  time_start;
  time_last_check              =  time_start;
  game_status                 &= ~( flag_move_now | flag_quit_ponder
				    | flag_search_error );
#if defined(DBG_EASY)
  easy_move                    =  0;
#endif

#if defined(TLP)
  ptree->tlp_abort             = 0;
  tlp_nsplit                   = 0;
  tlp_nabort                   = 0;
  tlp_nslot                    = 0;
#endif

#if defined(MPV)
  if ( ! ( game_status & flag_puzzling ) && mpv_num > 1 )
    {
      int i;

      for ( i = 0; i < 2*mpv_num+1; i++ ) { mpv_pv[i].length = 0; }
      
      last_pv.a[0]    = 0;
      last_pv.a[1]    = 0;
      last_pv.depth   = 0;
      last_pv.length  = 0;
      last_root_value = 0;

      root_mpv = 1;
    }
  else { root_mpv = 0; }
#endif


  for ( ply = 0; ply < PLY_MAX; ply++ )
    {
      ptree->amove_killer[ply].no1 = ptree->amove_killer[ply].no2 = 0U;
    }

  {
    unsigned int u =  node_per_second / 16U;
    if      ( u > TIME_CHECK_MAX_NODE ) { u = TIME_CHECK_MAX_NODE; }
    else if ( u < TIME_CHECK_MIN_NODE ) { u = TIME_CHECK_MIN_NODE; }
    node_next_signal = u;
  }

  set_search_limit_time( root_turn );
  adjust_fmg();

  /* look up last pv. */
  if ( last_pv.length )
    {
      Out( "- a pv was found in the previous search result.\n" );

      iteration_depth   = last_pv.depth;
      ptree->pv[0]      = last_pv;
      ptree->pv[0].type = prev_solution;
      root_value        = root_turn ? -last_root_value : last_root_value;
      out_pv( ptree, root_value, root_turn, 0 );
      Out( "\n" );
    }

  /* probe the transposition table, since last pv is not available.  */
  if ( ! last_pv.length
#if defined(MPV)
       && ! root_mpv
#endif
       )
    {
      unsigned int value_type;
      int alpha, beta;
    
      iret = ini_hash();
      if ( iret < 0 ) { return iret; }
      is_hash_learn_stored = 1;

      value = INT_MIN;
      for ( ply = 1; ply < PLY_MAX - 10; ply++ )
	{
	  alpha = -score_bound;
	  beta  =  score_bound;
	  
	  value_type = hash_probe( ptree, 1, ply*PLY_INC+PLY_INC/2,
				   root_turn, alpha, beta, 0 );
	  if ( value_type != value_exact )   { break; }
	  value = HASH_VALUE;
      }
      
      if ( -score_bound < value )
	{
	  Out( "- a pv was peeked through the transposition table.\n" );
	  iteration_depth     = ply-1;
	  ptree->pv[0].depth  = (unsigned char)(ply-1);
	  ptree->pv[0].type   = hash_hit;
	  root_value          = value;
	  out_pv( ptree, value, root_turn, 0 );
	  Out( "\n" );
	  
	  if ( ! ptree->pv[0].length )
	    {
	      iteration_depth         = 0;
	      ptree->pv[0].depth = 0;
	      root_value              = -score_bound;
#if ! defined(MINIMUM)
	      out_warning( "PEEK FAILED!!!" );
#endif
	    }
	}
    }

  /* root move generation */
  {
    unsigned int elapsed;
    
    Out( "- root move generation" );
    value = make_root_move_list( ptree, flag );
    if ( game_status & flag_search_error ) { return -1; }
    if ( game_status & ( flag_quit | flag_quit_ponder | flag_suspend ) )
      {
	return 1;
      }

    if ( ! root_nmove )
      {
	str_error = "No legal moves to search";
	return -2;
      }

    if ( ! ptree->pv[0].length || ptree->pv[0].a[1] != root_move_list[0].move )
      {
	iteration_depth          = 0;
	ptree->pv[0].a[1]   = root_move_list[0].move;
	ptree->pv[0].length = 1;
	ptree->pv[0].depth  = 1;
	ptree->pv[0].type   = no_rep;
	root_value               = value;
      }

#if defined(MPV)
    if ( root_mpv )
      {
	if ( root_nmove == 1 ) { root_mpv = 0; }
	easy_abs = 0;
      }
#endif

    if ( get_elapsed( &elapsed ) < 0 ) { return -1; }
    Out( " ... done (%d moves, %ss)\n",
	 root_nmove, str_time_symple( elapsed - time_start ) );
  }


  /* save preliminary result */
  assert( root_value != -score_bound );
  last_root_value = root_turn ? -root_value : root_value;
  last_pv         = ptree->pv[0];


  /* return, if previous pv is long enough */
  if ( abs(root_value) > score_max_eval
       || iteration_depth >= depth_limit
       || ( ( game_status & flag_puzzling )
	    && ( root_nmove == 1 || ptree->pv[0].depth > 4 ) ) )
    {
      return 1;
    }

  if ( ! is_hash_learn_stored )
    {
      iret = ini_hash();
      if ( iret < 0 ) { return iret; }
    }

  /* iterative deepening search */
#if defined(TLP)
  iret = tlp_start();
  if ( iret < 0 ) { return iret; }
#endif
  iteration_depth += 1;
  root_beta        = set_root_beta(  0, root_value );
  root_alpha       = set_root_alpha( 0, root_value );
  root_value       = root_alpha;
  add_rejections( ptree, root_turn, 1 );
  Out( "- drive an iterative deepening search starting from depth %d\n",
       iteration_depth );

  for ( ; iteration_depth < 30/*PLY_MAX-10*/; iteration_depth++ ) {

    if ( get_elapsed( &time_last_search ) < 0 ) { return -1; }
    
#if defined(MPV)
    if ( root_mpv )
      {
	int i;
	i = ( root_nmove < mpv_num ) ? root_nmove : mpv_num;
	for ( ; i < mpv_num*2; i++ ) { mpv_pv[i].length = 0; }
      }
#endif
    
    {
      unsigned int move;
      int tt, i, n;

      tt = root_turn;
      for ( ply = 1; ply <= ptree->pv[0].length; ply++ )
	{
	  move = ptree->pv[0].a[ply];
	  if ( ! is_move_valid( ptree, move, tt ) )
	    {
#if ! defined(MINIMUM)
	      out_warning( "Old pv has an illegal move!  ply=%d, move=%s",
			   ply, str_CSA_move(move) );
#endif
	      break;
	    }
	  MakeMove( tt, move, ply );
	  if ( InCheck(tt) )
	    {
#if ! defined(MINIMUM)
	      out_warning( "Old pv has an illegal evasion!  ply=%d, move=%s",
			   ply, str_CSA_move(move) );
#endif
	      UnMakeMove( tt, move, ply );
	      break;
	    }
	  tt = Flip(tt);
	}
      for ( ply--; ply > 0; ply-- )
	{
	  tt   = Flip(tt);
	  move = ptree->pv[0].a[ply];
	  UnMakeMove( tt, move, ply );
	  hash_store_pv( ptree, move, tt );
	}

      root_nfail_high = 0;
      root_nfail_low  = 0;

      n = root_nmove;
      root_move_list[0].status = flag_first;
      for ( i = 1; i < n; i++ ) { root_move_list[i].status = 0; }
    }

    /*  a trial of searches  */
    for ( ;; ) {
      value = searchr( ptree, root_alpha, root_beta, root_turn,
		       iteration_depth*PLY_INC + PLY_INC/2 );
      if ( game_status & flag_search_error ) { return -1; }
      if ( root_abort )                      { break; }

      assert( abs(value) < score_foul );

      if ( root_beta <= value )
	{
	  const char *str_move;
	  const char *str;
	  double dvalue;
	  
	  root_move_list[0].status &= ~flag_searched;
	  root_move_list[0].status |= flag_failhigh;
	  dvalue = (double)( root_turn ? -root_beta : root_beta );

	  do { root_beta  = set_root_beta( ++root_nfail_high, root_beta ); }
	  while ( value >= root_beta );

	  str = str_time_symple( time_last_result - time_start );
	  if ( root_move_list[0].status & flag_first )
	    {
	      Out( "(%2d)%6s %7.2f ", iteration_depth, str, dvalue / 100.0 );
	    }
	  else { Out( "    %6s %7.2f ", str, dvalue / 100.0 ); }
	  str      = str_fail_high( root_turn, root_nfail_high );
	  str_move = str_CSA_move_plus( ptree, ptree->pv[1].a[1], 1,
					root_turn );
	  Out( " 1.%c%s [%s!]\n", ach_turn[root_turn], str_move, str );
	  if ( game_status & flag_pondering )
	    {
	      OutCsaShogi( "info%+.2f %c%s %c%s [%s!]\n",
			   dvalue / 100.0, ach_turn[Flip(root_turn)],
			   str_CSA_move(ponder_move),
			   ach_turn[root_turn], str_move, str );
	    }
	  else {
	    OutCsaShogi( "info%+.2f %c%s [%s!]\n", dvalue / 100.0,
			 ach_turn[root_turn], str_move, str );
	  }
	}
      else if ( value <= root_alpha )
	{
	  const char *str_move;
	  const char *str;
	  unsigned int time_elapsed;
	  double dvalue;

	  if ( ! ( root_move_list[0].status & flag_first ) )
	    {
	      root_value = root_alpha;
	      break;
	    }

	  root_move_list[0].status &= ~flag_searched;
	  root_move_list[0].status |= flag_faillow;
	  dvalue = (double)( root_turn ? -root_alpha : root_alpha );

	  if ( get_elapsed( &time_elapsed ) < 0 ) { return -1; }

	  do { root_alpha = set_root_alpha( ++root_nfail_low, root_alpha ); }
	  while ( value <= root_alpha );
	  root_value = root_alpha;
	  str = str_time_symple( time_elapsed - time_start );
	  Out( "(%2d)%6s %7.2f ", iteration_depth, str, dvalue / 100.0 );

	  str      = str_fail_high( Flip(root_turn), root_nfail_low );
	  str_move = str_CSA_move_plus( ptree, root_move_list[0].move, 1,
					root_turn );
	  Out( " 1.%c%s [%s?]\n", ach_turn[root_turn], str_move, str );
	  if ( game_status & flag_pondering )
	    {
	      OutCsaShogi( "info%+.2f %c%s %c%s [%s?]\n",
			   dvalue / 100.0, ach_turn[Flip(root_turn)],
			   str_CSA_move(ponder_move),
			   ach_turn[root_turn], str_move, str );
	    }
	  else {
	    OutCsaShogi( "info%+.2f %c%s [%s?]\n", dvalue / 100.0,
			 ach_turn[root_turn], str_move, str );
	  }
	}
      else { break; }
    }

    /* the trial of search ended */
    if ( root_alpha < root_value && root_value < root_beta )
      {
	last_root_value = root_turn ? - root_value : root_value;
	last_pv         = ptree->pv[0];
      }

    if ( root_abort ) { break; }

    if ( root_alpha < root_value && root_value < root_beta )
      {
#if ! defined(MINIMUM)
	{
	  int i, n;
	  n = root_nmove;
	  for ( i = 0; i < n; i++ )
	    {
	      if ( root_move_list[i].status & flag_searched ) { continue; }
	      out_warning( "A root move %s is ignored\n",
			   str_CSA_move(root_move_list[i].move) );
	    }
	}
#endif

	if ( ( game_status & flag_problem ) && depth_limit == PLY_MAX )
	  {
	    if ( is_answer_right( ptree->pv[0].a[1] ) )
	      {
		if ( right_answer_made > 1 && iteration_depth > 3 ) { break; }
		right_answer_made++;
	      }
	    else { right_answer_made = 0; }
	  }
	
	if ( abs(value)      >  score_max_eval ) { break; }
	if ( iteration_depth >= depth_limit )    { break; }
	
	root_beta  = set_root_beta(  0, value );
	root_alpha = set_root_alpha( 0, value );
	root_value = root_alpha;
      }
#if ! defined(MINIMUM)
    else { out_warning(( "SEARCH INSTABILITY IS DETECTED!!!" )); }
#endif

    /* shell sort */
    {
      root_move_t root_move_swap;
      const int n = root_nmove;
      uint64_t sortv;
      int i, j, k, h;
      
      for ( k = SHELL_H_LEN - 1; k >= 0; k-- )
	{
	  h = ashell_h[k];
	  for ( i = n-h-1; i > 0; i-- )
	    {
	      root_move_swap = root_move_list[i];
	      sortv          = root_move_list[i].nodes;
	      for ( j = i+h; j < n && root_move_list[j].nodes > sortv; j += h )
		{
		  root_move_list[j-h] = root_move_list[j];
		}
	      root_move_list[j-h] = root_move_swap;
	    }
	}
    }
  }

  /* iteration ended */
  sub_rejections( ptree, root_turn, 1 );

  if ( game_status & flag_problem )
    {
      if ( is_answer_right( ptree->pv[0].a[1] ) )
	{
	  right_answer_made = 1;
	}
      else { right_answer_made = 0; }
    }

  {
    int i;
    
    for ( i = 0; i < 2*nsquare*(nsquare+7); i++ )
	{
	  ptree->history[0][0][i] /= 256U;
	}
  }
  /* prunings and extentions-statistics */
  {
    double drep, dreject, dhash, dnull, dfh1st;

    drep    = (double)ptree->nperpetual_check;
    drep   += (double)ptree->nfour_fold_rep;
    drep   += (double)ptree->nsuperior_rep;
    drep   *= 100.0 / (double)( ptree->nrep_tried + 1 );

    dreject  = 100.0 * (double)ptree->nreject_done;
    dreject /= (double)( ptree->nreject_tried + 1 );

    dhash   = (double)ptree->ntrans_exact;
    dhash  += (double)ptree->ntrans_inferior_hit;
    dhash  += (double)ptree->ntrans_superior_hit;
    dhash  += (double)ptree->ntrans_upper;
    dhash  += (double)ptree->ntrans_lower;
    dhash  *= 100.0 / (double)( ptree->ntrans_probe + 1 );

    dnull   = 100.0 * (double)ptree->null_pruning_done;
    dnull  /= (double)( ptree->null_pruning_tried + 1 );

    dfh1st  = 100.0 * (double)ptree->fail_high_first;
    dfh1st /= (double)( ptree->fail_high + 1 );

    Out( "    pruning  -> rep=%4.2f%%  reject=%4.2f%%\n", drep, dreject );
    
    Out( "    pruning  -> hash=%2.0f%%  null=%2.0f%%  fh1st=%4.1f%%\n",
	 dhash, dnull, dfh1st );
    
    Out( "    extension-> chk=%u recap=%u 1rep=%u\n",
	 ptree->check_extension_done, ptree->recap_extension_done,
	 ptree->onerp_extension_done );
  }

  /* futility threashold */
#if ! ( defined(NO_STDOUT) && defined(NO_LOGGING) )
  {
    int misc   = fmg_misc;
    int drop   = fmg_drop;
    int cap    = fmg_cap;
    int mt     = fmg_mt;
    int misc_k = fmg_misc_king;
    int cap_k  = fmg_cap_king;
    Out( "    futility -> misc=%d drop=%d cap=%d mt=%d misc(k)=%d cap(k)=%d\n",
	 misc, drop, cap, mt, misc_k, cap_k );
  }
#endif

  /* hashing-statistics */
  {
    double dalways, dprefer, dsupe, dinfe;
    double dlower, dupper, dsat;
    uint64_t word2;
    int ntrans_table, i, n;

    ntrans_table = 1 << log2_ntrans_table;
    if ( ntrans_table > 8192 ) { ntrans_table = 8192; }
    
    for ( i = 0, n = 0; i < ntrans_table; i++ )
      {
	word2 = ptrans_table[i].prefer.word2;
	SignKey( word2, ptrans_table[i].prefer.word1 );
	if ( trans_table_age == ( 7 & (int)word2 ) ) { n++; }

	word2 = ptrans_table[i].always[0].word2;
	SignKey( word2, ptrans_table[i].always[0].word1 );
	if ( trans_table_age == ( 7 & (int)word2 ) ) { n++; }

	word2 = ptrans_table[i].always[1].word2;
	SignKey( word2, ptrans_table[i].always[1].word1 );
	if ( trans_table_age == ( 7 & (int)word2 ) ) { n++; }
      }

    dalways  = 100.0 * (double)ptree->ntrans_always_hit;
    dalways /= (double)( ptree->ntrans_probe + 1 );

    dprefer  = 100.0 * (double)ptree->ntrans_prefer_hit;
    dprefer /= (double)( ptree->ntrans_probe + 1 );

    dsupe    = 100.0 * (double)ptree->ntrans_superior_hit;
    dsupe   /= (double)( ptree->ntrans_probe + 1 );

    dinfe    = 100.0 * (double)ptree->ntrans_inferior_hit;
    dinfe   /= (double)( ptree->ntrans_probe + 1 );

    Out( "    hashing  -> always=%2.0f%% prefer=%2.0f%% supe=%4.2f%% "
	 "infe=%4.2f%%\n", dalways, dprefer, dsupe, dinfe );

    dlower  = 100.0 * (double)ptree->ntrans_lower;
    dlower /= (double)( ptree->ntrans_probe + 1 );

    dupper  = 100.0 * (double)ptree->ntrans_upper;
    dupper /= (double)( ptree->ntrans_probe + 1 );

    dsat    = 100.0 * (double)n;
    dsat   /= (double)( 3 * ntrans_table );

    OutCsaShogi( "statsatu=%.0f", dsat );
    Out( "    hashing  -> "
	 "exact=%d lower=%2.0f%% upper=%4.2f%% sat=%2.0f%% age=%d\n",
	 ptree->ntrans_exact, dlower, dupper, dsat, trans_table_age );
    if ( dsat > 9.0 ) { trans_table_age  = ( trans_table_age + 1 ) & 0x7; }
  }

#if defined(TLP)
  if ( tlp_max > 1 )
    {
      Out( "    threading-> split=%d abort=%d slot=%d\n",
	   tlp_nsplit, tlp_nabort, tlp_nslot+1 );
      if ( tlp_nslot+1 == TLP_NUM_WORK )
	{
	  out_warning( "THREAD WORK AREA IS USED UP!!!" );
	}
    }
#endif

  {
    double dcpu_percent, dnps, dmat;
    unsigned int cpu, elapsed;

    Out( "    n=%" PRIu64 " quies=%u eval=%u rep=%u %u(chk) %u(supe)\n",
	 ptree->node_searched, ptree->nquies_called, ptree->neval_called,
	 ptree->nfour_fold_rep, ptree->nperpetual_check,
	 ptree->nsuperior_rep );

    if ( get_cputime( &cpu )     < 0 ) { return -1; }
    if ( get_elapsed( &elapsed ) < 0 ) { return -1; }

    cpu             -= cpu_start;
    elapsed         -= time_start;

    dcpu_percent     = 100.0 * (double)cpu;
    dcpu_percent    /= (double)( elapsed + 1U );

    dnps             = 1000.0 * (double)ptree->node_searched;
    dnps            /= (double)( elapsed + 1U );

#if defined(TLP)
    {
      double n = (double)tlp_max;
      node_per_second  = (unsigned int)( ( dnps + 0.5 ) / n );
    }
#else
    node_per_second  = (unsigned int)( dnps + 0.5 );
#endif

    dmat             = (double)MATERIAL;
    dmat            /= (double)MT_CAP_PAWN;

    OutCsaShogi( " cpu=%.0f nps=%.2f\n", dcpu_percent, dnps / 1e3 );
    Out( "    time=%s  ", str_time_symple( elapsed ) );
    Out( "cpu=%3.0f%%  mat=%.1f  nps=%.2fK", dcpu_percent, dmat, dnps / 1e3 );
    Out( "  time_eff=%s\n\n",
	 str_time_symple( time_last_eff_search - time_start ) );
  }

  if ( ( game_status & flag_problem ) && ! right_answer_made ) { iret = 0; }
  else                                                         { iret = 1; }

  return iret;
}


static int
ini_hash( void )
{
  unsigned int elapsed;
  int iret;

  if ( time_limit < 150U ) { return 1; }
  
  iret = all_hash_learn_store();
  if ( iret < 0 ) { return iret; }
  if ( iret )
    {
      if ( get_elapsed( &elapsed ) < 0 ) { return -1; }
      Out( "- load learnt hash values (%ss)\n",
	   str_time_symple( elapsed - time_start ) );
    }

  return 1;
}


static void
adjust_fmg( void )
{
  int misc, cap, drop, mt, misc_king, cap_king;

  misc      = fmg_misc      - FMG_MG      / 2;
  cap       = fmg_cap       - FMG_MG      / 2;
  drop      = fmg_drop      - FMG_MG      / 2;
  misc_king = fmg_misc_king - FMG_MG_KING / 2;
  cap_king  = fmg_cap_king  - FMG_MG_KING / 2;
  mt        = fmg_mt        - FMG_MG_MT   / 2;

  fmg_misc      = ( misc      < FMG_MISC      ) ? FMG_MISC      : misc;
  fmg_cap       = ( cap       < FMG_CAP       ) ? FMG_CAP       : cap;
  fmg_drop      = ( drop      < FMG_DROP      ) ? FMG_DROP      : drop;
  fmg_misc_king = ( misc_king < FMG_MISC_KING ) ? FMG_MISC_KING : misc_king;
  fmg_cap_king  = ( cap_king  < FMG_CAP_KING  ) ? FMG_CAP_KING  : cap_king;
  fmg_mt        = ( mt        < FMG_MT        ) ? FMG_MT        : mt;
}


static int
set_root_beta( int nfail_high, int root_beta_old )
{
  int aspiration_hwdth, aspiration_fail1;

  if ( time_max_limit != time_limit )
    {
      aspiration_hwdth = MT_CAP_DRAGON / 8;
      aspiration_fail1 = MT_CAP_DRAGON / 2;
    }
  else {
    aspiration_hwdth = MT_CAP_DRAGON / 4;
    aspiration_fail1 = ( MT_CAP_DRAGON * 3 ) / 4;
  }

  switch ( nfail_high )
    {
    case 0:  root_beta_old += aspiration_hwdth;                     break;
    case 1:  root_beta_old += aspiration_fail1 - aspiration_hwdth;  break;
    case 2:  root_beta_old  = score_bound;                          break;
    default:
      out_error( "Error at set_root_beta!" );
      exit(1);
    }
  if ( root_beta_old > score_max_eval ) { root_beta_old = score_bound; }

  return root_beta_old;
}


static int
set_root_alpha( int nfail_low, int root_alpha_old )
{
  int aspiration_hwdth, aspiration_fail1;

  if ( time_max_limit != time_limit )
    {
      aspiration_hwdth = MT_CAP_DRAGON / 8;
      aspiration_fail1 = MT_CAP_DRAGON / 2;
    }
  else {
    aspiration_hwdth = MT_CAP_DRAGON / 4;
    aspiration_fail1 = ( MT_CAP_DRAGON * 3 ) / 4;
  }

  switch ( nfail_low )
    {
    case 0:  root_alpha_old -= aspiration_hwdth;                     break;
    case 1:  root_alpha_old -= aspiration_fail1 - aspiration_hwdth;  break;
    case 2:  root_alpha_old  = -score_bound;                         break;
    default:
      out_error( "Error at set_root_alpha!" );
      exit(1);
    }
  if ( root_alpha_old < -score_max_eval ) { root_alpha_old = -score_bound; }

  return root_alpha_old;
}


static const char *
str_fail_high( int turn, int nfail_high )
{
  const char *str;

  if ( time_max_limit != time_limit )
    {
      if ( nfail_high == 1 ) { str = turn ? "-1" : "+1"; }
      else                   { str = turn ? "-4" : "+4"; }
    }
  else {
    if ( nfail_high == 1 ) { str = turn ? "-2" : "+2"; }
    else                   { str = turn ? "-6" : "+6"; }
  }
  return str;
}


static int
is_answer_right( unsigned int move )
{
  const char *str_anser;
  const char *str_move;
  int ianser, iret;
  
  iret     = 0;
  str_move = str_CSA_move( move );

  for ( ianser = 0; ianser < MAX_ANSWER; ianser++ )
    {
      str_anser = &( record_problems.info.str_move[ianser][0] );
      if ( str_anser[0] == '\0' ) { break; }
      if ( ! strcmp( str_anser+1, str_move ) )
	{
	  iret = 1;
	  break;
	}
    }

  return iret;
}
