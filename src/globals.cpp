#include "globals.hpp"

using namespace std;

/**
 * @brief This file contains all global variables except the OSM interface. 
 * 
 */


#pragma region LOGGING_SETTINGS
    LOG_LEVEL CURRENT_LEVEL= DEBUG;

    vector<string> LOG_LINES;
    bool FILE_LOGGING = false;
    string LOG_NAME;
    string OUT_DIR="./results";
    ofstream LOG_FILE;
    #define LOG_PARAMETER(parameter) LOG(INFO, boost::wformat(L"%1% = %2%") % #parameter % parameter)
    #define PUT_PARAMETER(parameter) root.put(#parameter, parameter);
    bool SIGINT_RECEIVED = false;


#pragma endregion

namespace MENHIR{

    #pragma region SET_BY_ANALYST

    //ATTENTION: These values should be fixed and are therefore set during compilation
    vector<AType> COLUMN_FORMAT {AType::INT, AType::INT, AType::FLOAT}; 
    size_t VALUE_SIZE=0; //Size of the value in bytes
   //ATTENTION: THIS HAS TO BE PROVIDED AND CAN ONLY BE DEDUCED FOR GENERATED DATA 
    vector<db_t> DATA_RESOLUTION {1,1,1.0f};

    #pragma endregion


    #pragma region SERVER_SETTINGS

    number PORT	 = RPC_PORT;
    #ifndef SERVERLESS
        rpc::server srv(PORT);
    #endif
    number WAIT_BETWEEN_QUERIES = 0uLL;
    string PASSWORD = "CHANGE_ME"; 
    #pragma endregion


    #pragma region DATASOURCE_SETTINGS

    DATASOURCE_T DATASOURCE = GENERATED;
    string DATASET	  = string("data/covid-data.csv");
    //string QUERYSET_TAG	  = string("QUERIES-PUMS-louisiana-0.5-uniform");
    bool INTERACTIVE_QUERIES=false; //only relevant if DATASOURCE != CROWD
    
    double QUERY_RESPONSE_EPSILON =1.0;
    uint NUM_ATTRIBUTES=COLUMN_FORMAT.size();

    vector<db_t> MAX_VALUE;
    vector<db_t> MIN_VALUE;

    #pragma endregion


    #pragma region VOLUME_PATTERN_SANITIZER_SETTINGS

    number DP_K		  = 2ull; //a smaller value to start with might be better 
    number DP_BETA	  = 20ull;
    double DP_EPSILON = 0.693;
    number DP_BUCKETS	  = 0ull;
    number DP_LEVELS	  = 100ull;
    bool USE_TRUNCATED_LAPLACE= false;

    #pragma endregion


    #pragma region GENERATED_DATA_SETTINGS
    int SEED = 1305;
    std::mt19937 GENERATOR;

    double POSSIBLE_TRUE_MIN=10000; //for generating data
    double POSSIBLE_TRUE_MAX=11000; //for generating data
    double SPREAD=0.25; //Spread of data for genrated data: The deviation is max SPREAD*Range from the true values
    number RANGEQUERY_RANGE=10;
    bool GENERATE_NORMAL_DISTRIBUTED_DATA=false;

    number NUM_DATAPOINTS= 20uLL; //for generating data
    
    number NUM_QUERIES = 10uLL; //for generating data
    AggregateFunc QUERY_FUNCTION=AggregateFunc::SUM; //for generating data
    uint QUERY_INDEX=0; //for generating data
    uint WHERE_INDEX=NUM_ATTRIBUTES>1?1:0; //for generating data
    bool POINT_QUERIES = false; //for generating data
    vector<Query> QUERIES; //for generating data
    vector<vector<db_t>> INPUT_DATA; //for generating data

    bool INSERT_BULK=false; //for inserting data in a bulk into the database (THIS OPERATION IS NOT OBLIVIOUS and only for measurment purposes)

    vector<number> RETRIEVE_EXACTLY;
    number RETRIEVE_EXACTLY_NOW;
    bool CONTROL_SELECTIVITY = false;
    double MAX_SENSITIVITY = 0.05; //value in percent, 1 is max
    double SENSITIVITY_STEPS =10; //number of steps for the selectivity


    double AVAILABLE_BUDGET=QUERY_RESPONSE_EPSILON;
    double TOLERANCE = 0.00001;



    #pragma endregion


    #pragma region ORAM_GLOBALS

    mutex PROFILE_MUTEX;
    vector<profile> PROFILES;
    vector<profile> ALL_PROFILES;

    number ORAM_LOG_CAPACITY= 16uLL;
    number ORAM_Z= 3uLL;
    number STASH_FACTOR=4ull;
    bool USE_ORAM= true;
    number BATCH_SIZE= 1uLL;

    bool USE_GAMMA=false;

    #pragma endregion


    #pragma region FILE_SETTINGS

    string FILES_DIR		 = "./storage-files";
    const string ORAM_STASH_FILE	 = "oram-stash";
    const string DATA_INPUT_FILE	 = "data-input";
    const string QUERY_INPUT_FILE	 = "query-input";
    const string SCHEMA_INPUT_FILE	 = "schema-input";
    const string INPUT_FILES_DIR = string("../../experiments-scripts/output/");
    const string RAM_LOCK_FILE= "ram-lock.db";

    #pragma endregion


    #pragma region MEASURMENT_GLOBALS
    vector<number> *INSERTION_MEASUREMENTS;
    vector<number> *DELETION_MEASUREMENTS;
    number INSERTION_TOTAL;
    vector<measurement> *QUERY_MEASUREMENTS;
    number NUM_THREADS=1;
    bool DELETION=false;
    chrono::steady_clock::time_point timestampBeforeORAMs;
    chrono::steady_clock::time_point timestampAfterORAMs;			 
    chrono::steady_clock::time_point startTimeQuery;
    chrono::steady_clock::time_point endTimeQuery;
    #pragma endregion
}