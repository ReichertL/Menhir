#include "definitions.h"
#include "utility.hpp"
#include "globals.hpp"
#include "output_utility.hpp"
#include "parse_args.hpp"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <numeric>
#include <string>

/**
 * @brief This file contains functions for parsing the command line arguments and logging these arguments. 
 */

using namespace std;
using namespace MENHIR;
namespace po = boost::program_options;

/**
 * @brief This function parses the command line arguments and sets the corresponding values in globals.cpp
 * 
 * @param argc 
 * @param argv 
 */
void readCommandlineArgs(int argc, char* argv[]){
	auto betaCheck			= [](number v) { if (v < 1) {  __throw_invalid_argument("malformed --beta, must be >= 1"); } };
	auto bucketsNumberCheck = [](int v) {
		auto logV = log(v) / log(DP_K);
		if (ceil(logV) != floor(logV))
		{
			std::ostringstream oss;
			oss<<"malformed --bucketsNumber, must be a power of "<<DP_K<<endl;
			 __throw_invalid_argument(oss.str().c_str());
		}
	};
	string columnsString, resolutionString, aggregateFuncString, logLevelString, dataSourceString, retrieveExactlyString="";
	string minString, maxString="";

	po::options_description desc("range query processor", 120);
	desc.add_options()("help,h", "produce help message");

	//basic options
	desc.add_options()("datasource,d", po::value<string>(&dataSourceString)->default_value(dataSourceString), "if set, will generate ORAM and tree indices, read files or conduct crowd sourcing. Options: GENERATED, FROM_REAL, FROM_FILE, CROWD. Default: GENERATED");
	desc.add_options()("dataset", po::value<string>(&DATASET)->default_value(DATASET), "When DATASOURCE=FROM_REAL: path to dataset file to be used");
	desc.add_options()("datapoints", po::value<number>(&NUM_DATAPOINTS)->default_value(NUM_DATAPOINTS), "number of synthetic records to generate. default:"+NUM_DATAPOINTS);
	desc.add_options()("seed", po::value<int>(&SEED)->default_value(SEED), "To use if in DEBUG mode (otherwise OpenSSL will sample fresh randomness). Default:"+SEED);
	desc.add_options()("cols,c", po::value<string>(&columnsString)->default_value(columnsString), "Format of columns for the table. Provide a combination of"\
																								" i and f to signify a column of int or float values. The resolution will be automatically deduced if not provided.");
	desc.add_options()("valueSize", po::value<size_t>(&VALUE_SIZE)->default_value(VALUE_SIZE), "Size of the unindexed value stored with each database entry. Is not indexed. Default size:"+VALUE_SIZE);
	desc.add_options()("port", po::value<number>(&PORT)->default_value(PORT), "Port to use in case data is collected from the CROWD (so if -d CROWD).Default:"+PORT);
	
	//dataset dependent options (required for volume sanitation). If data is collected from a crowd these values are set to the expected values. 
	desc.add_options()("resolution,r", po::value<string>(&resolutionString)->default_value(resolutionString), "Resolution of the incoming data per column. Will only be considered if cols is also set via the command line. "\
																												" If not set, this will be deduced automatically, which might lead to  suboptimal DP results."\
																												" Default resolution FLOAT: 0.01, Default resolution INT: 1");
	desc.add_options()("minVals", po::value<string>(&minString)->default_value(minString), "Minimum Values for each column. Used to compute Domain Size and DP Noise for Query Response. Values can not be smaller than the MIN_VALUE for this column.");
	desc.add_options()("maxVals", po::value<string>(&maxString)->default_value(maxString), "Maximum Values for each column. Used to compute Domain Size and DP Noise for Query Response. Values can not be larger than the MAX_VALUE for this column.");
	


	//options related to querying and query sanitation
	desc.add_options()("query-epsilon", po::value<double>(&QUERY_RESPONSE_EPSILON)->default_value(QUERY_RESPONSE_EPSILON), "Privacy Budget available at the beginning. This budget is used up by answering queries. DEFAULT:");
	desc.add_options()("interactiveQueries", po::value<bool>(&INTERACTIVE_QUERIES)->default_value(INTERACTIVE_QUERIES), "allows QUERIES to be posed in an interactive manner against artifical or from-file data. DEFAULT:"+INTERACTIVE_QUERIES);
	desc.add_options()("numQueries", po::value<number>(&NUM_QUERIES)->default_value(NUM_QUERIES), "number of synthetic QUERIES to generate or real QUERIES to read. DEFAULT:"+NUM_QUERIES);
	desc.add_options()("agg", po::value<string>(&aggregateFuncString)->default_value(aggregateFuncString), "defines the default function run for queries when automated querying. Options are: SUM, MEAN, VARIANCE, COUNT, MIN_COUNT, MAX_COUNT, COUNT_NO_DP. Default is SUM.");
	desc.add_options()("queryIndex", po::value<uint>(&QUERY_INDEX)->default_value(QUERY_INDEX), "Will run the QUERIES against the ith attribute. Index starts with 0. DEFAULT:"+QUERY_INDEX);
	desc.add_options()("whereIndex", po::value<uint>(&WHERE_INDEX)->default_value(WHERE_INDEX), "If there are more than one column, this column can be used for filtering data points. The aggregate is computed over the column given by QUERY_INDEX. Index starts with 0. DEFAULT:"+WHERE_INDEX);
	desc.add_options()("pointQueries", po::value<bool>(&POINT_QUERIES)->default_value(POINT_QUERIES), "if set, will run point QUERIES (against left endpoint) instead of range QUERIES. DEFAULT:"+POINT_QUERIES);
	
	//options related to volume sanitation
	desc.add_options()("useTruncatedLaplace", po::value<bool>(&USE_TRUNCATED_LAPLACE)->default_value(USE_TRUNCATED_LAPLACE), "Per default, standard Laplace  used for the sanitizing the volume of query responses. For consistent privacy guarantees in line with the paper, TruncatedLaplace needs to be used. This results in slightly less noise than standard Laplace as used by the Epsolut paper.");
	desc.add_options()("fanout,k", po::value<number>(&DP_K)->default_value(DP_K), "Tree fanout for volume pattern sanitizer.");
	desc.add_options()("sanitizer-epsilon", po::value<double>(&DP_EPSILON)->default_value(DP_EPSILON), "epsilon parameter for DP used to hide volume patterns.");
	desc.add_options()("useGamma", po::value<bool>(&USE_GAMMA)->default_value(USE_GAMMA), "set to true to use the GAMMA volume pattern sanitation mechanism to hide volume patterns if more than one OSM is used (i.e NUM_DATAPOINTs is larger than 2^16). Default:false");
	desc.add_options()("bucketsNumber,b", po::value<number>(&DP_BUCKETS)->notifier(bucketsNumberCheck)->default_value(DP_BUCKETS), "the number of buckets for volume pattern sanitizer (if 0, will choose max buckets such that less than the domain size)");
	desc.add_options()("beta", po::value<number>(&DP_BETA)->notifier(betaCheck)->default_value(DP_BETA), "Beta parameter (failure probability) for volume pattern sanitizer according to Epsolute paper. In case  truncated laplace is used, this parameter is used as delta_s for the (epsilon_s, delta_s)-volume sanitation. Pass x such that beta = 2^{-x}.");
	desc.add_options()("levels", po::value<number>(&DP_LEVELS)->default_value(DP_LEVELS), "number of levels to keep in DP tree of volume pattern sanitizer(0 for choosing optimal for given number of QUERIES)");


	
	//options related to the ORAM
	desc.add_options()("useOram,u", po::value<bool>(&USE_ORAM)->default_value(USE_ORAM), "if set will use ORAMs, otherwise for each query the whole database is processed (Naive Approach)");
	desc.add_options()("oramsZ,z", po::value<number>(&ORAM_Z)->default_value(ORAM_Z), "the Z parameter for ORAMs");
	desc.add_options()("stashFactor", po::value<number>(&STASH_FACTOR)->default_value(STASH_FACTOR), "Constant for changing the ORAMs Stash size. Usually it is 4  but can be changed if failures occure.");
	desc.add_options()("logcapacity", po::value<number>(&ORAM_LOG_CAPACITY)->default_value(ORAM_LOG_CAPACITY), "Depth of the tree in the ORAM. Usually 2^16.");
	desc.add_options()("batch", po::value<number>(&BATCH_SIZE)->default_value(BATCH_SIZE), "batch size to use in storage adapters"); 
	
	//options useful for evaluation
	desc.add_options()("insertBulk", po::value<bool>(&INSERT_BULK)->default_value(INSERT_BULK), "Set true to insert data in a bulk into the database. THIS OPERATION IS NOT OBLIVIOUS and only for measurment purposes.");
	desc.add_options()("retrieveExactly", po::value<string>(&retrieveExactlyString)->default_value(retrieveExactlyString), "if not zero: Overwrites other setting for querying and retrieves exactly the set number of values from the database.");
	desc.add_options()("controlSelectivity", po::value<bool>(&CONTROL_SELECTIVITY)->default_value(CONTROL_SELECTIVITY), "If Data is generated artifically, this setting can be used to control the query selectivity so various sensitivities can be measured.");
	desc.add_options()("maxSensitivity", po::value<double>(&MAX_SENSITIVITY)->default_value(MAX_SENSITIVITY), "Allows to set the maximal selectivity of generated queries. All queries will have the same selectivity and data will not follow noramal distribution. Selectivity queries is approximated and will approximate MAX_SELECIVITY in 10 steps.");
	desc.add_options()("normalDistribution", po::value<bool>(&GENERATE_NORMAL_DISTRIBUTED_DATA)->default_value(GENERATE_NORMAL_DISTRIBUTED_DATA), "if true: Data will be normale distributed. Only applicable if data is generated.");
	desc.add_options()("rangeQueryRange", po::value<number>(&RANGEQUERY_RANGE)->default_value(RANGEQUERY_RANGE), "Width of automatically generated range queries.");
	desc.add_options()("wait", po::value<number>(&WAIT_BETWEEN_QUERIES)->default_value(WAIT_BETWEEN_QUERIES), "if set, will wait specified number of milliseconds between QUERIES");
	desc.add_options()("deletion", po::value<bool>(&DELETION)->default_value(DELETION), "if set, all data points will be inserted. Then the last data point will be deleted. FOR MEASUREMENTS.");


	//options related to logging
	desc.add_options()("verbosity,v", po::value<string>(&logLevelString)->default_value(logLevelString), "verbosity level to output");
	desc.add_options()("fileLogging", po::value<bool>(&FILE_LOGGING)->default_value(FILE_LOGGING), "if set, log stream will be duplicated to file (noticeably slows down simulation)");
	desc.add_options()("outdir", po::value<string>(&OUT_DIR)->default_value(OUT_DIR), "Output directory for log and json files. Default: ./results");
	desc.add_options()("filesDir", po::value<string>(&FILES_DIR)->default_value(FILES_DIR), "if datasource is set to read FROM_FILE, this directory will be used as base directory.");



	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		cout << desc << "\n";
		exit(1);
	}

	//set loglevel
	if(logLevelString!=""){
		LOG_LEVEL temp=loglevelFromString(logLevelString);
		if(temp==LOG_LEVEL::LOG_LEVEL_INVALID){
			LOG(INFO, L"Option passed with -v was not valid. The Log Level will be set to " +toWString(toString(CURRENT_LEVEL)));
		}else{
			CURRENT_LEVEL=temp;
		}
	}

	//create directory for log file
	boost::filesystem::path filepath = std::string(OUT_DIR);
	bool filepathExists= boost::filesystem::create_directory(OUT_DIR);
	if(not filepathExists){
		LOG(INFO,boost::wformat(L"Created output directory  %s")%toWString(OUT_DIR));
	}

	// open log file
	auto timestamp = chrono::duration_cast<chrono::nanoseconds>(chrono::steady_clock::now().time_since_epoch()).count();
	auto rawtime   = time(nullptr);
	stringstream timestream;
	timestream << put_time(localtime(&rawtime), "%Y-%m-%d-%H-%M-%S");
	LOG_NAME = boost::str(boost::format("%1%--%2%--%3%") % SEED % timestream.str() % timestamp);


	//parse string containing column format
	if(columnsString!=""){
		COLUMN_FORMAT=vector<AType>();
		DATA_RESOLUTION=vector<db_t>();
		vector<string> valsC;	
		boost::algorithm::split(valsC, columnsString, boost::is_any_of(","));

		for (size_t i = 0; i < valsC.size(); i++){
			//LOG(DEBUG,boost::wformat(L"%d")%i);
			string c= valsC[i];
			if(c=="i"){
				COLUMN_FORMAT.push_back(AType::INT);
				DATA_RESOLUTION.push_back(db_t(1));
			}else if(c=="f"){
				COLUMN_FORMAT.push_back(AType::FLOAT);
				DATA_RESOLUTION.push_back(db_t(0.01f));
			}
		}
		NUM_ATTRIBUTES=COLUMN_FORMAT.size();
		LOG_PARAMETER(NUM_ATTRIBUTES);

		if(resolutionString!=""){
			DATA_RESOLUTION=vector<db_t>();
			vector<string> valsR;	
			boost::algorithm::split(valsR, resolutionString, boost::is_any_of(","));

			if(valsR.size()==NUM_ATTRIBUTES){			
				for (size_t i = 0; i < valsR.size(); i++){
					//LOG(DEBUG,boost::wformat(L"%d")%i);
					if(COLUMN_FORMAT[i]==AType::INT){
						DATA_RESOLUTION.push_back(db_t(stoi(valsR[i])));
					}else if(COLUMN_FORMAT[i]==AType::FLOAT){
						DATA_RESOLUTION.push_back(db_t(stof(valsR[i])));
					}
				}
			}else{
				LOG(CRITICAL, L"The provided data resolution had a different number of entries than the provided columns. This is not allowed.");
			}		
		}	
	}

	//parse string containing minimum value per column
	if(minString!=""){
		vector<string> valsC;	
		boost::algorithm::split(valsC, minString, boost::is_any_of(","));
		if(valsC.size()!=NUM_ATTRIBUTES){
			LOG(CRITICAL, L"The vector of minimal values passed to the programs was unequal to the number of attributes. Please provide a value for each column.");
		}
		for (size_t i = 0; i < NUM_ATTRIBUTES; i++){
			string smin=valsC[i];
			db_t dbt_min=DBT::fromString(smin,COLUMN_FORMAT[i]);
			MIN_VALUE.push_back(dbt_min);
		}
	}else{
		for(size_t i=0;i<NUM_ATTRIBUTES;i++){
			MIN_VALUE.push_back(DBT::fromDouble(POSSIBLE_TRUE_MIN,COLUMN_FORMAT[i]));
		}
	}

	//parse string containing maximal value per column
	if(maxString!=""){
		vector<string> valsC;	
		boost::algorithm::split(valsC, maxString, boost::is_any_of(","));
		if(valsC.size()!=NUM_ATTRIBUTES){
			LOG(CRITICAL, L"The vector of maximum values passed to the programms was uneqal to the number of attributes. Please provide a value for each column.");
		}
		for (size_t i = 0; i < NUM_ATTRIBUTES; i++){
			string smax=valsC[i];
			MAX_VALUE.push_back(DBT::fromString(smax,COLUMN_FORMAT[i]));
		}
	}else{
		for(size_t i=0;i<NUM_ATTRIBUTES;i++){
			MAX_VALUE.push_back(DBT::fromDouble(POSSIBLE_TRUE_MAX,COLUMN_FORMAT[i]));
		}
	}

	//parse string containing DP_k value for each column. This allows for improved volume sanitation dependent on the data format of each column.
	/*if(dpkString!=""){

	}else{
		DP_K=vector<number>(DP_K_default, NUM_ATTRIBUTES);	
	}*/

	/*if((DATASOURCE==GENERATED or DATASOURCE==FROM_FILE) and NUM_DATAPOINTS>ORAM_CAPACITY){
		LOG(WARNING, L"The current capacity of the ORAM would be to small to fit all generated data points. Will therefore increase the ORAM capacity.");
		ORAM_CAPACITY=NUM_DATAPOINTS;
	}*/



	if (FILE_LOGGING)
	{
		LOG_FILE.open(boost::str(boost::format("%1%/%2%.log") %OUT_DIR % LOG_NAME), ios::out);
	}


	if (CONTROL_SELECTIVITY and WHERE_INDEX!=QUERY_INDEX)
	{
		LOG(WARNING, L"WHERE_INDEX is ignored when sensitivity is managed for simplicity. Setting WHERE_INDEX=QUERY_INDEX.");
		WHERE_INDEX=QUERY_INDEX;
	}

	if (POINT_QUERIES and CONTROL_SELECTIVITY)
	{
		LOG(WARNING, L"Sensitivity of queries can only be managed for range queries. Selectivity is not managed.");
	}

	if (DELETION and INSERT_BULK)
	{
		INSERT_BULK=false;
		LOG(WARNING, L"If deletion is to be measured, not all data can be inserted as bulk. Setting: INSERT_BULK=0");
	}

	if (POINT_QUERIES)
	{
		LOG(WARNING, L"In POINT_QUERIES mode, DP tree becomes DP list. Setting DP_LEVELS to 1.");
		DP_LEVELS = 1;
	}

	if (NUM_ATTRIBUTES <=QUERY_INDEX)
	{
		LOG(WARNING, L"The Index of the attribute to be queried is larger than the number of columns of the table. Setting Query index to 0.");
		QUERY_INDEX=0;
	}

	if (NUM_ATTRIBUTES <=WHERE_INDEX)
	{
		LOG(WARNING, L"The Index of the attribute used in the where clause is larger than the number of columns of the table. Setting WHERE_INDEX to 0 if there is only one Columne otherwise setting it to 1.");
		if(NUM_ATTRIBUTES==1){
			WHERE_INDEX=0;
		}else{
			WHERE_INDEX=1;
		}
	}

	AVAILABLE_BUDGET=QUERY_RESPONSE_EPSILON;

	if(aggregateFuncString!=""){
		AggregateFunc temp=aggregateFuncFromString(aggregateFuncString);
		if(temp==AggregateFunc::AggregateFunc_INVALID){
 			LOG(INFO, L"Option passed with -agg was not valid. The aggregation function for query respones will be set to " +toWString(toString(QUERY_FUNCTION)));
		}else{
			QUERY_FUNCTION=temp;
		}
	}

	if(retrieveExactlyString!=""){
		RETRIEVE_EXACTLY=retieveExactlyfromString(retrieveExactlyString);
	}


	if(dataSourceString!=""){
		DATASOURCE_T temp=datasourcefromString(dataSourceString);
		if(temp==DATASOURCE_T::DATASOURCE_T_INVALID){
			LOG(INFO, L"Option passed with -v was not valid. The Log Level will be set to " +toWString(toString(DATASOURCE)));
		}else{
			DATASOURCE=temp;
		}
	}

	if (DATASOURCE==GENERATED){
		LOG(INFO, L"Generating indices...");		
		boost::filesystem::remove_all(FILES_DIR);
		boost::filesystem::create_directories(FILES_DIR);
	}else if (DATASOURCE==FROM_FILE){
		LOG(INFO,  L"Reading from input files located in "+toWString(FILES_DIR));		

	}
	
	//initalized arrays for storing measurements for evaluation
	MENHIR::INSERTION_MEASUREMENTS=new vector<number>();
	MENHIR::DELETION_MEASUREMENTS=new vector<number>();
	MENHIR::QUERY_MEASUREMENTS=new vector<measurement>();

	//initialize random generator used for generating data
	srand(SEED);
	GENERATOR.seed(SEED);

}

/**
 * @brief Write all global variables to the log.
 * 
 */
void logGlobals(){
	
	LOG_PARAMETER(FILE_LOGGING);
	LOG(INFO,boost::wformat(L"DATASOURCE = %1%") % (toInt(DATASOURCE)) );
	//LOG_PARAMETER(DATASOURCE);
	LOG(INFO,L"OUT_DIR: "+toWString(OUT_DIR));
	LOG(INFO,L"FILES_DIR: "+toWString(FILES_DIR));

	LOG_PARAMETER(NUM_DATAPOINTS);
	LOG_PARAMETER(NUM_QUERIES);
	LOG_PARAMETER(DELETION);
	LOG_PARAMETER(NUM_ATTRIBUTES);
	string columns="";
	for(size_t i=0;i<NUM_ATTRIBUTES;i++){
		columns+=DBT::aTypeToString(COLUMN_FORMAT[i]);
		if(i!=NUM_ATTRIBUTES-1){
			columns+=",";
		}
	}
	LOG(INFO, L"COLUMN_FORMAT: "+toWString(columns));

	string resolutions="";
	for(size_t i=0;i<NUM_ATTRIBUTES;i++){
		resolutions+=DBT::toString(DATA_RESOLUTION[i]);
		if(i!=NUM_ATTRIBUTES-1){
			resolutions+=",";
		}
	}	
	LOG(INFO,L"DATA_RESOLUTION: "+toWString(resolutions));

	string minString="";
	for(size_t i=0;i<NUM_ATTRIBUTES;i++){
		minString+=DBT::toString(MIN_VALUE[i]);
		if(i!=NUM_ATTRIBUTES-1){
			minString+=",";
		}
	}	
	LOG(INFO,L"MIN_VALUE: "+toWString(minString));

	string maxString="";
	for(size_t i=0;i<NUM_ATTRIBUTES;i++){
		maxString+=DBT::toString(MAX_VALUE[i]);
		if(i!=NUM_ATTRIBUTES-1){
			maxString+=",";
		}
	}	
	LOG(INFO,L"MAX_VALUE: "+toWString(maxString));
	LOG_PARAMETER(RANGEQUERY_RANGE);

	LOG(INFO, boost::wformat(L"DATASET = %1%") % toWString(DATASET));
	//LOG(INFO, boost::wformat(L"QUERYSET_TAG = %1%") % toWString(QUERYSET_TAG));


	LOG_PARAMETER(NUM_DATAPOINTS);
    
	ostringstream retrieveExactlyoss;
	for(size_t i=0;i<RETRIEVE_EXACTLY.size();i++){
		retrieveExactlyoss<<RETRIEVE_EXACTLY[i];
		if(i!=RETRIEVE_EXACTLY.size()-1){
			retrieveExactlyoss<<",";
		}
	}		
	LOG(INFO,L"RETRIEVE_EXACTLY: "+toWString(retrieveExactlyoss.str()));
	LOG_PARAMETER(QUERY_RESPONSE_EPSILON);
	LOG_PARAMETER(QUERY_INDEX);
	LOG_PARAMETER(WHERE_INDEX);
	LOG(INFO,boost::wformat(L"QUERY_FUNCTION = %1%") % (toInt(QUERY_FUNCTION)) );
	LOG_PARAMETER(POINT_QUERIES);
	LOG_PARAMETER(NUM_QUERIES);
	LOG_PARAMETER(MAX_SENSITIVITY);
	LOG_PARAMETER(SEED);
	LOG_PARAMETER(USE_GAMMA);

	LOG_PARAMETER(ORAM_LOG_CAPACITY);
	LOG_PARAMETER(ORAM_Z);
	LOG_PARAMETER(STASH_FACTOR);
	LOG_PARAMETER(USE_ORAM);
	LOG_PARAMETER(BATCH_SIZE);
	LOG_PARAMETER(DP_K);
	LOG_PARAMETER(DP_BETA);
	LOG_PARAMETER(DP_EPSILON);

}