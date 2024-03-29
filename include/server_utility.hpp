#pragma once

#include "definitions.h"
#include "globals_osm.hpp"

namespace MENHIR{

	//AUTHENTICATE THIS STEP
	pair<string,string> queryServer(string queryString,string pw);
	
	size_t receiveDataFromCrowd(string datastring);

	bool stopCollectionPhase(string pw);

	void stopQueryPhase(string pw);
}