/*
 * simulator.cpp
 *
 *  Created on: Apr 28, 2015
 *      Author: nbingham
 */

#include "simulator.h"
#include "graph.h"
#include <common/text.h>
#include <common/message.h>
#include <ucs/variable.h>
#include <interpret_boolean/export.h>

namespace hse
{

instability::instability()
{
}

instability::instability(const enabled_transition &cause) : enabled_transition(cause)
{
}

instability::~instability()
{

}

string instability::to_string(const hse::graph &g, const ucs::variable_set &v)
{
	string result;
	result = "unstable rule " + enabled_transition::to_string(g, v);

	result += " cause: {";

	for (int j = 0; j < (int)history.size(); j++)
	{
		if (j != 0)
			result += "; ";

		result += history[j].to_string(g, v);
	}
	result += "}";
	return result;
}

interference::interference()
{

}

interference::interference(const term_index &first, const term_index &second)
{
	if (first < second)
	{
		this->first = first;
		this->second = second;
	}
	else
	{
		this->first = second;
		this->second = first;
	}
}

interference::~interference()
{

}

string interference::to_string(const hse::graph &g, const ucs::variable_set &v)
{
	return "interfering assignments " + first.to_string(g, v) + " and " + second.to_string(g, v);
}

mutex::mutex()
{

}

mutex::mutex(const enabled_transition &first, const enabled_transition &second)
{
	if (first < second)
	{
		this->first = first;
		this->second = second;
	}
	else
	{
		this->first = second;
		this->second = first;
	}
}

mutex::~mutex()
{

}

string mutex::to_string(const hse::graph &g, const ucs::variable_set &v)
{
	return "non-exclusive guards in deterministic selection for assignments " + first.to_string(g, v) + " and " + second.to_string(g, v);
}

deadlock::deadlock()
{

}

deadlock::deadlock(const state &s) : state(s)
{

}

deadlock::deadlock(vector<token> tokens, boolean::cube encodings) : state(tokens, encodings)
{

}

deadlock::~deadlock()
{

}

string deadlock::to_string(const ucs::variable_set &v)
{
	return "deadlock detected at state " + state::to_string(v);
}

simulator::simulator()
{
	base = NULL;
	variables = NULL;
}

simulator::simulator(const graph *base, const ucs::variable_set *variables, state initial)
{
	this->base = base;
	this->variables = variables;
	if (base != NULL)
	{
		encoding = initial.encodings;
		global = initial.encodings;
		for (int k = 0; k < (int)initial.tokens.size(); k++) {
			tokens.push_back(hse::token(initial.tokens[k].index));
		}
	}
}

simulator::~simulator()
{

}

// Returns a vector of indices representing the transitions
// that this marking enabled and the term of each transition
// that's enabled.
int simulator::enabled(bool sorted)
{
	if (base == NULL)
	{
		internal("", "NULL pointer to simulator::base", __FILE__, __LINE__);
		return 0;
	}

	if (tokens.size() == 0)
		return 0;

	// Get the list of transitions have have a sufficient number of local at the input places
	vector<enabled_transition> preload;

	/*for (int i = 0; i < (int)loaded.size(); i++)
	{
		cout << "Loaded " << i << ": " << loaded[i].index << " " << (loaded[i].vacuous ? "vacuous" : "") << " {";
		for (int j = 0; j < (int)loaded[i].tokens.size(); j++)
			cout << loaded[i].tokens[j] << " ";
		cout << "}" << endl;
	}

	cout << endl;*/

	vector<int> disabled;
	for (vector<petri::arc>::const_iterator a = base->arcs[place::type].begin(); a != base->arcs[place::type].end(); a++) {
		// Check to see if we haven't already determined that this transition can't be enabled
		vector<int>::iterator d = lower_bound(disabled.begin(), disabled.end(), a->to.index);
		if (d == disabled.end() || *d != a->to.index)
		{
			// Find the index of this transition (if any) in the loaded pool
			bool loaded_found = false;
			for (int i = (int)preload.size()-1; i >= 0; i--) {
				if (preload[i].index == a->to.index) {
					loaded_found = true;

					// Check to see if there is any token at the input place of this arc and make sure that
					// this token has not already been consumed by this particular transition
					vector<int> matching_tokens;
					for (int j = 0; j < (int)tokens.size(); j++) {
						if (a->from.index == tokens[j].index) {
							// We have to implement a recursive...ish algorithm here
							// to check to see if this token has already been used by
							// any of the transitions in the chain
							bool used = false;
							vector<int> test = preload[i].tokens;
							for (int k = 0; k < (int)test.size() && !used; k++) {
								used = (test[k] == j);
							}

							if (!used)
								matching_tokens.push_back(j);
						}
					}

					// there might be more than one matching token
					for (int j = (int)matching_tokens.size()-1; j >= 1; j--)
					{
						preload.push_back(preload[i]);
						preload.back().tokens.push_back(matching_tokens[j]);
					}

					if ((int)matching_tokens.size() > 0)
						preload[i].tokens.push_back(matching_tokens[0]);
					else
					{
						// If we didn't find a token at the input place, then we know that this transition can't
						// be enabled. So lets remove this from the list of possibly enabled transitions
						disabled.insert(d, a->to.index);
						preload.erase(preload.begin() + i);
					}
				}
			}

			// If we didn't find this transition in the loaded list, then this is our
			// first encounter with this transition being possibly enabled for all of the
			// previous iterations. We need to add it to the loaded list.
			if (!loaded_found) {
				bool token_found = false;
				for (int j = 0; j < (int)tokens.size(); j++)
					if (a->from.index == tokens[j].index)
					{
						token_found = true;
						preload.push_back(enabled_transition(a->to.index));
						preload.back().tokens.push_back(j);
					}

				if (!token_found)
					disabled.insert(d, a->to.index);
			}
		}
	}

	/*for (int i = 0; i < (int)preload.size(); i++)
	{
		cout << "Preload " << i << ": " << preload[i].index << " " << (preload[i].vacuous ? "vacuous" : "") << " {";
		for (int j = 0; j < (int)preload[i].tokens.size(); j++)
			cout << preload[i].tokens[j] << " ";
		cout << "}" << endl;
	}

	cout << endl;*/


	bool has_vacuous = false;
	// Now we need to check the guards of all of the loaded transitions against the state
	for (int i = (int)preload.size()-1; i >= 0; i--)
	{
		// If this transition is a guard, we check to see if it passes (either stable or unstable)
		// if it is an assignment, then we check to see if it is vacuous. If either of these hold
		// true, then we can keep going.

		// If this transition is an assignment, then we need to check to make sure all of the
		// previous guards pass before we can execute this assignment.
		// the 'guard' expression contains all of the guards since the last assignments in the process
		// the 'sequence' expression contains all of the last assignments in the process. A dependency chain
		// from these assignments to our current assignment is necessary to implement sequentialism. So
		// Conservatively the assignments will always end up in the production rules for that purpose.
		// However, we can do better by finding those dependency chains in the guard and only taking the expressions
		// from the end of those chains.
		for (int j = 0; j < (int)preload[i].tokens.size(); j++) {
			preload[i].sequence &= tokens[preload[i].tokens[j]].sequence;
		}

		// TODO removing prs timing assumption
		preload[i].guard &= base->transitions[preload[i].index].guard;
		boolean::cover guard = preload[i].guard;
		
		//cout << "Starting Sequence: " << export_expression(preload[i].sequence, *variables).to_string() << endl;
		//cout << "Starting Rule: " << export_expression(preload[i].guard, *variables).to_string() << " -> " << export_composition(base->transitions[preload[i].index].local_action, *variables).to_string() << endl;

		/* TODO(edward.bingham) fix this before merging to master
		for (int j = 0; j < (int)preload[i].guard.cubes.size(); j++)
		{
			for (int k = 0; k < (int)preload[i].sequence.cubes.size(); k++)
			{
				//cout << "\tTerm " << j << "." << k << endl;
				boolean::cube covered = preload[i].guard.cubes[j];
				//cout << "\t\t" << export_expression(covered, *variables).to_string() << endl;
				for (list<pair<boolean::cube, term_index> >::reverse_iterator l = history.rbegin(); l != history.rend(); l++)
				{
					if ((covered & base->transitions[l->second.index].local_action[l->second.term].inverse().flipped_mask(covered.mask().flip())).is_null())
					{
						covered &= (l->first.mask(covered.mask()) & preload[i].sequence.cubes[k].flipped_mask(l->first.mask().flip())).xoutnulls().mask(base->transitions[preload[i].index].local_action[0].mask());
						//cout << "\t\t" << export_expression(covered, *variables).to_string() << endl;
					}
				}

				guard.cubes.push_back(preload[i].guard.cubes[j] & preload[i].sequence.cubes[k].mask(covered.mask()).mask(base->transitions[preload[i].index].local_action[0].mask()));
				//cout << "\t" << export_expression(guard.cubes.back(), *variables).to_string() << endl;
			}
		}*/
		//cout << "Ending Guard: " << export_expression(guard, *variables).to_string() << endl;

		// Check for unstable transitions
		bool previously_enabled = false;
		for (int j = 0; j < (int)loaded.size() && !previously_enabled; j++)
			if (loaded[j].index == preload[i].index)
			{
				preload[i].history = loaded[j].history;
				previously_enabled = true;
			}

		// Now we check to see if the current state passes the guard
		int isReady = boolean::passes_guard(encoding, global, guard, &preload[i].guard_action);
		if (isReady < 0 && previously_enabled)
			isReady = 0;

		preload[i].stable = (isReady > 0);
		preload[i].vacuous = boolean::vacuous_assign(global, base->transitions[preload[i].index].remote_action, preload[i].stable);
		preload[i].stable = preload[i].stable || preload[i].vacuous;

		if (isReady < 0) {
			/*cout << "Didn't Pass Guard " << i << ": " << preload[i].index << " " << (preload[i].vacuous ? "vacuous" : "") << " " << (!preload[i].stable ? "unstable" : "") << " {";
			for (int j = 0; j < (int)preload[i].tokens.size(); j++)
				cout << preload[i].tokens[j] << " ";
			cout << "}" << endl;*/

			preload.erase(preload.begin() + i);
		} else {
			has_vacuous = (has_vacuous or preload[i].vacuous);
		}
	}

	/*for (int i = 0; i < (int)preload.size(); i++)
	{
		cout << "Preload " << i << ": " << preload[i].index << " " << (preload[i].vacuous ? "vacuous" : "") << " {";
		for (int j = 0; j < (int)preload[i].tokens.size(); j++)
			cout << preload[i].tokens[j] << " ";
		cout << "}" << endl;
	}

	cout << endl;*/


	loaded = preload;
	ready.clear();

	for (int i = 0; i < (int)loaded.size(); i++) {
		if (!has_vacuous or (has_vacuous and loaded[i].vacuous)) {
			for (int j = 0; j < (int)base->transitions[loaded[i].index].local_action.cubes.size(); j++) {
				ready.push_back(pair<int, int>(i, j));
			}
		}
	}

	if (history.size() > 0)
		for (int i = 0; i < (int)loaded.size(); i++)
			if (loaded[i].stable)
				loaded[i].history.push_back(history.back().second);

	/*for (int i = 0; i < (int)tokens.size(); i++)
		cout << "Token " << i << ": " << tokens[i].index << " " << tokens[i].cause << " " << tokens[i].guard << endl;

	cout << endl;

	for (int i = 0; i < (int)loaded.size(); i++)
	{
		cout << "Loaded " << i << ": " << loaded[i].index << " " << (loaded[i].vacuous ? "vacuous" : "") << " {";
		for (int j = 0; j < (int)loaded[i].tokens.size(); j++)
			cout << loaded[i].tokens[j] << " ";
		cout << "}" << endl;
	}

	cout << endl;*/

	// TODO I don't know how I would treat the case when the execution enters a state that is not part of the HSE.
	// In this case, all of the transitions in the entire HSE are vacuous (possibly unstable) and the tokens
	// could just zoom around the HSE making no changes to the state information.

	return ready.size();
}

enabled_transition simulator::fire(int index)
{
	if (base == NULL)
	{
		internal("", "NULL pointer to simulator::base", __FILE__, __LINE__);
		return enabled_transition();
	}

	enabled_transition t = loaded[ready[index].first];
	int term = ready[index].second;

	// disable any transitions that were dependent on at least one of the same local tokens
	// This is only necessary to check for unstable transitions in the enabled() function
	for (int i = (int)loaded.size()-1, j = ready.size()-1; i >= 0; i--)
	{
		vector<int> intersect = vector_intersection(loaded[i].tokens, t.tokens);
		if (intersect.size() > 0)
		{
			// ASSUME ready array is sorted in ascending order
			bool is_effective = false;
			for (; j >= 0 && !is_effective; j--)
				is_effective = (ready[j].first == i);

			bool is_deterministic = true;
			for (int k = 0; k < (int)intersect.size() && is_deterministic; k++)
				is_deterministic = !base->places[tokens[intersect[k]].index].arbiter;

			if (is_effective && is_deterministic && loaded[i].index != t.index)
			{
				cout << "Intersect: (";
				for (int l = 0; l < (int)intersect.size(); l++)
					cout << tokens[intersect[l]].index << " ";
				cout << ")";
				cout << "Arbiters: (";
				for (int l = 0; l < (int)intersect.size(); l++) {
					if (base->places[tokens[intersect[l]].index].arbiter) {
						cout << tokens[intersect[l]].index << " ";
					}
				}
				cout << ")";
				mutex err = mutex(t, loaded[i]);
				vector<mutex>::iterator loc = lower_bound(mutex_errors.begin(), mutex_errors.end(), err);
				if (loc == mutex_errors.end() || *loc != err)
				{
					mutex_errors.insert(loc, err);
					error("", err.to_string(*base, *variables), __FILE__, __LINE__);
				}
			}

			loaded.erase(loaded.begin() + i);
		}
	}

	ready.clear();

	// Check to see if this transition is unstable
	if (!t.stable)
	{
		instability err = instability(t);
		vector<instability>::iterator loc = lower_bound(instability_errors.begin(), instability_errors.end(), err);
		if (loc == instability_errors.end() || *loc != err)
		{
			instability_errors.insert(loc, err);
			error("", err.to_string(*base, *variables), __FILE__, __LINE__);
		}
	}

	// Update the tokens
	sort(t.tokens.begin(), t.tokens.end());
	for (int i = t.tokens.size()-1; i >= 0; i--)
		tokens.erase(tokens.begin() + t.tokens[i]);
	
	for (vector<petri::arc>::const_iterator a = base->arcs[transition::type].begin(); a != base->arcs[transition::type].end(); a++) {
		if (a->from.index == t.index) {
			tokens.push_back(token(a->to.index, base->transitions[t.index].local_action[term]));
		}
	}

	// Check for interfering transitions. Interfering transitions are the active transitions that have fired since this
	// active transition was enabled.
	boolean::cube local_action = base->transitions[t.index].local_action[term];
	boolean::cube remote_action = base->transitions[t.index].remote_action[term];
	for (int j = 0; j < (int)t.history.size(); j++) {
		if (boolean::are_mutex(base->transitions[t.index].remote_action[term], base->transitions[t.history[j].index].local_action[t.history[j].term]))
		{
			interference err(term_index(t.index, term), t.history[j]);
			vector<interference>::iterator loc = lower_bound(interference_errors.begin(), interference_errors.end(), err);
			if (loc == interference_errors.end() || *loc != err)
			{
				interference_errors.insert(loc, err);
				error("", err.to_string(*base, *variables), __FILE__, __LINE__);
			}
		}

		local_action = boolean::interfere(local_action, base->transitions[t.history[j].index].remote_action[t.history[j].term]);
		remote_action = boolean::interfere(remote_action, base->transitions[t.history[j].index].remote_action[t.history[j].term]);
	}

	// Update the state
	if (t.stable)
	{
		global &= t.guard_action;
		encoding &= t.guard_action;
	}

	global = local_assign(global, remote_action, t.stable);
	encoding = remote_assign(local_assign(encoding, local_action, t.stable), global, true);

	// Update the history. The first thing we need to do is remove any assignments that no longer
	// have any effect on the global state. So we remove history items where all of the terms
	// in their assignments are conflicting with terms in more recent assignments.
	boolean::cube actions = base->transitions[t.index].local_action.cubes[term].mask();
	for (list<pair<boolean::cube, term_index> >::reverse_iterator i = history.rbegin(); i != history.rend();)
	{
		if (base->transitions[i->second.index].local_action.cubes[i->second.term].mask(actions).is_tautology())
		{
			i++;
			i = list<pair<boolean::cube, term_index> >::reverse_iterator(history.erase(i.base()));
		}
		else
		{
			actions = actions.combine_mask(base->transitions[i->second.index].local_action.cubes[i->second.term].mask());
			i++;
		}
	}

	// Add the latest firing to the history.
	history.push_back(pair<boolean::cube, term_index>(t.guard_action, term_index(t.index, term)));

	/*cout << "History" << endl;
	actions = 0;
	for (list<pair<boolean::cube, term_index> >::reverse_iterator i = history.rbegin(); i != history.rend(); i++)
	{
		cout << export_expression(i->first, *variables).to_string() << " -> " << export_composition(base->transitions[i->second.index].local_action.cubes[i->second.term].mask(actions), *variables).to_string() << endl;
		actions = actions.combine_mask(base->transitions[i->second.index].local_action.cubes[i->second.term].mask());
	}*/

	return t;
}

void simulator::merge_errors(const simulator &sim)
{
	if (instability_errors.size() > 0)
	{
		vector<instability> old_instability_errors;
		swap(instability_errors, old_instability_errors);
		instability_errors.resize(old_instability_errors.size() + sim.instability_errors.size());
		merge(sim.instability_errors.begin(), sim.instability_errors.end(), old_instability_errors.begin(), old_instability_errors.end(), instability_errors.begin());
		instability_errors.resize(unique(instability_errors.begin(), instability_errors.end()) - instability_errors.begin());
	}
	else
		instability_errors = sim.instability_errors;

	if (interference_errors.size() > 0)
	{
		vector<interference> old_interference_errors;
		swap(interference_errors, old_interference_errors);
		interference_errors.resize(sim.interference_errors.size() + old_interference_errors.size());
		merge(sim.interference_errors.begin(), sim.interference_errors.end(), old_interference_errors.begin(), old_interference_errors.end(), interference_errors.begin());
		interference_errors.resize(unique(interference_errors.begin(), interference_errors.end()) - interference_errors.begin());
	}
	else
		interference_errors = sim.interference_errors;

	if (mutex_errors.size() > 0)
	{
		vector<mutex> old_mutex_errors;
		swap(mutex_errors, old_mutex_errors);
		mutex_errors.resize(sim.mutex_errors.size() + old_mutex_errors.size());
		merge(sim.mutex_errors.begin(), sim.mutex_errors.end(), old_mutex_errors.begin(), old_mutex_errors.end(), mutex_errors.begin());
		mutex_errors.resize(unique(mutex_errors.begin(), mutex_errors.end()) - mutex_errors.begin());
	}
	else
		mutex_errors = sim.mutex_errors;
}

state simulator::get_state()
{
	state result;
	result.encodings = encoding;
	for (int i = 0; i < (int)tokens.size(); i++)
		result.tokens.push_back(tokens[i]);
	sort(result.tokens.begin(), result.tokens.end());
	return result;
}

state simulator::get_key()
{
	state result;
	result.encodings = encoding;
	for (int i = 0; i < (int)ready.size(); i++)
		result.tokens.push_back(petri::token(loaded[ready[i].first].index));
	sort(result.tokens.begin(), result.tokens.end());
	result.tokens.resize(unique(result.tokens.begin(), result.tokens.end()) - result.tokens.begin());
	return result;
}

vector<pair<int, int> > simulator::get_choices()
{
	vector<pair<int, int> > result;
	vector<vector<int> > tree;
	vector<vector<int> > visited;

	for (int i = 0; i < (int)loaded.size(); i++)
	{
		tree.push_back(loaded[i].tokens);
		visited.push_back(vector<int>(1, i));
	}

	for (int i = 0; i < (int)tree.size(); i++)
	{
		sort(tree[i].begin(), tree[i].end());
		tree[i].resize(unique(tree[i].begin(), tree[i].end()) - tree[i].begin());
		sort(visited[i].begin(), visited[i].end());
		visited[i].resize(unique(visited[i].begin(), visited[i].end()) - visited[i].begin());
	}

	vector<int> lindices;
	for (int i = 0; i < (int)ready.size(); i++)
		lindices.push_back(ready[i].first);
	sort(lindices.begin(), lindices.end());
	lindices.resize(unique(lindices.begin(), lindices.end()) - lindices.begin());

	for (int i = 0; i < (int)lindices.size(); i++)
		for (int j = i+1; j < (int)lindices.size(); j++)
		{
			vector<int> temp = loaded[lindices[j]].tokens;
			sort(temp.begin(), temp.end());
			temp.resize(unique(temp.begin(), temp.end()) - temp.begin());

			if (vector_intersects(tree[lindices[i]], temp))
				result.push_back(pair<int, int>(lindices[i], lindices[j]));
		}

	return result;
}

}
