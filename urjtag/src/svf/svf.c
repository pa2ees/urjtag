/*
 * $Id$
 *
 * Copyright (C) 2004, Arnim Laeuger
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written by Arnim Laeuger <arniml@users.sourceforge.net>, 2004.
 *
 * See "Serial Vector Format Specification", Revision E, 1999
 * ASSET InterTech, Inc.
 * http://www.asset-intertech.com/support/svf.pdf
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef SA_ONESHOT
#define SA_ONESHOT SA_RESETHAND
#endif

#include "sysdep.h"

#include <jtag.h>
#include <tap.h>
#include <tap_state.h>

#if 0
#include <urjtag/svf.h>         /* two include files w/ the same name
                                 * this will be resolved when we will have
                                 * include/urjtag/svf.h
                                 *                              RFHH */
#endif

#include <cmd.h>

#include "svf.h"
#include "svf_bison.h"

#ifdef __MINGW32__
#include "fclock.h"
#endif


/* define for debug messages */
#undef DEBUG


int urj_svf_parse (urj_svf_parser_priv_t *priv_data, urj_chain_t *chain);


/*
 * urj_svf_force_reset_state()
 *
 * Puts TAP controller into reset state by clocking 5 times with TMS = 1.
 */
static void
urj_svf_force_reset_state (urj_chain_t *chain)
{
    urj_tap_chain_clock (chain, 1, 0, 5);
    urj_tap_state_reset (chain);
}


/*
 * urj_svf_goto_state(state)
 *
 * Moves from any TAP state to the specified state.
 * The state traversal is done according to the SVF specification.
 *   See STATE of the Serial Vector Format Specification
 *
 * Encoding of state is according to the jtag suite's defines.
 *
 * Parameter:
 *   state : new TAP controller state
 */
static void
urj_svf_goto_state (urj_chain_t *chain, int new_state)
{
    int current_state;

    current_state = urj_tap_state (chain);

    /* handle unknown state */
    if (new_state == URJ_TAP_STATE_UNKNOWN_STATE)
        new_state = URJ_TAP_STATE_TEST_LOGIC_RESET;

    /* abort if new_state already reached */
    if (current_state == new_state)
        return;

    switch (current_state)
    {
    case URJ_TAP_STATE_TEST_LOGIC_RESET:
        urj_tap_chain_clock (chain, 0, 0, 1);
        break;

    case URJ_TAP_STATE_RUN_TEST_IDLE:
        urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    case URJ_TAP_STATE_SELECT_DR_SCAN:
    case URJ_TAP_STATE_SELECT_IR_SCAN:
        if (new_state == URJ_TAP_STATE_TEST_LOGIC_RESET ||
            new_state == URJ_TAP_STATE_RUN_TEST_IDLE ||
            (current_state & URJ_TAP_STATE_DR && new_state & URJ_TAP_STATE_IR)
            || (current_state & URJ_TAP_STATE_IR
                && new_state & URJ_TAP_STATE_DR))
            /* progress in select-idle/reset loop */
            urj_tap_chain_clock (chain, 1, 0, 1);
        else
            /* enter DR/IR branch */
            urj_tap_chain_clock (chain, 0, 0, 1);
        break;

    case URJ_TAP_STATE_CAPTURE_DR:
        if (new_state == URJ_TAP_STATE_SHIFT_DR)
            /* enter URJ_TAP_STATE_SHIFT_DR state */
            urj_tap_chain_clock (chain, 0, 0, 1);
        else
            /* bypass URJ_TAP_STATE_SHIFT_DR */
            urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    case URJ_TAP_STATE_CAPTURE_IR:
        if (new_state == URJ_TAP_STATE_SHIFT_IR)
            /* enter URJ_TAP_STATE_SHIFT_IR state */
            urj_tap_chain_clock (chain, 0, 0, 1);
        else
            /* bypass URJ_TAP_STATE_SHIFT_IR */
            urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    case URJ_TAP_STATE_SHIFT_DR:
    case URJ_TAP_STATE_SHIFT_IR:
        /* progress to URJ_TAP_STATE_EXIT1_DR/IR */
        urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    case URJ_TAP_STATE_EXIT1_DR:
        if (new_state == URJ_TAP_STATE_PAUSE_DR)
            /* enter URJ_TAP_STATE_PAUSE_DR state */
            urj_tap_chain_clock (chain, 0, 0, 1);
        else
            /* bypass URJ_TAP_STATE_PAUSE_DR */
            urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    case URJ_TAP_STATE_EXIT1_IR:
        if (new_state == URJ_TAP_STATE_PAUSE_IR)
            /* enter URJ_TAP_STATE_PAUSE_IR state */
            urj_tap_chain_clock (chain, 0, 0, 1);
        else
            /* bypass URJ_TAP_STATE_PAUSE_IR */
            urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    case URJ_TAP_STATE_PAUSE_DR:
    case URJ_TAP_STATE_PAUSE_IR:
        /* progress to URJ_TAP_STATE_EXIT2_DR/IR */
        urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    case URJ_TAP_STATE_EXIT2_DR:
        if (new_state == URJ_TAP_STATE_SHIFT_DR)
            /* enter URJ_TAP_STATE_SHIFT_DR state */
            urj_tap_chain_clock (chain, 0, 0, 1);
        else
            /* progress to URJ_TAP_STATE_UPDATE_DR */
            urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    case URJ_TAP_STATE_EXIT2_IR:
        if (new_state == URJ_TAP_STATE_SHIFT_IR)
            /* enter URJ_TAP_STATE_SHIFT_IR state */
            urj_tap_chain_clock (chain, 0, 0, 1);
        else
            /* progress to URJ_TAP_STATE_UPDATE_IR */
            urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    case URJ_TAP_STATE_UPDATE_DR:
    case URJ_TAP_STATE_UPDATE_IR:
        if (new_state == URJ_TAP_STATE_RUN_TEST_IDLE)
            /* enter URJ_TAP_STATE_RUN_TEST_IDLE */
            urj_tap_chain_clock (chain, 0, 0, 1);
        else
            /* progress to Select_DR/IR */
            urj_tap_chain_clock (chain, 1, 0, 1);
        break;

    default:
        urj_svf_force_reset_state (chain);
        break;
    }

    /* continue state changes */
    urj_svf_goto_state (chain, new_state);
}


/*
 * urj_svf_map_state(state)
 *
 * Maps the state encoding of the SVF parser to the
 * state encoding of the jtag suite.
 *
 * Parameter:
 *   state : state encoded by/for SVF parser
 *
 * Return value:
 *   state encoded for jtag tools
 */
static int
urj_svf_map_state (int state)
{
    int jtag_state;

    switch (state)
    {
    case URJ_JIM_RESET:
        jtag_state = URJ_TAP_STATE_TEST_LOGIC_RESET;
        break;
    case URJ_JIM_IDLE:
        jtag_state = URJ_TAP_STATE_RUN_TEST_IDLE;
        break;
    case DRSELECT:
        jtag_state = URJ_TAP_STATE_SELECT_DR_SCAN;
        break;
    case DRCAPTURE:
        jtag_state = URJ_TAP_STATE_CAPTURE_DR;
        break;
    case DRSHIFT:
        jtag_state = URJ_TAP_STATE_SHIFT_DR;
        break;
    case DREXIT1:
        jtag_state = URJ_TAP_STATE_EXIT1_DR;
        break;
    case DRPAUSE:
        jtag_state = URJ_TAP_STATE_PAUSE_DR;
        break;
    case DREXIT2:
        jtag_state = URJ_TAP_STATE_EXIT2_DR;
        break;
    case DRUPDATE:
        jtag_state = URJ_TAP_STATE_UPDATE_DR;
        break;

    case IRSELECT:
        jtag_state = URJ_TAP_STATE_SELECT_IR_SCAN;
        break;
    case IRCAPTURE:
        jtag_state = URJ_TAP_STATE_CAPTURE_IR;
        break;
    case IRSHIFT:
        jtag_state = URJ_TAP_STATE_SHIFT_IR;
        break;
    case IREXIT1:
        jtag_state = URJ_TAP_STATE_EXIT1_IR;
        break;
    case IRPAUSE:
        jtag_state = URJ_TAP_STATE_PAUSE_IR;
        break;
    case IREXIT2:
        jtag_state = URJ_TAP_STATE_EXIT2_IR;
        break;
    case IRUPDATE:
        jtag_state = URJ_TAP_STATE_UPDATE_IR;
        break;

    default:
        jtag_state = URJ_TAP_STATE_UNKNOWN_STATE;
        break;
    }

    return (jtag_state);
}


/*
 * urj_svf_hex2dec(nibble)
 *
 * Converts a hexadecimal nibble (4 bits) to its decimal value.
 *
 * Parameter:
 *   nibble : hexadecimal character
 *
 * Return value:
 *   decimal value of nibble or 0 if nibble is not a hexadecimal character
 */
static int
urj_svf_hex2dec (char nibble)
{
    int lower;

    if (nibble >= '0' && nibble <= '9')
        return ((int) (nibble - '0'));

    lower = tolower ((int) nibble);
    if (lower >= 'a' && lower <= 'f')
        return (lower - (int) 'a' + 10);

    return (0);
}


/*
 * urj_svf_build_bit_string(hex_string, len)
 *
 * Converts the hexadecimal string hex_string into a string of single bits
 * with len elements (bits).
 * If hex_string contains less nibbles than fit into len bits, the resulting
 * bit string is padded with 0 bits.
 *
 * Note:
 * The memory for the resulting bit string is calloc'ed and must be
 * free'd when the bit string is not used anymore.
 *
 * Example:
 *   hex string : 1a
 *   len        : 16
 *   bit string : 0000000000011010
 *
 * Parameter:
 *   hex_string : hex string to be converted
 *   len        : number of bits in resulting bit string
 *
 * Return value:
 *   pointer to new bit string
 *   NULL upon error
 */
static char *
urj_svf_build_bit_string (char *hex_string, int len)
{
    char *bit_string, *bit_string_pos;
    int nibble;
    char *hex_string_pos;
    int hex_string_idx;

    if (!(bit_string = (char *) calloc (len + 1, sizeof (char))))
    {
        printf (_("out of memory"));
        return (NULL);
    }

    /* copy reduced hexadecimal string to full bit string */
    hex_string_idx = strlen (hex_string);
    hex_string_pos = &(hex_string[hex_string_idx]);
    nibble = 3;
    bit_string_pos = &(bit_string[len]);
    do
    {
        bit_string_pos--;
        if (nibble == 3)
        {
            nibble = 0;
            hex_string_pos--;
            hex_string_idx--;
        }
        else
            nibble++;

        *bit_string_pos =
            urj_svf_hex2dec (hex_string_idx >=
                             0 ? *hex_string_pos : '0') & (1 << nibble) ? '1'
            : '0';
    }
    while (bit_string_pos != bit_string);

    bit_string[len] = '\0';

    return (bit_string);
}


/*
 * urj_svf_copy_hex_to_register(hex_string, reg, len)
 *
 * Copies the contents of the hexadecimal string hex_string into the given
 * tap register.
 *
 * Parameter:
 *   hex_string : hex string to be entered in reg
 *   reg        : tap register to hold the converted hex string
 *
 * Return value:
 *   1 : all ok
 *   0 : error occurred
 */
static int
urj_svf_copy_hex_to_register (char *hex_string, urj_tap_register_t *reg)
{
    char *bit_string;

    if (!(bit_string = urj_svf_build_bit_string (hex_string, reg->len)))
        return (0);

    urj_tap_register_init (reg, bit_string);

    /* free memory as we do not need the intermediate bit_string anymore */
    free (bit_string);

    return (1);
}


/*
 * urj_svf_compare_tdo(tdo, mask, reg)
 *
 * Compares the captured device output in tap register reg with the expected
 * hex_string tdo (specified in SVF command SDR/SDI.
 *
 * Comparison honours the "care" bits in mask ('1') while matching the contents
 * of reg with tdo.
 *
 * Parameter:
 *   tdo  : reference hex string
 *   mask : hex string for masking tdo
 *   reg  : hex string to be compared vs. tdo
 *
 * Return value:
 *   1 : tdo matches reg at all positions where mask is '1'
 *   0 : tdo and reg do not match or error occurred
 */
static int
urj_svf_compare_tdo (urj_svf_parser_priv_t *priv, char *tdo, char *mask,
                     urj_tap_register_t *reg, YYLTYPE *loc)
{
    char *tdo_bit, *mask_bit;
    int pos, mismatch, result = 1;

    if (!(tdo_bit = urj_svf_build_bit_string (tdo, reg->len)))
        return (0);
    if (!(mask_bit = urj_svf_build_bit_string (mask, reg->len)))
    {
        free (tdo_bit);
        return (0);
    }

    /* retrieve string representation */
    urj_tap_register_get_string (reg);

    mismatch = -1;
    for (pos = 0; pos < reg->len; pos++)
        if ((tdo_bit[pos] != reg->string[pos]) && (mask_bit[pos] == '1'))
            mismatch = pos;

    if (mismatch >= 0)
    {
        printf (_("Error %s: mismatch at position %d for TDO\n"), "svf",
                mismatch);
        if (loc != NULL)
        {
            printf
                (" in input file between line %d col %d and line %d col %d\n",
                 loc->first_line + 1, loc->first_column + 1,
                 loc->last_line + 1, loc->last_column + 1);
        }

#ifdef DEBUG
        printf ("Expected : %s\n", tdo_bit);
        printf ("Mask     : %s\n", mask_bit);
        printf ("TDO data : %s\n", reg->string);
#endif

        if (priv->svf_stop_on_mismatch)
            result = 0;
    }

    free (mask_bit);
    free (tdo_bit);

    return (result);
}


/*
 * urj_svf_remember_param(rem, new)
 *
 * Assigns the contents of the string new to the string rem.
 * By doing so, the responsability to free the memory occupied by new
 * is transferred to the code that handles *rem.
 * Nothing happens when new is NULL. In this case the current value of
 * rem has to be "remembered".
 *
 * Parameter:
 *   rem : hex string pointer pointing to the "remembered" string
 *   new : hex string that has to be rememberd
 *         memory of the string is free'd
 */
static void
urj_svf_remember_param (char **rem, char *new)
{
    if (new)
    {
        if (*rem)
            free (*rem);

        *rem = new;
    }
}


/*
 * urj_svf_all_care(string, number)
 *
 * Allocates a hex string of given length (number gives number of bits)
 * and sets it to all 'F'.
 * The allocated memory of the string has to be free'd by the caller.
 *
 * Parameter:
 *   string : is updated with the pointer to the allocated hex string
 *   number : number of required bits
 *
 * Return value:
 *   1 : all ok
 *   0 : error occurred
 */
static int
urj_svf_all_care (char **string, double number)
{
    char *ptr;
    int num, result;

    result = 1;

    num = (int) number;
    num = num % 4 == 0 ? num / 4 : num / 4 + 1;

    /* build string with all cares */
    if (!(ptr = (char *) calloc (num + 1, sizeof (char))))
    {
        printf (_("out of memory"));
        return (0);
    }
    memset (ptr, 'F', num);
    ptr[num] = '\0';

    urj_svf_remember_param (string, ptr);
    /* responsability for free'ing ptr is now at the code that
       operates on *string */

    return (result);
}


/* ***************************************************************************
 * urj_svf_endxr(ir_dr, state)
 *
 * Register end states for shifting IR and DR.
 *
 * Parameter:
 *   ir_dr : selects ENDIR or ENDDR
 *   state : required end state (SVF parser encoding)
 * ***************************************************************************/
void
urj_svf_endxr (urj_svf_parser_priv_t *priv,
               enum URJ_SVF_generic_irdr_coding ir_dr, int state)
{
    switch (ir_dr)
    {
    case URJ_SVF_generic_ir:
        priv->endir = urj_svf_map_state (state);
        break;
    case URJ_SVF_generic_dr:
        priv->enddr = urj_svf_map_state (state);
        break;
    }
}


/* ***************************************************************************
 * urj_svf_frequency(chain, freq)
 *
 * Implements the FREQUENCY command.
 *
 * Parameter:
 *   freq : frequency in HZ
 * ***************************************************************************/
void
urj_svf_frequency (urj_chain_t *chain, double freq)
{
    urj_tap_cable_set_frequency (chain->cable, freq);
}


/* ***************************************************************************
 * urj_svf_hxr(ir_dr, params)
 *
 * Handles HIR, HDR.
 *
 * Note:
 * Functionality not implemented.
 *
 * Parameter:
 *   ir_dr  : selects HIR or HDR
 *   params : paramter set for TXR, HXR and SXR
 *
 * Return value:
 *   1 : all ok
 *   0 : error occurred
 * ***************************************************************************/
int
urj_svf_hxr (enum URJ_SVF_generic_irdr_coding ir_dr,
             struct ths_params *params)
{
    if (params->number != 0.0)
        printf (_("Warning %s: command %s not implemented\n"), "svf",
                ir_dr == URJ_SVF_generic_ir ? "HIR" : "HDR");

    return (1);
}


static int max_time_reached;
static void
sigalrm_handler (int signal)
{
    max_time_reached = 1;
}


/* ***************************************************************************
 * urj_svf_runtest(params)
 *
 * Implements the RUNTEST command.
 *
 * Parameter:
 *   params : paramter set for RUNTEST
 *
 * Return value:
 *   1 : all ok
 *   0 : error occurred
 * ***************************************************************************/
int
urj_svf_runtest (urj_chain_t *chain, urj_svf_parser_priv_t *priv,
                 struct runtest *params)
{
    uint32_t run_count, frequency;

    /* check for restrictions */
    if (params->run_count > 0 && params->run_clk != TCK)
    {
        printf (_("Error %s: only TCK is supported for RUNTEST.\n"), "svf");
        return (0);
    }
    if (params->max_time > 0.0 && params->max_time < params->min_time)
    {
        printf (_
                ("Error %s: maximum time must be larger or equal to minimum time.\n"),
                "svf");
        return (0);
    }
    if (params->max_time > 0.0)
        if (!priv->issued_runtest_maxtime)
        {
            printf (_
                    ("Warning %s: maximum time for RUNTEST not guaranteed.\n"),
                    "svf");
            printf (_(" This message is only displayed once.\n"));
            priv->issued_runtest_maxtime = 1;
        }

    /* update default values for run_state and end_state */
    if (params->run_state != 0)
    {
        priv->runtest_run_state = urj_svf_map_state (params->run_state);

        if (params->end_state == 0)
            priv->runtest_end_state = urj_svf_map_state (params->run_state);
    }
    if (params->end_state != 0)
        priv->runtest_end_state = urj_svf_map_state (params->end_state);

    /* compute run_count */
    run_count = params->run_count;
    if (params->min_time > 0.0)
    {
        frequency =
            priv->ref_freq >
            0 ? priv->ref_freq : urj_tap_cable_get_frequency (chain->cable);
        if (frequency > 0)
        {
            uint32_t min_time_run_count = ceil (params->min_time * frequency);
            if (min_time_run_count > run_count)
            {
                run_count = min_time_run_count;
            }
        }
        else
        {
            printf (_
                    ("Error %s: Maximum cable clock frequency required for RUNTEST.\n"),
                    "svf");
            printf (_("  Set the cable frequency with 'FREQUENCY <Hz>'.\n"));
            return 0;
        }
    }

    urj_svf_goto_state (chain, priv->runtest_run_state);

#ifdef __MINGW32__
    if (params->max_time > 0.0)
    {
        double maxt = urj_lib_frealtime () + params->max_time;

        while (run_count-- > 0 && urj_lib_frealtime () < maxt)
        {
            urj_tap_chain_clock (chain, 0, 0, 1);
        }
    }
    else
        urj_tap_chain_clock (chain, 0, 0, run_count);

    urj_svf_goto_state (chain, priv->runtest_end_state);

#else
    /* set up the timer for max_time */
    if (params->max_time > 0.0)
    {
        struct sigaction sa;
        unsigned max_time;

        sa.sa_handler = sigalrm_handler;
        sa.sa_flags = SA_ONESHOT;
        sigemptyset (&sa.sa_mask);
        if (sigaction (SIGALRM, &sa, NULL) != 0)
        {
            perror ("sigaction");
            exit (EXIT_FAILURE);
        }

        max_time = floor (params->max_time / 1000000);
        if (max_time == 0)
        {
            max_time = 1;
        }
        ualarm (max_time, 0);
    }

    if (params->max_time > 0.0)
        while (run_count-- > 0 && !max_time_reached)
        {
            urj_tap_chain_clock (chain, 0, 0, 1);
        }
    else
        urj_tap_chain_clock (chain, 0, 0, run_count);

    urj_svf_goto_state (chain, priv->runtest_end_state);

    /* stop the timer */
    if (params->max_time > 0.0)
    {
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sa.sa_flags = 0;
        sigemptyset (&sa.sa_mask);
        if (sigaction (SIGALRM, &sa, NULL) != 0)
        {
            perror ("sigaction");
            exit (EXIT_FAILURE);
        }
    }
#endif

    return (1);
}


/* ***************************************************************************
 * urj_svf_state(path_states, stable_state)
 *
 * Implements the STATE command.
 *
 * Parameter:
 *   path_states  : states to traverse before reaching stable_state
 *                  (SVF parser encoding)
 *   stable_state : final stable state
 *                  (SVF parser encoding)
 *
 * Return value:
 *   1 : all ok
 *   0 : error occurred
 * ***************************************************************************/
int
urj_svf_state (urj_chain_t *chain, urj_svf_parser_priv_t *priv,
               struct path_states *path_states, int stable_state)
{
    int i;

    priv->svf_state_executed = 1;

    for (i = 0; i < path_states->num_states; i++)
        urj_svf_goto_state (chain,
                            urj_svf_map_state (path_states->states[i]));

    if (stable_state)
        urj_svf_goto_state (chain, urj_svf_map_state (stable_state));

    return (1);
}


/* ***************************************************************************
 * urj_svf_sxr(ir_dr, params)
 *
 * Implements the SIR and SDR commands.
 *
 * Parameter:
 *   ir_dr  : selects SIR or SDR
 *   params : paramter set for TXR, HXR and SXR
 *
 * Return value:
 *   1 : all ok
 *   0 : error occurred
 * ***************************************************************************/
int
urj_svf_sxr (urj_chain_t *chain, urj_svf_parser_priv_t *priv,
             enum URJ_SVF_generic_irdr_coding ir_dr,
             struct ths_params *params, YYLTYPE *loc)
{
    urj_svf_sxr_t *sxr_params;
    int len, result = 1;

    sxr_params =
        ir_dr ==
        URJ_SVF_generic_ir ? &(priv->sir_params) : &(priv->sdr_params);

    /* remember parameters */
    urj_svf_remember_param (&sxr_params->params.tdi, params->tdi);

    sxr_params->params.tdo = params->tdo;       /* tdo is not "remembered" */

    urj_svf_remember_param (&sxr_params->params.mask, params->mask);

    urj_svf_remember_param (&sxr_params->params.smask, params->smask);


    /* handle length change for MASK and SMASK */
    if (sxr_params->params.number != params->number)
    {
        sxr_params->no_tdi = 1;
        sxr_params->no_tdo = 1;

        if (!params->mask)
            if (!urj_svf_all_care (&sxr_params->params.mask, params->number))
                result = 0;
        if (!params->smask)
            if (!urj_svf_all_care (&sxr_params->params.smask, params->number))
                result = 0;
    }

    sxr_params->params.number = params->number;

    /* check consistency */
    if (sxr_params->no_tdi)
    {
        if (!params->tdi)
        {
            printf (_
                    ("Error %s: first %s command after length change must have a TDI value.\n"),
                    "svf", ir_dr == URJ_SVF_generic_ir ? "SIR" : "SDR");
            result = 0;
        }
        sxr_params->no_tdi = 0;
    }

    /* take over responsability for free'ing parameter strings */
    params->tdi = NULL;
    params->mask = NULL;
    params->smask = NULL;

    /* result of consistency check */
    if (!result)
        return (0);


    /*
     * handle tap registers
     */
    len = (int) sxr_params->params.number;
    switch (ir_dr)
    {
    case URJ_SVF_generic_ir:
        /* is SIR large enough? */
        if (priv->ir->value->len != len)
        {
            printf (_("Error %s: SIR command length inconsistent.\n"), "svf");
            if (loc != NULL)
            {
                printf
                    (" in input file between line %d col %d and line %d col %d\n",
                     loc->first_line + 1, loc->first_column + 1,
                     loc->last_line + 1, loc->last_column + 1);
            }
            return (0);
        }
        break;

    case URJ_SVF_generic_dr:
        /* check data register SDR */
        if (priv->dr->in->len != len)
        {
            /* length does not match, so install proper registers */
            urj_tap_register_free (priv->dr->in);
            priv->dr->in = NULL;
            urj_tap_register_free (priv->dr->out);
            priv->dr->out = NULL;

            if (!(priv->dr->in = urj_tap_register_alloc (len)))
            {
                printf (_("out of memory"));
                return (0);
            }
            if (!(priv->dr->out = urj_tap_register_alloc (len)))
            {
                printf (_("out of memory"));
                return (0);
            }
        }
        break;

    }

    /* fill register with value of TDI parameter */
    if (!urj_svf_copy_hex_to_register (sxr_params->params.tdi,
                                       ir_dr ==
                                       URJ_SVF_generic_ir ? priv->ir->
                                       value : priv->dr->in))
        return (0);


    /* shift selected instruction/register */
    switch (ir_dr)
    {
    case URJ_SVF_generic_ir:
        urj_svf_goto_state (chain, URJ_TAP_STATE_SHIFT_IR);
        urj_tap_chain_shift_instructions_mode (chain,
                                               sxr_params->params.tdo ? 1 : 0,
                                               0, URJ_CHAIN_EXITMODE_EXIT1);
        urj_svf_goto_state (chain, priv->endir);

        if (sxr_params->params.tdo)
            result =
                urj_svf_compare_tdo (priv, sxr_params->params.tdo,
                                     sxr_params->params.mask, priv->ir->out,
                                     loc);
        break;

    case URJ_SVF_generic_dr:
        urj_svf_goto_state (chain, URJ_TAP_STATE_SHIFT_DR);
        urj_tap_chain_shift_data_registers_mode (chain,
                                                 sxr_params->params.
                                                 tdo ? 1 : 0, 0,
                                                 URJ_CHAIN_EXITMODE_EXIT1);
        urj_svf_goto_state (chain, priv->enddr);

        if (sxr_params->params.tdo)
            result =
                urj_svf_compare_tdo (priv, sxr_params->params.tdo,
                                     sxr_params->params.mask, priv->dr->out,
                                     loc);
        break;
    }

    /* log mismatches */
    if (result == 0)
        priv->mismatch_occurred = 1;

    return (result);
}



/* ***************************************************************************
 * urj_svf_trst(int trst_mode)
 *
 * Sets TRST pin according to trst_mode.
 * TRST modes are encoded via defines in svf.h.
 *
 * Note:
 * The modes Z and ABSENT are not supported.
 *
 * Parameter:
 *   trst_mode : selected mode for TRST
 *
 * Return value:
 *   1 : all ok
 *   0 : error occurred
 * ***************************************************************************/
int
urj_svf_trst (urj_chain_t *chain, urj_svf_parser_priv_t *priv, int trst_mode)
{
    int trst_cable = -1;
    char *unimplemented_mode;

    if (priv->svf_trst_absent)
    {
        printf (_
                ("Error %s: no further TRST command allowed after mode ABSENT\n"),
                "svf");
        return (0);
    }

    switch (trst_mode)
    {
    case ON:
        trst_cable = 0;
        break;
    case OFF:
        trst_cable = 1;
        break;
    case Z:
        unimplemented_mode = "Z";
        break;
    case ABSENT:
        unimplemented_mode = "ABSENT";
        priv->svf_trst_absent = 1;

        if (priv->svf_state_executed)
        {
            printf (_
                    ("Error %s: TRST ABSENT must not be issued after a STATE command\n"),
                    "svf");
            return (0);
        }
        if (priv->sir_params.params.number > 0.0 ||
            priv->sdr_params.params.number > 0.0)
        {
            printf (_
                    ("Error %s: TRST ABSENT must not be issued after an SIR or SDR command\n"),
                    "svf");
        }
        break;
    default:
        unimplemented_mode = "UNKNOWN";
        break;
    }

    if (trst_cable < 0)
        printf (_("Warning %s: unimplemented mode '%s' for TRST\n"), "svf",
                unimplemented_mode);
    else
        urj_tap_cable_set_signal (chain->cable, URJ_POD_CS_TRST,
                                  trst_cable ? URJ_POD_CS_TRST : 0);

    return (1);
}


/* ***************************************************************************
 * urj_svf_txr(ir_dr, params)
 *
 * Handles TIR, TDR.
 *
 * Note:
 * Functionality not implemented.
 *
 * Parameter:
 *   ir_dr  : selects TIR or TDR
 *   params : paramter set for TXR, HXR and SXR
 *
 * Return value:
 *   1 : all ok
 *   0 : error occurred
 * ***************************************************************************/
int
urj_svf_txr (enum URJ_SVF_generic_irdr_coding ir_dr,
             struct ths_params *params)
{
    if (params->number != 0.0)
        printf (_("Warning %s: command %s not implemented\n"), "svf",
                ir_dr == URJ_SVF_generic_ir ? "TIR" : "TDR");

    return (1);
}


/* ***************************************************************************
 * urj_svf_run(chain, SVF_FILE, stop_on_mismatch, ref_freq)
 *
 * Main entry point for the 'svf' command. Calls the svf parser.
 *
 * Checks the jtag-environment (availability of SIR instruction and SDR
 * register). Initializes all svf-global variables and performs clean-up
 * afterwards.
 *
 * Parameter:
 *   chain            : pointer to global chain
 *   SVF_FILE         : file handle of SVF file
 *   stop_on_mismatch : 1 = stop upon tdo mismatch
 *                      0 = continue upon mismatch
 *   print_progress   : 1 = continually print progress status
 *                      0 = don't print
 *   ref_freq         : reference frequency for RUNTEST
 *
 * Return value:
 *   1 : all ok
 *   0 : error occurred
 * ***************************************************************************/
void
urj_svf_run (urj_chain_t *chain, FILE * SVF_FILE, int stop_on_mismatch,
             int print_progress, uint32_t ref_freq)
{
    const urj_svf_sxr_t sxr_default = { {0.0, NULL, NULL, NULL, NULL},
    1, 1
    };
    urj_svf_parser_priv_t priv;
    int c = ~EOF;
    int num_lines;
    uint32_t old_frequency = urj_tap_cable_get_frequency (chain->cable);

    /* get number of lines in svf file so we can give user some feedback on long
       files or slow cables */
    rewind (SVF_FILE);
    num_lines = 0;
    while (EOF != c)
    {
        c = fgetc (SVF_FILE);
        if ('\n' == c)
            num_lines++;
    }
    rewind (SVF_FILE);
    if (0 == num_lines)
        /* avoid those annoying divide/0 crashes */
        num_lines++;

    /* initialize
       - part
       - instruction register
       - data register */
    if (chain == NULL)
    {
        printf (_("Error %s: no JTAG chain available\n"), "svf");
        return;
    }
    if (chain->parts == NULL)
    {
        printf (_("Error %s: chain without any parts\n"), "svf");
        return;
    }
    priv.part = chain->parts->parts[chain->active_part];

    /* setup register SDR if not already existing */
    if (!(priv.dr = urj_part_find_data_register (priv.part, "SDR")))
    {
        char *register_cmd[] = { "register",
            "SDR",
            "32",
            NULL
        };

        if (urj_cmd_run (chain, register_cmd) < 1)
            return;

        if (!(priv.dr = urj_part_find_data_register (priv.part, "SDR")))
        {
            printf (_("Error %s: could not establish SDR register\n"), "svf");
            return;
        }
    }

    /* setup instruction SIR if not already existing */
    if (!(priv.ir = urj_part_find_instruction (priv.part, "SIR")))
    {
        char *instruction_cmd[] = { "instruction",
            "SIR",
            "",
            "SDR",
            NULL
        };
        char *instruction_string;
        int len, result;

        len = priv.part->instruction_length;
        if (len > 0)
        {
            if ((instruction_string =
                 (char *) calloc (len + 1, sizeof (char))) != NULL)
            {
                memset (instruction_string, '1', len);
                instruction_string[len] = '\0';
                instruction_cmd[2] = instruction_string;

                result = urj_cmd_run (chain, instruction_cmd);

                free (instruction_string);

                if (result < 1)
                    return;
            }
        }

        if (!(priv.ir = urj_part_find_instruction (priv.part, "SIR")))
        {
            printf (_("Error %s: could not establish SIR instruction\n"),
                    "svf");
            return;
        }
    }

    /* initialize variables for new parser run */
    priv.svf_stop_on_mismatch = stop_on_mismatch;

    priv.sir_params = priv.sdr_params = sxr_default;

    priv.endir = priv.enddr = URJ_TAP_STATE_RUN_TEST_IDLE;

    priv.runtest_run_state = priv.runtest_end_state =
        URJ_TAP_STATE_RUN_TEST_IDLE;

    priv.svf_trst_absent = 0;
    priv.svf_state_executed = 0;

    priv.mismatch_occurred = 0;

    /* set back flags for issued warnings */
    priv.issued_runtest_maxtime = 0;

    priv.ref_freq = ref_freq;

    /* select SIR instruction */
    urj_part_set_instruction (priv.part, "SIR");

    if (urj_svf_bison_init (&priv, SVF_FILE, num_lines, print_progress))
    {
        urj_svf_parse (&priv, chain);
        urj_svf_bison_deinit (&priv);
    }

    if (print_progress)
    {
        if (priv.mismatch_occurred > 0)
            printf (_
                    ("Mismatches occurred between scanned device output and expected TDO values.\n"));
        else
            printf (_
                    ("Scanned device output matched expected TDO values.\n"));
    }

    /* clean up */
    /* SIR */
    if (priv.sir_params.params.tdi)
        free (priv.sir_params.params.tdi);
    if (priv.sir_params.params.mask)
        free (priv.sir_params.params.mask);
    if (priv.sir_params.params.smask)
        free (priv.sir_params.params.smask);
    /* SDR */
    if (priv.sdr_params.params.tdi)
        free (priv.sdr_params.params.tdi);
    if (priv.sdr_params.params.mask)
        free (priv.sdr_params.params.mask);
    if (priv.sdr_params.params.smask)
        free (priv.sdr_params.params.smask);

    /* restore previous frequency setting, required by SVF spec */
    if (old_frequency != urj_tap_cable_get_frequency (chain->cable))
        urj_tap_cable_set_frequency (chain->cable, old_frequency);
}
