#pragma once

#include "definitions.h"
#include "struct_querying.hpp"
#include "struct_error.hpp"

#include <string>
#include <iostream>

/**
 * @brief This file contains function of general utility.
 */

int isRAMAvailable(int id,int required);

void freeRAM(int id);

string toString(LOG_LEVEL level);

LOG_LEVEL loglevelFromString(string logLevelString);

wstring toWString(LOG_LEVEL level);

void LOG(LOG_LEVEL level, wstring message);

void LOG(LOG_LEVEL level, boost::wformat message);

namespace MENHIR
{
	using namespace std;

	string toString(DATASOURCE_T source);
	int toInt(DATASOURCE_T source);
	DATASOURCE_T datasourcefromString(string dataSourceString);

	vector<number> retieveExactlyfromString(string retrieveExactlyString);
	string errToString(Error err);

	void print_err(Error err, LOG_LEVEL level=WARNING);
	
	wstring timeToString(long long time);

	wstring bytesToString(long long bytes);

	wstring toWString(string input);
	
	bytes bytesFromString(string s);
	
	number bytesToNumber(vector<uchar> b);

	string queryToString(Query q);


	string queryToCSVString(Query q);

	Query queryFromCSVString(string s);

	AggregateFunc getAggFromString(string s);

	vector<db_t> getEmptyRow(vector<AType> columnFormat);
}
