/**
 * @file sa_config.c
 * 
 * @brief Implementation of sa_config_t.
 * 
 */

/*
 * Copyright (C) 2005 Jan Hutter, Martin Willi
 * Hochschule fuer Technik Rapperswil
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

#include "sa_config.h"

#include <utils/linked_list.h>
#include <utils/allocator.h>
#include <utils/identification.h>
#include <utils/logger.h>

typedef struct private_sa_config_t private_sa_config_t;

/**
 * Private data of an sa_config_t object
 */
struct private_sa_config_t {

	/**
	 * Public part
	 */
	sa_config_t public;
	
	/**
	 * id to use to identify us
	 */
	identification_t *my_id;
	
	/**
	 * allowed id for other
	 */
	identification_t *other_id;
	
	/**
	 * authentification method to use
	 */
	auth_method_t auth_method;
	
	/**
	 * Lifetime of IKE_SA in milliseconds.
	 */
	u_int32_t ike_sa_lifetime;
	
	/**
	 * list for all proposals
	 */
	linked_list_t *proposals;
	
	/**
	 * list for traffic selectors for my site
	 */
	linked_list_t *my_ts;
	
	/**
	 * list for traffic selectors for others site
	 */
	linked_list_t *other_ts;

	/**
	 * select_traffic_selectors for both
	 */
	linked_list_t *(*select_traffic_selectors) (private_sa_config_t *,linked_list_t*,linked_list_t*);
};

/**
 * Implementation of sa_config_t.get_my_id
 */
static identification_t *get_my_id(private_sa_config_t *this)
{
	return this->my_id;
}

/**
 * Implementation of sa_config_t.get_other_id
 */
static identification_t *get_other_id(private_sa_config_t *this)
{
	return this->other_id;
}

/**
 * Implementation of sa_config_t.get_auth_method.
 */
static auth_method_t get_auth_method(private_sa_config_t *this)
{
	return this->auth_method;
}

/**
 * Implementation of sa_config_t.get_ike_sa_lifetime.
 */
static u_int32_t get_ike_sa_lifetime (private_sa_config_t *this)
{
	return this->ike_sa_lifetime;
}

/**
 * Implementation of sa_config_t.get_my_traffic_selectors
 */
static linked_list_t *get_my_traffic_selectors(private_sa_config_t *this)
{
	return this->my_ts;
}

/**
 * Implementation of sa_config_t.get_other_traffic_selectors
 */
static linked_list_t *get_other_traffic_selectors(private_sa_config_t *this, traffic_selector_t **traffic_selectors[])
{
	return this->other_ts;
}

/**
 * Implementation of private_sa_config_t.select_my_traffic_selectors
 */
static linked_list_t *select_my_traffic_selectors(private_sa_config_t *this, linked_list_t *supplied)
{
	return this->select_traffic_selectors(this, this->my_ts, supplied);
}

/**
 * Implementation of private_sa_config_t.select_other_traffic_selectors
 */
static linked_list_t *select_other_traffic_selectors(private_sa_config_t *this, linked_list_t *supplied)
{
	return this->select_traffic_selectors(this, this->other_ts, supplied);
}
/**
 * Implementation of private_sa_config_t.select_traffic_selectors
 */
static linked_list_t *select_traffic_selectors(private_sa_config_t *this, linked_list_t *stored, linked_list_t *supplied)
{
	iterator_t *supplied_iter, *stored_iter;
	traffic_selector_t *supplied_ts, *stored_ts, *selected_ts;
	linked_list_t *selected = linked_list_create();
	
	
	stored_iter = stored->create_iterator(stored, TRUE);
	supplied_iter = supplied->create_iterator(supplied, TRUE);
	
	/* iterate over all stored selectors */
	while (stored_iter->has_next(stored_iter))
	{
		stored_iter->current(stored_iter, (void**)&stored_ts);
		
		supplied_iter->reset(supplied_iter);
		/* iterate over all supplied traffic selectors */
		while (supplied_iter->has_next(supplied_iter))
		{
			supplied_iter->current(supplied_iter, (void**)&supplied_ts);
			
			selected_ts = stored_ts->get_subset(stored_ts, supplied_ts);
			if (selected_ts)
			{
				/* got a match, add to list */
				selected->insert_last(selected, (void*)selected_ts);
			}
		}
	}
	stored_iter->destroy(stored_iter);
	supplied_iter->destroy(supplied_iter);
	
	return selected;
}

/**
 * Implementation of sa_config_t.get_proposal_iterator
 */
static linked_list_t *get_proposals(private_sa_config_t *this)
{
	return this->proposals;
}

/**
 * Implementation of sa_config_t.select_proposal
 */
static proposal_t *select_proposal(private_sa_config_t *this, linked_list_t *proposals)
{
	iterator_t *stored_iter, *supplied_iter;
	proposal_t *stored, *supplied, *selected;
	
	stored_iter = this->proposals->create_iterator(this->proposals, TRUE);
	supplied_iter = proposals->create_iterator(proposals, TRUE);
	
	/* compare all stored proposals with all supplied. Stored ones are preferred. */
	while (stored_iter->has_next(stored_iter))
	{
		supplied_iter->reset(supplied_iter);
		stored_iter->current(stored_iter, (void**)&stored);

		while (supplied_iter->has_next(supplied_iter))
		{
			supplied_iter->current(supplied_iter, (void**)&supplied);
			selected = stored->select(stored, supplied);
			if (selected)
			{
				/* they match, return */
				stored_iter->destroy(stored_iter);
				supplied_iter->destroy(supplied_iter);
				return selected;
			}
		}
	}
	
	/* no proposal match :-(, will result in a NO_PROPOSAL_CHOSEN... */
	stored_iter->destroy(stored_iter);
	supplied_iter->destroy(supplied_iter);
	
	return NULL;
}

/**
 * Implementation of sa_config_t.add_my_traffic_selector
 */
static void add_my_traffic_selector(private_sa_config_t *this, traffic_selector_t *traffic_selector)
{
	this->my_ts->insert_last(this->my_ts, (void*)traffic_selector);
}

/**
 * Implementation of sa_config_t.add_other_traffic_selector
 */
static void add_other_traffic_selector(private_sa_config_t *this, traffic_selector_t *traffic_selector)
{
	this->other_ts->insert_last(this->other_ts, (void*)traffic_selector);
}

/**
 * Implementation of sa_config_t.add_proposal
 */
static void add_proposal(private_sa_config_t *this, proposal_t *proposal)
{
	this->proposals->insert_last(this->proposals, (void*)proposal);
}

/**
 * Implements sa_config_t.destroy.
 */
static status_t destroy(private_sa_config_t *this)
{	
	proposal_t *proposal;
	traffic_selector_t *traffic_selector;
	
	
	/* delete proposals */
	while(this->proposals->remove_last(this->proposals, (void**)&proposal) == SUCCESS)
	{
		proposal->destroy(proposal);
	}
	this->proposals->destroy(this->proposals);
	
	/* delete traffic selectors */
	while(this->my_ts->remove_last(this->my_ts, (void**)&traffic_selector) == SUCCESS)
	{
		traffic_selector->destroy(traffic_selector);
	}
	this->my_ts->destroy(this->my_ts);
	
	/* delete traffic selectors */
	while(this->other_ts->remove_last(this->other_ts, (void**)&traffic_selector) == SUCCESS)
	{
		traffic_selector->destroy(traffic_selector);
	}
	this->other_ts->destroy(this->other_ts);
	
	/* delete ids */
	this->my_id->destroy(this->my_id);
	this->other_id->destroy(this->other_id);
	
	allocator_free(this);
	return SUCCESS;
}

/*
 * Described in header-file
 */
sa_config_t *sa_config_create(id_type_t my_id_type, char *my_id, id_type_t other_id_type, char *other_id, auth_method_t auth_method, u_int32_t ike_sa_lifetime)
{
	private_sa_config_t *this = allocator_alloc_thing(private_sa_config_t);

	/* public functions */
	this->public.get_my_id = (identification_t*(*)(sa_config_t*))get_my_id;
	this->public.get_other_id = (identification_t*(*)(sa_config_t*))get_other_id;
	this->public.get_auth_method = (auth_method_t(*)(sa_config_t*))get_auth_method;
	this->public.get_ike_sa_lifetime = (u_int32_t(*)(sa_config_t*))get_ike_sa_lifetime;
	this->public.get_my_traffic_selectors = (linked_list_t*(*)(sa_config_t*))get_my_traffic_selectors;
	this->public.select_my_traffic_selectors = (linked_list_t*(*)(sa_config_t*,linked_list_t*))select_my_traffic_selectors;
	this->public.get_other_traffic_selectors = (linked_list_t*(*)(sa_config_t*))get_other_traffic_selectors;
	this->public.select_other_traffic_selectors = (linked_list_t*(*)(sa_config_t*,linked_list_t*))select_other_traffic_selectors;
	this->public.get_proposals = (linked_list_t*(*)(sa_config_t*))get_proposals;
	this->public.select_proposal = (proposal_t*(*)(sa_config_t*,linked_list_t*))select_proposal;
	this->public.add_my_traffic_selector = (void(*)(sa_config_t*,traffic_selector_t*))add_my_traffic_selector;
	this->public.add_other_traffic_selector = (void(*)(sa_config_t*,traffic_selector_t*))add_other_traffic_selector;
	this->public.add_proposal = (void(*)(sa_config_t*,proposal_t*))add_proposal;
	this->public.destroy = (void(*)(sa_config_t*))destroy;
	
	/* apply init values */
	this->my_id = identification_create_from_string(my_id_type, my_id);
	if (this->my_id == NULL)
	{
		allocator_free(this);
		return NULL;
	}
	this->other_id = identification_create_from_string(other_id_type, other_id);
	if (this->my_id == NULL)
	{
		this->other_id->destroy(this->other_id);
		allocator_free(this);
		return NULL;
	}
	
	/* init private members*/
	this->select_traffic_selectors = select_traffic_selectors;
	this->proposals = linked_list_create();
	this->my_ts = linked_list_create();
	this->other_ts = linked_list_create();
	this->auth_method = auth_method;
	this->ike_sa_lifetime = ike_sa_lifetime;

	return (&this->public);
}
