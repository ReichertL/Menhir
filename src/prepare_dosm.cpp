#include "definitions.h"
#include "utility.hpp"
#include "globals.hpp"
#include "output_utility.hpp"
#include "prepare_dosm.hpp"
#include "osm_interface.hpp"

#include <chrono>
#include <ctime>
#include <numeric>
#include <string>
#include <valgrind/callgrind.h>

using namespace std;
using namespace MENHIR;

/**
 * @brief This function prepares the DOSMs by calling the OSMInterface.
 * If the global variable INSERT_BULK is true, no measurements on insertion time are conducted.
 * If INSERT_BULK is false, the last element to be inserted is added and removed repeatedly until enough measurements are available.
 * Measurements are written to the global variable INSERTION_MEASUREMENTS and DELETION_MEASUREMENTS.
 * 
 */
void prepareDOSM(){
		
		CALLGRIND_START_INSTRUMENTATION;
		CALLGRIND_TOGGLE_COLLECT;


		LOG(INFO, L"Preparing DOSM: init AVL Tree with type db_t in ORAM");


		long long overheadTotal=0;

		if (DATASOURCE!=CROWD){

				//inserting data one by one into Oram and measuring the time			
			if(INSERT_BULK){
				LOG(INFO, L"Insert data in Bulk-> nonObliv Sorting, then constructing tree on sorted list(s).");
				LOG_PARAMETER(INPUT_DATA.size());
				LOG_PARAMETER(INSERT_BULK);

				INTERFACE=new OSMInterface();
				LOG(INFO, boost::wformat(L"BLOCK_SIZE=%d") %getNumBytesWhenSerialized(COLUMN_FORMAT,VALUE_SIZE));


			}else{
				LOG(INFO, boost::wformat(L"Insert data as bulk. Last data point inserted manually (for runtime measurements)."));
				LOG_PARAMETER(INSERT_BULK);

				vector<db_t> record=INPUT_DATA.back();
				INPUT_DATA.pop_back();

				INTERFACE=new OSMInterface();
				LOG(INFO, boost::wformat(L"BLOCK_SIZE=%d") %getNumBytesWhenSerialized(COLUMN_FORMAT,VALUE_SIZE));

				int REPETITIONS=100;
				int column=0;
				db_t key=record[column];
				int i=NUM_DATAPOINTS-1;
				size_t hash=i;

				for(int rep=0;rep<REPETITIONS; rep++){
					LOG(INFO, boost::wformat(L"Inserting %d/%d -repetition %d")%i %NUM_DATAPOINTS %rep);

					chrono::steady_clock::time_point timestampBefore = chrono::steady_clock::now();
					#ifdef NDEBUG 
						hash=INTERFACE->insert(record);
					#else
						INTERFACE->insert(record,hash);
					#endif
					chrono::steady_clock::time_point timestampAfter = chrono::steady_clock::now();
					long long overheadPut = chrono::duration_cast<chrono::microseconds>(timestampAfter - timestampBefore).count();
					LOG(TRACE, boost::wformat(L"Put the %d th  datapoint: { duration %7s}") 
									%(int)i
									% timeToString(overheadPut)); 
					overheadTotal=overheadTotal+overheadPut;
					MENHIR::INSERTION_MEASUREMENTS->push_back(overheadPut);


					chrono::steady_clock::time_point timestampBeforeDel = chrono::steady_clock::now();
					INTERFACE->deleteEntry(key,hash,column);
					chrono::steady_clock::time_point timestampAfterDel = chrono::steady_clock::now();
					long long overheadPutDel = chrono::duration_cast<chrono::microseconds>(timestampAfterDel - timestampBeforeDel).count();
					LOG(TRACE, boost::wformat(L"Removing the %d th  datapoint: { duration %7s}") 
									%(int)i
									% timeToString(overheadPutDel)); 
					MENHIR::DELETION_MEASUREMENTS->push_back(overheadPutDel);
				}

			}

			long long overheadMean=overheadTotal/(int) NUM_DATAPOINTS;
			MENHIR::INSERTION_TOTAL=overheadTotal;
			LOG(TRACE, boost::wformat(L"Put %d datapoints: { duration %7s, mean %7s}") 
							%(int)NUM_DATAPOINTS
							% timeToString(overheadTotal) 
							% timeToString(overheadMean));
			LOG(INFO, boost::wformat(L"Put %d datapoints: { duration %7s, mean %7s}") 
							%(int)NUM_DATAPOINTS
							% timeToString(overheadTotal) 
							% timeToString(overheadMean));		
			LOG(INFO,L"Finished inserting Datapoints");

		}
	
		CALLGRIND_TOGGLE_COLLECT;
		CALLGRIND_STOP_INSTRUMENTATION;


}
