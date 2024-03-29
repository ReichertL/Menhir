#include "volume_sanitizer_utility.hpp"

#include <boost/random/uniform_real.hpp>
#include <boost/random/laplace_distribution.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/variate_generator.hpp>
#include <cmath>

/**
 * @brief This file contains functions related to the volume sanitizer. Some functions are adapted from Epsolute Code (https://github.com/epsolute/Epsolute).
 * 
 */

using namespace std;
using namespace DBT;

namespace MENHIR{

/**
 * @brief Function from Epsolute Paper (https://github.com/epsolute/Epsolute).
 * See section 5.2 of the paper.
 * 
 * @param m: the parallelisation factor (i.e. number of OSMs)
 * @param beta : failure probability 
 * @param kZero : Number of records needed to answer the query
 * @return number : number of data points to retrieve from each OSM. 
 */
number gammaNodes(number m, double beta, number kZero){
		auto gamma = sqrt(-3 * (long)m * log(beta) / kZero);
		return (number)ceil((1 + gamma) * kZero / (long)m);
}

/**
 * @brief Function from Epsolute Paper (https://github.com/epsolute/Epsolute).
 * 
 * @param q_from 
 * @param q_to 
 * @param bucketSize 
 * @param min 
 * @param max 
 * @param buckets 
 * @return tuple<number, number, double,double> 
 */
tuple<number, number, double,double> getBuckets(double q_from, double q_to, double bucketSize, double min,double max, number buckets){
		LOG(TRACE,boost::wformat(L"getBuckets: %9.2f %9.2f %9.2f %9.2f %9.2f ")% q_from %q_to %min %max %bucketSize);
		

		number fromBucket=0;
		number toBucket=0;

		//we can assume that q_from is not smaller than min because we clipped before. Analogous to tha q_to
		if (q_from>=0) fromBucket = (number)floor(((q_from- min) / bucketSize));
		else fromBucket = (number)floor((abs(min-q_from) / bucketSize));

		if (q_to>=0) toBucket = (number)floor(((q_to - min) / bucketSize));
		else toBucket = (number)floor((abs(min-q_to) / bucketSize));


		fromBucket = fromBucket == buckets ? fromBucket - 1 : fromBucket;
		toBucket   = toBucket == buckets ? toBucket - 1 : toBucket;

		double nFrom=(double)fromBucket * bucketSize + min;
		double nTo= (double) toBucket * bucketSize + min;
		return {fromBucket, toBucket, nFrom,nTo };
}

/**
 * @brief Function from Epsolute Paper (https://github.com/epsolute/Epsolute).
 * 
 * @param beta 
 * @param N 
 * @param epsilon 
 * @return number 
 */
number optimalMuForPointQueries(double beta,  number N, double epsilon){

		auto mu = ceil(- log(2 - 2 * pow(1 - beta, 1.0 / N)) / epsilon);
		if(mu<0){
			mu=0;
		}
		return (number) mu;
}

/**
 * @brief Function from Epsolute Paper (https://github.com/epsolute/Epsolute).
 * 
 * @param beta 
 * @param k 
 * @param N 
 * @param epsilon 
 * @param levels 
 * @return number 
 */
number optimalMuForRangeQueries(double beta, number k, number N, double epsilon, number levels){
		auto nodes	 = 0uLL;
		auto atLevel = (number)(log(N) / log(k));
		for (auto level = 0uLL; level <= levels; level++)
		{
			nodes += atLevel;
			atLevel /= k;
		}

		auto mu = ceil(-(double)levels * log(2 - 2 * pow(1 - beta, 1.0 / nodes)) / epsilon);
		if(mu<0){
			mu=0;
		}
		return (number) mu;
}

/**
 * @brief Function from Epsolute Paper (https://github.com/epsolute/Epsolute).
 * 
 * @param mu 
 * @param lambda 
 * @return double 
 */
double sampleLaplace(double mu, double lambda){
		boost::minstd_rand generator(SEED);
		auto laplaceDistribution = boost::random::laplace_distribution<double>(mu, lambda);
		boost::variate_generator<boost::minstd_rand, boost::random::laplace_distribution<double>> variateGenerator(generator, laplaceDistribution);

		return variateGenerator();
}

/*
Only required for sampling the noise for volume hiding of a specific query. 
The minimum for the noise is 0 (so lower=0). This means we will not retrieve less data points than the number of those that fall into the query interval. 
This function was not used during evaluation, but added for completeness.
*/
int sampleTruncatedLaplace_integer(double mu, double lambda, double lower, double upper){
		boost::minstd_rand generator(SEED);

		//create array with as many slots as can fit between lower and upper bound
		double mu_dash=round(mu-lower);
		vector<double> cdf_arr(round(upper-lower));
		
		double sum_p=0;
		for(size_t i=0;i<cdf_arr.size();i++){
			double p= cdf_laplace(mu_dash, lambda,(double)i);
			cdf_arr[i]=p;
			sum_p=sum_p+p;
		}
		//normalize
		for(size_t i=0;i<cdf_arr.size();i++){
			cdf_arr[i]=cdf_arr[i]/sum_p;
		}

		//draw from uniform distribution
		auto uniformDistribution = boost::random::uniform_01<>();
		boost::variate_generator<boost::minstd_rand, boost::random::uniform_01<>> variateGeneratorUniform(generator, uniformDistribution);
		double u=variateGeneratorUniform();

		int x=cdf_arr.size();
		for(size_t i=0;i<cdf_arr.size();i++){
			if(cdf_arr[i]>=u){
				x=i;
			}else{
				x=x;
			}
		}
		return x+lower;
}

/**
 * @brief Returns a sample from a truncated Laplace Function through inverse sampling (https://en.wikipedia.org/wiki/Inverse_transform_sampling).
 * First, uniformly draw sample from interval [0,1].
 * Then use this value as input for the quantile function of Laplace distribution.
 * It returns the value x, for which the accumulated probability mass surpasses x.
 * This is then returned as random sample.  
 * 
 * @param mu : location parameter of the distribution
 * @param lambda : spread of the distribution
 * @param lower : 
 * @param upper 
 * @return double 
 */
double sampleTruncatedLaplace_double(double mu, double lambda, double lower, double upper){


		// take inverse cdf  of laplace d("quantile function")
		//create array with as many slots as can fit between lower and upper bound

		double F_lower= cdf_laplace(mu,lambda,lower);
		double F_upper= cdf_laplace(mu,lambda,upper);

		//draw from uniform distribution
		boost::minstd_rand generator(SEED);
		auto uniformDistribution = boost::random:: uniform_real_distribution<>(F_lower,F_upper);
		boost::variate_generator<boost::minstd_rand, boost::random:: uniform_real_distribution<>> variateGeneratorUniform(generator, uniformDistribution);
		double u=variateGeneratorUniform();

		double x= quantileFunction_laplace(mu,lambda,u);
		return x;

}

/**
 * @brief According to https://en.wikipedia.org/wiki/Laplace_distribution
 * 
 * @param mu : location parameter of the distribution
 * @param lambda : spread of the distribution, also called "b" in the wikipedia article
 * @param F: accumulated probability
 * @return value :value for which the probability mass is larger than F
 */
double quantileFunction_laplace(double mu, double lambda,double F){
	double value=0;
	if(F<=0.5){
		value=mu+lambda*log(2*F);
	}else{
		value=mu-lambda*log(2-2*F);
	}
	return value;
}

/**
 * @brief CDF function for a Laplace distribution as in https://en.wikipedia.org/wiki/Laplace_distribution.
 * 
 * @param mu : location parameter of the distribution
 * @param lambda : spread of the distribution, also called "b" in the wikipedia article
 * @param x  : value to be checked
 * @return double : probability of x
 */
double cdf_laplace( double mu, double lambda, double x){
	double f_x=0.0;
	if	(x<= mu){
		f_x=0.5*exp((x-mu)/lambda);
	}else{
		f_x=1-(0.5*exp(-(x-mu)/lambda));
	}
	return f_x;
}

/**
 * @brief Function from Epsolute Paper (https://github.com/epsolute/Epsolute).
 * TODO: make alog oblivious 
 */
vector<pair<number, number>> BRC(number fanout, number from, number to, number maxLevel){
	//level , to bucket
	vector<pair<number, number>> result;
	number level = 0; // leaf-level, bottom

	do{
		// move FROM to the right within the closest parent, but no more than TO;
		// exit if FROM hits a boundary AND level is not maximal;
		while ((from % fanout != 0 || level == maxLevel) && from < to){
			result.push_back({level, from});
			from++;
		}

		// move TO to the left within the closest parent, but no more than FROM;
		// exit if TO hits a boundary AND level is not maximal
		while ((to % fanout != fanout - 1 || level == maxLevel) && from < to){
			result.push_back({level, to});
			to--;
		}

		// after we moved FROM and TO towards each other they may or may not meet
		if (from != to){
			// if they have not met, we climb one more level
			from /= fanout;
			to /= fanout;

			level++;
		}else{
			// otherwise we added this last node to which both FROM and TO are pointing, and return
			result.push_back({level, from});
			return result;
		}
	} while (true);
}

}