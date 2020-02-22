/*
 * $Id$
 *
 * Copyright (C) 2003 ETC s.r.o.
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
 * Written by Marcel Telka <marcel@telka.sk>, 2003.
 *
 */

#include <sysdep.h>

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include <urjtag/error.h>
#include <urjtag/part.h>
#include <urjtag/part_instruction.h>
#include <urjtag/chain.h>

#include <urjtag/cmd.h>

#include "cmd.h"

static int
cmd_get_instruction_run (urj_chain_t *chain, char *params[])
{
    urj_part_t *part;
    //long unsigned len;
    //urj_part_instruction_t *i;
    const char *i;

    if (urj_cmd_test_cable (chain) != URJ_STATUS_OK)
        return URJ_STATUS_FAIL;

    part = urj_tap_chain_active_part (chain);
    if (part == NULL)
        return URJ_STATUS_FAIL;


    i = urj_part_get_instruction (part);
    if (i == NULL)
    {
        return URJ_STATUS_FAIL;
    }
    urj_log (URJ_LOG_LEVEL_NORMAL, _("%s"), i);
    

    return URJ_STATUS_OK;

}

static void
cmd_get_instruction_help (void)
{
    urj_log (URJ_LOG_LEVEL_NORMAL,
             _("Usage: %s\n"),
             "get_instruction");
}

static void
cmd_get_instruction_complete (urj_chain_t *chain, char ***matches, size_t *match_cnt,
                          char * const *tokens, const char *text, size_t text_len,
                          size_t token_point)
{
    urj_part_t *part;
    urj_part_instruction_t *i;

    if (token_point != 1)
        return;

    part = urj_tap_chain_active_part (chain);
    if (part == NULL)
        return;

    i = part->instructions;
    while (i)
    {
        urj_completion_mayben_add_match (matches, match_cnt, text, text_len,
                                         i->name);
        i = i->next;
    }
}

const urj_cmd_t urj_cmd_get_instruction = {
    "get_instruction",
    N_("get active instruction for a part"),
    cmd_get_instruction_help,
    cmd_get_instruction_run,
    cmd_get_instruction_complete,
};
