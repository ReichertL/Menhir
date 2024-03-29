#pragma once

#include "database_type.hpp"
#include "definitions.h"

#include <vector>
#include <chrono>

/**
 * @brief This file contains a code for a naive database in an ORAM based on linear scanning. 
 * This exists solely for comparing runtimes of Menhir against the naive approach.
 * 
 */


namespace LinearDB{    
using namespace MENHIR;
using namespace DBT;

using listEntry = tuple<vector<db_t>,size_t,bytes>;



class LinearOblivDB{



    vector<AType> columnFormat;
    vector<listEntry> listOfNodes;

    size_t numColumns;
    time_t startTime = chrono::system_clock::to_time_t(chrono::system_clock::now());


    public:
        LinearOblivDB(vector<AType> columnFormat);
        LinearOblivDB(vector<AType> cF, vector<vector<db_t>> *inputData, size_t numDatapointsAtStart);
        size_t insert(vector<db_t> keys, MENHIR::bytes value);
        size_t insert(vector<db_t> keys);
        size_t insert(vector<db_t> key , size_t nodeHash);
        dbResponse findInterval(db_t startKey, db_t endKey, ushort column, number estimate);
        size_t size();
};  
}