#pragma once

#include "definitions.h"
#include "database_type.hpp"
#include "struct_querying.hpp"
#include "struct_volume_sanitizer.hpp"

#include <fstream>
#include <mutex>
#include <random>
#include <string.h>
#include <rpc/server.h>
#include <chrono>

/**
 * @brief This file contains all global variables except the OSM interface. 
 * 
 */

#pragma region LOGGING_SETTINGS

    extern LOG_LEVEL CURRENT_LEVEL;
    extern vector<string> LOG_LINES;
    extern bool FILE_LOGGING ;
    extern string LOG_NAME;
    extern string OUT_DIR;
    extern ofstream LOG_FILE;
   
    extern bool SIGINT_RECEIVED ;


#pragma endregion


namespace MENHIR{


    using namespace std;

    #pragma region GLOBALS



    #pragma region SERVER_SETTINGS

    extern number PORT;
    #ifndef SERVERLESS
        extern rpc::server srv;
    #endif

    extern number WAIT_BETWEEN_QUERIES;
    extern string PASSWORD;

    #pragma endregion


    #pragma region DATASOURCE_SETTINGS

    extern DATASOURCE_T DATASOURCE;
    extern string DATASET;
    extern string QUERYSET_TAG;
    extern bool INTERACTIVE_QUERIES;

    extern vector<db_t> MIN_VALUE;
    extern vector<db_t> MAX_VALUE;


    extern int SEED;
    extern std::mt19937 GENERATOR;


    extern double QUERY_RESPONSE_EPSILON;
    #pragma endregion


    #pragma region VOLUME_PATTERN_SANITIZER_SETTINGS

    extern number DP_K;
    extern number DP_BETA;
    extern double DP_EPSILON;
    extern number DP_BUCKETS;
    extern number DP_LEVELS;
    extern bool USE_TRUNCATED_LAPLACE;

    #pragma endregion


    #pragma region DATABASE_SETTINGS

    extern vector<AType> COLUMN_FORMAT;
    extern vector<db_t> DATA_RESOLUTION;
    extern size_t VALUE_SIZE;
    extern number NUM_DATAPOINTS;
    extern uint NUM_ATTRIBUTES;
    
    extern double POSSIBLE_TRUE_MAX; //for generating data
    extern double POSSIBLE_TRUE_MIN; //for generating data
    extern double SPREAD;
    extern number RANGEQUERY_RANGE;
    extern bool GENERATE_NORMAL_DISTRIBUTED_DATA;
    
    extern number NUM_QUERIES;
    extern AggregateFunc QUERY_FUNCTION;
    extern uint QUERY_INDEX;
    extern uint WHERE_INDEX;//only applicable if there are more than one column
    extern bool POINT_QUERIES;
    extern vector<Query> QUERIES;
    extern vector<vector<db_t>> INPUT_DATA;

    extern bool INSERT_BULK;

    extern vector<number> RETRIEVE_EXACTLY;
    extern number RETRIEVE_EXACTLY_NOW;
    extern bool CONTROL_SELECTIVITY;
    extern double MAX_SENSITIVITY;
    extern double SENSITIVITY_STEPS;

    extern double AVAILABLE_BUDGET;
    extern double TOLERANCE;



    #pragma endregion


    #pragma region ORAM_GLOBALS

    extern mutex PROFILE_MUTEX;
    extern vector<profile> PROFILES;
    extern vector<profile> ALL_PROFILES;

    extern number ORAM_LOG_CAPACITY;
    extern number ORAM_Z;
    extern number STASH_FACTOR;
    extern bool USE_ORAM;
    extern number BATCH_SIZE;

   
    extern bool USE_GAMMA;

    #pragma endregion


    #pragma region FILE_SETTINGS

    extern string FILES_DIR;
    extern const string DATA_INPUT_FILE;
    extern const string QUERY_INPUT_FILE;
    extern const string SCHEMA_INPUT_FILE;
    extern const string INPUT_FILES_DIR ;
    extern const string RAM_LOCK_FILE; 



    #pragma endregion


    extern vector<number> *INSERTION_MEASUREMENTS;
    extern vector<number> *DELETION_MEASUREMENTS;    
    extern number INSERTION_TOTAL;
    extern vector<measurement> *QUERY_MEASUREMENTS;
    extern number NUM_THREADS;
    extern bool DELETION;
    extern chrono::steady_clock::time_point timestampBeforeORAMs;
    extern chrono::steady_clock::time_point timestampAfterORAMs;			 
    extern chrono::steady_clock::time_point startTimeQuery;
    extern chrono::steady_clock::time_point endTimeQuery;

}