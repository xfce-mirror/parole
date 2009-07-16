/*
 * * Copyright (C) 2009 Ali <aliov@xfce.org>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __RC_UTILS_
#define __RC_UTILS_

void			parole_rc_write_entry_bool	(const gchar *property,
							 gboolean value);

void			parole_rc_write_entry_int	(const gchar *property,
							 gint value);
							 
void 			parole_rc_write_entry_string	(const gchar *property, 
							 const gchar *value);
							 
gboolean 		parole_rc_read_entry_bool	(const gchar *property,
							 gboolean fallback);

gint			parole_rc_read_entry_int	(const gchar *property,
							 gint fallback);

const gchar	       *parole_rc_read_entry_string	(const gchar *property,
							 const gchar *fallback);


#endif /* __RC_UTILS_ */
