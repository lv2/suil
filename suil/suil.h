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

/** @file
 * Public Suil API.
 */

#ifndef SUIL_SUIL_H
#define SUIL_SUIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef SUIL_SHARED
	#if defined _WIN32 || defined __CYGWIN__
		#define SUIL_LIB_IMPORT __declspec(dllimport)
		#define SUIL_LIB_EXPORT __declspec(dllexport)
	#else
		#define SUIL_LIB_IMPORT __attribute__ ((visibility("default")))
		#define SUIL_LIB_EXPORT __attribute__ ((visibility("default")))
	#endif
	#ifdef SUIL_INTERNAL
		#define SUIL_API SUIL_LIB_EXPORT
	#else
		#define SUIL_API SUIL_LIB_IMPORT
	#endif
#else
	#define SUIL_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup suil Suil
 * A library for hosting LV2 plugin UIs.
 * @{
 */


/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SUIL_SUIL_H */
