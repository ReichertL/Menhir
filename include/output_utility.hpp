#pragma once

#include "definitions.h"
#include "globals.hpp"
#include "utility.hpp"
#include "database_type.hpp"
#include "struct_querying.hpp"
#include "globals_osm.hpp"

#include <iostream>
#include <numeric>
#include <string>

/**
 * @brief This file contains functions related to writing output files.
 * 
 */
 
namespace MENHIR
{
	using namespace std;
	using namespace DBT;
	using namespace DOSM;


	string filename(string filename, int i);
	template <class INPUT, class OUTPUT>
	vector<OUTPUT> transform(const vector<INPUT>& input, function<OUTPUT(const INPUT&)> application);



	void storeInputs(vector<Query>& QUERIES, vector<vector<db_t>> &data);
	void loadInputs();
	pair<number, number> avg(function<number(const measurement&)> getter);


	void writeOutput();

	bool regexmatch_float(string s);
	bool regexmatch_int(string s);


	template <class INPUT, class OUTPUT>
	vector<OUTPUT> transform(const vector<INPUT>& input, function<OUTPUT(const INPUT&)> application)
	{
		vector<OUTPUT> output;
		output.resize(input.size());
		transform(input.begin(), input.end(), output.begin(), application);

		return output;
	}




}


