#pragma once

#include "definitions.h"
#include "database_type.hpp"
#include <sstream>


/**
 * @brief File defining Query object and aggregate functions for queries . 
 * 
 */

namespace MENHIR{
	
	enum AggregateFunc{
		SUM , 
		MEAN , 
		VARIANCE ,  //requires ddof is passed
		COUNT ,
		MAX_COUNT , //returns element with the hightest count - only for int, requires a width parameter otherwise using 1
		MIN_COUNT, //returns element with the lowest count- only for int, requires a width parameter otherwise using 1
		AggregateFunc_INVALID
	};


    string toString(AggregateFunc func);
    int toInt(AggregateFunc func);
    AggregateFunc aggregateFuncFromString(string aggregateFuncString);



	typedef struct {
		uint attributeIndex;
		db_t from;
		db_t to;  //not used if pointQuery is true
		bool pointQuery;
		uint whereIndex;
		db_t whereFrom;
		db_t whereTo; 	
		AggregateFunc agg;
		double epsilon;
		int extra; //only applicable if agg = MAX_COUNT or agg=VARIANCE
	} Query;

}