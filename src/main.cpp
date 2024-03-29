#include "definitions.h"
#include "utility.hpp"
#include "globals.hpp"
#include "output_utility.hpp"
#include "get_data_and_queries.hpp"
#include "prepare_dosm.hpp"
#include "querying.hpp"
#include "parse_args.hpp"
#include "server_utility.hpp"
#include "state_table.hpp"

#include <ctime>
#include <numeric>
#include <string>


using namespace std;
using namespace MENHIR;
using namespace STATE_MACHINE;


/**
 * @brief Main function of Menhir. The program can be either run in server mode or run automated measurements for performance evaluation.
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char* argv[])
{
	// to use wcout properly
	try
	{
		setlocale(LC_ALL, "en_US.utf8");
		locale loc("en_US.UTF-8");
		wcout.imbue(loc);
	}
	catch (...)
	{
		LOG(WARNING, L"Could not set locale: en_US.UTF-8");
	}


	readCommandlineArgs( argc, argv);

	if(DATASOURCE != CROWD){
		//Data is not collected from a crowd, but instead generated or read from file
		transitionServerState(101);
		getDataset();
		logGlobals();
		prepareDOSM();


		if(INTERACTIVE_QUERIES){
			//queries can be posed via HTTP requests.
			transitionServerState(203);
			#ifndef SERVERLESS
			srv.bind("queryServer", &queryServer);
			srv.bind("stopQueryPhase", &stopQueryPhase); //writesOutput
			srv.run();
			#endif
			return 0;

		}else{
			//Pre-Generated Queries are posed one after another
			transitionServerState(201);
			automated_querying();
			transitionServerState(401);
			writeOutput();
			return 0;
		}

	}else{
		//Data is collected from a crowd (via HTTP messages). After data has been collected, the state of the server can be switched. In the query phase, only queries can be posed (via HTTP messages) until the privacy budget is used up. 
		prepareDOSM();
		logGlobals();

		transitionServerState(102);

		#ifndef SERVERLESS
		srv.bind("receiveDataFromCrowd", &receiveDataFromCrowd);
		srv.bind("stopCollectionPhase", &stopCollectionPhase);
		srv.bind("queryServer", &queryServer); 
		srv.bind("stopQueryPhase", &stopQueryPhase); 
		srv.run();
		#endif

	}

	return 0;
}


