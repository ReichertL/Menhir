#pragma once

#include "definitions.h"
#include "struct_error.hpp"
#include "database_type.hpp"
#include "globals.hpp"

/**
 * @brief This class contains functions for computing a differentially private aggregate based on some data. 
 * 
 */

bool checkBudget(double usedBudged );
double laplace_mech(double value, double sensitivity, double epsilon);
vector<db_t> clipping(vector<db_t> data, db_t lower, db_t upper);
vector<double> clipping(vector<double> data, double lower, double upper);

tuple<double,Error>  dp_count(vector<db_t> data, vector<bool> ignoring, double epsilon, db_t lower, db_t upper, bool clip);
tuple<double,Error>  dp_count(vector<double> data, vector<bool> ignoring, double epsilon, double lower, double upper, bool clip);
tuple<double,Error>  count(vector<db_t> data, vector<bool> ignoring);

tuple<double,Error>  dp_sum_int(vector<db_t> data, vector<bool> ignoring, double epsilon, db_t lower, db_t upper, bool clip);
double kahan_sum(vector<double> vec, vector<bool> ignoring);
double pairwise_sum(vector<double> vec, vector<bool> ignoring);
tuple<double,Error>  dp_sum_double(vector<double> data, vector<bool> ignoring, double epsilon, double lower, double upper, bool clip);
tuple<double,Error>  dp_sum(vector<db_t> data, vector<bool> ignoring, double epsilon, db_t lower, db_t upper, bool clip = true);

tuple<double,Error>  dp_mean(vector<db_t> data, vector<bool> ignoring,  double epsilon, db_t lower, db_t upper, bool clip=true);
tuple<double,Error> dp_var(vector<db_t> data, vector<bool> ignoring, double epsilon, int ddof, db_t lower, db_t upper, bool clip=true);



vector<int> func_count(vector<db_t> data,vector<bool> ignoring, double lower, double upper);
tuple<db_t,Error> report_noisy_max_finite_int( vector<db_t> data,vector<bool> ignoring, int lower, int upper, double epsilon, int width=1, int mode=1);
tuple<db_t,Error> report_noisy_min_finite_int( vector<db_t> data, vector<bool> ignoring, int lower, int upper, double epsilon, int width=1, int mode=1);


