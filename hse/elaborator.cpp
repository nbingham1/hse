/*
 * elaborator.cpp
 *
 *  Created on: Aug 13, 2015
 *      Author: nbingham
 */

#include "elaborator.h"
#include <common/text.h>

namespace hse
{

void elaborate(graph &g, const ucs::variable_set &variables, bool report_progress)
{
	g.parallel_nodes.clear();

	for (int i = 0; i < (int)g.places.size(); i++)
	{
		g.places[i].predicate = boolean::cover();
		g.places[i].effective = boolean::cover();
	}

	hashtable<state, 10000> states;
	vector<simulator> simulations;
	vector<deadlock> deadlocks;
	simulations.reserve(2000);

	if (g.reset.size() > 0)
	{
		for (int i = 0; i < (int)g.reset.size(); i++)
		{
			simulator sim(&g, &variables, g.reset[i]);
			sim.enabled();

			if (states.insert(sim.get_state()))
				simulations.push_back(sim);
		}
	}
	else
	{
		for (int i = 0; i < (int)g.source.size(); i++)
		{
			simulator sim(&g, &variables, g.source[i]);
			sim.enabled();

			if (states.insert(sim.get_state()))
				simulations.push_back(sim);
		}
	}

	int count = 0;
	while (simulations.size() > 0)
	{
		simulator sim = simulations.back();
		simulations.pop_back();
		//simulations.back().merge_errors(sim);

		if (report_progress)
			progress("", ::to_string(count) + " " + ::to_string(simulations.size()) + " " + ::to_string(states.max_bucket_size()) + "/" + ::to_string(states.count) + " " + ::to_string(sim.ready.size()), __FILE__, __LINE__);

		simulations.reserve(simulations.size() + sim.ready.size());
		for (int i = 0; i < (int)sim.ready.size(); i++)
		{
			simulations.push_back(sim);

			simulations.back().fire(i);
			simulations.back().enabled();

			if (!states.insert(simulations.back().get_state()))
				simulations.pop_back();
		}

		if (sim.ready.size() == 0)
		{
			deadlock d = sim.get_state();
			vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
			if (dloc == deadlocks.end() || *dloc != d)
			{
				error("", d.to_string(variables), __FILE__, __LINE__);
				deadlocks.insert(dloc, d);
			}

			simulations.back().merge_errors(sim);
		}

		// Figure out which nodes are in parallel
		vector<vector<int> > tokens(1, vector<int>());
		vector<vector<int> > visited;
		for (int i = 0; i < (int)sim.tokens.size(); i++)
			if (sim.tokens[i].cause < 0)
				tokens[0].push_back(i);

		while (tokens.size() > 0)
		{
			vector<int> curr = tokens.back();
			sort(curr.begin(), curr.end());
			tokens.pop_back();

			vector<vector<int> >::iterator loc = lower_bound(visited.begin(), visited.end(), curr);
			if (loc == visited.end())
			{
				vector<int> enabled_loaded;
				for (int i = 0; i < (int)sim.loaded.size(); i++)
				{
					bool ready = true;
					for (int j = 0; j < (int)sim.loaded[i].tokens.size() && ready; j++)
						if (find(curr.begin(), curr.end(), sim.loaded[i].tokens[j]) == curr.end())
							ready = false;

					if (ready)
						enabled_loaded.push_back(i);
				}

				for (int i = 0; i < (int)curr.size(); i++)
				{
					for (int j = i+1; j < (int)curr.size(); j++)
					{
						if (sim.tokens[curr[i]].index <= sim.tokens[curr[j]].index)
							g.parallel_nodes.push_back(pair<petri::iterator, petri::iterator>(petri::iterator(petri::place::type, sim.tokens[curr[i]].index), petri::iterator(petri::place::type, sim.tokens[curr[j]].index)));
						else
							g.parallel_nodes.push_back(pair<petri::iterator, petri::iterator>(petri::iterator(petri::place::type, sim.tokens[curr[j]].index), petri::iterator(petri::place::type, sim.tokens[curr[i]].index)));
					}

					for (int j = 0; j < (int)enabled_loaded.size(); j++)
						if (find(sim.loaded[enabled_loaded[j]].tokens.begin(), sim.loaded[enabled_loaded[j]].tokens.end(), curr[i]) == sim.loaded[enabled_loaded[j]].tokens.end())
							g.parallel_nodes.push_back(pair<petri::iterator, petri::iterator>(petri::iterator(petri::place::type, sim.tokens[curr[i]].index), petri::iterator(petri::transition::type, sim.loaded[enabled_loaded[j]].index)));
				}

				for (int i = 0; i < (int)enabled_loaded.size(); i++)
				{
					tokens.push_back(curr);
					for (int j = (int)tokens.back().size()-1; j >= 0; j--)
						if (find(sim.loaded[enabled_loaded[i]].tokens.begin(), sim.loaded[enabled_loaded[i]].tokens.end(), tokens.back()[j]) != sim.loaded[enabled_loaded[i]].tokens.end())
							tokens.back().erase(tokens.back().begin() + j);

					tokens.back().insert(tokens.back().end(), sim.loaded[enabled_loaded[i]].output_marking.begin(), sim.loaded[enabled_loaded[i]].output_marking.end());
				}

				visited.insert(loc, curr);
			}
		}

		/* The effective predicate represents the state encodings that don't have duplicates
		 * in later states.
		 *
		 * I have to loop through all of the enabled transitions, and then loop through all orderings
		 * of their dependent guards, saving the state
		 *
		 * TODO check the wchb
		 */
		vector<set<int> > en_in(sim.tokens.size(), set<int>());
		vector<set<int> > en_out(sim.tokens.size(), set<int>());

		bool change = true;
		while (change)
		{
			change = false;
			for (int i = 0; i < (int)sim.loaded.size(); i++)
			{
				set<int> total_in;
				for (int j = 0; j < (int)sim.loaded[i].tokens.size(); j++)
					total_in.insert(en_in[sim.loaded[i].tokens[j]].begin(), en_in[sim.loaded[i].tokens[j]].end());
				total_in.insert(sim.loaded[i].index);

				for (int j = 0; j < (int)sim.loaded[i].output_marking.size(); j++)
				{
					set<int> old_in = en_in[sim.loaded[i].output_marking[j]];
					en_in[sim.loaded[i].output_marking[j]].insert(total_in.begin(), total_in.end());
					if (en_in[sim.loaded[i].output_marking[j]] != old_in)
						change = true;
				}
			}
		}

		for (int i = 0; i < (int)sim.loaded.size(); i++)
			for (int j = 0; j < (int)sim.loaded[i].tokens.size(); j++)
				en_out[sim.loaded[i].tokens[j]].insert(sim.loaded[i].index);

		for (int i = 0; i < (int)sim.tokens.size(); i++)
		{
			boolean::cover en = 1;
			boolean::cover dis = 1;
			for (set<int>::iterator j = en_in[i].begin(); j != en_in[i].end(); j++)
				en &= g.transitions[*j].local_action;

			for (set<int>::iterator j = en_out[i].begin(); j != en_out[i].end(); j++)
				dis &= ~g.transitions[*j].local_action;

			g.places[sim.tokens[i].index].predicate |= (sim.encoding.xoutnulls() & en).flipped_mask(g.places[sim.tokens[i].index].mask);
			g.places[sim.tokens[i].index].effective |= (sim.encoding.xoutnulls() & en & dis).flipped_mask(g.places[sim.tokens[i].index].mask);
		}

		count++;
	}

	if (report_progress)
		done_progress();

	sort(g.parallel_nodes.begin(), g.parallel_nodes.end());
	g.parallel_nodes.resize(unique(g.parallel_nodes.begin(), g.parallel_nodes.end()) - g.parallel_nodes.begin());
	g.parallel_nodes_ready = true;

	for (int i = 0; i < (int)g.places.size(); i++)
	{
		if (report_progress)
			progress("", "Espresso " + ::to_string(i) + "/" + ::to_string(g.places.size()), __FILE__, __LINE__);

		g.places[i].effective.espresso();
		sort(g.places[i].effective.cubes.begin(), g.places[i].effective.cubes.end());
		g.places[i].predicate.espresso();
	}

	if (report_progress)
		done_progress();
}

/** to_state_graph()
 *
 * This converts a given graph to the fully expanded state space through simulation. It systematically
 * simulates all possible transition orderings and determines all of the resulting state information.
 */
graph to_state_graph(const graph &g, const ucs::variable_set &variables, bool report_progress)
{
	graph result;
	hashmap<state, petri::iterator, 10000> states;
	vector<pair<simulator, petri::iterator> > simulations;
	vector<deadlock> deadlocks;

	if (g.reset.size() > 0)
	{
		for (int i = 0; i < (int)g.reset.size(); i++)
		{
			simulator sim(&g, &variables, g.reset[i]);
			sim.enabled();

			map<state, petri::iterator>::iterator loc;
			if (states.insert(sim.get_key(), petri::iterator(), &loc))
			{
				loc->second = result.create(place(loc->first.encodings));

				// Set up the initial state which is determined by the reset behavior
				result.reset.push_back(state(vector<petri::token>(1, petri::token(loc->second.index)), g.reset[i].encodings));

				// Set up the first simulation that starts at the reset state
				simulations.push_back(pair<simulator, petri::iterator>(sim, loc->second));
			}
		}
	}
	else
	{
		for (int i = 0; i < (int)g.source.size(); i++)
		{
			simulator sim(&g, &variables, g.source[i]);
			sim.enabled();

			map<state, petri::iterator>::iterator loc;
			if (states.insert(sim.get_key(), petri::iterator(), &loc))
			{
				loc->second = result.create(place(loc->first.encodings));

				// Set up the initial state which is determined by the reset behavior
				result.reset.push_back(state(vector<petri::token>(1, petri::token(loc->second.index)), g.source[i].encodings));

				// Set up the first simulation that starts at the reset state
				simulations.push_back(pair<simulator, petri::iterator>(sim, loc->second));
			}
		}
	}

	int count = 0;
	while (simulations.size() > 0)
	{
		pair<simulator, petri::iterator> sim = simulations.back();
		simulations.pop_back();
		simulations.back().first.merge_errors(sim.first);

		if (report_progress)
			progress("", ::to_string(count) + " " + ::to_string(simulations.size()) + " " + ::to_string(states.max_bucket_size()) + "/" + ::to_string(states.count) + " " + ::to_string(sim.first.ready.size()), __FILE__, __LINE__);

		simulations.reserve(simulations.size() + sim.first.ready.size());
		for (int i = 0; i < (int)sim.first.ready.size(); i++)
		{
			simulations.push_back(sim);
			int index = simulations.back().first.loaded[simulations.back().first.ready[i].first].index;
			int term = simulations.back().first.ready[i].second;

			simulations.back().first.fire(i);
			simulations.back().first.enabled();

			map<state, petri::iterator>::iterator loc;
			if (states.insert(simulations.back().first.get_key(), petri::iterator(), &loc))
			{
				loc->second = result.create(place(loc->first.encodings));
				petri::iterator trans = result.create(g.transitions[index].subdivide(term));
				result.connect(simulations.back().second, trans);
				result.connect(trans, loc->second);
				simulations.back().second = loc->second;
			}
			else
			{
				petri::iterator trans = result.create(g.transitions[index].subdivide(term));
				result.connect(simulations.back().second, trans);
				result.connect(trans, loc->second);
				simulations.pop_back();
			}
		}

		if (sim.first.ready.size() == 0)
		{
			deadlock d = sim.first.get_state();
			result.sink.push_back(state(vector<petri::token>(1, petri::token(sim.second.index)), d.encodings));
			vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
			if (dloc == deadlocks.end() || *dloc != d)
			{
				error("", d.to_string(variables), __FILE__, __LINE__);
				deadlocks.insert(dloc, d);
			}

			simulations.back().first.merge_errors(sim.first);
		}

		count++;
	}

	for (int i = 0; i < (int)g.source.size(); i++)
	{
		simulator sim(&g, &variables, g.source[i]);
		sim.enabled();

		map<state, petri::iterator>::iterator loc;
		if (states.insert(sim.get_key(), petri::iterator(), &loc))
			loc->second = result.create(place(loc->first.encodings));

		// Set up the initial state which is determined by the reset behavior
		result.source.push_back(state(vector<petri::token>(1, petri::token(loc->second.index)), g.source[i].encodings));
	}

	if (report_progress)
		done_progress();

	return result;
}

struct half_synchronization
{
	half_synchronization()
	{
		active.index = 0;
		active.cube = 0;
		passive.index = 0;
		passive.cube = 0;
	}

	~half_synchronization()
	{

	}

	struct
	{
		int index;
		int cube;
	} active, passive;
};

struct synchronization_region
{
	synchronization_region()
	{

	}

	~synchronization_region()
	{

	}

	boolean::cover action;
	vector<petri::iterator> nodes;
};

/* to_petri_net()
 *
 * Converts the HSE into a petri net using index-priority simulation.
 */
graph to_petri_net(const graph &g, const ucs::variable_set &variables, bool report_progress)
{
	graph result = g;

	// Petri nets don't support internal parallelism or choice, so we have to expand those transitions.
	for (petri::iterator i(petri::transition::type, result.transitions.size()-1); i >= 0; i--)
	{
		if (result.transitions[i.index].behavior == transition::active && result.transitions[i.index].local_action.cubes.size() > 1)
		{
			vector<petri::iterator> k = result.duplicate(petri::choice, i, result.transitions[i.index].local_action.cubes.size(), false);
			for (int j = 0; j < (int)k.size(); j++)
				result.transitions[k[j].index].local_action = boolean::cover(result.transitions[k[j].index].local_action.cubes[j]);
		}
	}

	for (petri::iterator i(petri::transition::type, result.transitions.size()-1); i >= 0; i--)
	{
		if (result.transitions[i.index].behavior == transition::active && result.transitions[i.index].local_action.cubes.size() == 1)
		{
			vector<int> vars = result.transitions[i.index].local_action.cubes[0].vars();
			if (vars.size() > 1)
			{
				vector<petri::iterator> k = result.duplicate(petri::parallel, i, vars.size(), false);
				for (int j = 0; j < (int)k.size(); j++)
					result.transitions[k[j].index].local_action.cubes[0] = boolean::cube(vars[j], result.transitions[k[j].index].local_action.cubes[0].get(vars[j]));
			}
		}
	}

	// Find all of the guards
	vector<petri::iterator> passive;
	for (petri::iterator i(petri::transition::type, result.transitions.size()-1); i >= 0; i--)
		if (result.transitions[i.index].behavior == transition::passive)
			passive.push_back(i);

	vector<half_synchronization> synchronizations;

	half_synchronization sync;
	for (sync.passive.index = 0; sync.passive.index < (int)result.transitions.size(); sync.passive.index++)
		if (result.transitions[sync.passive.index].behavior == transition::passive && !result.transitions[sync.passive.index].local_action.is_tautology())
			for (sync.active.index = 0; sync.active.index < (int)result.transitions.size(); sync.active.index++)
				if (result.transitions[sync.active.index].behavior == transition::active && !result.transitions[sync.active.index].local_action.is_tautology())
					for (sync.passive.cube = 0; sync.passive.cube < (int)result.transitions[sync.passive.index].local_action.cubes.size(); sync.passive.cube++)
						for (sync.active.cube = 0; sync.active.cube < (int)result.transitions[sync.active.index].local_action.cubes.size(); sync.active.cube++)
							if (similarity_g0(result.transitions[sync.active.index].local_action.cubes[sync.active.cube], result.transitions[sync.passive.index].local_action.cubes[sync.passive.cube]))
								synchronizations.push_back(sync);

	/* Implement each guard in the petri net. As of now, this is done
	 * syntactically meaning that a few assumptions are made. First, if
	 * there are two transitions that affect the same variable in a guard,
	 * then they must be composed in condition. If they are not, then the
	 * net will not be 1-bounded. Second, you cannot have two guards on
	 * separate conditional branches that both have a cube which checks the
	 * same value for the same variable. Once again, if you do, the net
	 * will not be one bounded. There are probably other issues that I haven't
	 * uncovered yet, but just beware.
	 */
	for (int i = 0; i < (int)passive.size(); i++)
	{
		if (!result.transitions[passive[i].index].local_action.is_tautology())
		{
			// Find all of the half synchronization local_actions for this guard and group them accordingly:
			// outside group is by disjunction, middle group is by conjunction, and inside group is by value and variable.
			vector<vector<vector<petri::iterator> > > sync;
			sync.resize(result.transitions[passive[i].index].local_action.cubes.size());
			for (int j = 0; j < (int)synchronizations.size(); j++)
				if (synchronizations[j].passive.index == passive[i].index)
				{
					bool found = false;
					for (int l = 0; l < (int)sync[synchronizations[j].passive.cube].size(); l++)
						if (similarity_g0(result.transitions[sync[synchronizations[j].passive.cube][l][0].index].local_action.cubes[0], result.transitions[synchronizations[j].active.index].local_action.cubes[0]))
						{
							found = true;
							sync[synchronizations[j].passive.cube][l].push_back(petri::iterator(petri::transition::type, synchronizations[j].active.index));
						}

					if (!found)
						sync[synchronizations[j].passive.cube].push_back(vector<petri::iterator>(1, petri::iterator(petri::transition::type, synchronizations[j].active.index)));
				}

			/* Finally implement the connections. The basic observation being that a guard affects when the next
			 * transition can fire.
			 */
			vector<petri::iterator> n = result.next(passive[i]);
			vector<petri::iterator> g = result.create(place(), n.size());
			for (int j = 0; j < (int)n.size(); j++)
				result.connect(g[j], result.next(n[j]));

			for (int j = 0; j < (int)sync.size(); j++)
			{
				petri::iterator t = result.create(transition());
				result.connect(t, g);
				for (int k = 0; k < (int)sync[j].size(); k++)
				{
					petri::iterator p = result.create(place());
					result.connect(p, t);
					result.connect(sync[j][k], p);
				}
			}
		}
	}

	// Remove the guards from the graph
	for (int i = 0; i < (int)passive.size(); i++)
	{
		result.transitions[passive[i].index].local_action = 1;
		result.transitions[passive[i].index].remote_action = 1;
	}
	/*vector<petri::iterator> n = result.next(passive), p = result.prev(passive);
	passive.insert(passive.end(), n.begin(), n.end());
	passive.insert(passive.end(), p.begin(), p.end());
	sort(passive.begin(), passive.end());
	passive.resize(unique(passive.begin(), passive.end()) - passive.begin());

	result.erase(passive);*/

	synchronizations.clear();

	// Clean up
	result.post_process(variables, false);

	return result;
}

}