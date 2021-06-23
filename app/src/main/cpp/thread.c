#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#  include <process.h>
#else
#  include <sched.h>
#endif
#include "shogi.h"

#if defined(TLP)

#  if defined(_WIN32)
static unsigned int __stdcall start_address( void *arg );
#  else
static void *start_address( void *arg );
#  endif

static tree_t *find_child( void );
static void init_state( const tree_t * restrict parent,
			tree_t * restrict child );
static void copy_state( tree_t * restrict parent,
			const tree_t * restrict child, int value );
static void wait_work( int tid, tree_t *parent );


int
tlp_start( void )
{
  int work[ TLP_MAX_THREADS ];
  int num;

  if ( tlp_num ) { return 1; }

  for ( num = 1; num < tlp_max; num++ )
    {
      work[num] = num;
      
#  if defined(_WIN32)
      if ( ! _beginthreadex( 0, 0, start_address, work+num, 0, 0 ) )
	{
	  str_error = "_beginthreadex() failed.";
	  return -2;
	}
#  else
      {
	pthread_t pt;
	if ( pthread_create( &pt, &pthread_attr, start_address, work+num ) )
	  {
	    str_error = "pthread_create() failed.";
	    return -2;
	  }
      }
#  endif
    }
  while ( tlp_num +1 < tlp_max ) { tlp_yield(); }

  return 1;
}


void
tlp_end( void )
{
  tlp_abort = 1;
  while ( tlp_num ) { tlp_yield(); }
  tlp_abort = 0;
}


int
tlp_split( tree_t * restrict ptree )
{
  tree_t *child;
  int num, nchild;

  lock( &tlp_lock );

  if ( ! tlp_idle || ptree->tlp_abort )
    {
      unlock( &tlp_lock );
      return 0;
    }

  tlp_ptrees[ ptree->tlp_id ] = NULL;
  ptree->tlp_nsibling         = 0;
  ptree->tlp_best             = ptree->tlp_value;
  nchild                      = 0;
  for ( num = 0; num < tlp_max; num++ )
    {
      if ( tlp_ptrees[num] ) { ptree->tlp_ptrees_sibling[num] = 0; }
      else {
	child = find_child();
	if ( ! child ) { continue; }

	init_state( ptree, child );

	nchild += 1;
	child->tlp_ptree_parent         = ptree;
	child->tlp_id                   = (unsigned char)num;
	ptree->tlp_ptrees_sibling[num]  = child;
	ptree->tlp_nsibling            += 1;

	tlp_ptrees[num] = child;
      }
    }

  if ( ! nchild )
    {
      tlp_ptrees[ ptree->tlp_id ] = ptree;
      unlock( &tlp_lock );
      return 0;
    }
  
  tlp_nsplit += 1;
  tlp_idle   += 1;

  unlock( &tlp_lock );
  
  wait_work( ptree->tlp_id, ptree );

  return 1;
}


void
tlp_set_abort( tree_t * restrict ptree )
{
  int num;

  ptree->tlp_abort = 1;
  for ( num = 0; num < tlp_max; num++ )
    if ( ptree->tlp_ptrees_sibling[num] )
      {
	tlp_set_abort( ptree->tlp_ptrees_sibling[num] );
      }
}


int
tlp_is_descendant( const tree_t * restrict ptree, int slot_ancestor )
{
  int slot = (int)ptree->tlp_slot;

  for ( ;; ) {
    if ( slot == slot_ancestor ) { return 1; }
    else if ( ! slot )           { return 0; }
    else { slot = tlp_atree_work[slot].tlp_ptree_parent->tlp_slot; }
  }
}


int
lock_init( lock_t *plock )
{
#  if defined(_MSC_VER)
  *plock = 0;
#  elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
  *plock = 0;
#  else
  if ( pthread_mutex_init( plock, 0 ) )
    {
      str_error = "pthread_mutex_init() failed.";
      return -1;
    }
#  endif
  return 1;
}


int
lock_free( lock_t *plock )
{
#  if defined(_MSC_VER)
  *plock = 0;
#  elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
  *plock = 0;
#  else
  if ( pthread_mutex_destroy( plock ) )
    {
      str_error = "pthread_mutex_destroy() failed.";
      return -1;
    }
#  endif
  return 1;
}


void
unlock( lock_t *plock )
{
#  if defined(_MSC_VER)
  *plock = 0;
#  elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
  *plock = 0;
#  else
  pthread_mutex_unlock( plock );
#  endif
}


void
lock( lock_t *plock )
{
#  if defined(_MSC_VER)
  long l;

  for ( ;; )
    {
      l = _InterlockedExchange( (void *)plock, 1 );
      if ( ! l ) { return; }
      while ( *plock );
    }
#  elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
  int itemp;

  for ( ;; )
    {
      asm ( "1:   movl     $1,  %1 \n\t"
	    "     xchgl   (%0), %1 \n\t"
	    : "=g" (plock), "=r" (itemp) : "0" (plock) );
      if ( ! itemp ) { return; }
      while ( *plock );
    }
#  else
  pthread_mutex_lock( plock );
#  endif
}


void
tlp_yield( void )
{
#if defined(_WIN32)
  Sleep( 0 );
#else
  sched_yield();
#endif
}


#  if defined(_MSC_VER)
static unsigned int __stdcall start_address( void *arg )
#  else
static void *start_address( void *arg )
#endif
{
  int tid = *(int *)arg;

  tlp_ptrees[tid] = NULL;

  lock( &tlp_lock );
  Out( "Hi from thread no.%d\n", tid );
  tlp_num  += 1;
  tlp_idle += 1;
  unlock( &tlp_lock );

  wait_work( tid, NULL );

  lock( &tlp_lock );
  Out( "Bye from thread no.%d\n", tid );
  tlp_num  -= 1;
  tlp_idle -= 1;
  unlock( &tlp_lock );

  return 0;
}


static void
wait_work( int tid, tree_t *parent )
{
  tree_t *slot;
  int value;

  for ( ;; ) {

    for ( ;; ) {
      if ( tlp_ptrees[tid] )                  { break; }
      if ( parent && ! parent->tlp_nsibling ) { break; }
      if ( tlp_abort )                        { return; }

      tlp_yield();
    }

    lock( &tlp_lock );
    if ( ! tlp_ptrees[tid] ) { tlp_ptrees[tid] = parent; }
    tlp_idle -= 1;
    unlock( &tlp_lock );

    slot = tlp_ptrees[tid];
    if ( slot == parent ) { return; }

    value = tlp_search( slot, slot->tlp_alpha, slot->tlp_beta,
			slot->tlp_turn, slot->tlp_depth, slot->tlp_ply,
			slot->tlp_state_node );
    
    lock( &tlp_lock );
    copy_state( slot->tlp_ptree_parent, slot, value );
    slot->tlp_ptree_parent->tlp_nsibling            -= 1;
    slot->tlp_ptree_parent->tlp_ptrees_sibling[tid]  = NULL;
    slot->tlp_used = 0;

    tlp_ptrees[tid]  = NULL;
    tlp_idle        += 1;
    unlock( &tlp_lock);
  }
}


static tree_t *
find_child( void )
{
  int i;

  for ( i = 1; i < TLP_NUM_WORK && tlp_atree_work[i].tlp_used; i++ );
  if ( i == TLP_NUM_WORK ) { return NULL; }
  if ( i > tlp_nslot ) { tlp_nslot = i; }

  return tlp_atree_work + i;
}


static void
init_state( const tree_t * restrict parent, tree_t * restrict child )
{
  int i, ply;

  for ( i = 0; i < tlp_max; i++ ) { child->tlp_ptrees_sibling[i] = NULL; }

  child->tlp_abort       = 0;
  child->tlp_used        = 1;
  child->tlp_alpha       = parent->tlp_alpha;
  child->tlp_beta        = parent->tlp_beta;
  child->tlp_value       = parent->tlp_value;
  child->tlp_depth       = parent->tlp_depth;
  child->tlp_state_node  = parent->tlp_state_node;
  child->tlp_turn        = parent->tlp_turn;
  child->tlp_ply         = parent->tlp_ply;
  child->posi            = parent->posi;

  ply = parent->tlp_ply;

  for ( i = 0; i < root_nrep + ply - 1; i++ )
    {
      child->rep_board_list[i] = parent->rep_board_list[i];
      child->rep_hand_list[i]  = parent->rep_hand_list[i];
    }
  for ( i = ply+1; i < PLY_MAX; i++ )
    {
      child->amove_killer[i] = parent->amove_killer[i];
    }

  memcpy( child->history, parent->history, sizeof(parent->history) );

  child->anext_move[ply].value_cap1 = parent->anext_move[ply].value_cap1;
  child->anext_move[ply].value_cap2 = parent->anext_move[ply].value_cap2;
  child->anext_move[ply].move_cap1  = parent->anext_move[ply].move_cap1;
  child->anext_move[ply].move_cap2  = parent->anext_move[ply].move_cap2;

  child->move_last[ply]        = child->amove;
  child->pv[ply]               = parent->pv[ply];
  child->stand_pat[ply]        = parent->stand_pat[ply];
  child->current_move[ply-1]   = parent->current_move[ply-1];
  child->nsuc_check[ply-1]     = parent->nsuc_check[ply-1];
  child->nsuc_check[ply]       = parent->nsuc_check[ply];
  child->node_searched         = 0;
  child->null_pruning_done     = 0;
  child->null_pruning_tried    = 0;
  child->check_extension_done  = 0;
  child->recap_extension_done  = 0;
  child->onerp_extension_done  = 0;
  child->neval_called          = 0;
  child->nquies_called         = 0;
  child->nfour_fold_rep        = 0;
  child->nperpetual_check      = 0;
  child->nsuperior_rep         = 0;
  child->nrep_tried            = 0;
  child->nreject_tried         = 0;
  child->nreject_done          = 0;
  child->ntrans_always_hit     = 0;
  child->ntrans_prefer_hit     = 0;
  child->ntrans_probe          = 0;
  child->ntrans_exact          = 0;
  child->ntrans_lower          = 0;
  child->ntrans_upper          = 0;
  child->ntrans_superior_hit   = 0;
  child->ntrans_inferior_hit   = 0;
  child->fail_high             = 0;
  child->fail_high_first       = 0;
}


static void
copy_state( tree_t * restrict parent, const tree_t * restrict child,
	    int value )
{
  int i, ply;

  if ( child->node_searched )
    {
      if ( ! child->tlp_abort && value > parent->tlp_best )
	{
	  ply = parent->tlp_ply;
	  parent->tlp_best = (short)value;
	  parent->pv[ply]  = child->pv[ply];

	  lock( & parent->tlp_lock );
	  for ( i = ply+1; i < PLY_MAX; i++ ) 
	    {
	      parent->amove_killer[i]   = child->amove_killer[i];
	    }
	  memcpy( parent->history, child->history, sizeof(child->history) );
	  unlock( & parent->tlp_lock );
	}

      parent->node_searched         += child->node_searched;
      parent->null_pruning_done     += child->null_pruning_done;
      parent->null_pruning_tried    += child->null_pruning_tried;
      parent->onerp_extension_done  += child->onerp_extension_done;
      parent->neval_called          += child->neval_called;
      parent->nquies_called         += child->nquies_called;
      parent->nrep_tried            += child->nrep_tried;
      parent->nfour_fold_rep        += child->nfour_fold_rep;
      parent->nperpetual_check      += child->nperpetual_check;
      parent->nsuperior_rep         += child->nsuperior_rep;
      parent->nreject_tried         += child->nreject_tried;
      parent->nreject_done          += child->nreject_done;
      parent->ntrans_always_hit     += child->ntrans_always_hit;
      parent->ntrans_prefer_hit     += child->ntrans_prefer_hit;
      parent->ntrans_probe          += child->ntrans_probe;
      parent->ntrans_exact          += child->ntrans_exact;
      parent->ntrans_lower          += child->ntrans_lower;
      parent->ntrans_upper          += child->ntrans_upper;
      parent->ntrans_superior_hit   += child->ntrans_superior_hit;
      parent->ntrans_inferior_hit   += child->ntrans_inferior_hit;
      parent->fail_high_first       += child->fail_high_first;
      parent->fail_high             += child->fail_high;
    }
  parent->check_extension_done  += child->check_extension_done;
  parent->recap_extension_done  += child->recap_extension_done;
}

#endif /* TLP */
