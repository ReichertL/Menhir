#pragma once

#include "definitions.h"
#include "globals.hpp"
#include "utility.hpp"
#include "database_type.hpp"

#include <numeric>
#include <string>

/**
 * @brief This file contains functions related to the volume sanitizer. Some functions are adapted from Epsolute Code (https://github.com/epsolute/Epsolute).
 * 
 */

namespace MENHIR
{
	using namespace std;
	using namespace DBT;
	
	tuple<number, number, double, double> getBuckets(double q_from, double q_to, double bucketSize,double min,double max, number buckets);

	number optimalMuForPointQueries(double beta, number N, double epsilon);

	number optimalMuForRangeQueries(double beta, number k, number N, double epsilon, number levels);

	vector<pair<number, number>> BRC(number fanout, number from, number to, number maxLevel);

	double sampleLaplace(double mu, double lambda);
	
	int sampleTruncatedLaplace_integer(double mu, double lambda, double lower, double upper);
	
	double sampleTruncatedLaplace_double(double mu, double lambda, double lower, double upper);

	double cdf_laplace(double x, double mu, double sigma);

	double quantileFunction_laplace(double mu, double lambda,double F);


	number gammaNodes(number m, double beta, number kZero);
}



