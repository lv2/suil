/* Suil, an LV2 plugin UI hosting library.
 * Copyright 2011 David Robillard <d@drobilla.net>
 *
 * Suil is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Suil is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _XOPEN_SOURCE 500

#include <string.h>

#include "suil_internal.h"

SUIL_API
SuilUIs
suil_uis_new(const char* plugin_uri)
{
	SuilUIs uis = (SuilUIs)malloc(sizeof(struct _SuilUIs));
	uis->plugin_uri = strdup(plugin_uri);
	uis->uis        = malloc(sizeof(SuilUI));
	uis->uis[0]     = (SuilUI)NULL;
	uis->n_uis      = 0;
	return uis;
}

SUIL_API
void
suil_uis_free(SuilUIs uis)
{
	free(uis->plugin_uri);
	free(uis);
}

SUIL_API
void
suil_uis_add(SuilUIs     uis,
             const char* uri,
             const char* type_uri,
             const char* bundle_path,
             const char* binary_path)
{
	SuilUI ui = (SuilUI)malloc(sizeof(struct _SuilUI));
	ui->uri         = strdup(uri);
	ui->type_uri    = strdup(type_uri);
	ui->bundle_path = strdup(bundle_path);
	ui->binary_path = strdup(binary_path);

	++uis->n_uis;
	uis->uis = realloc(uis->uis, sizeof(SuilUI) * (uis->n_uis + 1));
	assert(uis->uis[uis->n_uis - 1] == NULL);
	uis->uis[uis->n_uis - 1] = ui;
	uis->uis[uis->n_uis]     = NULL;
}
