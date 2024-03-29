#include "server_utility.hpp"
#include "utility.hpp"
#include "globals.hpp"
#include "state_table.hpp"
#include "output_utility.hpp"
#include "querying.hpp"
#include "struct_querying.hpp"

#include <rpc/server.h>

/**
 * @brief This file provides functions for setting up a an rpc server expose certain database functionalities via HTTP.
 * The state of the server is represented via a state machine. Querying and data insertion is not possible at the same time as this would cause issues with the privacy budget.
 * TODO: Password string should be replaced with a proper password mechanism.
 * 
 */


using namespace std;
using namespace STATE_MACHINE;

namespace MENHIR{
	

	/**
	 * @brief Stops crowdsourced data collection if state machine allows it. 
	 * 
	 * @param pw :  password string
	 * @return true 
	 * @return false 
	 */
	bool stopCollectionPhase(string pw){
		if(pw==PASSWORD){
			transitionServerState(301);
			return true;
		}
		return false;
	}

	/**
	 * @brief Stops interactiv querying if state machine allows it.
	 * 
	 * @param pw :  password string
	 */
	void stopQueryPhase(string pw){
		if(pw==PASSWORD){
			STATE new_state=transitionServerState(501);
			if(new_state==CLEANUP){
				LOG(INFO, L"Storing relevant meansuremnts.");
				writeOutput();
				exit(0);
			}
		}
	}

	/**
	 * @brief Receives string of data to be inserted into the database. Data is only inserted if it follows the column format.
	 * 
	 * @param datastring 
	 * @return size_t 
	 */
	size_t receiveDataFromCrowd(string datastring){
		vector<string> vals;
		boost::algorithm::split(vals, datastring, boost::is_any_of(","));
		
		vector<db_t> data(NUM_ATTRIBUTES);

		bool validFlag= !(bool) (NUM_ATTRIBUTES-vals.size()); //set to invalid if to many attributes
		for(int i=0;i<(int)NUM_ATTRIBUTES;i++){
			string vi=vals[i];

			if(COLUMN_FORMAT[i]==AType::INT){
				if(regexmatch_int(vi)){ 

					try{
						db_t d=fromString(vi, AType::INT);
						data.push_back(d);
					}catch(...){
						validFlag=false;
					}
				}else{
					validFlag=false;
				}
		
			}else{
				if(regexmatch_float(vi)){ 
					try{
						db_t d=fromString(vi, AType::FLOAT);
						data.push_back(d);
					}catch(...){
						validFlag=false;
					}
				}else{
					validFlag=false;
				}
			}

		}
		//The attacker learns that a submitted data point was valid
		if(validFlag){
			
			if(USE_ORAM){
				size_t hash=INTERFACE->insert(data);
				return hash;
			}else{
				size_t hash=INTERFACE->insert(data);
				return hash;
			}
		}
		return (size_t)0;
		

	}


	/**
	 * @brief 
	 * TODO: Clean data before allowing analysis and Extensivly verify query
	 * 
	 * @param queryString 
	 * @param pw 
	 * @return pair<string, string> 
	 */
	pair<string, string> queryServer(string queryString, string pw){
		if(pw!=PASSWORD){

			return {"", "Access denied."};
		}
		Query query=queryFromCSVString(queryString);
		if (query.attributeIndex >= COLUMN_FORMAT.size()){
			throw std::invalid_argument( "attributeIndex larger than number of attributs." );
		}else if (query.whereIndex >= COLUMN_FORMAT.size()){
			throw std::invalid_argument( "whereIndex larger than number of attributs." );
		}
		if(query.agg==AggregateFunc::VARIANCE){
			if(query.extra<=0){
				query.extra=1;
			}
		}else if(query.agg==AggregateFunc::MAX_COUNT or query.agg==AggregateFunc::MIN_COUNT){
			if(query.extra<=0){
				query.extra=1;
			}
		}

		LOG(INFO, L"run Query on Server: "+toWString(queryToString(query)));
		auto[result_dbt,result_d,err]=runQuery(query);
		if(query.agg==AggregateFunc::MAX_COUNT or query.agg==AggregateFunc::MIN_COUNT){
			return {DBT::toString(result_dbt), errToString(err)};
		}else{
			std::ostringstream oss;
			oss << result_d;
			return {oss.str(), errToString(err)};
		}
	}	
}