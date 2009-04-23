/*
 * $Id$
 *
 * Copyright (C) 2002 ETC s.r.o.
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
 * Written by Marcel Telka <marcel@telka.sk>, 2002.
 *
 */

#ifndef URJ_PART_H
#define URJ_PART_H

#include <stdio.h>

#include "bssignal.h"
#include "instruction.h"
#include "data_register.h"
#include "bsbit.h"

#define URJ_PART_MANUFACTURER_MAXLEN    25
#define URJ_PART_PART_MAXLEN            20
#define URJ_PART_STEPPING_MAXLEN                8

typedef struct urj_part urj_part_t;

struct urj_part
{
    urj_tap_register_t *id;
    char *alias;                /* djf refdes */
    char manufacturer[URJ_PART_MANUFACTURER_MAXLEN + 1];
    char part[URJ_PART_PART_MAXLEN + 1];
    char stepping[URJ_PART_STEPPING_MAXLEN + 1];
    urj_part_signal_t *signals;
    urj_part_salias_t *saliases;
    int instruction_length;
    urj_instruction_t *instructions;
    urj_instruction_t *active_instruction;
    urj_data_register_t *data_registers;
    int boundary_length;
    urj_bsbit_t **bsbits;
};

urj_part_t *urj_part_alloc (const urj_tap_register_t *id);
void urj_part_free (urj_part_t *p);
urj_part_t *read_part (FILE * f, urj_tap_register_t *idr);
urj_instruction_t *urj_part_find_instruction (urj_part_t *p,
                                              const char *iname);
urj_data_register_t *urj_part_find_data_register (urj_part_t *p,
                                                  const char *drname);
urj_part_signal_t *urj_part_find_signal (urj_part_t *p,
                                         const char *signalname);
void urj_part_set_instruction (urj_part_t *p, const char *iname);
void urj_part_set_signal (urj_part_t *p, urj_part_signal_t *s, int out,
                          int val);
int urj_part_get_signal (urj_part_t *p, urj_part_signal_t *s);
void urj_part_print (urj_part_t *p);

typedef struct urj_parts urj_parts_t;

struct urj_parts
{
    int len;
    urj_part_t **parts;
};

urj_parts_t *urj_part_parts_alloc (void);
void urj_part_parts_free (urj_parts_t *ps);
int urj_part_parts_add_part (urj_parts_t *ps, urj_part_t *p);
void urj_part_parts_set_instruction (urj_parts_t *ps, const char *iname);
void urj_part_parts_print (urj_parts_t *ps);

#endif /* URJ_PART_H */
