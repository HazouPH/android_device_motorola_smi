#include "viddec_pm_parse.h"
#include "viddec_fw_debug.h"

#define FIRST_STARTCODE_BYTE        0x00
#define SECOND_STARTCODE_BYTE       0x00
#define THIRD_STARTCODE_BYTE        0x01

/* BIG ENDIAN: Must be the second and fourth byte of the bytestream for this to work */
/* LITTLE ENDIAN: Must be the second and fourth byte of the bytestream for this to work */
/* these are little-endian defines */
#define SC_BYTE_MASK0               0x00ff0000  /* little-endian */
#define SC_BYTE_MASK1               0x000000ff  /* little-endian */

// This is the 2.25 clocks per byte loop
#define USE_2p25_CLOCK_PER_BYTE_LOOP

#ifdef USE_2p25_CLOCK_PER_BYTE_LOOP
static int parser_find_next_startcode(
    const unsigned char        *buf,
    int                         i,
    int                         len,
    unsigned int               *pphase )
{
    int            sc_pos = -1;
    int            in_slow_loop;
    register unsigned int   scphase;

    scphase = *pphase;

    in_slow_loop = 1;
    if (  (0 == (0x3 & i)) &&  /* dword aligned */
            (0 == scphase) &&    /* no "potential" SC detected */
            ((len - i) >= 4) )   /* more than four bytes left */
    {
        in_slow_loop = 0; /* go to fast loop */
    }

    while ( i < len )
    {
        if ( in_slow_loop )
        {
            /* ------- slow SC Detect Loop, used when 0 detected in stream --------*/
sc_detect_slow_loop:

            while ( i < len )
            {
                unsigned char  ch;

                ch = buf[i];

                /* searching for a zero, ignore phase for now */
                if ( FIRST_STARTCODE_BYTE == ch )
                {
                    /* if we've already got two zeros, hold at phase == 2 */
                    if ( scphase < 2 )
                    {
                        scphase++;
                    }
                    else if ( scphase > 2 )
                    {
                        /* RARE Valid Condition, SC == 00 00 01 00 */
                        /* if we've already got two zeros hold at phase == 2
                         * we also enter here of we're at phase 3
                         * meaning we've got 00 00 01 00 which is a valid SC
                         */
                        /* 00 00 01 00 */
                        sc_pos = i;
                        *pphase = scphase;
                        return(sc_pos);
                    }
                    else  /* implies scphase == 2, holding receiving 0's */
                    {
                    }
                }
                else if ( THIRD_STARTCODE_BYTE == ch )
                {
                    if ( 2 == scphase )
                    {
                        /* next byte is the SC */
                        scphase++;
                    }
                    else if ( scphase < 2 )
                    {
                        scphase = 0;   /* start over */
                    }
                    else if ( scphase > 2 )
                    {
                        /* RARE Valid Condition, SC == 00 00 01 01 */
                        sc_pos = i;
                        *pphase = scphase;
                        return(sc_pos);
                    }
                }
                else if ( 3 == scphase )
                {
                    /* Valid Condition, SC == 00 00 01 xx */
                    sc_pos = i;
                    *pphase = scphase;
                    return(sc_pos);
                }
                else
                {
                    scphase = 0;

                    if (  (3 == (0x3 & i)) &&  /* dword aligned? */
                            ((len - i) > 4) )   /* more than four bytes left */
                    {
                        i++;
                        in_slow_loop = 0; /* go to fast loop */

                        /* WARNING: Performance GoTo */
                        goto sc_detect_fast_loop;
                    }
                }

                i++;
            }
        }
        else  /* we're in the fast loop */
        {
            /* ------- FAST SC Detect Loop, used to skip at high bandwidth --------*/
sc_detect_fast_loop:

            /* FAST start-code scanning loop (Krebs Algorithm) */
            while ( i <= (len - 4) )
            {
                register unsigned int      dw;

                dw = *((unsigned int *)&buf[i]);
#ifndef MFDBIGENDIAN
                dw = SWAP_WORD(dw);
#endif
                if ( 0 != (dw & SC_BYTE_MASK0) )
                {
                    if ( 0 != (dw & SC_BYTE_MASK1) )
                    {
                        /* most common code path */
                        i += 4;
                        continue;
                    }
                }

                break;
            }
            /* potential SC detected or at end of loop */
            in_slow_loop = 1;

            /* WARNING: performance goto */
            goto sc_detect_slow_loop;
        }
    }

    *pphase = scphase;
    return(sc_pos);
}
unsigned int viddec_parse_sc(void *in, void *pcxt)
{
    viddec_sc_parse_cubby_cxt_t *cxt;
    int                          boff;
    int                          retval=0;

    cxt = (viddec_sc_parse_cubby_cxt_t *)in;

    /* get to four-byte alignment */
    boff = (int)cxt->buf & 0x3;

    cxt->sc_end_pos = parser_find_next_startcode(
                          (const unsigned char *)cxt->buf - boff,
                          boff,
                          cxt->size + boff,
                          &cxt->phase );

    if ( (int)cxt->sc_end_pos >= 0 )
    {
        cxt->sc_end_pos -= boff;

        /* have not fully finished the buffer */
        if ( cxt->sc_end_pos < cxt->size )
            cxt->phase++;

        retval = 1;
    }
    else
    {
        /* No startcode found */
    }

    return(retval);
}
#endif
