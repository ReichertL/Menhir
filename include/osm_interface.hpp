#pragma once 

#include <vector>
#include <string>
#include <thread>
#include <future>


#include "definitions.h"
#include "globals.hpp"
#include "database_type.hpp"
#include "avl_multiset.hpp"
#include "linear_db.hpp"
#include "volume_sanitizer_utility.hpp"

/**
 * @brief This file contains functions for parallelized access to the AVLTrees. Each Tree is stored in one ORAM. 
 * Parallelization improves the runtime of queries.
 * 
 */



using namespace PathORAM;
using namespace DBT;
using namespace MENHIR;
using hist_t=vector<pair<db_t,number>>;

class OSMInterface {


    void createNewTree();
    void createNewTreeWithDataParallel(size_t osmIndex, vector<vector<db_t>> inputSplit, size_t thisSize,promise<tuple<void *, vector<hist_t>>> *promise);
    vector<hist_t> createNewHistogram();
	void generateVolumeSanitizer();

public:
    size_t numOSMs;
    vector<DOSM::AVLTree *> trees;
    vector<LinearDB::LinearOblivDB *> lists;
    size_t maxPerTree;
    vector<vector<hist_t>> histograms; //one for ODB and each column one histogram 
    bool USE_ORAM=true;
    vector<VolumeSanitizer> volumeSanitizers;

    OSMInterface();

    size_t insert(vector<db_t> key);
    #ifndef NDEBUG
        void insert(vector<db_t> key, size_t nodeHash);
    #endif
    void deleteEntry(db_t key, size_t nodeHash, ulong column);

    tuple<number,vector<number>> getTotalFromHistograms(db_t from, db_t to, size_t column);
    void findIntervalParallel(size_t osmIndex, db_t startKey, db_t endKey, ushort column, number estimate,promise<dbResponse> *promise);
    dbResponse findInterval(db_t startKey, db_t endKey, ushort column, vector<number> estimates);

};
