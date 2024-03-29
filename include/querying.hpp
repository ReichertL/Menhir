#pragma once

#include "definitions.h"
#include "struct_querying.hpp"
#include "struct_error.hpp"
#include  "globals_osm.hpp"

/**
 * @brief This file contains functions for querying the database and determining the amount of volume sanitation required.
 * 
 */

namespace MENHIR{
	using namespace DBT;
	tuple<db_t,double, Error> runQuery(Query query);
	tuple<number,dbResponse,Error> runQueryEval(Query query);
	tuple<db_t,double, Error> computeQueryFunctionPrivate(Query query, vector<db_t> values,  vector<bool> ignoring);


	tuple<number, vector<number>,Error> getNoisePointQuery(Query query);
	tuple<vector<pair<number,number>>,Error> getNoiseNodesForRangeQuery(Query query);
	tuple<number, vector<number>,Error> getNoiseRangeQuery(Query query);
	tuple<number,vector<number>,Error> getTotalNoise(Query query);

	void automated_querying();
	void processMeasurement(Query query, int q, number estimate, dbResponse allRecords);

}