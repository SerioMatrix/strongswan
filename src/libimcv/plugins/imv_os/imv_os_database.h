/*
 * Copyright (C) 2012 Andreas Steffen
 * HSR Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/**
 *
 * @defgroup imv_os_database_t imv_os_database
 * @{ @ingroup imv_os_database
 */

#ifndef IMV_OS_DATABASE_H_
#define IMV_OS_DATABASE_H_

#include <library.h>

typedef struct imv_os_database_t imv_os_database_t;

/**
 * Internal state of an imv_os_database_t instance
 */
struct imv_os_database_t {

	/**
	 * Check Installed Packages for a given OS
	 *
	 * @param os_info				OS name and version
	 * @param package_enumerator	enumerates over installed packages
	 */
	status_t (*check_packages)(imv_os_database_t *this, char* os_info,
							   enumerator_t *package_enumerator);

	/**
	* Destroys an imv_os_database_t object.
	*/
	void (*destroy)(imv_os_database_t *this);

};

/**
 * Create an imv_os_database_t instance
 *
 * @param uri			database uri
 */
imv_os_database_t* imv_os_database_create(char *uri);

#endif /** IMV_OS_DATABASE_H_ @}*/