#include "querying.hpp"
#include "utility.hpp"
#include "globals.hpp"
#include "output_utility.hpp"
#include "volume_sanitizer_utility.hpp"
#include "struct_volume_sanitizer.hpp"
#include "dp_query_functions.hpp"
#include "utility.hpp"

#include <chrono>
#include <ctime>
#include <numeric>
#include <signal.h> //for chatching SIGINT
#include <string>
#include <valgrind/callgrind.h>

/**
 * @brief This file contains functions for querying the database and determining the amount of volume sanitation required.
 * 
 */

using namespace std;
using namespace DBT;
namespace MENHIR{

	/**
	 * @brief This function processes a query. 
	 * It first determins the noise required for volume padding for the query to ge executed.
	 * Then it executes a query by calling the corresponding function of the OSM Interface.
	 * With the results, the corrsponding query function is called to compute a differentially private aggregate.
	 * 
	 * @param query 
	 * @return tuple<db_t, double,Error> : Tuple containing the differentially private aggregate as db_t or double (depending on the function) or, if an Error occurred, the corrsponding information on the error. 
	 */
	tuple<db_t, double,Error> runQuery(Query query){
		
		auto[estimate,noiseToAdd, err]=getTotalNoise(query);
		if(err.code!=0){
			//This clauses catches cases where the DP tree is not deep enough
			return make_tuple(db_t(0),0.0,err);
	
		}

		if(estimate==0){
	        Error err=Error(400,L"The query could not be answered as it was estimated that no elements with in this range apply. This could be because to little data has been collected.");
			return make_tuple(db_t(0),0.0,err);
		
		}
		
		if(CURRENT_LEVEL==DEBUG) LOG(DEBUG,boost::wformat(L"Query : %s - estimate: %d") 
			% toWString(queryToString(query))
			% estimate);


		if(RETRIEVE_EXACTLY.size()!=0){
			//this is so overhead from the data processing does not interfere with the retrieval measurements
			estimate=RETRIEVE_EXACTLY_NOW;
		}

		auto allRecords=INTERFACE->findInterval(query.whereFrom, query.whereTo, query.whereIndex, noiseToAdd);//
		

				
		vector<db_t> values=vector<db_t>();
		vector<bool> ignoring=vector<bool>();
		for(size_t i=0;i<allRecords.size();i++){
			vector<db_t> record;
			bool dummy=false;
			tie(record,dummy)=allRecords[i];
			values.push_back(record[query.attributeIndex]);
			bool padding =(record[query.whereIndex]>query.whereTo or record[query.whereIndex]<query.whereFrom);
			dummy=padding*true+ (not padding)* dummy;
			ignoring.push_back(dummy);
		}

		auto ret=computeQueryFunctionPrivate(query,values,ignoring);
		return ret;
	}

	/**
	 * @brief This function processes a query in the setting of automated evaluation.
	 * It first determins the noise required for volume padding for the query to ge executed.
	 * Then it executes a query by calling the corresponding function of the OSM Interface.
	 * With the results, the corrsponding query function is called to compute a differentially private aggregate.
	 * 
	 * @param query 
	 * @return tuple<number,dbResponse,Error> : Tuple containing the differentially private aggregate as db_t or double (depending on the function) or, if an Error occurred, the corrsponding information on the error. 
	 */
	tuple<number,dbResponse,Error> runQueryEval(Query query){

		dbResponse allRecords;

		auto[estimate,noiseToAdd, err]=getTotalNoise(query);
		if(err.code!=0){
			//This clauses catches cases where the DP tree is not deep enough
			return make_tuple((number)0, allRecords,err);

		}

		if(CURRENT_LEVEL==DEBUG) LOG(DEBUG,boost::wformat(L"Retrieval: number of additional datapoints %d ") % estimate);
		
		if(estimate==0){
			wstring errString=L"The query could not be answered as it was estimated that no elements with in this range apply. This could be because to little data has been collected.";
	        Error err=Error(errString);
			LOG(ERROR, err.err_string);
			return make_tuple(estimate,allRecords,err);
		}

		if(RETRIEVE_EXACTLY.size()!=0){
			estimate=RETRIEVE_EXACTLY_NOW;
            noiseToAdd.clear();
            for(size_t i=0;i<INTERFACE->numOSMs;i++){
                noiseToAdd.push_back(RETRIEVE_EXACTLY_NOW);
            }
		}

		allRecords=INTERFACE->findInterval(query.whereFrom, query.whereTo, query.whereIndex, noiseToAdd);

		vector<db_t> values=vector<db_t>();
		vector<bool> ignoring=vector<bool>();
		
		ostringstream oss;
		for(size_t i=0;i<allRecords.size();i++){
			bool dummy=false;
			vector<db_t> record;
			tie(record,dummy)=allRecords[i];
			values.push_back(record[query.attributeIndex]);
			bool padding =(record[query.whereIndex]>query.whereTo or record[query.whereIndex]<query.whereFrom);
			dummy=padding*true+ (not padding)* dummy;
			ignoring.push_back(dummy);
		}

		computeQueryFunctionPrivate(query,values,ignoring);
		Error noErr=Error();
		return make_tuple(estimate, allRecords, noErr);
	}

	/**
	 * @brief Depending on the aggregate type set in the query, this function calls the corresponding differentially private aggregate function for the passed data.
	 * 
	 * @param query 
	 * @param values : Vector of data returned by the database for the requested interval
	 * @param ignoring : Vector of bool containing information if the corresponding value is to be ignored.
	 * @return tuple<db_t,double, Error> : Tuple containing the differentially private aggregate as db_t or double (depending on the function) or, if an Error occurred, the corrsponding information on the error. 
	 */
	tuple<db_t,double, Error> computeQueryFunctionPrivate(Query query, vector<db_t> values,  vector<bool> ignoring){
		db_t result_dbt=db_t(0);
		double result_d=0;
		Error err;

		if(query.agg == AggregateFunc::SUM){
			db_t lower= MIN_VALUE[query.attributeIndex];
			db_t upper= MAX_VALUE[query.attributeIndex];
			tie(result_d, err)=dp_sum(values,ignoring, query.epsilon, lower, upper);

		} else if(query.agg== AggregateFunc::MEAN){
			db_t lower= MIN_VALUE[query.attributeIndex];
			db_t upper= MAX_VALUE[query.attributeIndex];
			tie(result_d, err)=dp_mean(values,ignoring,query.epsilon, lower, upper);

		}else if(query.agg == AggregateFunc::VARIANCE){
			int ddof=1;
			if(query.extra!=0) ddof=query.extra;
			db_t lower= MIN_VALUE[query.attributeIndex];
			db_t upper= MAX_VALUE[query.attributeIndex];			
			tie(result_d, err)=dp_var(values,ignoring, query.epsilon,ddof,lower, upper);

		}else if(query.agg == AggregateFunc::COUNT){
			//Clipping is not always necessary here			
			tie(result_d, err)=dp_count(values,ignoring, query.epsilon,db_t(0),db_t(0),false);

		}else if(query.agg == AggregateFunc:: MAX_COUNT){
			//returns an element!
			if(COLUMN_FORMAT[query.attributeIndex]!=AType::INT){
				wstring err_MAX_COUNT= L"The differentially private algorithm MAX_COUNT can only be applied to data of type INT. No privacy budget was consumed.";
				err=Error(err_MAX_COUNT);
				print_err(err);
				return make_tuple(result_dbt, result_d,err);
			}

			int lower, upper;
			if(query.pointQuery){
				// take whole interval
				lower=MIN_VALUE[query.attributeIndex].val.i; //lower bound
				upper=MAX_VALUE[query.attributeIndex].val.i; //lower bound
			}else{
				lower=query.from.val.i;
				upper=query.to.val.i;
			}
			int width = DATA_RESOLUTION[query.attributeIndex].val.i;
			tie(result_dbt,err)= report_noisy_max_finite_int( values, ignoring,upper, lower, query.epsilon, width,query.extra);

		}else if(query.agg == AggregateFunc:: MIN_COUNT){
			//returns an element!
			if(COLUMN_FORMAT[query.attributeIndex]!=AType::INT){
				wstring err_MIN_COUNT= L"The differentially private algorithm MIN_COUNT can only be applied to data of type INT. No privacy budget was consumed.";
				err=Error(err_MIN_COUNT);
				print_err(err);
				return make_tuple(result_dbt, result_d,err);
			}

			int lower, upper;
			if(query.pointQuery){
				// take whole interval
				lower=MIN_VALUE[query.attributeIndex].val.i; //lower bound
				upper=MAX_VALUE[query.attributeIndex].val.i; //lower bound
			}else{
				lower=query.from.val.i;
				upper=query.to.val.i;
			}
			int width = DATA_RESOLUTION[query.attributeIndex].val.i;
			tie(result_dbt,err)= report_noisy_min_finite_int( values, ignoring,upper, lower, query.epsilon, width, query.extra);

		}else{
			err=Error(boost::wformat(L"No valid aggregation function set for Query "\
				"%s. Setting results to zero.") 
				%toWString(queryToString(query)));
			print_err(err);
		}
		return make_tuple(result_dbt, result_d,err);
	}

	/**
	 * @brief Runs a set of QUERIES which are either read from file or generated synthetically. 
	*  Conducts extensive measurements to assess runtime overhead.
	 * 
	 */
	void automated_querying(){

		LOG(INFO, boost::wformat(L"Running %1% QUERIES...") % QUERIES.size());

		// setup Ctrl+C (SIGINT) handler
		/*struct sigaction sigIntHandler;
		sigIntHandler.sa_handler = [](int s) {
			if (SIGINT_RECEIVED)
			{
				LOG(WARNING, L"Second SIGINT caught. Terminating.");
				exit(1);
			}
			SIGINT_RECEIVED = true;
			LOG(WARNING, L"SIGINT caught. Will stop processing QUERIES. Ctrl+C again to force-terminate.");
		};
		sigemptyset(&sigIntHandler.sa_mask);
		sigIntHandler.sa_flags = 0;
		sigaction(SIGINT, &sigIntHandler, NULL);
		*/
		int q=0;


		for (auto query : QUERIES){
			
			if(RETRIEVE_EXACTLY.size()>0){
				RETRIEVE_EXACTLY_NOW=RETRIEVE_EXACTLY[q%RETRIEVE_EXACTLY.size()];
			}
			q++;
			

			//uses the values query.whereIndex, query.whereFrom and query.whereTo to find padding and noise

			LOG(DEBUG, L"Query: "+toWString(queryToString(query)));

			int maxTries=5;

			for(int j=0;j<maxTries;j++){
				try{
					startTimeQuery = chrono::steady_clock::now();
					auto[estimate,allRecords,err]=runQueryEval(query);
					endTimeQuery = chrono::steady_clock::now();

					if(err.code!=0){
						LOG(ERROR, err.err_string);
						LOG(ERROR, L"Caught error for when running query. This query will be skipped. No data is added to the output.");
					}else{
						processMeasurement(query,q, estimate,allRecords);
						break;
					}
				}catch (const std::exception &exc) { 
					//catch all
					boost::wformat errString= boost::wformat(L" Caught error from Query %3i / %3i. Try %d/%d. No data is added to the output.Error:%s") 
								% q % QUERIES.size()
								% j % maxTries
								%toWString(exc.what());
					LOG(ERROR,errString);
				}
			}
			
		}
	}

	/**
	 * @brief Subfunction of automated_querying() for processing measurements. Measurements are stored in global variable QUERY_MEASUREMENTS.
	 * 
	 * @param query 
	 * @param q : index of query
	 * @param estimate : number of dummy data points
	 * @param allRecords : vector of data as returned by the OSMInterface 
	 */
	void processMeasurement(Query query, int q, number estimate,  dbResponse allRecords){
		auto queryOverheadBefore = chrono::duration_cast<chrono::nanoseconds>(timestampBeforeORAMs - startTimeQuery).count();
		auto queryOverheadORAMs	 = chrono::duration_cast<chrono::nanoseconds>(timestampAfterORAMs - timestampBeforeORAMs).count();
		auto queryOverheadAfter	 = chrono::duration_cast<chrono::nanoseconds>(endTimeQuery- timestampAfterORAMs).count();

				
				
		//LOG(INFO,boost::wformat(L"Query: %s") % toWString(queryToString(query)));
		LOG(INFO, L"Query: "+toWString(queryToString(query)));

		LOG(INFO,boost::wformat(L"Retrieval: number of datapoints %6i noise %d , interval [%s,%s]") 
					% allRecords.size()
					% estimate
					% DBT::toWString(query.whereFrom)
					% DBT::toWString(query.whereTo));



		int noise=0;
		int paddingRecordsNumber=0;
		ostringstream oss;
		for(size_t i=0;i<allRecords.size();i++){
			vector<db_t> record;
			bool dummy;
			tie(record,dummy)=allRecords[i];
		
			LOG(DEBUG, boost::wformat(L"Datapoint %d/%6i - val:%s dummy:%d") 
						% (i+1) 
						% allRecords.size()
						% DBT::toWString(record[query.whereIndex]) 
						% dummy);
			if(dummy){
				noise++;
			}else if(record[query.attributeIndex]>query.to or record[query.attributeIndex]<query.from){
				paddingRecordsNumber++;
			}
		}	
				
		auto realRecordsNumber = allRecords.size()-noise-paddingRecordsNumber;
		auto elapsed = chrono::duration_cast<chrono::nanoseconds>(endTimeQuery - startTimeQuery).count();
				
		if(estimate>0){
			QUERY_MEASUREMENTS->push_back({elapsed, queryOverheadORAMs,queryOverheadBefore,queryOverheadAfter, realRecordsNumber, paddingRecordsNumber, noise,  allRecords.size()});
		}


		LOG(INFO, boost::wformat(L"Query %3i / %3i : real records %6i ( +%6i padding +%6i noise, %6i total) (%7s, or %7s / record)") 
				% q 
				% QUERIES.size() 
				% realRecordsNumber
				% paddingRecordsNumber
				% noise 
				% allRecords.size()
				% timeToString(elapsed) 
				% (realRecordsNumber > 0 ? timeToString(elapsed / realRecordsNumber) : L"0 ns"));

		LOG(INFO, boost::wformat(L"Runtime Measurment: {before: %7s, ORAMs: %7s, after: %7s}") 
				% timeToString(queryOverheadBefore) % timeToString(queryOverheadORAMs) % timeToString(queryOverheadAfter));

		QUERY_INDEX++;
		usleep(WAIT_BETWEEN_QUERIES * 1000);
	}

	/**
	 * @brief Get the amount of volume padding ("noise") for point queries.
	 * 
	 * @param query 
	 * @return tuple<number, vector<number>,Error> : Tuple containing the differentially private aggregate as db_t or double (depending on the function) or, if an Error occurred, the corrsponding information on the error. 
	 */
	tuple<number, vector<number>,Error> getNoisePointQuery(Query query){
		uint index=query.whereIndex; //index
		VolumeSanitizer np=INTERFACE->volumeSanitizers[index];

		vector<number> noiseToAdd;
		number totalNoise=0;
		for(size_t i=0;i<INTERFACE->numOSMs;i++){
			double N= DBT::toDouble(MAX_VALUE[index]-MIN_VALUE[index]); // number of bins.
			double beta=1.0 / (1 << DP_BETA);
			long double root=pow(1.0-beta,1.0/N);
			long double top=-log(2.0-2.0*root);
			long double ap=ceil(top/DP_EPSILON);
				
			int value = (int) sampleLaplace(np.dp_alpha,1.0/DP_EPSILON);			
			noiseToAdd.push_back(value);
			totalNoise+=value;
			LOG(DEBUG, boost::wformat(L"N %9.2f, beta %9.2f, root %9.2f, top %9.2f, ap %9.2f, value %9.2f" ) % N % beta %root %top %ap %value);
		}
		return make_tuple(totalNoise,noiseToAdd, Error());
	}


	/**
	 * @brief Subroutine of getNoiseRangeQuery(). 
	 * Gets the number of noise nodes for the range query. 
	 * This function is adapted from Epsolute (https://github.com/epsolute/Epsolute).
	 * Sets Error object in returned tuple if error occurs.
	 * 
	 * @param query 
	 * @return tuple<vector<pair<number,number>>,Error> 
	 */
	tuple<vector<pair<number,number>>,Error> getNoiseNodesForRangeQuery(Query query){
		uint i=query.whereIndex; //index
		VolumeSanitizer np=INTERFACE->volumeSanitizers[i];
		double n_min= toDouble(MIN_VALUE[i]);
		double n_max=toDouble(MAX_VALUE[i]);
		double resolution=toDouble(DATA_RESOLUTION[i]);

		double q_from= toDouble(query.whereFrom );
		double q_to= toDouble(query.whereTo);
		if(CURRENT_LEVEL==DEBUG){
			LOG(DEBUG, boost::wformat(L"Get Total Noise: min %9.2f max %9.2f q_from %9.2f q_to %9.2f" ) % n_min % n_max % q_from % q_to);
		}
		if ( q_from < n_min||  q_to > n_max){
			LOG(WARNING, L"Query endpoints are out of bounds. Clipping");
			q_from=n_min;
			q_to=n_max;
		}

		auto[fromBucket, toBucket, n_from, n_to] = getBuckets(q_from,q_to, resolution, n_min, n_max, np.dp_buckets);

		LOG(DEBUG, boost::wformat(L"getBuckets result: fromBucket %d toBucket %d n_from %d n_to %d" )
			% fromBucket % toBucket % n_from % n_to);

		if ( n_from < n_min){
			LOG(ERROR, L"Query endpoints are out of bounds. Clipping.");
			LOG(ERROR, boost::wformat(L" Buckets [%d, %d], endpoints (%d, %d). Bounds [%9.2f,%9.2f]")		 
				% fromBucket % toBucket 
				% n_from % n_to
				%n_min %n_to);
				n_from=n_min;
		}

		if (  n_to > n_max){
			LOG(ERROR, L"Query endpoints are out of bounds. Clipping.");
			LOG(ERROR, boost::wformat(L" Buckets [%d, %d], endpoints (%d, %d). Bounds [%9.2f,%9.2f]")		 
				% fromBucket % toBucket 
				% n_from % n_to
				%n_from %n_max);
				n_to=n_max;
		}

		if(CURRENT_LEVEL==DEBUG){
			LOG(DEBUG, boost::wformat(L" Buckets [%d, %d], endpoints (%d, %d).")% fromBucket % toBucket % n_from % n_to);
		}
		

		vector<pair<number,number>> noiseNodes = BRC(DP_K, fromBucket, toBucket, np.dp_levels-1);
		for (auto node : noiseNodes){
			if (node.first >= np.dp_levels){
				boost::wformat errString= boost::wformat(L"DP tree is not high enough. Level %1% is not generated.\n"\
								"Buckets [%2%, %3%], endpoints (%4%, %5%).\nnode.first= %6%, np.dp_levels=%7%" ) 
								% node.first % fromBucket % toBucket  		 
								% n_from % n_to
								%node.first %np.dp_levels;
				LOG(ERROR, errString);
				Error err=Error(errString);
				return make_tuple(noiseNodes, err); 
			}
		}
		return make_tuple(noiseNodes,Error());
	}

	/**
	 * @brief Gets the amount of volume padding ("noise") for range queries.
	 * This function is adapted from Epsolute (https://github.com/epsolute/Epsolute).
	 * Sets Error object in returned tuple if error occurs.
	 * 
	 * @param query 
	 * @return tuple<number, vector<number>,Error> 
	 */
	tuple<number, vector<number>,Error> getNoiseRangeQuery(Query query){
		Error err;
		vector<pair<number,number>> noiseNodes;
		tie(noiseNodes,err)=getNoiseNodesForRangeQuery(query);
		LOG(INFO, boost::wformat(L"Noise nodes %d") %noiseNodes.size());
		if(err.code!=0){
			return make_tuple((number)0, vector<number>(),err);
		}

		vector<number> noiseToAdd;
		number totalNoise=0ull;

		if (USE_GAMMA){
			VolumeSanitizer np=INTERFACE->volumeSanitizers[query.whereIndex]; //for each column and each osm there is one sanitizer
			number kZeroTilda;
			vector<number> totalPerOSM;
			tie(kZeroTilda,totalPerOSM) = INTERFACE->getTotalFromHistograms(query.whereFrom ,query.whereTo, query.whereIndex);
			if(CURRENT_LEVEL==INFO){
				for(size_t i=0;i<INTERFACE->numOSMs;i++){
					LOG(INFO, boost::wformat(L"num in osm %d: %d") %i %totalPerOSM[i]);
				}
			}
			for (auto node : noiseNodes){
				kZeroTilda += np.noises[node];
			}
			if (kZeroTilda == 0){
				LOG(CRITICAL, L"Something is wrong, kZeroTilda cannot be 0.");
			}

			auto maxRecords = gammaNodes(INTERFACE->numOSMs, 1.0 / (1 << DP_BETA), kZeroTilda);

			for (auto i = 0uLL; i < INTERFACE->numOSMs; i++){
				auto extra = totalPerOSM[i] < maxRecords ? maxRecords - totalPerOSM[i] : 0;
				noiseToAdd.push_back(extra);
				totalNoise+=extra;
			}
		}else{
			for(size_t osmIndex=0;osmIndex<INTERFACE->numOSMs;osmIndex++){
				VolumeSanitizer np=INTERFACE->volumeSanitizers[query.whereIndex+osmIndex*NUM_ATTRIBUTES]; //for each column and each osm there is one sanitizer
				number extra=0ull;
				for (auto node : noiseNodes){
					extra += np.noises[node];
					totalNoise+=np.noises[node];
				}
				noiseToAdd.push_back(min(NUM_DATAPOINTS,extra));
			}
		}
			
		if(CURRENT_LEVEL==DEBUG) {
			LOG(DEBUG, boost::wformat(L"Query {%9.2f, %9.2f} will have added total of %4i noisy records") 
					% DBT::toWString(query.whereFrom) % DBT::toWString(query.whereTo) % totalNoise);
		}

		return make_tuple(totalNoise,noiseToAdd, err);	
	}


	/**
	 * @brief Wrapper for getting the volume padding for either range or point queries depending on the query type.
	 *  Error object in returned tuple will be set if error occurs, i.e. due to insufficient construction of the volume sanitizer.
	 * 
	 * @param query 
	 * @return tuple<number, vector<number>,Error> 
	 */
	tuple<number, vector<number>,Error> getTotalNoise(Query query){
		if(query.from == query.to and POINT_QUERIES){ 
			//is point Query
			//this is to ensure that PointQuery volume pattern hiding is only used when point queries are expected.
			return getNoisePointQuery(query);

		}else{
			//is range Query
			return getNoiseRangeQuery(query);
		}
	}
}