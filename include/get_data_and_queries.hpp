#pragma once

#include "definitions.h"
#include "globals.hpp"
#include "database_type.hpp"
#include <future>

/**
 * @brief This file contains functions for generating artificial data an queries. The basis of these can either be real data, specifically generated normal distributions or uniform distributions.
 * The latter case is relevant when a fixed number of elements are supposed to exist for each key. 
 * 
 */

namespace MENHIR{
   	using genType	 = tuple<vector<vector<db_t>>,vector<db_t>,vector<db_t>>;
   	using genType2	 = vector<db_t>;
 
    void getDataset();
    void generateFromReal();
    void generateArtificial();


    void createDatasetNormalDistribution(vector<float> *true_means, vector<float> *true_stddev );
	void generateRowsNormalDistribution(vector<float> ptrue_means,vector<float> ptrue_stddev, int n, promise<genType> * promise);
    void generateQueries();
    void generatePointQueries();
    void generateQueriesWithSensitivity();

	void createDatasetandQueriesWithSelectivity(vector<float> *ptrue_means, vector<float> *ptrue_stddev );
	void generateColumnNormalDistribution(int index, vector<float> ptrue_means,vector<float> ptrue_stddev, promise<genType2> * promise);
	void generateColumnWithSelectivity(int index, vector<float> ptrue_means,vector<float> ptrue_stddev, promise<genType2> * promise);
    


}