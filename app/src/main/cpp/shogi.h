#ifndef SHOGI_H
#define SHOGI_H

#include <stdio.h>
#include "param.h"

#if defined(_WIN32)

#  include <Winsock2.h>
#  define CONV_CDECL         __cdecl
#  define SCKT_NULL         INVALID_SOCKET
typedef SOCKET sckt_t;

#else

#  include <pthread.h>
#  include <sys/times.h>
#  define CONV_CDECL
#  define SCKT_NULL         -1
#  define SOCKET_ERROR      -1
typedef int sckt_t;

#endif

/* Microsoft C/C++ */
#if defined(_MSC_VER)

#  define _CRT_DISABLE_PERFCRIT_LOCKS
#  define UINT64_MAX    ULLONG_MAX
#  define PRIu64        "I64u"
#  define PRIx64        "I64x"
#  define UINT64_C(u)  ( u )

#  define restrict      __restrict
#  define strtok_r      strtok_s
#  define read          _read
#  define strncpy( dst, src, len ) strncpy_s( dst, len, src, _TRUNCATE )
#  define snprintf( buf, size, fmt, ... )   \
          _snprintf_s( buf, size, _TRUNCATE, fmt, __VA_ARGS__ )
#  define vsnprintf( buf, size, fmt, list ) \
          _vsnprintf_s( buf, size, _TRUNCATE, fmt, list )
typedef unsigned __int64 uint64_t;
typedef volatile long lock_t;

/* GNU C and Intel C/C++ on x86 and x86-64 */
#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )

#  include <inttypes.h>
#  define restrict __restrict
typedef volatile int lock_t;

/* other targets. */
#else

#  include <inttypes.h>
typedef pthread_mutex_t lock_t;
extern unsigned char aifirst_one[512];
extern unsigned char ailast_one[512];

#endif

/*
  #define BK_ULTRA_NARROW
  #define BK_COM
  #define BK_SMALL
  #define NO_NULL_PRUNE
  #define NO_STDOUT
  #define DBG_EASY
*/

#if defined(CSASHOGI)
#  define NO_STDOUT
#  if ! defined(WIN32_PIPE)
#    define WIN32_PIPE
#  endif
#endif

#if defined(TLP)
#  define SHARE volatile
#  define SEARCH_ABORT ( root_abort || ptree->tlp_abort )
#else
#  define SHARE
#  define SEARCH_ABORT root_abort
#endif

#define QUIES_PLY_LIMIT         7
#define SHELL_H_LEN             7
#define MAX_ANSWER              8
#define PLY_INC                 8
#define PLY_MAX                 48
#define RAND_N                  624
#define TC_NMOVE                37U
#define SEC_MARGIN              30U
#define SEC_KEEP_ALIVE          180U
#define TIME_RESPONSE           200U
#define RESIGN_THRESHOLD       ( ( MT_CAP_DRAGON * 5 ) /  8 )
#define BNZ_VER                 "4.1.3"

#define REP_MAX_PLY             32
#define REP_HIST_LEN            256

#define EHASH_MASK              0x3fffffU  /* occupies 32MB */

#define REJEC_MASK              0x0ffffU
#define REJEC_MIN_DEPTH        ( ( PLY_INC  * 5 ) )

#define EXT_RECAP1             ( ( PLY_INC  * 1 ) /  4 )
#define EXT_RECAP2             ( ( PLY_INC  * 2 ) /  4 )
#define EXT_ONEREP             ( ( PLY_INC  * 2 ) /  4 )
#define EXT_CHECK              ( ( PLY_INC  * 4 ) /  4 )

#define EFUTIL_MG1             ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define EFUTIL_MG2             ( ( MT_CAP_DRAGON * 2 ) /  8 )

#define FMG_MG                 ( ( MT_CAP_DRAGON * 2 ) / 16 )
#define FMG_MG_KING            ( ( MT_CAP_DRAGON * 3 ) / 16 )
#define FMG_MG_MT              ( ( MT_CAP_DRAGON * 8 ) / 16 )
#define FMG_MISC               ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_CAP                ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_DROP               ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_MT                 ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_MISC_KING          ( ( MT_CAP_DRAGON * 2 ) /  8 )
#define FMG_CAP_KING           ( ( MT_CAP_DRAGON * 2 ) /  8 )

#define HASH_REG_HIST_LEN       256
#define HASH_REG_MINDIFF       ( ( MT_CAP_DRAGON * 1 ) /  8 )
#define HASH_REG_THRESHOLD     ( ( MT_CAP_DRAGON * 8 ) /  8 )

#define FV_WINDOW               256
#define FV_SCALE                32
#define FV_PENALTY             ( 0.2 / (double)FV_SCALE )

#define MPV_MAX_PV              16

#define TLP_DEPTH_SPLIT        ( PLY_INC * 3 )
#define TLP_MAX_THREADS         8
#define TLP_NUM_WORK           ( TLP_MAX_THREADS * 8 )

#define TIME_CHECK_MIN_NODE     10000U
#define TIME_CHECK_MAX_NODE     100000U

#define SIZE_FILENAME           256
#define SIZE_PLAYERNAME         256
#define SIZE_MESSAGE            512
#define SIZE_CMDLINE            512
#define SIZE_CSALINE            512
#define SIZE_CMDBUFFER          512

#define IsMove(move)           ( (move) & 0xffffffU )
#define MOVE_NA                 0x00000000U
#define MOVE_PASS               0x01000000U
#define MOVE_PONDER_FAILED      0xfe000000U
#define MOVE_RESIGN             0xff000000U

#define MAX_LEGAL_MOVES         1024
#define MAX_LEGAL_EVASION       256
#define MOVE_LIST_LEN           16384

#define MAX_SIZE_SECTION        0xffff
#define NUM_SECTION             0x4000

#define MATERIAL            (ptree->posi.material)
#define HAND_B              (ptree->posi.hand_black)
#define HAND_W              (ptree->posi.hand_white)

#define BB_BOCCUPY          (ptree->posi.b_occupied)
#define BB_BTGOLD           (ptree->posi.b_tgold)
#define BB_B_HDK            (ptree->posi.b_hdk)
#define BB_B_BH             (ptree->posi.b_bh)
#define BB_B_RD             (ptree->posi.b_rd)
#define BB_BPAWN_ATK        (ptree->posi.b_pawn_attacks)
#define BB_BPAWN            (ptree->posi.b_pawn)
#define BB_BLANCE           (ptree->posi.b_lance)
#define BB_BKNIGHT          (ptree->posi.b_knight)
#define BB_BSILVER          (ptree->posi.b_silver)
#define BB_BGOLD            (ptree->posi.b_gold)
#define BB_BBISHOP          (ptree->posi.b_bishop)
#define BB_BROOK            (ptree->posi.b_rook)
#define BB_BKING            (abb_mask[SQ_BKING])
#define BB_BPRO_PAWN        (ptree->posi.b_pro_pawn)
#define BB_BPRO_LANCE       (ptree->posi.b_pro_lance)
#define BB_BPRO_KNIGHT      (ptree->posi.b_pro_knight)
#define BB_BPRO_SILVER      (ptree->posi.b_pro_silver)
#define BB_BHORSE           (ptree->posi.b_horse)
#define BB_BDRAGON          (ptree->posi.b_dragon)

#define BB_WOCCUPY          (ptree->posi.w_occupied)
#define BB_WTGOLD           (ptree->posi.w_tgold)
#define BB_W_HDK            (ptree->posi.w_hdk)
#define BB_W_BH             (ptree->posi.w_bh)
#define BB_W_RD             (ptree->posi.w_rd)
#define BB_WPAWN_ATK        (ptree->posi.w_pawn_attacks)
#define BB_WPAWN            (ptree->posi.w_pawn)
#define BB_WLANCE           (ptree->posi.w_lance)
#define BB_WKNIGHT          (ptree->posi.w_knight)
#define BB_WSILVER          (ptree->posi.w_silver)
#define BB_WGOLD            (ptree->posi.w_gold)
#define BB_WBISHOP          (ptree->posi.w_bishop)
#define BB_WROOK            (ptree->posi.w_rook)
#define BB_WKING            (abb_mask[SQ_WKING])
#define BB_WPRO_PAWN        (ptree->posi.w_pro_pawn)
#define BB_WPRO_LANCE       (ptree->posi.w_pro_lance)
#define BB_WPRO_KNIGHT      (ptree->posi.w_pro_knight)
#define BB_WPRO_SILVER      (ptree->posi.w_pro_silver)
#define BB_WHORSE           (ptree->posi.w_horse)
#define BB_WDRAGON          (ptree->posi.w_dragon)

#define OCCUPIED_FILE       (ptree->posi.occupied_rl90)
#define OCCUPIED_DIAG1      (ptree->posi.occupied_rr45)
#define OCCUPIED_DIAG2      (ptree->posi.occupied_rl45)
#define BOARD               (ptree->posi.asquare)

#define SQ_BKING            (ptree->posi.isquare_b_king)
#define SQ_WKING            (ptree->posi.isquare_w_king)

#define HASH_KEY            (ptree->posi.hash_key)
#define HASH_VALUE          (ptree->sort_value[0])
#define MOVE_CURR           (ptree->current_move[ply])
#define MOVE_LAST           (ptree->current_move[ply-1])

#define NullDepth(d) ( (d) <  PLY_INC*26/4 ? (d)-PLY_INC*12/4 :              \
                     ( (d) <= PLY_INC*30/4 ? PLY_INC*14/4                    \
                                           : (d)-PLY_INC*16/4) )

#define LimitExtension(e,ply) if ( (e) && (ply) > 2 * iteration_depth ) {     \
                                if ( (ply) < 4 * iteration_depth ) {          \
                                  e *= 4 * iteration_depth - (ply);           \
                                  e /= 2 * iteration_depth;                   \
                                } else { e = 0; } }

#define Flip(turn)          ((turn)^1)
#define Inv(sq)             (nsquare-1-sq)
#define PcOnSq(k,i)         (*p_pc_on_sq)[k][(i)*((i)+3)/2]
#define PcPcOnSq(k,i,j)     (*p_pc_on_sq)[k][(i)*((i)+1)/2+(j)]

/*
  xxxxxxxx xxxxxxxx xxx11111  pawn
  xxxxxxxx xxxxxxxx 111xxxxx  lance
  xxxxxxxx xxxxx111 xxxxxxxx  knight
  xxxxxxxx xx111xxx xxxxxxxx  silver
  xxxxxxx1 11xxxxxx xxxxxxxx  gold
  xxxxx11x xxxxxxxx xxxxxxxx  bishop
  xxx11xxx xxxxxxxx xxxxxxxx  rook
 */
#define I2HandPawn(hand)       (((hand) >>  0) & 0x1f)
#define I2HandLance(hand)      (((hand) >>  5) & 0x07)
#define I2HandKnight(hand)     (((hand) >>  8) & 0x07)
#define I2HandSilver(hand)     (((hand) >> 11) & 0x07)
#define I2HandGold(hand)       (((hand) >> 14) & 0x07)
#define I2HandBishop(hand)     (((hand) >> 17) & 0x03)
#define I2HandRook(hand)        ((hand) >> 19)
#define IsHandPawn(hand)       ((hand) & 0x000001f)
#define IsHandLance(hand)      ((hand) & 0x00000e0)
#define IsHandKnight(hand)     ((hand) & 0x0000700)
#define IsHandSilver(hand)     ((hand) & 0x0003800)
#define IsHandGold(hand)       ((hand) & 0x001c000)
#define IsHandBishop(hand)     ((hand) & 0x0060000)
#define IsHandRook(hand)       ((hand) & 0x0180000)
/*
  xxxxxxxx xxxxxxxx x1111111  destination
  xxxxxxxx xx111111 1xxxxxxx  starting square or drop piece+nsquare-1
  xxxxxxxx x1xxxxxx xxxxxxxx  flag for promotion
  xxxxx111 1xxxxxxx xxxxxxxx  piece to move
  x1111xxx xxxxxxxx xxxxxxxx  captured piece
 */
#define To2Move(to)             ((unsigned int)(to)   <<  0)
#define From2Move(from)         ((unsigned int)(from) <<  7)
#define Drop2Move(piece)        ((nsquare-1+(piece))  <<  7)
#define Drop2From(piece)         (nsquare-1+(piece))
#define FLAG_PROMO               (1U                  << 14)
#define Piece2Move(piece)       ((piece)              << 15)
#define Cap2Move(piece)         ((piece)              << 19)
#define I2To(move)              (((move) >>  0) & 0x007fU)
#define I2From(move)            (((move) >>  7) & 0x007fU)
#define I2FromTo(move)          (((move) >>  0) & 0x3fffU)
#define I2IsPromote(move)       ((move) & FLAG_PROMO)
#define I2PieceMove(move)       (((move) >> 15) & 0x000fU)
#define UToFromToPromo(u)       ( (u) & 0x7ffffU )
#define UToCap(u)               (((u)     >> 19) & 0x000fU)
#define From2Drop(from)         ((from)-nsquare+1)


#define BBIni(bb)                (bb).p[0] = (bb).p[1] = (bb).p[2] = 0
#define BBToU(bb)                ((bb).p[0] | (bb).p[1] | (bb).p[2])
#define BBToUShift(bb)           ((bb).p[0]<<2 | (bb).p[1]<<1 | (bb).p[2])
#define PopuCount(bb)            popu_count012( bb.p[0], bb.p[1], bb.p[2] )
#define FirstOne(bb)             first_one012( bb.p[0], bb.p[1], bb.p[2] )
#define LastOne(bb)              last_one210( bb.p[2], bb.p[1], bb.p[0] )

#define BBCmp(bb1,bb2)           ( (bb1).p[0] != (bb2).p[0]                   \
				   || (bb1).p[1] != (bb2).p[1]                \
				   || (bb1).p[2] != (bb2).p[2] )

#define BBNot(bb,bb1)            (bb).p[0] = ~(bb1).p[0],                     \
                                 (bb).p[1] = ~(bb1).p[1],                     \
                                 (bb).p[2] = ~(bb1).p[2]

#define BBOr(bb,bb1,bb2)         (bb).p[0] = (bb1).p[0] | (bb2).p[0],         \
                                 (bb).p[1] = (bb1).p[1] | (bb2).p[1],         \
                                 (bb).p[2] = (bb1).p[2] | (bb2).p[2]

#define BBAnd(bb,bb1,bb2)        (bb).p[0] = (bb1).p[0] & (bb2).p[0],         \
                                 (bb).p[1] = (bb1).p[1] & (bb2).p[1],         \
                                 (bb).p[2] = (bb1).p[2] & (bb2).p[2]

#define BBXor(bb,b1,b2)          (bb).p[0] = (b1).p[0] ^ (b2).p[0],           \
                                 (bb).p[1] = (b1).p[1] ^ (b2).p[1],           \
                                 (bb).p[2] = (b1).p[2] ^ (b2).p[2]

#define BBAndOr(bb,bb1,bb2)      (bb).p[0] |= (bb1).p[0] & (bb2).p[0],        \
                                 (bb).p[1] |= (bb1).p[1] & (bb2).p[1],        \
                                 (bb).p[2] |= (bb1).p[2] & (bb2).p[2]

#define BBNotAnd(bb,bb1)         bb.p[0] &= ~bb1.p[0];                        \
                                 bb.p[1] &= ~bb1.p[1];                        \
                                 bb.p[2] &= ~bb1.p[2]

#define BBContractShift(bb1,bb2) ( ( (bb1).p[0] & (bb2).p[0] ) << 2           \
                                     | ( (bb1).p[1] & (bb2).p[1] ) << 1       \
                                     | ( (bb1).p[2] & (bb2).p[2] ) )

#define BBContract(bb1,bb2)      ( ( (bb1).p[0] & (bb2).p[0] )                \
                                     | ( (bb1).p[1] & (bb2).p[1] )            \
                                     | ( (bb1).p[2] & (bb2).p[2] ) )

#define Xor(i,bb)            (bb).p[0] ^= abb_mask[i].p[0],      \
                             (bb).p[1] ^= abb_mask[i].p[1],      \
                             (bb).p[2] ^= abb_mask[i].p[2]

#define XorFile(i,bb)        (bb).p[0] ^= abb_mask_rl90[i].p[0], \
                             (bb).p[1] ^= abb_mask_rl90[i].p[1], \
                             (bb).p[2] ^= abb_mask_rl90[i].p[2]

#define XorDiag1(i,bb)       (bb).p[0] ^= abb_mask_rr45[i].p[0], \
                             (bb).p[1] ^= abb_mask_rr45[i].p[1], \
                             (bb).p[2] ^= abb_mask_rr45[i].p[2]

#define XorDiag2(i,bb)       (bb).p[0] ^= abb_mask_rl45[i].p[0], \
                             (bb).p[1] ^= abb_mask_rl45[i].p[1], \
                             (bb).p[2] ^= abb_mask_rl45[i].p[2]

#define SetClear(bb)         (bb).p[0] ^= (bb_set_clear.p[0]), \
                             (bb).p[1] ^= (bb_set_clear.p[1]), \
                             (bb).p[2] ^= (bb_set_clear.p[2])

#define SetClearFile(i1,i2,bb) \
    (bb).p[0] ^= ((abb_mask_rl90[i1].p[0])|(abb_mask_rl90[i2].p[0])), \
    (bb).p[1] ^= ((abb_mask_rl90[i1].p[1])|(abb_mask_rl90[i2].p[1])), \
    (bb).p[2] ^= ((abb_mask_rl90[i1].p[2])|(abb_mask_rl90[i2].p[2]))

#define SetClearDiag1(i1,i2,bb) \
    (bb).p[0] ^= ((abb_mask_rr45[i1].p[0])|(abb_mask_rr45[i2].p[0])), \
    (bb).p[1] ^= ((abb_mask_rr45[i1].p[1])|(abb_mask_rr45[i2].p[1])), \
    (bb).p[2] ^= ((abb_mask_rr45[i1].p[2])|(abb_mask_rr45[i2].p[2]))

#define SetClearDiag2(i1,i2,bb) \
    (bb).p[0] ^= ((abb_mask_rl45[i1].p[0])|(abb_mask_rl45[i2].p[0])), \
    (bb).p[1] ^= ((abb_mask_rl45[i1].p[1])|(abb_mask_rl45[i2].p[1])), \
    (bb).p[2] ^= ((abb_mask_rl45[i1].p[2])|(abb_mask_rl45[i2].p[2]))

#define AttackFile(i)  (abb_file_attacks[i]                               \
                         [((ptree->posi.occupied_rl90.p[aslide[i].irl90]) \
                            >> aslide[i].srl90) & 0x7f])

#define AttackRank(i)  (ai_rook_attacks_r0[i]                             \
                         [((ptree->posi.b_occupied.p[aslide[i].ir0]       \
                            |ptree->posi.w_occupied.p[aslide[i].ir0])     \
                             >> aslide[i].sr0) & 0x7f ])

#define AttackDiag1(i)                                         \
          (abb_bishop_attacks_rr45[i]                        \
            [((ptree->posi.occupied_rr45.p[aslide[i].irr45]) \
               >> aslide[i].srr45) & 0x7f])

#define AttackDiag2(i)                                         \
          (abb_bishop_attacks_rl45[i]                        \
            [((ptree->posi.occupied_rl45.p[aslide[i].irl45]) \
               >> aslide[i].srl45) & 0x7f])

#define BishopAttack0(i) ( AttackDiag1(i).p[0] | AttackDiag2(i).p[0] )
#define BishopAttack1(i) ( AttackDiag1(i).p[1] | AttackDiag2(i).p[1] )
#define BishopAttack2(i) ( AttackDiag1(i).p[2] | AttackDiag2(i).p[2] )
#define AttackBLance(bb,i) BBAnd( bb, abb_minus_rays[i], AttackFile(i) )
#define AttackWLance(bb,i) BBAnd( bb, abb_plus_rays[i],  AttackFile(i) )
#define AttackHorse(bb,i)  AttackBishop(bb,i); BBOr(bb,bb,abb_king_attacks[i])
#define AttackDragon(bb,i) AttackRook(bb,i);   BBOr(bb,bb,abb_king_attacks[i])

#define InCheck(turn)                                        \
         ( (turn) ? is_white_attacked( ptree, SQ_WKING )     \
                  : is_black_attacked( ptree, SQ_BKING ) )

#define MakeMove(turn,move,ply)                                \
                ( (turn) ? make_move_w( ptree, move, ply ) \
                         : make_move_b( ptree, move, ply ) )

#define UnMakeMove(turn,move,ply)                                \
                ( (turn) ? unmake_move_w( ptree, move, ply ) \
                         : unmake_move_b( ptree, move, ply ) )

#define IsMoveCheck( ptree, turn, move )                        \
                ( (turn) ? is_move_check_w( ptree, move )   \
                         : is_move_check_b( ptree, move ) )

#define GenCaptures(turn,pmove) ( (turn) ? w_gen_captures( ptree, pmove )   \
                                         : b_gen_captures( ptree, pmove ) )

#define GenNoCaptures(turn,pmove)                                             \
                               ( (turn) ? w_gen_nocaptures( ptree, pmove )  \
                                        : b_gen_nocaptures( ptree, pmove ) )

#define GenDrop(turn,pmove)     ( (turn) ? w_gen_drop( ptree, pmove )       \
                                         : b_gen_drop( ptree, pmove ) )

#define GenCapNoProEx2(turn,pmove)                                 \
                ( (turn) ? w_gen_cap_nopro_ex2( ptree, pmove )   \
                         : b_gen_cap_nopro_ex2( ptree, pmove ) )

#define GenNoCapNoProEx2(turn,pmove)                                \
                ( (turn) ? w_gen_nocap_nopro_ex2( ptree, pmove )  \
                         : b_gen_nocap_nopro_ex2( ptree, pmove ) )

#define GenEvasion(turn,pmove)                                  \
                ( (turn) ? w_gen_evasion( ptree, pmove )      \
                         : b_gen_evasion( ptree, pmove ) )

#define GenCheck(turn,pmove)                                  \
                ( (turn) ? w_gen_checks( ptree, pmove )      \
                         : b_gen_checks( ptree, pmove ) )

#define IsMateIn1Ply(turn)                                    \
                ( (turn) ? is_w_mate_in_1ply( ptree )         \
                         : is_b_mate_in_1ply( ptree ) )

#define IsDiscoverBK(from,to)                                  \
          idirec = (int)adirec[SQ_BKING][from],               \
          ( idirec && ( idirec!=(int)adirec[SQ_BKING][to] )   \
            && is_pinned_on_black_king( ptree, from, idirec ) )

#define IsDiscoverWK(from,to)                                  \
          idirec = (int)adirec[SQ_WKING][from],               \
          ( idirec && ( idirec!=(int)adirec[SQ_WKING][to] )   \
            && is_pinned_on_white_king( ptree, from, idirec ) )
#define IsMateWPawnDrop(ptree,to) ( BOARD[(to)+9] == king                 \
                                     && is_mate_w_pawn_drop( ptree, to ) )

#define IsMateBPawnDrop(ptree,to) ( BOARD[(to)-9] == -king                \
                                     && is_mate_b_pawn_drop( ptree, to ) )

enum { b0000, b0001, b0010, b0011, b0100, b0101, b0110, b0111,
       b1000, b1001, b1010, b1011, b1100, b1101, b1110, b1111 };

enum { A9 = 0, B9, C9, D9, E9, F9, G9, H9, I9,
           A8, B8, C8, D8, E8, F8, G8, H8, I8,
           A7, B7, C7, D7, E7, F7, G7, H7, I7,
           A6, B6, C6, D6, E6, F6, G6, H6, I6,
           A5, B5, C5, D5, E5, F5, G5, H5, I5,
           A4, B4, C4, D4, E4, F4, G4, H4, I4,
           A3, B3, C3, D3, E3, F3, G3, H3, I3,
           A2, B2, C2, D2, E2, F2, G2, H2, I2,
           A1, B1, C1, D1, E1, F1, G1, H1, I1 };

enum { promote = 8, empty = 0,
       pawn, lance, knight, silver, gold, bishop, rook, king, pro_pawn,
       pro_lance, pro_knight, pro_silver, piece_null, horse, dragon };

enum { npawn_max = 18,  nlance_max  = 4,  nknight_max = 4,  nsilver_max = 4,
       ngold_max = 4,   nbishop_max = 2,  nrook_max   = 2,  nking_max   = 2 };

enum { rank1 = 0, rank2, rank3, rank4, rank5, rank6, rank7, rank8, rank9 };
enum { file1 = 0, file2, file3, file4, file5, file6, file7, file8, file9 };

enum { nhand = 7, nfile = 9,  nrank = 9,  nsquare = 81 };

enum { mask_file1 = (( 1U << 18 | 1U << 9 | 1U ) << 8) };

enum { flag_diag1 = b0001, flag_plus = b0010 };

enum { score_draw     =     1,
       score_max_eval = 30000,
       score_mate1ply = 32598,
       score_inferior = 32599,
       score_bound    = 32600,
       score_foul     = 32600 };

enum { phase_hash      = b0001,
       phase_killer1   = b0001 << 1,
       phase_killer2   = b0010 << 1,
       phase_killer    = b0011 << 1,
       phase_cap1      = b0001 << 3,
       phase_cap_misc  = b0010 << 3,
       phase_cap       = b0011 << 3,
       phase_history1  = b0001 << 5,
       phase_history2  = b0010 << 5,
       phase_history   = b0011 << 5,
       phase_misc      = b0100 << 5 };

enum { next_move_hash = 0,  next_move_capture,   next_move_history2,
       next_move_misc };

/* next_evasion_hash should be the same as next_move_hash */
enum { next_evasion_hash = 0, next_evasion_genall, next_evasion_misc };

enum { next_quies_gencap, next_quies_captures, next_quies_misc };

enum { no_rep = 0, four_fold_rep, perpetual_check, perpetual_check2,
       black_superi_rep, white_superi_rep, hash_hit, prev_solution, book_hit,
       pv_fail_high, mate_search };

enum { record_misc, record_eof, record_next, record_resign, record_drawn,
       record_error };

enum { black = 0, white = 1 };

enum { direc_misc           = b0000,
       direc_file           = b0010, /* | */
       direc_rank           = b0011, /* - */
       direc_diag1          = b0100, /* / */
       direc_diag2          = b0101, /* \ */
       flag_cross           = b0010,
       flag_diag            = b0100 };

enum { value_null           = b0000,
       value_upper          = b0001,
       value_lower          = b0010,
       value_exact          = b0011,
       flag_value_up_exact  = b0001,
       flag_value_low_exact = b0010,
       node_do_null         = b0100,
       node_do_recap        = b1000,
       node_do_mate         = b0001 << 4,
       node_mate_threat     = b0010 << 4, /* <- don't change it */
       node_do_futile       = b0100 << 4,
       state_node_end };
/* note: maximum bits are 8.  tlp_state_node uses type unsigned char. */

enum { flag_from_ponder     = b0001,
       flag_refer_rest      = b0010 };

enum { flag_time            = b0001,
       flag_history         = b0010,
       flag_rep             = b0100,
       flag_detect_hang     = b1000,
       flag_rejections      = b0001 << 4,
       flag_nomake_move     = b0010 << 4,
       flag_nofmargin       = b0100 << 4 };

/* flags represent status of root move */
enum { flag_searched        = b0001,
       flag_failhigh        = b0010,
       flag_faillow         = b0100,
       flag_first           = b1000 };

enum { flag_mated           = b0001,
       flag_resigned        = b0010,
       flag_drawn           = b0100,
       flag_suspend         = b1000,
       mask_game_end        = b1111,
       flag_quit            = b0001 << 4,
       flag_puzzling        = b0010 << 4,
       flag_pondering       = b0100 << 4,
       flag_thinking        = b1000 << 4,
       flag_problem         = b0001 << 8,
       flag_move_now        = b0010 << 8,
       flag_quit_ponder     = b0100 << 8,
       flag_search_error    = b0001 << 12,
       flag_quiet           = b0010 << 12,
       flag_reverse         = b0100 << 12,
       flag_narrow_book     = b1000 << 12,
       flag_time_extendable = b0001 << 16,
       flag_learning        = b0010 << 16,
       flag_nobeep          = b0100 << 16,
       flag_nostress        = b1000 << 16,
       flag_nopeek          = b0001 << 20,
       flag_noponder        = b0010 << 20 };


enum { flag_hand_pawn       = 1 <<  0,
       flag_hand_lance      = 1 <<  5,
       flag_hand_knight     = 1 <<  8,
       flag_hand_silver     = 1 << 11,
       flag_hand_gold       = 1 << 14,
       flag_hand_bishop     = 1 << 17,
       flag_hand_rook       = 1 << 19 };

enum { f_hand_pawn   =    0,
       e_hand_pawn   =   19,
       f_hand_lance  =   38,
       e_hand_lance  =   43,
       f_hand_knight =   48,
       e_hand_knight =   53,
       f_hand_silver =   58,
       e_hand_silver =   63,
       f_hand_gold   =   68,
       e_hand_gold   =   73,
       f_hand_bishop =   78,
       e_hand_bishop =   81,
       f_hand_rook   =   84,
       e_hand_rook   =   87,
       fe_hand_end   =   90,
       f_pawn        =   81,
       e_pawn        =  162,
       f_lance       =  225,
       e_lance       =  306,
       f_knight      =  360,
       e_knight      =  441,
       f_silver      =  504,
       e_silver      =  585,
       f_gold        =  666,
       e_gold        =  747,
       f_bishop      =  828,
       e_bishop      =  909,
       f_horse       =  990,
       e_horse       = 1071,
       f_rook        = 1152,
       e_rook        = 1233,
       f_dragon      = 1314,
       e_dragon      = 1395,
       fe_end        = 1476,

       kkp_hand_pawn   =   0,
       kkp_hand_lance  =  19,
       kkp_hand_knight =  24,
       kkp_hand_silver =  29,
       kkp_hand_gold   =  34,
       kkp_hand_bishop =  39,
       kkp_hand_rook   =  42,
       kkp_hand_end    =  45,
       kkp_pawn        =  36,
       kkp_lance       = 108,
       kkp_knight      = 171,
       kkp_silver      = 252,
       kkp_gold        = 333,
       kkp_bishop      = 414,
       kkp_horse       = 495,
       kkp_rook        = 576,
       kkp_dragon      = 657,
       kkp_end         = 738 };

enum { pos_n = fe_end * ( fe_end + 1 ) / 2 };

typedef struct { unsigned int p[3]; } bitboard_t;

typedef struct { bitboard_t gold, silver, knight, lance; } check_table_t;

#if defined(MINIMUM)
typedef const signed char cchar_f_t;
typedef signed char char_f_t;
#else
typedef short cchar_f_t;
typedef short char_f_t;
typedef struct { fpos_t fpos;  unsigned int games, moves, lines; } rpos_t;
typedef struct {
  double pawn, lance, knight, silver, gold, bishop, rook;
  double pro_pawn, pro_lance, pro_knight, pro_silver, horse, dragon;
  float pc_on_sq[nsquare][fe_end*(fe_end+1)/2];
  float kkp[nsquare][nsquare][kkp_end];
} param_t;
#endif

typedef enum { mode_write, mode_read_write, mode_read } record_mode_t;

typedef struct { uint64_t word1, word2; }                        trans_entry_t;
typedef struct { trans_entry_t prefer, always[2]; }              trans_table_t;
typedef struct { int count;  unsigned int cnst[2], vec[RAND_N]; }rand_work_t;

typedef struct {
  uint64_t root;
  SHARE uint64_t sibling;
} rejections_t;

typedef struct {
  int no1_value, no2_value;
  unsigned int no1, no2;
} move_killer_t;

typedef struct {
  union { char str_move[ MAX_ANSWER ][ 8 ]; } info;
  char str_name1[ SIZE_PLAYERNAME ];
  char str_name2[ SIZE_PLAYERNAME ];
  char* buf;
  char* write_ptr;
  int buf_size;
  //  FILE *pf;
  unsigned int games, moves, lines;
} record_t;

typedef struct {
  unsigned int a[PLY_MAX];
  unsigned char type;
  unsigned char length;
  unsigned char depth;
} pv_t;

typedef struct {
  unsigned char ir0,   sr0;
  unsigned char irl90, srl90;
  unsigned char irl45, srl45;
  unsigned char irr45, srr45;
} slide_tbl_t;


typedef struct {
  uint64_t hash_key;
  bitboard_t b_occupied,     w_occupied;
  bitboard_t occupied_rl90,  occupied_rl45, occupied_rr45;
  bitboard_t b_hdk,          w_hdk;
  bitboard_t b_tgold,        w_tgold;
  bitboard_t b_bh,           w_bh;
  bitboard_t b_rd,           w_rd;
  bitboard_t b_pawn_attacks, w_pawn_attacks;
  bitboard_t b_lance,        w_lance;
  bitboard_t b_knight,       w_knight;
  bitboard_t b_silver,       w_silver;
  bitboard_t b_bishop,       w_bishop;
  bitboard_t b_rook,         w_rook;
  bitboard_t b_horse,        w_horse;
  bitboard_t b_dragon,       w_dragon;
  bitboard_t b_pawn,         w_pawn;
  bitboard_t b_gold,         w_gold;
  bitboard_t b_pro_pawn,     w_pro_pawn;
  bitboard_t b_pro_lance,    w_pro_lance;
  bitboard_t b_pro_knight,   w_pro_knight;
  bitboard_t b_pro_silver,   w_pro_silver;
  unsigned int hand_black, hand_white;
  int material;
  signed char asquare[nsquare];
  unsigned char isquare_b_king, isquare_w_king;
} posi_t;


typedef struct {
  unsigned int hand_black, hand_white;
  char turn_to_move;
  signed char asquare[nsquare];
} min_posi_t;

typedef struct {
  uint64_t nodes;
  unsigned int move, status;
} root_move_t;

typedef struct {
  unsigned int *move_last;
  unsigned int move_cap1;
  unsigned int move_cap2;
  int phase_done, next_phase, remaining, value_cap1, value_cap2;
} next_move_t;

/* data: 31  1bit flag_learned */
/*       30  1bit is_flip      */
/*       15 16bit value        */
typedef struct {
  uint64_t key_book;
  unsigned int key_responsible, key_probed, key_played;
  unsigned int hand_responsible, hand_probed, hand_played;
  unsigned int move_played, move_responsible, move_probed, data;
} history_book_learn_t;

typedef struct tree tree_t;
struct tree {
  posi_t posi;
  uint64_t rep_board_list[ REP_HIST_LEN ];
  uint64_t node_searched;
  unsigned int *move_last[ PLY_MAX ];
  next_move_t anext_move[ PLY_MAX ];
  pv_t pv[ PLY_MAX ];
  move_killer_t amove_killer[ PLY_MAX ];
  unsigned int null_pruning_done;
  unsigned int null_pruning_tried;
  unsigned int check_extension_done;
  unsigned int recap_extension_done;
  unsigned int onerp_extension_done;
  unsigned int neval_called;
  unsigned int nquies_called;
  unsigned int nfour_fold_rep;
  unsigned int nperpetual_check;
  unsigned int nsuperior_rep;
  unsigned int nrep_tried;
  unsigned int nreject_tried;
  unsigned int nreject_done;
  unsigned int ntrans_always_hit;
  unsigned int ntrans_prefer_hit;
  unsigned int ntrans_probe;
  unsigned int ntrans_exact;
  unsigned int ntrans_lower;
  unsigned int ntrans_upper;
  unsigned int ntrans_superior_hit;
  unsigned int ntrans_inferior_hit;
  unsigned int fail_high;
  unsigned int fail_high_first;
  unsigned int rep_hand_list[ REP_HIST_LEN ];
  unsigned int amove_hash[ PLY_MAX ];
  unsigned int amove[ MOVE_LIST_LEN ];
  unsigned int history[2][nsquare][nsquare+7];
  unsigned int current_move[ PLY_MAX ];
  int sort_value[ MAX_LEGAL_MOVES ];
  short save_material[ PLY_MAX ];
  short stand_pat[ PLY_MAX+1 ];
  unsigned char nsuc_check[ PLY_MAX+1 ];
#if defined(TLP)
  struct tree *tlp_ptrees_sibling[ TLP_MAX_THREADS ];
  struct tree *tlp_ptree_parent;
  lock_t tlp_lock;
  volatile int tlp_abort;
  volatile int tlp_used;
  unsigned short tlp_slot;
  short tlp_alpha;
  short tlp_beta;
  short tlp_value;
  short tlp_best;
  volatile unsigned char tlp_nsibling;
  unsigned char tlp_depth;
  unsigned char tlp_state_node;
  unsigned char tlp_id;
  char tlp_turn;
  char tlp_ply;
#endif
};


extern SHARE unsigned int game_status;
extern history_book_learn_t history_book_learn[ HASH_REG_HIST_LEN ];

extern int npawn_box;
extern int nlance_box;
extern int nknight_box;
extern int nsilver_box;
extern int ngold_box;
extern int nbishop_box;
extern int nrook_box;

extern unsigned int ponder_move_list[ MAX_LEGAL_MOVES ];
extern unsigned int ponder_move;
extern int ponder_nmove;

extern root_move_t root_move_list[ MAX_LEGAL_MOVES ];
extern SHARE int root_abort;
extern int root_nrep;
extern int root_nmove;
extern int root_value;
extern int root_alpha;
extern int root_beta;
extern int root_turn;
extern int root_move_cap;
extern int root_nfail_high;
extern int root_nfail_low;
extern int resign_threshold;
extern int n_nobook_move;

extern uint64_t node_limit;
extern unsigned int node_per_second;
extern unsigned int node_next_signal;
extern unsigned int node_last_check;

extern unsigned int hash_mask;
extern int trans_table_age;

extern pv_t last_pv;
extern pv_t last_pv_save;
extern int last_root_value;
extern int last_root_value_save;

extern SHARE trans_table_t *ptrans_table;
extern trans_table_t *ptrans_table_orig;
extern int log2_ntrans_table;

extern int depth_limit;

extern unsigned int time_last_result;
extern unsigned int time_last_eff_search;
extern unsigned int time_last_search;
extern unsigned int time_last_check;
extern unsigned int time_turn_start;
extern unsigned int time_start;
extern unsigned int time_max_limit;
extern unsigned int time_limit;
extern unsigned int time_response;
extern unsigned int sec_limit;
extern unsigned int sec_limit_up;
extern unsigned int sec_limit_depth;
extern unsigned int sec_elapsed;
extern unsigned int sec_b_total;
extern unsigned int sec_w_total;

extern record_t record_problems;
extern record_t record_game;
extern FILE *pf_book;
extern FILE *pf_hash;
extern int irecord_game;

extern short p_value[31];

typedef struct {
  uint64_t ehash_tbl[ EHASH_MASK + 1 ];
  unsigned char hash_rejections_parent[ REJEC_MASK+1 ];
  rejections_t hash_rejections[ REJEC_MASK+1 ];
} large_object_t;

short (*p_pc_on_sq)[nsquare][fe_end*(fe_end+1)/2];
short (*p_kkp)[nsquare][nsquare][kkp_end];

extern large_object_t* large_object;
extern rand_work_t rand_work;
extern slide_tbl_t aslide[ nsquare ];
extern bitboard_t abb_b_knight_attacks[ nsquare ];
extern bitboard_t abb_b_silver_attacks[ nsquare ];
extern bitboard_t abb_b_gold_attacks[ nsquare ];
extern bitboard_t abb_w_knight_attacks[ nsquare ];
extern bitboard_t abb_w_silver_attacks[ nsquare ];
extern bitboard_t abb_w_gold_attacks[ nsquare ];
extern bitboard_t abb_king_attacks[ nsquare ];
extern bitboard_t abb_obstacle[ nsquare ][ nsquare ];
extern bitboard_t abb_bishop_attacks_rl45[ nsquare ][ 128 ];
extern bitboard_t abb_bishop_attacks_rr45[ nsquare ][ 128 ];
extern bitboard_t abb_file_attacks[ nsquare ][ 128 ];
extern bitboard_t abb_mask[ nsquare ];
extern bitboard_t abb_mask_rl90[ nsquare ];
extern bitboard_t abb_mask_rl45[ nsquare ];
extern bitboard_t abb_mask_rr45[ nsquare ];
extern bitboard_t abb_plus_rays[ nsquare ];
extern bitboard_t abb_minus_rays[ nsquare ];
extern uint64_t b_pawn_rand[ nsquare ];
extern uint64_t b_lance_rand[ nsquare ];
extern uint64_t b_knight_rand[ nsquare ];
extern uint64_t b_silver_rand[ nsquare ];
extern uint64_t b_gold_rand[ nsquare ];
extern uint64_t b_bishop_rand[ nsquare ];
extern uint64_t b_rook_rand[ nsquare ];
extern uint64_t b_king_rand[ nsquare ];
extern uint64_t b_pro_pawn_rand[ nsquare ];
extern uint64_t b_pro_lance_rand[ nsquare ];
extern uint64_t b_pro_knight_rand[ nsquare ];
extern uint64_t b_pro_silver_rand[ nsquare ];
extern uint64_t b_horse_rand[ nsquare ];
extern uint64_t b_dragon_rand[ nsquare ];
extern uint64_t b_hand_pawn_rand[ npawn_max ];
extern uint64_t b_hand_lance_rand[ nlance_max ];
extern uint64_t b_hand_knight_rand[ nknight_max ];
extern uint64_t b_hand_silver_rand[ nsilver_max ];
extern uint64_t b_hand_gold_rand[ ngold_max ];
extern uint64_t b_hand_bishop_rand[ nbishop_max ];
extern uint64_t b_hand_rook_rand[ nrook_max ];
extern uint64_t w_pawn_rand[ nsquare ];
extern uint64_t w_lance_rand[ nsquare ];
extern uint64_t w_knight_rand[ nsquare ];
extern uint64_t w_silver_rand[ nsquare ];
extern uint64_t w_gold_rand[ nsquare ];
extern uint64_t w_bishop_rand[ nsquare ];
extern uint64_t w_rook_rand[ nsquare ];
extern uint64_t w_king_rand[ nsquare ];
extern uint64_t w_pro_pawn_rand[ nsquare ];
extern uint64_t w_pro_lance_rand[ nsquare ];
extern uint64_t w_pro_knight_rand[ nsquare ];
extern uint64_t w_pro_silver_rand[ nsquare ];
extern uint64_t w_horse_rand[ nsquare ];
extern uint64_t w_dragon_rand[ nsquare ];
extern uint64_t w_hand_pawn_rand[ npawn_max ];
extern uint64_t w_hand_lance_rand[ nlance_max ];
extern uint64_t w_hand_knight_rand[ nknight_max ];
extern uint64_t w_hand_silver_rand[ nsilver_max ];
extern uint64_t w_hand_gold_rand[ ngold_max ];
extern uint64_t w_hand_bishop_rand[ nbishop_max ];
extern uint64_t w_hand_rook_rand[ nrook_max ];
extern unsigned int ai_rook_attacks_r0[ nsquare ][ 128 ];
extern unsigned int move_evasion_pchk;
extern int p_value_ex[31];
extern int benefit2promo[15];
extern int easy_abs;
extern int easy_min;
extern int easy_max;
extern int easy_value;
extern SHARE int fmg_misc;
extern SHARE int fmg_cap;
extern SHARE int fmg_drop;
extern SHARE int fmg_mt;
extern SHARE int fmg_misc_king;
extern SHARE int fmg_cap_king;
extern int iteration_depth;
extern unsigned char book_section[ MAX_SIZE_SECTION+1 ];
extern unsigned char adirec[nsquare][nsquare];
extern unsigned char is_same[16][16];
extern char str_message[ SIZE_MESSAGE ];
extern char str_cmdline[ SIZE_CMDLINE ];
extern char str_buffer_cmdline[ SIZE_CMDBUFFER ];
extern const char *str_error;

extern const char *astr_table_piece[ 16 ];
extern const char *str_resign;
extern const char *str_repetition;
extern const char *str_jishogi;
extern const char *str_record_error;
extern const char *str_unexpect_eof;
extern const char *str_ovrflw_line;
extern const char *str_warning;
extern const char *str_on;
extern const char *str_off;
extern const char *str_book;
extern const char *str_hash;
extern const char *str_fv;
extern const char *str_book_error;
extern const char *str_perpet_check;
extern const char *str_bad_cmdline;
extern const char *str_busy_think;
extern const char *str_bad_record;
extern const char *str_bad_board;
extern const char *str_delimiters;
extern const char *str_fmt_line;
extern const char *str_illegal_move;
extern const char *str_double_pawn;
extern const char *str_mate_drppawn;
extern const char *str_fopen_error;
extern const char *str_game_ended;
// extern const char *str_io_error;
extern const char *str_spaces;
extern const char *str_king_hang;
#if defined(CSA_LAN)
extern const char *str_server_err;
#endif
extern const char *str_myname;
extern const char *str_version;
extern const min_posi_t min_posi_no_handicap;
extern const short aipos[31];
extern const char ach_turn[2];
extern const char ashell_h[ SHELL_H_LEN ];
extern const unsigned char aifile[ nsquare ];
extern const unsigned char airank[ nsquare ];

void pv_close( tree_t * restrict ptree, int ply, int type );
void pv_copy( tree_t * restrict ptree, int ply );
void set_derivative_param( void );
void set_search_limit_time( int turn );
void ehash_clear( void );
void hash_store_pv( const tree_t * restrict ptree, unsigned int move,
		    int turn );
void check_futile_score_quies( const tree_t * restrict ptree,
			       unsigned int move, int old_val, int new_val,
			       int turn );
void out_file( FILE *pf, const char *format, ... );
void out_warning( const char *format, ... );
void out_error( const char *format, ... );
void show_prompt( void );
void make_move_w( tree_t * restrict ptree, unsigned int move, int ply );
void make_move_b( tree_t * restrict ptree, unsigned int move, int ply );
void unmake_move_b( tree_t * restrict ptree, unsigned int move, int ply );
void unmake_move_w( tree_t * restrict ptree, unsigned int move, int ply );
void ini_rand( unsigned int s );
void out_CSA( tree_t * restrict ptree, record_t *pr, unsigned int move );
void out_pv( tree_t * restrict ptree, int value, int turn, unsigned int time );
void hash_store( const tree_t * restrict ptree, int ply, int depth, int turn,
		 int value_type, int value, unsigned int move,
		 unsigned int state_node );
void *memory_alloc( size_t nbytes );
void unmake_move_root( tree_t * restrict ptree, unsigned int move );
void add_rejections_root( tree_t * restrict ptree, unsigned int move_made );
void sub_rejections_root( tree_t * restrict ptree, unsigned int move_made );
void add_rejections( tree_t * restrict ptree, int turn, int ply );
void sub_rejections( tree_t * restrict ptree, int turn, int ply );
void adjust_time( unsigned int elapsed_new, int turn );
int popu_count012( unsigned int u0, unsigned int u1, unsigned int u2 );
int first_one012( unsigned int u0, unsigned int u1, unsigned int u2 );
int last_one210( unsigned int u2, unsigned int u1, unsigned int u0 );
int first_one01( unsigned int u0, unsigned int u1 );
int first_one12( unsigned int u1, unsigned int u2 );
int last_one01( unsigned int u0, unsigned int u1 );
int last_one12( unsigned int u1, unsigned int u2 );
int first_one1( unsigned int u1 );
int first_one2( unsigned int u2 );
int last_one0( unsigned int u0 );
int last_one1( unsigned int u1 );
int memory_free( void *p );
int reset_time( unsigned int b_remain, unsigned int w_remain );
int all_hash_learn_store( void );
int gen_legal_moves( tree_t * restrict ptree, unsigned int *p0 );
int rejections_probe( tree_t * restrict ptree, int turn, int ply );
int ini( tree_t * restrict ptree );
int fin( void );
int ponder( tree_t * restrict ptree );
int hash_learn( const tree_t * restrict ptree, unsigned int move, int value,
		int depth );
int book_on( void );
int book_off( void );
int hash_learn_on( void );
int hash_learn_off( void );
int is_move_check_b( const tree_t * restrict ptree, unsigned int move );
int is_move_check_w( const tree_t * restrict ptree, unsigned int move );
int solve_problems( tree_t * restrict ptree, unsigned int nposition );
int read_board_rep1( const char *str_line, min_posi_t *pmin_posi );
int com_turn_start( tree_t * restrict ptree, int flag );
int read_record( tree_t * restrict ptree, const char *str_file,
		 unsigned int moves, int flag );
int out_board( const tree_t * restrict ptree, FILE *pf, unsigned int move,
	       int flag );
int make_root_move_list( tree_t * restrict ptree, int flag );
int record_wind( record_t *pr );
int book_probe( tree_t * restrict ptree );
int detect_repetition( tree_t * restrict ptree, int ply, int turn, int nth );
int is_mate( tree_t * restrict ptree, int ply );
int is_mate_w_pawn_drop( tree_t * restrict ptree, int sq_drop );
int is_mate_b_pawn_drop( tree_t * restrict ptree, int sq_drop );
int clear_trans_table( void );
int eval_max_score( const tree_t * restrict ptree, unsigned int move,
		    int stand_pat, int turn, int diff );
int estimate_score_diff( const tree_t * restrict ptree, unsigned int move,
			 int turn );
int eval_material( const tree_t * restrict ptree );
int ini_trans_table( void );
int is_hand_eq_supe( unsigned int u, unsigned int uref );
int is_move_valid( tree_t * restrict ptree, unsigned int move, int turn );
int is_hash_move_valid( tree_t * restrict ptree, unsigned int move, int turn );
int iterate( tree_t * restrict ptree, int flag );
int gen_next_move( tree_t * restrict ptree, int ply, int turn );
int gen_next_evasion( tree_t * restrict ptree, int ply, int turn );
int ini_game( tree_t * restrict ptree, const min_posi_t *pmin_posi, int flag,
	      const char *str_name1, const char *str_name2 );
int open_history( const char *str_name1, const char *str_name2 );
int next_cmdline( int is_wait );
int procedure( tree_t * restrict ptree );
int get_cputime( unsigned int *ptime );
int get_elapsed( unsigned int *ptime );
int interpret_CSA_move( tree_t * restrict ptree, unsigned int *pmove,
			const char *str );
int in_CSA( tree_t * restrict ptree, record_t *pr, unsigned int *pmove,
	    int do_history );
int in_CSA_record( FILE * restrict pf, tree_t * restrict ptree );
int renovate_time( int turn );
int exam_tree( const tree_t * restrict ptree );
int rep_check_root( tree_t * restrict ptree );
int make_move_root( tree_t * restrict ptree, unsigned int move, int flag );
int search_quies( tree_t * restrict ptree, int alpha, int beta, int turn,
		  int ply, int qui_ply );
int search( tree_t * restrict ptree, int alpha, int beta, int turn,
	    int depth, int ply, unsigned int state_node );
int searchr( tree_t * restrict ptree, int alpha, int beta, int turn,
	     int depth );
int evaluate( tree_t * restrict ptree, int ply, int turn );
int swap( const tree_t * restrict ptree, unsigned int move, int alpha,
	  int beta, int turn );
int file_close( FILE *pf );
int record_open( record_t *pr, const char *str_file,
		 record_mode_t record_mode, const char *str_name1,
		 const char *str_name2 );
int record_close( record_t *pr );
unsigned int is_mate_in3ply( tree_t * restrict ptree, int turn, int ply );
unsigned int is_b_mate_in_1ply( tree_t * restrict ptree );
unsigned int is_w_mate_in_1ply( tree_t * restrict ptree );
unsigned int hash_probe( tree_t * restrict ptree, int ply, int depth, int turn,
			 int alpha, int beta, unsigned int state_node );
unsigned int rand32( void );
unsigned int is_black_attacked( const tree_t * restrict ptree, int sq );
unsigned int is_white_attacked( const tree_t * restrict ptree, int sq );
unsigned int is_pinned_on_black_king( const tree_t * restrict ptree,
				     int isquare, int idirec );
unsigned int is_pinned_on_white_king( const tree_t * restrict ptree,
				     int isquare, int idirec );
unsigned int *b_gen_captures( const tree_t * restrict ptree,
			      unsigned int * restrict pmove );
unsigned int *b_gen_nocaptures( const tree_t * restrict ptree,
				unsigned int * restrict pmove );
unsigned int *b_gen_drop( tree_t * restrict ptree,
			  unsigned int * restrict pmove );
unsigned int *b_gen_evasion( tree_t *restrict ptree,
			     unsigned int * restrict pmove );
unsigned int *b_gen_checks( tree_t * restrict __ptree__,
			    unsigned int * restrict pmove );
unsigned int *b_gen_cap_nopro_ex2( const tree_t * restrict ptree,
				   unsigned int * restrict pmove );
unsigned int *b_gen_nocap_nopro_ex2( const tree_t * restrict ptree,
				     unsigned int * restrict pmove );
unsigned int *w_gen_captures( const tree_t * restrict ptree,
			      unsigned int * restrict pmove );
unsigned int *w_gen_nocaptures( const tree_t * restrict ptree,
				unsigned int * restrict pmove );
unsigned int *w_gen_drop( tree_t * restrict ptree,
			  unsigned int * restrict pmove );
unsigned int *w_gen_evasion( tree_t * restrict ptree,
			     unsigned int * restrict pmove );
unsigned int *w_gen_checks( tree_t * restrict __ptree__,
			    unsigned int * restrict pmove );
unsigned int *w_gen_cap_nopro_ex2( const tree_t * restrict ptree,
				   unsigned int * restrict pmove );
unsigned int *w_gen_nocap_nopro_ex2( const tree_t * restrict ptree,
				     unsigned int * restrict pmove );
uint64_t hash_func( const tree_t * restrict ptree );
uint64_t rand64( void );
trans_entry_t hash_learn_store( const tree_t * restrict ptree, int depth,
				  int value, unsigned int move );
FILE *file_open( const char *str_file, const char *str_mode );
bitboard_t attacks_to_piece( const tree_t * restrict ptree, int sq );
bitboard_t horse_attacks( const tree_t * restrict ptree, int i );
const char *str_time( unsigned int time );
const char *str_time_symple( unsigned int time );
const char *str_CSA_move( unsigned int move );
const char *str_CSA_move_plus( tree_t * restrict ptree, unsigned int move,
			       int ply, int turn );

#if defined(MPV)
int root_mpv;
int mpv_num;
int mpv_width;
pv_t mpv_pv[ MPV_MAX_PV*2 + 1 ];
#endif

#if defined(TLP)
#  define SignKey(word2, word1) word2 ^= ( word1 )
#  define TlpEnd()              tlp_end();
#  if ! defined(_WIN32)
extern pthread_attr_t pthread_attr;
#  endif
void tlp_yield( void );
void tlp_set_abort( tree_t * restrict ptree );
void lock( lock_t *plock );
void unlock( lock_t *plock );
void tlp_end( void );
int tlp_search( tree_t * restrict ptree, int alpha, int beta, int turn,
		int depth, int ply, unsigned int state_node );
int tlp_split( tree_t * restrict ptree );
int tlp_start( void );
int tlp_is_descendant( const tree_t * restrict ptree, int slot_ancestor );
int lock_init( lock_t *plock );
int lock_free( lock_t *plock );
extern lock_t tlp_lock;
extern lock_t tlp_lock_io;
extern lock_t tlp_lock_root;
extern volatile int tlp_abort;
extern volatile int tlp_idle;
extern volatile int tlp_num;
extern int tlp_max;
extern int tlp_nsplit;
extern int tlp_nabort;
extern int tlp_nslot;
extern SHARE unsigned short tlp_rejections_slot[ REJEC_MASK+1 ];
extern tree_t tlp_atree_work[ TLP_NUM_WORK ];
extern tree_t * volatile tlp_ptrees[ TLP_MAX_THREADS ];
#else /* no TLP */
#  define SignKey(word2, word1)
#  define TlpEnd()
extern tree_t tree;
#endif

#if ! defined(_WIN32)
extern clock_t clk_tck;
#endif

#if ! defined(NDEBUG)
int exam_bb( const tree_t *ptree );
#endif

#if ! ( defined(NO_STDOUT) && defined(NO_LOGGING) )
#  define Out( ... ) out( __VA_ARGS__ )
void out( const char *format, ... );
#else
#  define Out( ... )
#endif

#if ! defined(NO_LOGGING)
extern FILE *pf_log;
extern const char *str_dir_logs;
#endif

#if defined(NO_STDOUT) || defined(WIN32_PIPE)
#  define OutBeep()
#  define StdoutStress(x,y) 1
#  define StdoutNormal()    1
#else
#  define OutBeep()         out_beep()
#  define StdoutStress(x,y) stdout_stress(x,y)
#  define StdoutNormal()    stdout_normal()
void out_beep( void );
int stdout_stress( int is_promote, int ifrom );
int stdout_normal( void );
#endif

#if defined(CSA_LAN)
#  define ShutdownClient sckt_shutdown( sckt_csa );  sckt_csa = SCKT_NULL
int client_next_game( tree_t * restrict ptree );
sckt_t sckt_connect( const char *str_addr, int iport );
int sckt_shutdown( sckt_t sd );
int sckt_check( sckt_t sd );
int sckt_in( sckt_t sd, char *str, int n );
int sckt_out( sckt_t sd, const char *fmt, ... );
extern int client_turn;
extern int client_ngame;
extern int client_max_game;
extern unsigned int time_last_send;
extern long client_port;
extern char client_str_addr[256];
extern char client_str_id[256];
extern char client_str_pwd[256];
extern sckt_t sckt_csa;
#else
#  define ShutdownClient
#endif

#if defined(DEKUNOBOU) || defined(CSA_LAN)
const char *str_WSAError( const char *str );
#endif

#if defined(DEKUNOBOU) && defined(_WIN32)
#  define OutDek( ... ) if ( dek_ngame ) { int i = dek_out( __VA_ARGS__ ); \
                                         if ( i < 0 ) { return i; } }
const char *str_WSAError( const char *str );
int dek_start( const char *str_sddr, int port_dek, int port_bnz );
int dek_next_game( tree_t * restrict ptree );
int dek_in( char *str, int n );
int dek_out( const char *format, ... );
int dek_parse( char *str, int len );
int dek_check( void );
extern SOCKET dek_socket_in;
extern SOCKET dek_s_accept;
extern u_long dek_ul_addr;
extern unsigned int dek_ngame;
extern unsigned int dek_lost;
extern unsigned int dek_win;
extern int dek_turn;
extern u_short dek_ns;
#else
#  define OutDek( ... )
#endif

#if defined(CSASHOGI)
#  define OutCsaShogi( ... ) out_csashogi( __VA_ARGS__ )
void out_csashogi( const char *format, ... );
#else
#  define OutCsaShogi( ... )
#endif

#define AttackBishop(bb,i)   BBOr( bb, AttackDiag1(i), AttackDiag2(i) )
#define AttackRook(bb,i)     (bb) = AttackFile(i);                  \
                             (bb).p[aslide[i].ir0] |= AttackRank(i)



extern check_table_t b_chk_tbl[nsquare];
extern check_table_t w_chk_tbl[nsquare];

#if defined(DBG_EASY)
extern unsigned int easy_move;
#endif

#if defined(MINIMUM)

#  define MT_CAP_PAWN       ( DPawn      + DPawn )
#  define MT_CAP_LANCE      ( DLance     + DLance )
#  define MT_CAP_KNIGHT     ( DKnight    + DKnight )
#  define MT_CAP_SILVER     ( DSilver    + DSilver )
#  define MT_CAP_GOLD       ( DGold      + DGold )
#  define MT_CAP_BISHOP     ( DBishop    + DBishop )
#  define MT_CAP_ROOK       ( DRook      + DRook )
#  define MT_CAP_PRO_PAWN   ( DProPawn   + DPawn )
#  define MT_CAP_PRO_LANCE  ( DProLance  + DLance )
#  define MT_CAP_PRO_KNIGHT ( DProKnight + DKnight )
#  define MT_CAP_PRO_SILVER ( DProSilver + DSilver )
#  define MT_CAP_HORSE      ( DHorse     + DBishop )
#  define MT_CAP_DRAGON     ( DDragon    + DRook )
#  define MT_CAP_KING       ( DKing      + DKing )
#  define MT_PRO_PAWN       ( DProPawn   - DPawn )
#  define MT_PRO_LANCE      ( DProLance  - DLance )
#  define MT_PRO_KNIGHT     ( DProKnight - DKnight )
#  define MT_PRO_SILVER     ( DProSilver - DSilver )
#  define MT_PRO_BISHOP     ( DHorse     - DBishop )
#  define MT_PRO_ROOK       ( DDragon    - DRook )

#else

#  define MT_CAP_PAWN       ( p_value_ex[ 15 + pawn ] )
#  define MT_CAP_LANCE      ( p_value_ex[ 15 + lance ] )
#  define MT_CAP_KNIGHT     ( p_value_ex[ 15 + knight ] )
#  define MT_CAP_SILVER     ( p_value_ex[ 15 + silver ] )
#  define MT_CAP_GOLD       ( p_value_ex[ 15 + gold ] )
#  define MT_CAP_BISHOP     ( p_value_ex[ 15 + bishop ] )
#  define MT_CAP_ROOK       ( p_value_ex[ 15 + rook ] )
#  define MT_CAP_PRO_PAWN   ( p_value_ex[ 15 + pro_pawn ] )
#  define MT_CAP_PRO_LANCE  ( p_value_ex[ 15 + pro_lance ] )
#  define MT_CAP_PRO_KNIGHT ( p_value_ex[ 15 + pro_knight ] )
#  define MT_CAP_PRO_SILVER ( p_value_ex[ 15 + pro_silver ] )
#  define MT_CAP_HORSE      ( p_value_ex[ 15 + horse ] )
#  define MT_CAP_DRAGON     ( p_value_ex[ 15 + dragon ] )
#  define MT_CAP_KING       ( DKing + DKing )
#  define MT_PRO_PAWN       ( benefit2promo[ 7 + pawn ] )
#  define MT_PRO_LANCE      ( benefit2promo[ 7 + lance ] )
#  define MT_PRO_KNIGHT     ( benefit2promo[ 7 + knight ] )
#  define MT_PRO_SILVER     ( benefit2promo[ 7 + silver ] )
#  define MT_PRO_BISHOP     ( benefit2promo[ 7 + bishop ] )
#  define MT_PRO_ROOK       ( benefit2promo[ 7 + rook ] )

void fill_param_zero( void );
void ini_param( param_t *p );
void add_param( param_t *p1, const param_t *p2 );
void inc_param( const tree_t * restrict ptree, param_t * restrict pd,
		double dinc );
void param_sym( param_t *p );
void renovate_param( const param_t *pd );
int learn( tree_t * restrict ptree, int is_ini, int nsteps,
	   unsigned int max_games, int max_iterations,
	   int nworker1, int nworker2 );
int record_setpos( record_t *pr, const rpos_t *prpos );
int record_getpos( record_t *pr, rpos_t *prpos );
int record_rewind( record_t *pr );
int book_create( tree_t * restrict ptree );
int hash_learn_create( void );
int out_param( void );
double calc_penalty( void );

#endif /* no MINIMUM */

#if ( REP_HIST_LEN - PLY_MAX ) < 1
#  error "REP_HIST_LEN - PLY_MAX is too small."
#endif

#if defined(CSA_LAN) && '\n' != 0x0a
#  error "'\n' is not the ASCII code of LF (0x0a)."
#endif

#endif /* SHOGI_H */
