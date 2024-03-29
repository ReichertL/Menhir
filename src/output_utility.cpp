#include "output_utility.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <numeric>
#include <string>
#include <regex>

/**
 * @brief This file contains functions related to writing output files.
 * 
 */

using namespace std;
using namespace DBT;
namespace pt = boost::property_tree;

namespace MENHIR{

/**
 * @brief Constructs a file path, based on global variable FILES_DIR, the filename and an integer.
 * 
 * @param filename 
 * @param i 
 * @return string 
 */
string filename(string filename, int i)
{
	return string(FILES_DIR) + "/" + filename + (i > -1 ? ("-" + to_string(i)) : "") + ".bin";
}


/**
 * @brief Compute average for measurements.
 * 
 * @param getter 
 * @return pair<number, number> 
 */
pair<number, number> avg(function<number(const measurement&)> getter){
	auto values = transform<measurement, number>(*QUERY_MEASUREMENTS, getter);
	auto sum	= accumulate(values.begin(), values.end(), 0LL);
	return {sum, values.size() > 0 ? sum / values.size() : 0};
};

/**
 * @brief Determine if string is a float.
 * 
 * @param s : string to be tested
 * @return true 
 * @return false 
 */
bool regexmatch_float(string s){

    regex e (R"([-+]?([0-9]+\.[0-9]+|[0-9]+))");

    if (regex_match (s,e))
        return true;

    return false;
}

/**
 * @brief Determine if string is an int.
 * 
 * @param s : string to be tested.
 * @return true 
 * @return false 
 */
bool regexmatch_int(string s){

    regex e (R"([-+]?([0-9]*))");

    if (regex_match (s,e))
        return true;

    return false;
}



/**
 * @brief Determines is a string only contains the allowed chars.
 * This function is somewhat sidechannel free (expect compiler messes with it).
 * 
 * @param val_cut 
 * @param len 
 * @param allowed 
 * @return true 
 * @return false 
 */
bool containsOnly(string val_cut, int len, vector<char> allowed){
	vector<bool> iIsValid (len);
	for (int i=0;i<len;i++){
		vector<bool> isThisChar (allowed.size());
		for(int j=0; i< (int)allowed.size();j++){
			int diff=(int)allowed[j]-(int)val_cut[i];
			isThisChar.push_back(!(bool)diff);
		}
    	bool charValid = std::accumulate(isThisChar.begin(), isThisChar.end(), false);
		iIsValid.push_back(charValid);
	}
    int numValid = std::accumulate(iIsValid.begin(), iIsValid.end(), 0);
	int diffChars =len - numValid;
	return (bool) diffChars;

}

/**
 * @brief Writes output file as json, containing all relevant parameters and the measurements for insertion time, deletion time and querying.
 * Logging is stopped afterwards.
 * 
 * 
 */
void writeOutput(){

	pair<number, number> timePair = avg([](measurement v) { return get<0>(v); }); //auto [timeTotal, timePerQuery]
	pair<number, number> realPair = avg([](measurement v) { return get<1>(v); }); //auto [realTotal, realPerQuery]
	auto paddingPerQuery		   = avg([](measurement v) { return get<2>(v); }).second;
	auto noisePerQuery		       = avg([](measurement v) { return get<3>(v); }).second;
	auto totalPerQuery			   = avg([](measurement v) { return get<4>(v); }).second;

	LOG(INFO, boost::wformat(L"For %1% QUERIES: total: %2%, average: %3% / query,  query, %4%" \
							"/ fetched item; (%5%+%6%+%7%=%8%) records / query") 
	% NUM_QUERIES
	% timeToString(timePair.first)
	% timeToString(timePair.second)
	% timeToString(realPair.first > 0 ? timePair.first / realPair.first : 0) % realPair.second
	% paddingPerQuery
	% noisePerQuery 
	% totalPerQuery);


	wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	pt::ptree root;
	pt::ptree overheadsNode;

	for (auto measurement : *QUERY_MEASUREMENTS)
	{
		pt::ptree overhead;
		overhead.put("overhead", get<0>(measurement));
		overhead.put("orams", get<1>(measurement));
		overhead.put("beforeOrams", get<2>(measurement));
		overhead.put("afterOrams", get<3>(measurement));
		overhead.put("real", get<4>(measurement));
		overhead.put("padding", get<5>(measurement));
		overhead.put("noise", get<6>(measurement));
		overhead.put("total", get<7>(measurement));
		overheadsNode.push_back({"", overhead});
	}

	root.put("DATASOURCE", toInt(DATASOURCE));
	root.put("FILES_DIR",(FILES_DIR));
	PUT_PARAMETER(NUM_ATTRIBUTES);


	string columns="";
	for(size_t i=0;i<NUM_ATTRIBUTES;i++){
		columns+=DBT::aTypeToString(COLUMN_FORMAT[i]);
		if(i!=NUM_ATTRIBUTES-1){
			columns+=",";
		}
	}
	root.put("COLUMN_FORMAT",columns);

	string resolutions="";
	for(size_t i=0;i<NUM_ATTRIBUTES;i++){
		resolutions+=DBT::toString(DATA_RESOLUTION[i]);
		if(i!=NUM_ATTRIBUTES-1){
			resolutions+=",";
		}
	}	
	root.put("DATA_RESOLUTION",resolutions);

	string minValues="";
	for(size_t i=0;i<NUM_ATTRIBUTES;i++){
		minValues+=DBT::toString(MIN_VALUE[i]);
		if(i!=NUM_ATTRIBUTES-1){
			minValues+=",";
		}
	}	
	root.put("MIN_VALUES",minValues);

	string maxValues="";
	for(size_t i=0;i<NUM_ATTRIBUTES;i++){
		maxValues+=DBT::toString(MAX_VALUE[i]);
		if(i!=NUM_ATTRIBUTES-1){
			maxValues+=",";
		}
	}	
	root.put("MAX_VALUE",maxValues);	

	ostringstream domainSizes;
	for(size_t i=0;i<NUM_ATTRIBUTES;i++){
		domainSizes<<INTERFACE->volumeSanitizers[i].dp_domain;
		if(i!=NUM_ATTRIBUTES-1){
			domainSizes<<",";
		}
	}	

	PUT_PARAMETER(USE_GAMMA);
	PUT_PARAMETER(RANGEQUERY_RANGE);
	PUT_PARAMETER(VALUE_SIZE);
	PUT_PARAMETER(NUM_DATAPOINTS);
	PUT_PARAMETER(DELETION)
	PUT_PARAMETER(SEED);

	ostringstream retrieveExactlyoss;
	for(size_t i=0;i<RETRIEVE_EXACTLY.size();i++){
		retrieveExactlyoss<<RETRIEVE_EXACTLY[i];
		if(i!=RETRIEVE_EXACTLY.size()-1){
			retrieveExactlyoss<<",";
		}
	}		
	root.put("RETRIEVE_EXACTLY",retrieveExactlyoss.str());	

	PUT_PARAMETER(QUERY_RESPONSE_EPSILON);
	PUT_PARAMETER(QUERY_INDEX);
	PUT_PARAMETER(WHERE_INDEX);
	root.put("QUERY_FUNCTION", toInt(QUERY_FUNCTION));
	PUT_PARAMETER(POINT_QUERIES);
	PUT_PARAMETER(NUM_QUERIES);
	PUT_PARAMETER(MAX_SENSITIVITY);
	PUT_PARAMETER(DATASET);
	root.put("BLOCK_SIZE",getNumBytesWhenSerialized(COLUMN_FORMAT,VALUE_SIZE));

	root.put("NUM_OSMs",INTERFACE->numOSMs);
	PUT_PARAMETER(ORAM_LOG_CAPACITY);
	PUT_PARAMETER(ORAM_Z);
	PUT_PARAMETER(STASH_FACTOR);
	PUT_PARAMETER(USE_ORAM);
	PUT_PARAMETER(BATCH_SIZE);
	PUT_PARAMETER(DP_BUCKETS);
	PUT_PARAMETER(DP_K);
	PUT_PARAMETER(DP_BETA);
	PUT_PARAMETER(DP_EPSILON);

	PUT_PARAMETER(FILE_LOGGING);
	PUT_PARAMETER(OUT_DIR);

	root.put("LOG_FILENAME", LOG_NAME);

	ostringstream oss;
	for(number i: *INSERTION_MEASUREMENTS){
		oss<<i<<",";
	}
	string s= oss.str();
	string insertionTimeString = s.substr(0, s.size()-1);
	root.put("InsetionTimes",insertionTimeString);
	
	ostringstream oss2;
	for(number i: *DELETION_MEASUREMENTS){
		oss2<<i<<",";
	}
	string sd= oss2.str();
	string deletionTimeString = sd.substr(0, sd.size()-1);
	root.put("DeletionTimes",deletionTimeString);



	pt::ptree aggregates;
	aggregates.put("insertionTotal", INSERTION_TOTAL);
	aggregates.put("insertionMean", INSERTION_TOTAL/NUM_DATAPOINTS);
	aggregates.put("timeTotal", timePair.first);
	aggregates.put("timePerQuery", timePair.second);
	aggregates.put("realTotal", realPair.first);
	aggregates.put("realPerQuery", realPair.second);
	aggregates.put("paddingPerQuery", paddingPerQuery);
	aggregates.put("noisePerQuery", noisePerQuery);
	aggregates.put("totalPerQuery", totalPerQuery);
	root.add_child("aggregates", aggregates);



	root.add_child("QUERIES", overheadsNode);

	auto filename = boost::str(boost::format("%1%/%2%.json")%OUT_DIR % LOG_NAME);

	pt::write_json(filename, root);

	LOG(INFO, boost::wformat(L"Log written to %1%") % converter.from_bytes(filename));

	if (FILE_LOGGING)
	{
		LOG_FILE.close();
	}

}

/**
 * @brief Writes queries and data to file. One file for the database format, one file for the input data and one file for the query data. 
 * Files are stored in FILES_DIR.
 * 
 * @param queries : queries i.e. newly generated ones
 * @param data : input data stored in the database (without value)
 */
void storeInputs(vector<Query>& queries, vector<vector<db_t>> &data){


		ofstream schemaFile(filename(SCHEMA_INPUT_FILE, -1));

		LOG_PARAMETER(NUM_ATTRIBUTES);
		schemaFile << NUM_ATTRIBUTES << endl;
		for (int i=0; i< (int)NUM_ATTRIBUTES;i++){
			string s="";
			if(COLUMN_FORMAT[i]==AType::INT) s="i";
			if(COLUMN_FORMAT[i]==AType::FLOAT) s="f";
			schemaFile << s;
			if(i<(int) NUM_ATTRIBUTES-1) schemaFile<<",";

		}
		schemaFile<<endl;

		for (int i=0; i< (int)NUM_ATTRIBUTES;i++){
			schemaFile << DBT::toString(MAX_VALUE[i])<<","<<DBT::toString(MIN_VALUE[i])<<endl;
		}

		schemaFile.close();


		ofstream queryFile(filename(QUERY_INPUT_FILE, -1));

		for (auto&& query : queries)
		{
			queryFile << queryToCSVString(query)<< endl;
		}

		queryFile.close();

		ofstream dataFile(filename(DATA_INPUT_FILE, -1));

		for (auto&& datapoint : data)
		{
			for (size_t i = 0; i < NUM_ATTRIBUTES; i++)
			{
				dataFile << DBT::toString(datapoint[i]);
				if(i<NUM_ATTRIBUTES-1){
					dataFile<<",";
				}else{
					dataFile<<endl;
				}
			}
			
		}
		dataFile.close();

}

/**
 * @brief Read database schema, input data and queries from files stored in FILES_DIR.
 * 
 */
void loadInputs(){

		ifstream schemaFile(filename(SCHEMA_INPUT_FILE, -1));

		string line = "";
		getline(schemaFile,line);
		uint num_attributes_file=stoi(line);
		if(num_attributes_file<NUM_ATTRIBUTES){
			LOG(ERROR, L"The loaded file does not provide as many columns as required by the COLUM_FORMAT.");
			exit(1);
		}

		for(int i=0; i<(int)NUM_ATTRIBUTES+1;i++){
			getline(schemaFile,line);
			LOG(DEBUG,toWString(line));
			vector<string> vals;
			boost::algorithm::split(vals, line, boost::is_any_of(","));
			if(i==0){
				vector<AType> vec;
				for(auto&& c : vals){
					if(c=="i") vec.push_back(AType::INT);
					if(c=="f") vec.push_back(AType::FLOAT);
				}
				COLUMN_FORMAT = {vec.begin(), vec.begin()+NUM_ATTRIBUTES}; 
			}else{
				AType type=COLUMN_FORMAT[i-1];
				MAX_VALUE.push_back(fromString(vals[0],type));
				MIN_VALUE.push_back(fromString(vals[1],type));

			}
		}
		schemaFile.close();


		ifstream queryFile(filename(QUERY_INPUT_FILE, -1));
		line = "";
		int i=0;
		while (getline(queryFile, line) and i<(int)NUM_QUERIES)
		{
			Query query=queryFromCSVString(line);
			QUERIES.push_back(query);
			i++;
		}
		queryFile.close();
		NUM_QUERIES=QUERIES.size();

		line = "";
		double number_of_lines=0;
		ifstream dataFile_count(filename(DATA_INPUT_FILE, -1));
		while (std::getline(dataFile_count, line)){
        	++number_of_lines;
		}
		dataFile_count.close();
		int samplingRate=floor(number_of_lines/(double) NUM_DATAPOINTS);
		LOG(INFO, boost::wformat(L"Sampling Rate for Data File is %d") %samplingRate);

		ifstream dataFile(filename(DATA_INPUT_FILE, -1));
		line = "";
		i=0;
		while (getline(dataFile, line) and INPUT_DATA.size()<(int)NUM_DATAPOINTS)
		{
			//skipping lines to subsample large datasets and get and somewhat even data distribution
			if(i%samplingRate !=0){
				i++;
				continue;
			}
			vector<db_t> dataPoint;
			vector<string> sData;
			boost::algorithm::split(sData, line, boost::is_any_of(","));
			
			for(size_t i=0; i<NUM_ATTRIBUTES;i++){
				auto k= fromString(sData[i], COLUMN_FORMAT[i]);
				dataPoint.push_back(k);
			}
			INPUT_DATA.push_back(dataPoint);
			i++;
		}
		dataFile.close();
		NUM_DATAPOINTS=INPUT_DATA.size();
}

}