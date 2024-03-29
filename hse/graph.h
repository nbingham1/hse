/*
 * graph.h
 *
 *  Created on: Feb 1, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cover.h>
#include <ucs/variable.h>
#include <petri/graph.h>

#include "state.h"

#ifndef hse_graph_h
#define hse_graph_h

namespace hse
{

using petri::iterator;
using petri::parallel;
using petri::choice;
using petri::sequence;

struct place : petri::place
{
	place();
	place(boolean::cover predicate);
	~place();

	boolean::cover predicate;
	boolean::cover effective;
	boolean::cube mask;
	bool arbiter;

	static place merge(int composition, const place &p0, const place &p1);
};

struct transition : petri::transition
{
	transition(boolean::cover guard = 1, boolean::cover local_action = 1, boolean::cover remote_action = 1);
	~transition();

	boolean::cover guard;
	boolean::cover local_action;
	boolean::cover remote_action;

	transition subdivide(int term) const;

	static transition merge(int composition, const transition &t0, const transition &t1);
	static bool mergeable(int composition, const transition &t0, const transition &t1);

	bool is_infeasible() const;
	bool is_vacuous() const;
};

struct graph : petri::graph<hse::place, hse::transition, petri::token, hse::state>
{
	typedef petri::graph<hse::place, hse::transition, petri::token, hse::state> super;

	graph();
	~graph();

	pair<boolean::cover, boolean::cover> get_guard(petri::iterator a) const;
	vector<vector<int> > get_dependency_tree(petri::iterator a) const;
	vector<int> get_implicant_tree(petri::iterator a) const;
	bool common_arbiter(vector<vector<int> > a, vector<vector<int> > b) const;

	void post_process(const ucs::variable_set &variables, bool proper_nesting = false, bool aggressive = false);
	void check_variables(const ucs::variable_set &variables);
	vector<int> first_assigns();
	vector<int> associated_assigns(vector<int> tokens);
};

}

#endif
