/*
 * graph.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "graph.h"
#include "simulator.h"
#include <common/message.h>
#include <common/text.h>
#include <interpret_boolean/export.h>

namespace hse
{
graph::graph()
{
}

graph::~graph()
{

}

iterator graph::begin(int type)
{
	return iterator(type, 0);
}

iterator graph::end(int type)
{
	return iterator(type, type == place::type ? (int)places.size() : (int)transitions.size());
}

iterator graph::begin_arc(int type)
{
	return iterator(type, 0);
}

iterator graph::end_arc(int type)
{
	return iterator(type, (int)arcs[type].size());
}

iterator graph::create(place p)
{
	places.push_back(p);
	return iterator(place::type, (int)places.size()-1);
}

iterator graph::create(transition t)
{
	transitions.push_back(t);
	return iterator(transition::type, (int)transitions.size()-1);
}

iterator graph::create(int n)
{
	if (n == place::type)
		return create(place());
	else if (n == transition::type)
		return create(transition());
	else
		return iterator();
}

vector<iterator> graph::create(vector<place> p)
{
	vector<iterator> result;
	for (int i = 0; i < (int)p.size(); i++)
	{
		places.push_back(p[i]);
		result.push_back(iterator(place::type, (int)places.size()-1));
	}
	return result;
}

vector<iterator> graph::create(vector<transition> t)
{
	vector<iterator> result;
	for (int i = 0; i < (int)t.size(); i++)
	{
		transitions.push_back(t[i]);
		result.push_back(iterator(transition::type, (int)transitions.size()-1));
	}
	return result;
}

vector<iterator> graph::create(place p, int num)
{
	vector<iterator> result;
	for (int i = 0; i < num; i++)
	{
		places.push_back(p);
		result.push_back(iterator(place::type, (int)places.size()-1));
	}
	return result;
}

vector<iterator> graph::create(transition t, int num)
{
	vector<iterator> result;
	for (int i = 0; i < num; i++)
	{
		transitions.push_back(t);
		result.push_back(iterator(transition::type, (int)transitions.size()-1));
	}
	return result;
}

vector<iterator> graph::create(int n, int num)
{
	if (n == place::type)
		return create(place(), num);
	else if (n == transition::type)
		return create(transition(), num);
	else
		return vector<iterator>();
}

iterator graph::copy(iterator i)
{
	if (i.type == place::type)
	{
		places.push_back(places[i.index]);
		return iterator(i.type, places.size()-1);
	}
	else
	{
		transitions.push_back(transitions[i.index]);
		return iterator(i.type, transitions.size()-1);
	}
}

vector<iterator> graph::copy(iterator i, int num)
{
	vector<iterator> result;
	if (i.type == place::type)
		for (int j = 0; j < num; j++)
		{
			places.push_back(places[i.index]);
			result.push_back(iterator(i.type, places.size()-1));
		}
	else
		for (int j = 0; j < num; j++)
		{
			transitions.push_back(transitions[i.index]);
			result.push_back(iterator(i.type, transitions.size()-1));
		}
	return result;
}

vector<iterator> graph::copy(vector<iterator> i, int num)
{
	vector<iterator> result;
	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<iterator> temp = copy(i[j], num);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

iterator graph::copy_combine(int relation, iterator i0, iterator i1)
{
	if (i0.type == place::type && i1.type == place::type)
		return create(hse::merge(relation, places[i0.index], places[i1.index]));
	else if (i0.type == transition::type && i1.type == transition::type)
	{
		if (transitions[i0.index].behavior == transitions[i1.index].behavior)
			return create(hse::merge(relation, transitions[i0.index], transitions[i1.index]));
		else
		{
			internal("hse::graph::copy_combine", "transition behaviors do not match", __FILE__, __LINE__);
			return iterator();
		}
	}
	else
	{
		internal("hse::graph::copy_combine", "iterator types do not match", __FILE__, __LINE__);
		return iterator();
	}
}

iterator graph::combine(int relation, iterator i0, iterator i1)
{
	if (i0.type == place::type && i1.type == place::type)
	{
		places[i0.index] = hse::merge(relation, places[i0.index], places[i1.index]);
		return i0;
	}
	else if (i0.type == transition::type && i1.type == transition::type)
	{
		if (transitions[i0.index].behavior == transitions[i1.index].behavior)
		{
			transitions[i0.index] = hse::merge(relation, transitions[i0.index], transitions[i1.index]);
			return i0;
		}
		else
		{
			internal("hse::graph::copy_combine", "transition behaviors do not match", __FILE__, __LINE__);
			return iterator();
		}
	}
	else
	{
		internal("hse::graph::copy_combine", "iterator types do not match", __FILE__, __LINE__);
		return iterator();
	}
}

iterator graph::connect(iterator from, iterator to)
{
	if (from.type == place::type && to.type == place::type)
	{
		iterator mid = create(transition());
		arcs[from.type].push_back(arc(from, mid));
		arcs[mid.type].push_back(arc(mid, to));
	}
	else if (from.type == transition::type && to.type == transition::type)
	{
		iterator mid = create(place());
		arcs[from.type].push_back(arc(from, mid));
		arcs[mid.type].push_back(arc(mid, to));
	}
	else
		arcs[from.type].push_back(arc(from, to));
	return to;
}

vector<iterator> graph::connect(iterator from, vector<iterator> to)
{
	for (int i = 0; i < (int)to.size(); i++)
		connect(from, to[i]);
	return to;
}

iterator graph::connect(vector<iterator> from, iterator to)
{
	for (int i = 0; i < (int)from.size(); i++)
		connect(from[i], to);
	return to;
}

vector<iterator> graph::connect(vector<iterator> from, vector<iterator> to)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)to.size(); j++)
			connect(from[i], to[i]);
	return to;
}

iterator graph::insert(iterator a, place n)
{
	iterator i[2];
	i[place::type] = create(n);
	i[transition::type] = create(transition());
	arcs[a.type].push_back(arc(i[a.type], arcs[a.type][a.index].to));
	arcs[1-a.type].push_back(arc(i[1-a.type], i[a.type]));
	arcs[a.type][a.index].to = i[1-a.type];
	return i[place::type];
}

iterator graph::insert(iterator a, transition n)
{
	iterator i[2];
	i[place::type] = create(place());
	i[transition::type] = create(n);
	arcs[a.type].push_back(arc(i[a.type], arcs[a.type][a.index].to));
	arcs[1-a.type].push_back(arc(i[1-a.type], i[a.type]));
	arcs[a.type][a.index].to = i[1-a.type];
	return i[transition::type];
}

iterator graph::insert(iterator a, int n)
{
	if (n == place::type)
		return insert(a, place());
	else if (n == transition::type)
		return insert(a, transition());
	else
		return iterator();
}

iterator graph::insert_alongside(iterator from, iterator to, place n)
{
	iterator i = create(n);
	if (from.type == i.type)
	{
		iterator j = create(transition());
		connect(from, j);
		connect(j, i);
	}
	else
		connect(from, i);

	if (to.type == i.type)
	{
		iterator j = create(transition());
		connect(i, j);
		connect(j, to);
	}
	else
		connect(i, to);

	return i;
}

iterator graph::insert_alongside(iterator from, iterator to, transition n)
{
	iterator i = create(n);
	if (from.type == i.type)
	{
		iterator j = create(place());
		connect(from, j);
		connect(j, i);
	}
	else
		connect(from, i);

	if (to.type == i.type)
	{
		iterator j = create(place());
		connect(i, j);
		connect(j, to);
	}
	else
		connect(i, to);

	return i;
}

iterator graph::insert_alongside(iterator from, iterator to, int n)
{
	if (n == place::type)
		return insert_alongside(from, to, place());
	else if (n == transition::type)
		return insert_alongside(from, to, transition());
	else
		return iterator();
}

iterator graph::insert_before(iterator to, place n)
{
	iterator i[2];
	i[transition::type] = create(transition());
	i[place::type] = create(n);
	for (int j = 0; j < (int)arcs[1-to.type].size(); j++)
		if (arcs[1-to.type][j].to.index == to.index)
			arcs[1-to.type][j].to.index = i[to.type].index;
	connect(i[1-to.type], to);
	connect(i[to.type], i[1-to.type]);
	return i[place::type];
}

iterator graph::insert_before(iterator to, transition n)
{
	iterator i[2];
	i[transition::type] = create(n);
	i[place::type] = create(place());
	for (int j = 0; j < (int)arcs[1-to.type].size(); j++)
		if (arcs[1-to.type][j].to.index == to.index)
			arcs[1-to.type][j].to.index = i[to.type].index;
	connect(i[1-to.type], to);
	connect(i[to.type], i[1-to.type]);
	return i[transition::type];
}

iterator graph::insert_before(iterator to, int n)
{
	if (n == place::type)
		return insert_before(to, place());
	else if (n == transition::type)
		return insert_before(to, transition());
	else
		return iterator();
}

iterator graph::insert_after(iterator from, place n)
{
	iterator i[2];
	i[transition::type] = create(transition());
	i[place::type] = create(n);
	for (int j = 0; j < (int)arcs[from.type].size(); j++)
		if (arcs[from.type][j].from.index == from.index)
			arcs[from.type][j].from.index = i[from.type].index;
	connect(from, i[1-from.type]);
	connect(i[1-from.type], i[from.type]);
	return i[place::type];
}


iterator graph::insert_after(iterator from, transition n)
{
	iterator i[2];
	i[transition::type] = create(n);
	i[place::type] = create(place());
	for (int j = 0; j < (int)arcs[from.type].size(); j++)
		if (arcs[from.type][j].from.index == from.index)
			arcs[from.type][j].from.index = i[from.type].index;
	connect(from, i[1-from.type]);
	connect(i[1-from.type], i[from.type]);
	return i[transition::type];
}

iterator graph::insert_after(iterator from, int n)
{
	if (n == place::type)
		return insert_after(from, place());
	else if (n == transition::type)
		return insert_after(from, transition());
	else
		return iterator();
}

pair<vector<iterator>, vector<iterator> > graph::cut(iterator n, vector<iterator> *i0, vector<iterator> *i1)
{
	pair<vector<iterator>, vector<iterator> > result;
	for (int i = (int)arcs[n.type].size()-1; i >= 0; i--)
	{
		if (arcs[n.type][i].from.index == n.index)
		{
			result.second.push_back(arcs[n.type][i].to);
			arcs[n.type].erase(arcs[n.type].begin() + i);
		}
		else if (arcs[n.type][i].from.index > n.index)
			arcs[n.type][i].from.index--;
	}
	for (int i = (int)arcs[1-n.type].size()-1; i >= 0; i--)
	{
		if (arcs[1-n.type][i].to.index == n.index)
		{
			result.first.push_back(arcs[1-n.type][i].from);
			arcs[1-n.type].erase(arcs[1-n.type].begin() + i);
		}
		else if (arcs[1-n.type][i].to.index > n.index)
			arcs[1-n.type][i].to.index--;
	}

	if (n.type == place::type)
	{
		for (int j = 0; j < (int)source.size(); j++)
			for (int i = (int)source[j].size()-1; i >= 0; i--)
			{
				if (source[j][i].index == n.index)
					source[j].erase(source[j].begin() + i);
				else if (source[j][i].index > n.index)
					source[j][i].index--;
			}

		for (int j = 0; j < (int)sink.size(); j++)
			for (int i = (int)sink[j].size()-1; i >= 0; i--)
			{
				if (sink[j][i].index == n.index)
					sink[j].erase(sink[j].begin() + i);
				else if (sink[j][i].index > n.index)
					sink[j][i].index--;
			}
	}

	if (i0 != NULL)
	{
		for (int i = (int)i0->size()-1; i >= 0; i--)
		{
			if (i0->at(i) == n)
				i0->erase(i0->begin() + i);
			else if (i0->at(i).type == n.type && i0->at(i).index > n.index)
				i0->at(i).index--;
		}
	}
	if (i1 != NULL)
	{
		for (int i = (int)i1->size()-1; i >= 0; i--)
		{
			if (i1->at(i) == n)
				i1->erase(i1->begin() + i);
			else if (i1->at(i).type == n.type && i1->at(i).index > n.index)
				i1->at(i).index--;
		}
	}

	if (n.type == place::type)
		places.erase(places.begin() + n.index);
	else if (n.type == transition::type)
		transitions.erase(transitions.begin() + n.index);

	return result;
}

void graph::cut(vector<iterator> n, vector<iterator> *i0, vector<iterator> *i1, bool rsorted)
{
	if (!rsorted)
		sort(n.rbegin(), n.rend());
	for (int i = 0; i < (int)n.size(); i++)
		cut(n[i], i0, i1);
}

iterator graph::duplicate(int relation, iterator i, bool add)
{
	iterator d = copy(i);
	if (i.type == relation)
	{
		for (int j = (int)arcs[i.type].size()-1; j >= 0; j--)
			if (arcs[i.type][j].from == i)
				connect(d, arcs[i.type][j].to);
		for (int j = (int)arcs[1-i.type].size()-1; j >= 0; j--)
			if (arcs[1-i.type][j].to == i)
				connect(arcs[1-i.type][j].from, d);
	}
	else if (add)
	{
		vector<iterator> x = create(1-i.type, 4);
		vector<iterator> y = create(i.type, 2);

		for (int j = (int)arcs[i.type].size()-1; j >= 0; j--)
			if (arcs[i.type][j].from == i)
				arcs[i.type][j].from = y[1];
		for (int j = (int)arcs[1-i.type].size()-1; j >= 0; j--)
			if (arcs[1-i.type][j].to == i)
				arcs[1-i.type][j].to = y[0];

		connect(y[0], x[0]);
		connect(y[0], x[1]);
		connect(x[0], i);
		connect(x[1], d);
		connect(i, x[2]);
		connect(d, x[3]);
		connect(x[2], y[1]);
		connect(x[3], y[1]);
	}
	else
	{
		vector<iterator> n = next(i);
		vector<iterator> p = prev(i);

		for (int j = 0; j < 2; j++)
			for (int k = (int)arcs[j].size()-1; k >= 0; k--)
				if (arcs[j][k].from == i || arcs[j][k].to == i)
					arcs[j].erase(arcs[j].begin() + k);

		vector<iterator> n1, p1;
		for (int l = 0; l < (int)n.size(); l++)
			n1.push_back(duplicate(relation, n[l]));
		for (int l = 0; l < (int)p.size(); l++)
			p1.push_back(duplicate(relation, p[l]));

		connect(p1, d);
		connect(d, n1);
		connect(p, i);
		connect(i, n);
	}

	if (i.type == place::type)
	{
		for (int j = 0; j < (int)source.size(); j++)
			for (int k = 0; k < (int)source[j].size(); k++)
				if (source[j][k].index == i.index)
					source[j].push_back(reset_token(d.index, source[j][k].state, source[j][k].remotable));

		for (int j = 0; j < (int)sink.size(); j++)
			for (int k = 0; k < (int)sink[j].size(); k++)
				if (sink[j][k].index == i.index)
					sink[j].push_back(reset_token(d.index, sink[j][k].state, source[j][k].remotable));
	}

	return d;
}

vector<iterator> graph::duplicate(int relation, iterator i, int num, bool add)
{
	vector<iterator> d = copy(i, num-1);
	if (i.type == relation)
	{
		for (int j = (int)arcs[i.type].size()-1; j >= 0; j--)
			if (arcs[i.type][j].from == i)
				for (int k = 0; k < (int)d.size(); k++)
					connect(d[k], arcs[i.type][j].to);
		for (int j = (int)arcs[1-i.type].size()-1; j >= 0; j--)
			if (arcs[1-i.type][j].to == i)
				for (int k = 0; k < (int)d.size(); k++)
					connect(arcs[1-i.type][j].from, d[k]);
	}
	else if (add)
	{
		vector<iterator> x = create(1-i.type, 2*(num-1));
		vector<iterator> y = create(i.type, 2);
		vector<iterator> z = create(1-i.type, 2);

		for (int j = (int)arcs[i.type].size()-1; j >= 0; j--)
			if (arcs[i.type][j].from == i)
				arcs[i.type][j].from = y[1];
		for (int j = (int)arcs[1-i.type].size()-1; j >= 0; j--)
			if (arcs[1-i.type][j].to == i)
				arcs[1-i.type][j].to = y[0];

		connect(y[0], z[0]);
		connect(z[0], i);
		connect(i, z[1]);
		connect(z[1], y[1]);

		for (int k = 0; k < (int)d.size(); k++)
		{
			connect(y[0], x[k*2 + 0]);
			connect(x[k*2 + 0], d[k]);
			connect(d[k], x[k*2 + 1]);
			connect(x[k*2 + 1], y[1]);
		}
	}
	else
	{
		vector<iterator> n = next(i);
		vector<iterator> p = prev(i);

		for (int j = 0; j < 2; j++)
			for (int k = (int)arcs[j].size()-1; k >= 0; k--)
				if (arcs[j][k].from == i || arcs[j][k].to == i)
					arcs[j].erase(arcs[j].begin() + k);

		for (int k = 0; k < num-1; k++)
		{
			vector<iterator> n1, p1;
			for (int l = 0; l < (int)n.size(); l++)
				n1.push_back(duplicate(relation, n[l]));
			for (int l = 0; l < (int)p.size(); l++)
				p1.push_back(duplicate(relation, p[l]));

			connect(p1, d[k]);
			connect(d[k], n1);
		}
		connect(p, i);
		connect(i, n);
	}

	if (i.type == place::type)
	{
		for (int j = 0; j < (int)source.size(); j++)
			for (int k = 0; k < (int)source[j].size(); k++)
				if (source[j][k].index == i.index)
					for (int l = 0; l < (int)d.size(); l++)
						source[j].push_back(reset_token(d[l].index, source[j][k].state, source[j][k].remotable));

		for (int j = 0; j < (int)sink.size(); j++)
			for (int k = 0; k < (int)sink[j].size(); k++)
				if (sink[j][k].index == i.index)
					for (int l = 0; l < (int)d.size(); l++)
						sink[j].push_back(reset_token(d[l].index, sink[j][k].state, source[j][k].remotable));
	}

	d.push_back(i);

	return d;
}

vector<iterator> graph::duplicate(int relation, vector<iterator> n, int num, bool interleaved, bool add)
{
	vector<iterator> result;
	result.reserve(n.size()*num);
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = duplicate(relation, n[i], num, add);
		if (interleaved && i > 0)
			for (int j = 0; j < (int)temp.size(); j++)
				result.insert(result.begin() + j*(i+1) + 1, temp[j]);
		else
			result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

void graph::pinch(iterator n, vector<iterator> *i0, vector<iterator> *i1)
{
	pair<vector<iterator>, vector<iterator> > neighbors = cut(n, i0, i1);

	vector<iterator> left = duplicate(1-n.type, neighbors.first, neighbors.second.size(), false);
	vector<iterator> right = duplicate(1-n.type, neighbors.second, neighbors.first.size(), true);

	for (int i = 0; i < (int)right.size(); i++)
	{
		combine(right[i].type, left[i], right[i]);

		for (int j = 0; j < (int)arcs[right[i].type].size(); j++)
			if (arcs[right[i].type][j].from == right[i])
				arcs[right[i].type][j].from = left[i];

		for (int j = 0; j < (int)arcs[1-right[i].type].size(); j++)
			if (arcs[1-right[i].type][j].to == right[i])
				arcs[1-right[i].type][j].to = left[i];

		if (right[i].type == place::type)
		{
			for (int j = 0; j < (int)source.size(); j++)
				for (int k = 0; k < (int)source[j].size(); k++)
					if (source[j][k].index == right[i].index)
						source[j].push_back(reset_token(left[i].index, source[j][k].state, source[j][k].remotable));

			for (int j = 0; j < (int)sink.size(); j++)
				for (int k = 0; k < (int)sink[j].size(); k++)
					if (sink[j][k].index == right[i].index)
						sink[j].push_back(reset_token(left[i].index, source[j][k].state, source[j][k].remotable));
		}
	}

	cut(right, i0, i1);
}

vector<iterator> graph::next(iterator n) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[n.type].size(); i++)
		if (arcs[n.type][i].from.index == n.index)
			result.push_back(arcs[n.type][i].to);
	return result;
}

vector<iterator> graph::next(vector<iterator> n) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = next(n[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<iterator> graph::prev(iterator n) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[1-n.type].size(); i++)
		if (arcs[1-n.type][i].to.index == n.index)
			result.push_back(arcs[1-n.type][i].from);
	return result;
}

vector<iterator> graph::prev(vector<iterator> n) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = prev(n[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<int> graph::next(int type, int n) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[type].size(); i++)
		if (arcs[type][i].from.index == n)
			result.push_back(arcs[type][i].to.index);
	return result;
}

vector<int> graph::next(int type, vector<int> n) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[type].size(); i++)
		if (find(n.begin(), n.end(), arcs[type][i].from.index) != n.end())
			result.push_back(arcs[type][i].to.index);
	return result;
}

vector<int> graph::prev(int type, int n) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (arcs[1-type][i].to.index == n)
			result.push_back(arcs[1-type][i].from.index);
	return result;
}

vector<int> graph::prev(int type, vector<int> n) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (find(n.begin(), n.end(), arcs[1-type][i].to.index) != n.end())
			result.push_back(arcs[1-type][i].from.index);
	return result;
}

vector<iterator> graph::out(iterator n) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[n.type].size(); i++)
		if (arcs[n.type][i].from.index == n.index)
			result.push_back(iterator(n.type, i));
	return result;
}

vector<iterator> graph::out(vector<iterator> n) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = out(n[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<iterator> graph::in(iterator n) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[1-n.type].size(); i++)
		if (arcs[1-n.type][i].to.index == n.index)
			result.push_back(iterator(1-n.type, i));
	return result;
}

vector<iterator> graph::in(vector<iterator> n) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = in(n[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<int> graph::out(int type, int n) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[type].size(); i++)
		if (arcs[type][i].from.index == n)
			result.push_back(i);
	return result;
}

vector<int> graph::out(int type, vector<int> n) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[type].size(); i++)
		if (find(n.begin(), n.end(), arcs[type][i].from.index) != n.end())
			result.push_back(i);
	return result;
}

vector<int> graph::in(int type, int n) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (arcs[1-type][i].to.index == n)
			result.push_back(i);
	return result;
}

vector<int> graph::in(int type, vector<int> n) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (find(n.begin(), n.end(), arcs[1-type][i].to.index) != n.end())
			result.push_back(i);
	return result;
}

vector<iterator> graph::next_arcs(iterator a) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[1-a.type].size(); i++)
		if (arcs[1-a.type][i].from == arcs[a.type][a.index].to)
			result.push_back(iterator(1-a.type, i));
	return result;
}

vector<iterator> graph::next_arcs(vector<iterator> a) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)a.size(); i++)
	{
		vector<iterator> temp = next_arcs(a[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<iterator> graph::prev_arcs(iterator a) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[1-a.type].size(); i++)
		if (arcs[1-a.type][i].to == arcs[a.type][a.index].from)
			result.push_back(iterator(1-a.type, i));
	return result;
}

vector<iterator> graph::prev_arcs(vector<iterator> a) const
{
	vector<iterator> result;
	for (int i = 0; i < (int)a.size(); i++)
	{
		vector<iterator> temp = prev_arcs(a[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<int> graph::next_arcs(int type, int a) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (arcs[1-type][i].from == arcs[type][a].to)
			result.push_back(i);
	return result;
}

vector<int> graph::next_arcs(int type, vector<int> a) const
{
	vector<int> result;
	for (int i = 0; i < (int)a.size(); i++)
	{
		vector<int> temp = next_arcs(type, a[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<int> graph::prev_arcs(int type, int a) const
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (arcs[1-type][i].to == arcs[type][a].from)
			result.push_back(i);
	return result;
}

vector<int> graph::prev_arcs(int type, vector<int> a) const
{
	vector<int> result;
	for (int i = 0; i < (int)a.size(); i++)
	{
		vector<int> temp = prev_arcs(type, a[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

bool graph::is_floating(iterator n) const
{
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < (int)arcs[i].size(); j++)
			if (arcs[i][j].from == n || arcs[i][j].to == n)
				return false;
	return true;
}

map<iterator, iterator> graph::merge(int relation, const graph &g, bool remote)
{
	map<iterator, iterator> result;

	places.reserve(places.size() + g.places.size());
	for (int i = 0; i < (int)g.places.size(); i++)
	{
		result.insert(pair<iterator, iterator>(iterator(place::type, i), iterator(place::type, (int)places.size())));
		places.push_back(g.places[i]);
	}

	transitions.reserve(transitions.size() + g.transitions.size());
	for (int i = 0; i < (int)g.transitions.size(); i++)
	{
		result.insert(pair<iterator, iterator>(iterator(transition::type, i), iterator(transition::type, (int)transitions.size())));
		transitions.push_back(g.transitions[i]);
	}

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < (int)g.arcs[i].size(); j++)
			arcs[i].push_back(arc(result[g.arcs[i][j].from], result[g.arcs[i][j].to]));

	if (relation == choice || source.size() == 0)
	{
		for (int i = 0; i < (int)g.source.size(); i++)
		{
			source.push_back(vector<reset_token>());
			for (int j = 0; j < (int)g.source[i].size(); j++)
				source.back().push_back(reset_token(result[iterator(place::type, g.source[i][j].index)].index, g.source[i][j].state, g.source[i][j].remotable || remote));
		}
	}
	else if (relation == parallel)
	{
		int s = (int)source.size();
		for (int i = 0; i < (int)g.source.size()-1; i++)
		{
			for (int j = 0; j < s; j++)
			{
				source.push_back(source[j]);
				for (int k = 0; k < (int)g.source[i].size(); k++)
					source.back().push_back(reset_token(result[iterator(place::type, g.source[i][k].index)].index, g.source[i][k].state, g.source[i][k].remotable || remote));
			}
		}

		if (g.source.size() > 0)
			for (int j = 0; j < s; j++)
				for (int k = 0; k < (int)g.source.back().size(); k++)
					source[j].push_back(reset_token(result[iterator(place::type, g.source.back()[k].index)].index, g.source.back()[k].state, g.source.back()[k].remotable || remote));
	}

	if (relation == choice || sink.size() == 0)
	{
		for (int i = 0; i < (int)g.sink.size(); i++)
		{
			sink.push_back(vector<reset_token>());
			for (int j = 0; j < (int)g.sink[i].size(); j++)
				sink.back().push_back(reset_token(result[iterator(place::type, g.sink[i][j].index)].index, g.sink[i][j].state, g.sink[i][j].remotable || remote));
		}
	}
	else if (relation == parallel)
	{
		int s = (int)sink.size();
		for (int i = 0; i < (int)g.sink.size()-1; i++)
		{
			for (int j = 0; j < s; j++)
			{
				sink.push_back(sink[j]);
				for (int k = 0; k < (int)g.sink[i].size(); k++)
					sink.back().push_back(reset_token(result[iterator(place::type, g.sink[i][k].index)].index, g.sink[i][k].state, g.sink[i][k].remotable || remote));
			}
		}

		if (g.sink.size() > 0)
			for (int j = 0; j < s; j++)
				for (int k = 0; k < (int)g.sink.back().size(); k++)
					sink[j].push_back(reset_token(result[iterator(place::type, g.sink.back()[k].index)].index, g.sink.back()[k].state, g.sink.back()[k].remotable || remote));
	}
	else if (relation == sequence)
	{
		for (int i = 0; i < (int)sink.size(); i++)
			for (int j = 0; j < (int)g.source.size(); j++)
			{
				iterator c = create(transition());
				for (int k = 0; k < (int)sink[i].size(); k++)
					connect(iterator(place::type, sink[i][k].index), c);
				for (int k = 0; k < (int)g.source[j].size(); k++)
					connect(c, result[iterator(place::type, g.source[j][k].index)]);
			}

		sink.clear();
		for (int i = 0; i < (int)g.sink.size(); i++)
		{
			sink.push_back(vector<reset_token>());
			for (int j = 0; j < (int)g.sink[i].size(); j++)
				sink.back().push_back(reset_token(result[iterator(place::type, g.sink[i][j].index)].index, g.sink[i][j].state, g.sink[i][j].remotable || remote));
		}
	}

	return result;
}

void graph::wrap()
{
	if (source.size() > 1 || (source.size() > 0 && source[0].size() > 1))
	{
		iterator split = create(place());

		for (int i = 0; i < (int)source.size(); i++)
		{
			boolean::cube state;
			for (int j = 0; j < (int)source[i].size(); j++)
				state &= source[i][j].state;

			iterator split_t = create(transition(transition::active, state));
			connect(split, split_t);

			for (int j = 0; j < (int)source[i].size(); j++)
				connect(split_t, iterator(place::type, source[i][j].index));
		}

		source.clear();
		source.push_back(vector<reset_token>(1, reset_token(split.index, boolean::cube(), false)));
	}

	if (sink.size() > 1 || (sink.size() > 0 && sink[0].size() > 1))
	{
		iterator merge = create(place());
		for (int i = 0; i < (int)sink.size(); i++)
		{
			iterator merge_t = create(transition());
			connect(merge_t, merge);

			for (int j = 0; j < (int)sink[i].size(); j++)
				connect(iterator(place::type, sink[i][j].index), merge_t);
		}

		sink.clear();
		sink.push_back(vector<reset_token>(1, reset_token(merge.index, boolean::cube(), false)));
	}
}

vector<vector<iterator> > graph::cycles() const
{
	vector<vector<hse::iterator> > curr;
	vector<vector<hse::iterator> > result;
	for (int i = 0; i < (int)source.size(); i++)
		for (int j = 0; j < (int)source[i].size(); j++)
			curr.push_back(vector<hse::iterator>(1, hse::iterator(hse::place::type, source[i][j].index)));

	sort(curr.begin(), curr.end());
	curr.resize(unique(curr.begin(), curr.end()) - curr.begin());

	while (curr.size() > 0)
	{
		vector<hse::iterator> x = curr.back();
		curr.pop_back();

		vector<hse::iterator> n = next(x.back());
		for (int j = 0; j < (int)n.size(); j++)
		{
			vector<hse::iterator>::iterator loopback = find(x.begin(), x.end(), n[j]);
			if (loopback != x.end())
			{
				result.push_back(x);
				result.back().erase(result.back().begin(), result.back().begin() + (loopback - x.begin()));
			}
			else
			{
				curr.push_back(x);
				curr.back().push_back(n[j]);
			}
		}
	}

	return result;
}

void graph::compact(const boolean::variable_set &variables, bool proper_nesting)
{
	bool change = true;
	while (change)
	{
		change = false;

		for (iterator i(transition::type, 0); i < (int)transitions.size() && !change; )
		{
			vector<iterator> n = next(i);
			vector<iterator> p = prev(i);

			sort(n.begin(), n.end());
			sort(p.begin(), p.end());

			bool affect = false;
			// If it doesn't have any input places, then we need to add one.
			if (!affect && p.size() == 0)
			{
				p.push_back(create(place::type));
				connect(p, i);
				affect = true;
			}

			// If it doesn't have any output places, then we need to add one.
			if (!affect && n.size() == 0)
			{
				n.push_back(create(place::type));
				connect(i, n);
				affect = true;
			}

			// A transition will never be enabled if its action is not physically possible.
			// These transitions may be removed while preserving proper nesting, token flow
			// stability, non interference, and deadlock freedom. At this point, it is not
			// possible for this transition to be in the source list.
			if (!affect && transitions[i.index].action == 0)
			{
				cut(i);
				affect = true;
			}

			// We can know for sure if a transition is vacuous before we've elaborated the state space if
			// the transition is the universal cube. These transitions may be pinched while preserving token flow,
			// stability, non interference, and deadlock freedom. However, proper nesting is not necessarily
			// preserved. We have to take special precautions if we want to preserver proper nesting.
			if (!affect && transitions[i.index].action == 1)
			{
				if (!proper_nesting)
				{
					pinch(i);
					affect = true;
				}
				else
				{
					vector<iterator> np = next(p);
					vector<iterator> pn = prev(n);

					if (p.size() == 1 && n.size() == 1 && np.size() == 1 && pn.size() == 1)
					{
						pinch(i);
						affect = true;
					}
					else
					{
						vector<iterator> nn = next(n);
						vector<iterator> nnp = next(np);
						vector<iterator> pp = prev(p);
						vector<iterator> ppn = prev(pn);

						if ((n.size() == 1 && nn.size() == 1 && nnp.size() == 1 && np.size() == 1) ||
							(p.size() == 1 && pp.size() == 1 && ppn.size() == 1 && pn.size() == 1))
						{
							pinch(i);
							affect = true;
						}
					}
				}
			}

			if (!affect)
				i++;
			else
				change = true;
		}

		for (iterator i(place::type, 0); i < (int)places.size() && !change; )
		{
			bool i_is_reset = false;
			for (int j = 0; j < (int)source.size() && !i_is_reset; j++)
				for (int k = 0; k < (int)source[j].size() && !i_is_reset; k++)
					if (source[j][k].index == i.index)
						i_is_reset = true;

			vector<iterator> n = next(i);
			vector<iterator> p = prev(i);

			sort(n.begin(), n.end());
			sort(p.begin(), p.end());

			bool affect = false;

			// We know a place will never be marked if it is not in the initial marking and it has no input arcs.
			// This means that its output transitions will never fire.
			if (p.size() == 0 && (!i_is_reset || n.size() == 0))
			{
				cut(n);
				cut(i);
				affect = true;
			}

			// Check to see if there are any excess places whose existence doesn't affect the behavior of the circuit
			for (iterator j = i+1; j < (int)places.size(); )
			{
				bool j_is_reset = false;
				for (int k = 0; k < (int)source.size() && !j_is_reset; k++)
					for (int l = 0; l < (int)source[k].size() && !j_is_reset; l++)
						if (source[k][l].index == j.index)
							j_is_reset = true;

				vector<iterator> n2 = next(j);
				vector<iterator> p2 = prev(j);

				sort(n2.begin(), n2.end());
				sort(p2.begin(), p2.end());

				if (n == n2 && p == p2 && i_is_reset == j_is_reset)
				{
					cut(j);
					affect = true;
				}
				else
					j++;
			}

			if (!affect)
				i++;
			else
				change = true;
		}


		/*vector<iterator> left;
		vector<iterator> right;

		vector<vector<iterator> > n, p;
		vector<vector<pair<vector<iterator>, vector<iterator> > > > nx, px;

		for (iterator i(transition::type, 0); i < (int)transitions.size() && !change; i++)
		{
			n.push_back(next(i));
			p.push_back(prev(i));

			sort(n.back().begin(), n.back().end());
			sort(p.back().begin(), p.back().end());

			nx.push_back(vector<pair<vector<iterator>, vector<iterator> > >());
			px.push_back(vector<pair<vector<iterator>, vector<iterator> > >());
			for (int j = 0; j < (int)n.back().size(); j++)
			{
				nx.back().push_back(pair<vector<iterator>, vector<iterator> >(prev(n.back()[j]), next(n.back()[j])));
				sort(nx.back().back().first.begin(), nx.back().back().first.end());
				sort(nx.back().back().second.begin(), nx.back().back().second.end());
			}
			for (int j = 0; j < (int)p.back().size(); j++)
			{
				px.back().push_back(pair<vector<iterator>, vector<iterator> >(prev(p.back()[j]), next(p.back()[j])));
				sort(px.back().back().first.begin(), px.back().back().first.end());
				sort(px.back().back().second.begin(), px.back().back().second.end());
			}

			for (iterator j = i-1; j >= 0 && !change; j--)
			{
				if (transitions[j.index].behavior == transitions[i.index].behavior)
				{
					// Find internally conditioned transitions. Transitions are internally conditioned if they are the same type
					// share all of the same input and output places.
					if (n[j.index] == n[i.index] && p[j.index] == p[i.index])
					{
						transitions[j.index] = hse::merge(choice, transitions[i.index], transitions[j.index]);
						cut(i);
						change = true;
					}

					// Find internally parallel transitions. A pair of transitions A and B are internally parallel if
					// they are the same type, have disjoint sets of input and output places that share a single input
					// or output transition and have no output or input transitions other than A or B.
					else if (vector_intersection_size(n[i.index], n[j.index]) == 0 && vector_intersection_size(p[i.index], p[j.index]) == 0 && nx[i.index] == nx[j.index] && px[i.index] == px[j.index])
					{
						transitions[j.index] = hse::merge(parallel, transitions[i.index], transitions[j.index]);
						vector<iterator> tocut;
						tocut.push_back(i);
						tocut.insert(tocut.end(), n[i.index].begin(), n[i.index].end());
						tocut.insert(tocut.end(), p[i.index].begin(), p[i.index].end());
						cut(tocut);
						change = true;
					}
				}
			}
		}*/

		if (!change)
		{
			for (int i = (int)source.size()-1; i >= 0; i--)
			{
				simulator sim(this, &variables, i, false);
				int enabled = sim.enabled();

				for (int j = 0; j < enabled; j++)
				{
					bool firable = transitions[sim.local.ready[j].index].action.cubes.size() <= 1;
					for (int k = 0; k < (int)sim.local.ready[j].tokens.size() && firable; k++)
					{
						for (int l = 0; l < (int)arcs[transition::type].size() && firable; l++)
							if (arcs[transition::type][l].to.index == sim.local.tokens[sim.local.ready[j].tokens[k]].index)
								firable = false;
						for (int l = 0; l < (int)arcs[place::type].size() && firable; l++)
							if (arcs[place::type][l].from.index == sim.local.tokens[sim.local.ready[j].tokens[k]].index && arcs[place::type][l].to.index != sim.local.ready[j].index)
								firable = false;
					}

					if (firable)
					{
						sim.fire(j);
						change = true;
						enabled = sim.enabled();
						j = -1;
					}
				}

				for (int j = 0; j < (int)sim.local.tokens.size(); j++)
					sim.local.tokens[j].guard.clear();

				source[i].clear();
				source[i].insert(source[i].end(), sim.local.tokens.begin(), sim.local.tokens.end());
			}
		}
	}
}

void graph::reachability()
{
	int nodes = (int)(places.size() + transitions.size());
	reach.assign(nodes*nodes, nodes);
	for (int i = 0; i < (int)places.size(); i++)
		reach[i*nodes + i] = 0;
	for (int i = 0; i < (int)transitions.size(); i++)
		reach[(places.size() + i)*nodes + (places.size() + i)] = 0;

	bool change = true;
	while (change)
	{
		change = false;
		for (int i = 0; i < 2; i++)
			for (int j = 0; j < (int)arcs[i].size(); j++)
				for (int k = 0; k < nodes; k++)
				{
					int m = min(reach[(places.size()*arcs[i][j].from.type + arcs[i][j].from.index)*nodes + k] + 1, reach[(places.size()*arcs[i][j].to.type + arcs[i][j].to.index)*nodes + k]);
					if (m > nodes)
						m = nodes;
					change = change || (reach[(places.size()*arcs[i][j].to.type + arcs[i][j].to.index)*nodes + k] != m);
					reach[(places.size()*arcs[i][j].to.type + arcs[i][j].to.index)*nodes + k] = m;
				}
	}
}

bool graph::is_reachable(iterator from, iterator to)
{
	return (reach[(places.size()*from.type + from.index)*(places.size() + transitions.size()) + (places.size()*to.type + to.index)] < (int)(places.size() + transitions.size()));
}

bool graph::is_parallel(iterator a, iterator b)
{
	return (find(parallel_nodes.begin(), parallel_nodes.end(), pair<iterator, iterator>(a, b)) != parallel_nodes.end());
}

void graph::synchronize()
{
	half_synchronization sync;
	for (sync.passive.index = 0; sync.passive.index < (int)transitions.size(); sync.passive.index++)
		if (transitions[sync.passive.index].behavior == transition::passive && !transitions[sync.passive.index].action.is_tautology())
			for (sync.active.index = 0; sync.active.index < (int)transitions.size(); sync.active.index++)
				if (transitions[sync.active.index].behavior == transition::active && !transitions[sync.active.index].action.is_tautology())
					for (sync.passive.cube = 0; sync.passive.cube < (int)transitions[sync.passive.index].action.cubes.size(); sync.passive.cube++)
						for (sync.active.cube = 0; sync.active.cube < (int)transitions[sync.active.index].action.cubes.size(); sync.active.cube++)
							if (similarity_g0(transitions[sync.active.index].action.cubes[sync.active.cube], transitions[sync.passive.index].action.cubes[sync.passive.cube]))
								synchronizations.push_back(sync);
}

void graph::petrify(const boolean::variable_set &variables)
{
	// Petri nets don't support internal parallelism or choice, so we have to expand those transitions.
	for (iterator i(transition::type, transitions.size()-1); i >= 0; i--)
	{
		if (transitions[i.index].behavior == transition::active && transitions[i.index].action.cubes.size() > 1)
		{
			vector<iterator> k = duplicate(choice, i, transitions[i.index].action.cubes.size(), false);
			for (int j = 0; j < (int)k.size(); j++)
				transitions[k[j].index].action = boolean::cover(transitions[k[j].index].action.cubes[j]);
		}
	}

	for (iterator i(transition::type, transitions.size()-1); i >= 0; i--)
	{
		if (transitions[i.index].behavior == transition::active && transitions[i.index].action.cubes.size() == 1)
		{
			vector<int> vars = transitions[i.index].action.cubes[0].vars();
			if (vars.size() > 1)
			{
				vector<iterator> k = duplicate(parallel, i, vars.size(), false);
				for (int j = 0; j < (int)k.size(); j++)
					transitions[k[j].index].action.cubes[0] = boolean::cube(vars[j], transitions[k[j].index].action.cubes[0].get(vars[j]));
			}
		}
	}

	// Find all of the guards
	vector<iterator> passive;
	for (iterator i(transition::type, transitions.size()-1); i >= 0; i--)
		if (transitions[i.index].behavior == transition::passive)
			passive.push_back(i);

	synchronize();

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
		if (!transitions[passive[i].index].action.is_tautology())
		{
			// Find all of the half synchronization actions for this guard and group them accordingly:
			// outside group is by disjunction, middle group is by conjunction, and inside group is by value and variable.
			vector<vector<vector<iterator> > > sync;
			sync.resize(transitions[passive[i].index].action.cubes.size());
			for (int j = 0; j < (int)synchronizations.size(); j++)
				if (synchronizations[j].passive.index == passive[i].index)
				{
					bool found = false;
					for (int l = 0; l < (int)sync[synchronizations[j].passive.cube].size(); l++)
						if (similarity_g0(transitions[sync[synchronizations[j].passive.cube][l][0].index].action.cubes[0], transitions[synchronizations[j].active.index].action.cubes[0]))
						{
							found = true;
							sync[synchronizations[j].passive.cube][l].push_back(iterator(transition::type, synchronizations[j].active.index));
						}

					if (!found)
						sync[synchronizations[j].passive.cube].push_back(vector<iterator>(1, iterator(transition::type, synchronizations[j].active.index)));
				}

			/* Finally implement the connections. The basic observation being that a guard affects when the next
			 * transition can fire.
			 */
			vector<iterator> n = next(passive[i]);
			vector<iterator> g = create(place(), n.size());
			for (int j = 0; j < (int)n.size(); j++)
				connect(g[j], next(n[j]));

			for (int j = 0; j < (int)sync.size(); j++)
			{
				iterator t = create(transition());
				connect(t, g);
				for (int k = 0; k < (int)sync[j].size(); k++)
				{
					iterator p = create(place());
					connect(p, t);
					connect(sync[j][k], p);
				}
			}
		}
	}

	// Remove the guards from the graph
	vector<iterator> n = next(passive), p = prev(passive);
	passive.insert(passive.end(), n.begin(), n.end());
	passive.insert(passive.end(), p.begin(), p.end());
	sort(passive.begin(), passive.end());
	passive.resize(unique(passive.begin(), passive.end()) - passive.begin());

	cut(passive);

	synchronizations.clear();

	// Clean up
	compact(variables);
}

void graph::elaborate(const boolean::variable_set &variables, bool report)
{
	// TODO
	//cout << "Elaborating" << endl;
	for (int i = 0; i < (int)places.size(); i++)
	{
		places[i].predicate = boolean::cover();
		places[i].effective = boolean::cover();
	}

	vector<simulator::state> states;
	vector<simulator> simulations;
	vector<deadlock> deadlocks;
	simulations.reserve(20000);

	// Set up the first simulation that starts at the reset state
	for (int i = 0; i < (int)source.size(); i++)
		simulations.push_back(simulator(this, &variables, i, true));

	int count = 0;
	while (simulations.size() > 0)
	{
		simulator sim = simulations.back();
		simulations.pop_back();

		int enabled = sim.possible();
		if (enabled > 0)
		{
			for (int i = 0; i < (int)sim.remote.ready.size(); i++)
			{
				if (vector_intersection_size(sim.remote.ready[0].tokens, sim.remote.ready[i].tokens) > 0)
				{
					simulations.push_back(sim);
					simulations.back().begin(i);
					simulations.back().end();
					simulations.back().environment();
				}
			}
		}
		else
		{
			// Look the see if the resulting state is already in the state graph. If it is, then we are done with this simulation,
			// and if it is not, then we need to put the state in and continue on our way.
			simulator::state state = sim.get_state();
			vector<simulator::state>::iterator loc = lower_bound(states.begin(), states.end(), state);
			if (loc != states.end() && state == *loc && state.is_subset_of(*loc))
			{
				if (simulations.size() > 1)
					simulations.back().merge_errors(sim);

				/*bool found = false;
				for (int i = 0; i < (int)sim.local.tokens.size(); i++)
					if (sim.local.tokens[i].index == 5)
						found = true;

				if (found)
				{
					cout << "Found state " << export_disjunction(sim.global, variables).to_string() << endl << "{" << endl;
					for (int i = 0; i < (int)sim.local.tokens.size(); i++)
						cout << sim.local.tokens[i].index << " " << export_conjunction(sim.local.tokens[i].state, variables).to_string() << endl;
					cout << "}" << endl;
				}*/
			}
			else
			{
				if (loc != states.end() && state == *loc)
					loc->merge(state);
				else
					states.insert(loc, state);

				enabled = sim.enabled();
				vector<pair<int, int> > vacuous_choices = sim.get_vacuous_choices();
				int index = -1;
				for (int i = 0; i < (int)sim.local.ready.size() && index == -1; i++)
					if (sim.local.ready[i].vacuous)
					{
						index = i;
						for (int j = 0; j < (int)vacuous_choices.size() && index != -1; j++)
							if (vacuous_choices[j].first == i || vacuous_choices[j].second == i)
								index = -1;
					}

				if (index != -1)
				{
					simulations.push_back(sim);
					simulations.back().fire(index);
				}
				else if (vacuous_choices.size() > 0)
				{
					string err = "vacuous transitions break mutual exclusion ";
					for (int i = 0; i < (int)vacuous_choices.size(); i++)
						err += "{" + sim.local.ready[vacuous_choices[i].first].to_string(*this, variables) + ", " + sim.local.ready[vacuous_choices[i].second].to_string(*this, variables) + "} ";
					error("", err, __FILE__, __LINE__);

					for (int i = 0; i < (int)sim.local.ready.size(); i++)
						if (sim.local.ready[i].vacuous)
						{
							simulations.push_back(sim);
							enabled_transition e = simulations.back().local.ready[i];

							// Fire the transition in the simulation
							simulations.back().fire(i);
						}
				}
				else
				{
					// Handle the local tokens
					if (report)
						progress("", to_string(count) + " " + to_string(simulations.size()) + " " + to_string(states.size()) + " " + to_string(enabled), __FILE__, __LINE__);

					for (int i = 0; i < (int)sim.local.ready.size(); i++)
					{
						simulations.push_back(sim);
						enabled_transition e = simulations.back().local.ready[i];

						for (int j = 0; j < (int)e.tokens.size(); j++)
							places[simulations.back().local.tokens[e.tokens[j]].index].predicate |= simulations.back().local.tokens[e.tokens[j]].state;

						// Fire the transition in the simulation
						simulations.back().fire(i);
					}

					if (sim.local.ready.size() == 0)
					{
						deadlock d(sim.local.tokens);
						vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
						if (dloc == deadlocks.end() || *dloc != d)
						{
							error("", d.to_string(variables), __FILE__, __LINE__);
							deadlocks.insert(dloc, d);
						}

						simulations.back().merge_errors(sim);
					}

					for (int i = 0; i < (int)sim.local.tokens.size(); i++)
					{
						for (int j = i+1; j < (int)sim.local.tokens.size(); j++)
							parallel_nodes.push_back(pair<iterator, iterator>(iterator(place::type, sim.local.tokens[i].index), iterator(place::type, sim.local.tokens[j].index)));
						for (int j = 0; j < (int)sim.local.ready.size(); j++)
							if (find(sim.local.ready[j].tokens.begin(), sim.local.ready[j].tokens.end(), i) == sim.local.ready[j].tokens.end())
								parallel_nodes.push_back(pair<iterator, iterator>(iterator(place::type, sim.local.tokens[i].index), iterator(transition::type, sim.local.ready[j].index)));
					}

					/*bool found = false;
					for (int i = 0; i < (int)sim.local.tokens.size(); i++)
						if (sim.local.tokens[i].index == 5)
							found = true;

					if (found)
					{
						cout << "Saved state " << export_disjunction(sim.global, variables).to_string() << endl << "{" << endl;
						for (int i = 0; i < (int)sim.local.tokens.size(); i++)
							cout << sim.local.tokens[i].index << " " << export_conjunction(sim.local.tokens[i].state, variables).to_string() << endl;
						cout << "}" << endl;
					}*/

					boolean::cover en = 1;
					//vector<pair<bool, boolean::cover> > acting(sim.local.tokens.size(), pair<bool, boolean::cover>(false, 1));
					for (int i = 0; i < (int)sim.local.ready.size(); i++)
						en &= ~transitions[sim.local.ready[i].index].action.cubes[sim.local.ready[i].term];
						/*for (int j = 0; j < (int)sim.local.ready[i].tokens.size(); j++)
						{
							acting[sim.local.ready[i].tokens[j]].second &= ~transitions[sim.local.ready[i].index].action.cubes[sim.local.ready[i].term];
							acting[sim.local.ready[i].tokens[j]].first = true;
						}*/

					for (int i = 0; i < (int)sim.local.tokens.size(); i++)
						//if (acting[i].first)
							places[sim.local.tokens[i].index].effective |= (sim.local.tokens[i].state & en);//acting[i].second);

					count++;
				}
			}
		}
	}

	if (report)
		done_progress();

	sort(parallel_nodes.begin(), parallel_nodes.end());
	parallel_nodes.resize(unique(parallel_nodes.begin(), parallel_nodes.end()) - parallel_nodes.begin());

	for (int i = 0; i < (int)places.size(); i++)
	{
		places[i].effective.espresso();
		sort(places[i].effective.cubes.begin(), places[i].effective.cubes.end());
		places[i].predicate.espresso();
	}
}

/** to_state_graph()
 *
 * This converts a given graph to the fully expanded state space through simulation. It systematically
 * simulates all possible transition orderings and determines all of the resulting state information.
 */
graph graph::to_state_graph(const boolean::variable_set &variables)
{
	graph result;
	vector<pair<simulator::state, iterator> > states;
	vector<pair<simulator, iterator> > simulations;
	vector<deadlock> deadlocks;
	vector<pair<int, int> > to_merge;

	for (int i = 0; i < (int)source.size(); i++)
	{
		boolean::cube reset = 1;
		for (int j = 0; j < (int)source[i].size(); j++)
			reset = reset & source[i][j].state;

		// Set up the initial state which is determined by the reset behavior
		iterator init = result.create(place(reset));
		result.source.push_back(vector<reset_token>(1, reset_token(init.index, reset, false)));

		// Set up the first simulation that starts at the reset state
		simulations.push_back(pair<simulator, iterator>(simulator(this, &variables, i, false), init));

		// Record the reset state in our map of visited states
		simulator::state state = simulations.back().first.get_state();
		vector<pair<simulator::state, iterator> >::iterator loc = lower_bound(states.begin(), states.end(), pair<simulator::state, iterator>(state, iterator()));
		if (loc != states.end() && loc->first == state)
			loc->first.merge(state);
		else
			states.insert(loc, pair<simulator::state, iterator>(state, init));
	}

	while (simulations.size() > 0)
	{
		pair<simulator, iterator> sim = simulations.back();
		simulations.pop_back();

		int enabled = sim.first.enabled();
		simulations.reserve(simulations.size() + enabled);
		for (int i = 0; i < enabled; i++)
		{
			simulations.push_back(sim);
			enabled_transition e = simulations.back().first.local.ready[i];

			if (transitions[e.index].behavior == transition::active)
			{
				// Look to see if this transition has already been drawn in the state graph
				// We know by construction that every transition in the state graph will have exactly one cube
				// and that every transition will be active
				iterator n(place::type, -1);
				for (int j = 0; j < (int)result.arcs[place::type].size() && n.index == -1; j++)
					if (result.arcs[place::type][j].from == simulations.back().second &&
						result.transitions[result.arcs[place::type][j].to.index].action.cubes[0] == transitions[e.index].action.cubes[e.term])
					{
						simulations.back().second = result.arcs[place::type][j].to;
						for (int k = 0; k < (int)result.arcs[transition::type].size() && n == -1; k++)
							if (result.arcs[transition::type][k].from == simulations.back().second)
								n = result.arcs[transition::type][k].to;
					}

				// Since it isn't already in the state graph, we need to draw it
				if (n.index == -1)
					simulations.back().second = result.push_back(simulations.back().second, transitions[e.index].subdivide(e.term));

				// Fire the transition in the simulation
				simulations.back().first.fire(i);

				// Look the see if the resulting state is already in the state graph. If it is, then we are done with this simulation,
				// and if it is not, then we need to put the state in and continue on our way.
				simulator::state state = simulations.back().first.get_state();
				vector<pair<simulator::state, iterator> >::iterator loc = lower_bound(states.begin(), states.end(), pair<simulator::state, iterator>(state, iterator()));
				if (loc != states.end() && loc->first == state)
				{
					if (simulations.size() > 1)
						simulations[simulations.size()-2].first.merge_errors(simulations.back().first);

					if (n.index == -1)
						result.connect(simulations.back().second, loc->second);
					else if (n.index != -1)
					{
						// If we find duplicate records of the same state, then we'll need to merge them later.
						if (loc->second.index > n.index)
							to_merge.push_back(pair<int, int>(loc->second.index, n.index));
						else if (n.index > loc->second.index)
							to_merge.push_back(pair<int, int>(n.index, loc->second.index));
					}

					simulations.pop_back();
				}
				else if ((loc == states.end() || loc->first != state) && n.index == -1)
				{
					boolean::cover encodings = 1;
					for (int j = 0; j < (int)state.encodings.size(); j++)
						encodings &= state.encodings[j];

					simulations.back().second = result.push_back(simulations.back().second, place(encodings));
					states.insert(loc, pair<simulator::state, iterator>(state, simulations.back().second));
				}
				else
				{
					simulations.back().second = n;
					states.insert(loc, pair<simulator::state, iterator>(state, simulations.back().second));
				}
			}
			else
			{
				simulations.back().first.fire(i);
				simulator::state state = simulations.back().first.get_state();
				vector<pair<simulator::state, iterator> >::iterator loc = lower_bound(states.begin(), states.end(), pair<simulator::state, iterator>(state, iterator()));
				if (loc != states.end() && loc->first == state)
				{
					if (simulations.size() > 1)
						simulations[simulations.size()-2].first.merge_errors(simulations.back().first);

					// If we find duplicate records of the same state, then we'll need to merge them later.
					if (loc->second.index > simulations.back().second.index)
						to_merge.push_back(pair<int, int>(loc->second.index, simulations.back().second.index));
					else if (simulations.back().second.index > loc->second.index)
						to_merge.push_back(pair<int, int>(simulations.back().second.index, loc->second.index));

					simulations.pop_back();
				}
				else
					states.insert(loc, pair<simulator::state, iterator>(state, simulations.back().second));
			}
		}

		if (enabled == 0)
		{
			deadlock d(sim.first.local.tokens);
			vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
			if (dloc == deadlocks.end() || *dloc != d)
			{
				error("", d.to_string(variables), __FILE__, __LINE__);
				deadlocks.insert(dloc, d);
			}

			simulations.back().first.merge_errors(sim.first);
		}
	}

	sort(to_merge.rbegin(), to_merge.rend());
	to_merge.resize(unique(to_merge.begin(), to_merge.end()) - to_merge.begin());
	for (int i = 0; i < (int)to_merge.size(); i++)
	{
		for (int j = 0; j < (int)result.arcs[place::type].size(); j++)
		{
			if (result.arcs[place::type][j].from.index == to_merge[i].first)
				result.arcs[place::type][j].from.index = to_merge[i].second;
			else if (result.arcs[place::type][j].from.index > to_merge[i].first)
				result.arcs[place::type][j].from.index--;
		}
		for (int j = 0; j < (int)result.arcs[transition::type].size(); j++)
		{
			if (result.arcs[transition::type][j].to.index == to_merge[i].first)
				result.arcs[transition::type][j].to.index = to_merge[i].second;
			else if (result.arcs[transition::type][j].to.index > to_merge[i].first)
				result.arcs[transition::type][j].to.index--;
		}
		for (int j = 0; j < (int)result.source.size(); j++)
			for (int k = 0; k < (int)result.source[j].size(); k++)
			{
				if (result.source[j][k].index == to_merge[i].first)
					result.source[j][k].index = to_merge[i].second;
				else if (result.source[j][k].index > to_merge[i].first)
					result.source[j][k].index--;
			}
		for (int j = 0; j < (int)result.sink.size(); j++)
			for (int k = 0; k < (int)result.sink[j].size(); k++)
			{
				if (result.sink[j][k].index == to_merge[i].first)
					result.sink[j][k].index = to_merge[i].second;
				else if (result.sink[j][k].index > to_merge[i].first)
					result.sink[j][k].index--;
			}

		result.places.erase(result.places.begin() + to_merge[i].first);
	}

	return result;
}

/* to_petri_net()
 *
 * Converts the HSE into a petri net using index-priority simulation.
 */
graph graph::to_petri_net()
{
	graph result;


	return result;
}

}
