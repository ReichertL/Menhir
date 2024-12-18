#include "dp_query_functions.hpp"
#include "utility.hpp"
#include "volume_sanitizer_utility.hpp"

#include <stdexcept>
#include <cfloat>

using namespace std;
using namespace DBT;
using namespace MENHIR;

/**
 * @brief This class contains functions for computing a differentially private aggregate based on some data. 
 * 
 */



wstring error_budget= L"Privacy Budget (QUERY_RESPONSE_EPSILON %9.2f was not sufficent to execute this operation (selected epsilon %9.2f) . Please select a smaller epsilon for this query.";

/**
 * @brief Verifies if privacy budget is still available to compute the function.
 * 
 * @param usedBudged 
 * @return true 
 * @return false 
 */
bool checkBudget(double usedBudged ){

    if(AVAILABLE_BUDGET-usedBudged>0){
        return true;
    }else if(AVAILABLE_BUDGET-usedBudged+TOLERANCE>=0){
        return true;
    }
    return false;


}



/**
 * @brief Calls function to sample from a laplace distribution to generate DP noise according to sensitivity and epsilon. Floating point function.
* Then returns a the noisy value.
 * 
 * @param value 
 * @param sensitivity 
 * @param epsilon 
 * @return double 
 */
double laplace_mech(double value, double sensitivity, double epsilon){

    double scale=sensitivity/epsilon;
    double noise= sampleLaplace(0.0, scale);

    return value+noise;
}

/**
 * @brief Calls function to sample from a laplace distribution to generate DP noise according to sensitivity and epsilon. Integer Function.
 * Then returns a the noisy value.
 * 
 * @param value 
 * @param sensitivity 
 * @param epsilon 
 * @return int 
 */
int laplace_mech(int value, int sensitivity, double epsilon){

    double scale=sensitivity/epsilon;
    int noise= (int) sampleLaplace(0.0, scale);

    return value+noise;
}



/**
 * @brief  Cuts of values of a data set  which are higher than a given upper or lower value. Replaces these values with the upper or lower value. Works on db_t data.
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param lower 
 * @param upper 
 * @return vector<db_t> 
 */
vector<db_t> clipping(vector<db_t> data, db_t lower, db_t upper){
    for(size_t i=0; i<data.size();i++){
        if(data[i]<lower) data[i]= lower;
        else if(data[i]>upper) data[i]=upper;
    }
    return data;
}

/**
 * @brief  Cuts of values of a data set  which are higher than a given upper or lower value. Replaces these values with the upper or lower value. Works on doubles.
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param lower 
 * @param upper 
 * @return vector<double> 
 */
vector<double> clipping(vector<double> data, double lower, double upper){
    for(size_t i=0; i<data.size();i++){
        if(data[i]<lower) data[i]= lower;
        else if(data[i]>upper) data[i]=upper;
    }
    return data;
}



/**
 * @brief Returns the number of elements in data sanitized with differential privacy. Used for or COUNT(*) QUERIES.
 * Clipping is not always necessary here.
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation.
 * @param epsilon : privacy budget to be used during this operation
 * @param lower : lower cutoff point for clipping
 * @param upper : upper cutoff point for clipping
 * @param clip : wether or not data needs to be clipped before processing
 * @return tuple<double,Error> : tuple containing the aggregate and an Error object. If not enough privacy budget was available the aggregate will be Null and the Error object will contain information.
 */
tuple<double,Error>  dp_count(vector<db_t> data, vector<bool> ignoring, double epsilon, db_t lower, db_t upper, bool clip){
    if(! checkBudget(epsilon)){
        Error err=Error(boost::wformat(error_budget)%QUERY_RESPONSE_EPSILON % epsilon);
        print_err(err);
		return make_tuple(NULL,err); 
    }
    AVAILABLE_BUDGET-=epsilon;
    
    if(clip) data=clipping(data, lower, upper);
    double count_sensitivity=1;
    int numIgnoring = std::accumulate(ignoring.begin(), ignoring.end(), 0);    
    double value=(double) data.size() - numIgnoring;
    double result= laplace_mech(value,count_sensitivity, epsilon);
    return make_tuple(result, Error());
}


/**
 * @brief Function that simply count how many real values fall into the interval. NO Differential Privacy is applied by this function! The Query poser needs to be trusted if this function is used!s
 * 
 * @param data 
 * @param ignoring 
 * @return tuple<double,Error> 
 */
tuple<double,Error>  count(vector<db_t> data, vector<bool> ignoring){

    double count_sensitivity=1;
    int numIgnoring = std::accumulate(ignoring.begin(), ignoring.end(), 0);    
    double value=(double) data.size() - numIgnoring;
    return make_tuple(value, Error());
}


/**
 * @brief Returns the number of elements in data sanitized with differential privacy. Used for COUNT(*) QUERIES.
 * This functions accepts a vector of doubles instead a vector of db_t
 * Clipping is not always necessary here.
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation.
 * @param epsilon : privacy budget to be used during this operation
 * @param lower : lower cutoff point for clipping
 * @param upper : upper cutoff point for clipping
 * @param clip : wether or not data needs to be clipped before processing
 * @return tuple<double,Error> : tuple containing the aggregate and an Error object. If not enough privacy budget was available the aggregate will be Null and the Error object will contain information.
 */
tuple<double,Error>  dp_count(vector<double> data, vector<bool> ignoring, double epsilon, double lower, double upper, bool clip){
    if(! checkBudget(epsilon)){
        Error err=Error(boost::wformat(error_budget)%QUERY_RESPONSE_EPSILON % epsilon);
        print_err(err);
		return make_tuple(NULL,err); 
    }
    AVAILABLE_BUDGET-=epsilon;
    
    if(clip) data=clipping(data, lower, upper);
    double count_sensitivity=1;
    int numIgnoring = std::accumulate(ignoring.begin(), ignoring.end(), 0);    
    double value=(double) data.size() - numIgnoring;
    double result= laplace_mech(value,count_sensitivity, epsilon);
    return make_tuple(result, Error());
}




/**
 * @brief Following algorithms for summation of float from Casacuberta et al.[2022]: "compensated summations".
 * Sensitivity is O(n/2^k)*max(|Lower|, Upper) where k is the number of bit in a double.
 * 
 * @param vec : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from vec during computation. This is for volume patter sanitation.
 * @return double : Differentially private sum.
 */
double kahan_sum(vector<double> vec, vector<bool> ignoring){
    double sum=0.0;
    double c=0.0;
    for(size_t i=0;i<vec.size();i++){
        double y= vec[i]-c;
        double t=sum+y;
        c=(t-sum) -y;
        bool dummy=ignoring[i];
        sum=dummy*0+(not dummy)*t;
    }
    return sum;

}



/**
 * @brief Following algorithms for summation of float from Casacuberta et al.[2022].
 * Takes log(n) steps, apparently the numpy default.
 *  Sensitivity is O(n*log(n)/2^k)*max(|Lower|,Upper), where k is the number of bit in a double.
 * 
 * @param vec : Data for computing the aggregate. Some dummy values are also in this vector. 
 * @param ignoring : Which values to ignore from vec during computation. This is for volume patter sanitation. 
 * @return double : Differentially private sum.
 */
double pairwise_sum(vector<double> vec, vector<bool> ignoring){
    size_t n=  vec.size();
    if(vec.size()<2ull){
        bool dummy= ignoring[0];
        return 0*dummy+vec[0]*(not dummy);
    }else{
        size_t m= floor(n/2ull);
        vector<double> split_lo(vec.begin(), vec.begin() + m);
        vector<double> split_hi(vec.begin() + m, vec.end());
        vector<bool> splitIgnoring_lo(ignoring.begin(), ignoring.begin() + m);
        vector<bool> splitIgnoring_hi(ignoring.begin() + m, ignoring.end());

        double sum=pairwise_sum(split_lo,splitIgnoring_lo)+pairwise_sum(split_hi,splitIgnoring_hi);
        return sum;
    }
}

/**
 * @brief Returns the differentially private sum for data.
 * We are using Kahan summation according to Casacuberta et al.[2022] for better sensitivity bounds.
 * We are using unbounded Differential Privacy here. Therefore data is randomly permuted and truncated before summation.
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation.
 * @param epsilon : privacy budget to be used during this operation
 * @param lower : lower cutoff point for clipping
 * @param upper : upper cutoff point for clipping
 * @param clip : wether or not data needs to be clipped before processing
 * @return tuple<double,Error> : tuple containing the aggregate and an Error object. If not enough privacy budget was available the aggregate will be Null and the Error object will contain information.
 */
tuple<double,Error>  dp_sum_double(vector<double> data, vector<bool> ignoring, double epsilon, double lower, double upper, bool clip){
    if(! checkBudget(epsilon)){
        Error err=Error(boost::wformat(error_budget)%QUERY_RESPONSE_EPSILON % epsilon);
        print_err(err);
		return make_tuple(NULL,err); 
    }

    if(abs(lower)>upper){
        Error err=Error(L"Error in Summation algorithm!"\
        "Limits need to be the following to ensure privacy: abs(lower)<=upper but was not. "\
        " No privacy budget was consumed by this action.");
        print_err(err);
		return make_tuple(NULL,err); 
    }   


    AVAILABLE_BUDGET-=epsilon;

    std::vector<int> indexes;
    indexes.reserve(data.size());
    for (size_t i = 0; i < data.size(); ++i)
        indexes.push_back(i);
    std::shuffle(indexes.begin(), indexes.end(), GENERATOR);

    vector<double> shuffledData;
    vector<bool> shuffledIgnoring;
    for(size_t i=0;i<indexes.size();i++){
        int oldIndex=indexes[i];
        shuffledData.push_back(data[oldIndex]);
        shuffledIgnoring.push_back(ignoring[oldIndex]);
    }
    data=shuffledData;
    ignoring=shuffledIgnoring;

    //truncating vector to ensure unbounded differential privacy
    // following Casacuberta et al. , Remark 6.27
    double n_max=670000000;
    data.resize(n_max);

    if(clip) data=clipping(data, lower, upper);

    double twok= (double) pow(2,DBL_MANT_DIG); //since double uses a Matissa with 53 bit
    
    double kahan_sensivity;
    if((upper<=0 and lower<=0) or (upper>=0 and lower>=0) ){
        kahan_sensivity= (1+(n_max/twok))*max(abs(lower),upper);
    }else{
        kahan_sensivity=(1+n_max/twok)*max(abs(lower),max(upper, upper-lower));
    }

    double sum= kahan_sum(data,ignoring);
    double result= laplace_mech(sum,kahan_sensivity, epsilon);
    return make_tuple(result, Error());
}



/**
 * @brief Returns the differentially private sum for data.
 *  This mechanism  has modular sensitivity according to Casacuberta et al.[2022]
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation.
 * @param epsilon : privacy budget to be used during this operation
 * @param lower : lower cutoff point for clipping
 * @param upper : upper cutoff point for clipping
 * @param clip : wether or not data needs to be clipped before processing
 * @return tuple<double,Error> : tuple containing the aggregate and an Error object. If not enough privacy budget was available the aggregate will be Null and the Error object will contain information.
 */
tuple<double,Error>  dp_sum_int(vector<db_t> data, vector<bool> ignoring, double epsilon, db_t lower, db_t upper, bool clip){
    if(! checkBudget(epsilon)){
        Error err=Error(boost::wformat(error_budget)%QUERY_RESPONSE_EPSILON % epsilon);
        print_err(err);
		return make_tuple(NULL,err); 
    }
    
    AVAILABLE_BUDGET-=epsilon;

    if(clip) data=clipping(data, lower, upper);

    int sensitivity= upper.val.i; //uppper is the sensitivity because we are using unbounded differential privacy
    int sum=0;
    for (size_t i=0;i<data.size();i++){
        int val=data[i].val.i;
        bool dummy=ignoring[i];
        sum+=0*(dummy)+val*(not dummy);
    }
    int result= laplace_mech(sum,sensitivity, epsilon);
    return make_tuple(result, Error());
}


/**
 * @brief Returns the differentially private sum for data. Uses the improved noisy bounded sum algorithms by Casacuberta.
 * Floating point arithmetic and integer arithmetic are two different cases so two different algorithms are used.
 * Clipping is necessary unless the data has been clipped in a prior step.
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation.
 * @param epsilon : privacy budget to be used during this operation
 * @param lower : lower cutoff point for clipping
 * @param upper : upper cutoff point for clipping
 * @param clip : wether or not data needs to be clipped before processing
 * @return tuple<double,Error> : tuple containing the aggregate and an Error object. If not enough privacy budget was available the aggregate will be Null and the Error object will contain information.
 */
tuple<double,Error>  dp_sum(vector<db_t> data, vector<bool> ignoring, double epsilon,  db_t lower, db_t upper, bool clip){
    if(data.size()==0){
        return make_tuple(0.0,Error());
    }
    if(detectType(data[0])==AType::FLOAT){
        vector<double> data_d=vecToDouble(data);
        double lower_d=toDouble(lower);
        double upper_d=toDouble(upper);
        return dp_sum_double(data_d, ignoring, epsilon,lower_d,upper_d,true );
    }else if(detectType(data[0])==AType::INT){
        return dp_sum_int(data,ignoring, epsilon, lower, upper, true);
    }else{
        throw std::invalid_argument("Unrecognized type encountered while typecasting db_t to number.");
    }
}


/**
 * @brief Returns a differentially private mean for data. Splits the the calculation of mean into two laplace algorithms.
 * Privacy cost is epsilon_sum + epsilon_count.
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation.
 * @param epsilon : privacy budget to be used during this operation
 * @param lower : lower cutoff point for clipping
 * @param upper : upper cutoff point for clipping
 * @param clip : wether or not data needs to be clipped before processing 
 * @return tuple<double,Error> : tuple containing the aggregate and an Error object. If not enough privacy budget was available the aggregate will be Null and the Error object will contain information.
 */
tuple<double,Error> dp_mean(vector<db_t> data, vector<bool> ignoring, double epsilon, db_t lower, db_t upper, bool clip){
    if(! checkBudget(epsilon)){
        Error err=Error(boost::wformat(error_budget)%QUERY_RESPONSE_EPSILON % epsilon);
        print_err(err);
		return make_tuple(NULL,err); 
    }
  
    if(clip){
 
        data=clipping(data, lower, upper);
    }

    int sum,count;
    double mean;
    Error err =Error();
    tie(sum, err) = dp_sum(data,ignoring, epsilon/2, lower, upper,false);
    if(err.is_err()) return make_tuple(NULL,err);
    tie(count, err) = dp_count(data, ignoring,epsilon/2,lower, upper,false);
    if(err.is_err()) return make_tuple(NULL,err);
    mean=sum/count;

    return make_tuple(mean, err);
}


/**
 * @brief Returns the differentially private variance of data. Privacy cost is epsilon_mean+epsilon_sum + epsilon_count.
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector. 
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation. 
 * @param epsilon : privacy budget to be used during this operation 
 * @param ddof : "Delta Degrees of Freedom”
 * @param lower : lower cutoff point for clipping 
 * @param upper : upper cutoff point for clipping 
 * @param clip : wether or not data needs to be clipped before processing 
 * @return tuple<double,Error> : tuple containing the variance and an Error object. If not enough privacy budget was available the aggregate will be Null and the Error object will contain information.
 */
tuple<double,Error> dp_var(vector<db_t> data, vector<bool> ignoring, double epsilon, int ddof, db_t lower, db_t upper, bool clip){
    if(! checkBudget(epsilon)){
        Error err=Error(boost::wformat(error_budget)%QUERY_RESPONSE_EPSILON % epsilon);
        print_err(err);
		return make_tuple(NULL,err); 
    }
    if(clip){ 
        data=clipping(data, lower, upper);
    }

    double mean_x,sum,count,var;
    vector<double> diffs_d;
    Error err;
    tie(mean_x, err)=dp_mean(data,ignoring, epsilon/2, lower, upper, false);
    if(err.is_err()) return make_tuple(NULL,err);
    for(db_t a: data){
        double da=toDouble(a);
        diffs_d.push_back(std::abs(da-mean_x));
    }
    double lower_d= toDouble(lower);
    double upper_d = toDouble(upper);
    tie(sum,err)=dp_sum_double(diffs_d, ignoring, epsilon/4.0, lower_d, upper_d, false);
    if(err.is_err()) return make_tuple(NULL,err);
    tie(count,err)=dp_count(diffs_d, ignoring, epsilon/4.0,  lower_d, upper_d, false);
    if(err.is_err()) return make_tuple(NULL,err);

    var= sum/(count-ddof);
    return make_tuple(var,err);
}

/**
 * @brief Subfunction to be used by report_noisy_max and report_noisy_min.
 * Function report_noisy_max returns the most common element between upper and lower if func_count is used (mode=1).
 * 
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation.
 * @param lower : lower cutoff point 
 * @param upper : upper cutoff point
 * @return vector<int>  : an array of counts containing how many element fall into each bin
 */
vector<int> func_count(vector<db_t> data, vector<bool> ignoring, double lower, double upper){
    vector<int> scores ( upper-lower+1,0 ); 
    //using the count funtion to calculate the score for each option
    for(size_t i=0;i<data.size();i++){
        int val = data[i].val.i;
        bool dummy=ignoring[i];
        scores[val-lower]+= dummy*0+(not dummy)*1;
    }
    return scores;
}





/**
 * @brief This function returns the most common element between upper and lower if func_count is used (mode=1).
 * This is useful if upper and lower represent categories.
 * This algorithm only performs for a finite number of options the less options, the better the result.
 * It only has the privacy budget epsilon because only one element is released.
 * This fuction can only be used if the data is of type int.

 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation.
 * @param lower : lower cutoff point for clipping
 * @param upper : upper cutoff point for clipping
 * @param epsilon : privacy budget to be used during this operation
 * @param width : This defines the width of each bucket. Currently unused.
 * @param mode : What type of subfunction should be executed. Currently only Mode=1 is implemented which defaults to: func_count.
 * @return tuple<db_t,Error> : tuple containing the result and an Error object. If not enough privacy budget was available the result will be Null and the Error object will contain information.
 */
tuple<db_t,Error> report_noisy_max_finite_int( vector<db_t> data, vector<bool> ignoring, int lower, int upper, double epsilon, int width, int mode){
    if(! checkBudget(epsilon)){
        Error err=Error(boost::wformat(error_budget)%QUERY_RESPONSE_EPSILON % epsilon);
        print_err(err);
		return make_tuple(db_t(),err);
    }
    AVAILABLE_BUDGET-=epsilon;



    double num_options =(upper-lower+1)/width;
    if (round(num_options)!= num_options){
		wstring err_string= L"The differentially private algorithm MAX_COUNT can only be applied to data of type INT. No privacy budget was consumed.";
        Error err=Error(err_string);
        print_err(err);
		return make_tuple(db_t(),err);
    }


    vector<int> scores;

    double sensitivity;
    if (mode==1){
        //using the count funtion to calculate the score for each option
        sensitivity=1;
        scores=func_count(data, ignoring,  lower,upper);
    }


    vector<double> noisy_scores;
    for(int i=0; i<(int)scores.size();i++){
        double value=(double)scores[i];
        double noisy=laplace_mech(value,sensitivity, epsilon);
        noisy_scores.push_back(noisy);
    }

    int index_max=std::distance(noisy_scores.begin(), std::max_element(noisy_scores.end(), noisy_scores.end()));

    db_t result=db_t(lower+index_max);

    return make_tuple(result,Error());

}



/**
 * @brief This function returns the most common element between upper and lower if func_count is used (mode=1).
 * This is useful if upper and lower represent categories.
 * This algorithm only performs for a finite number of options the less options, the better the result.
 * It only has the privacy budget epsilon because only one element is released.
 * This fuction can only be used if the data is of type int.
 * 
 * @param data : Data for computing the aggregate. Some dummy values are also in this vector.
 * @param ignoring : Which values to ignore from data during computation. This is for volume patter sanitation.
 * @param lower : lower cutoff point for clipping
 * @param upper : upper cutoff point for clipping
 * @param epsilon : privacy budget to be used during this operation
 * @param width : This defines the width of each bucket. Currently unused.
 * @param mode : What type of subfunction should be executed. Currently only Mode=1 is implemented which defaults to: func_count.
 * @return tuple<db_t,Error> : tuple containing the aggregate and an Error object. If not enough privacy budget was available the aggregate will be Null and the Error object will contain information.
 */
tuple<db_t,Error> report_noisy_min_finite_int( vector<db_t> data, vector<bool> ignoring, int lower, int upper, double epsilon, int width, int mode){
    if(! checkBudget(epsilon)){
        Error err=Error(boost::wformat(error_budget)%QUERY_RESPONSE_EPSILON % epsilon);
        print_err(err);
		return make_tuple(db_t(),err);
    }
    AVAILABLE_BUDGET-=epsilon;

    double num_options =(upper-lower+1)/width;
    if (round(num_options)!= num_options){
		wstring err_string= L"The differentially private algorithm MAX_COUNT can only be applied to data of type INT. No privacy budget was consumed.";
        Error err=Error(err_string);
        print_err(err);
		return make_tuple(db_t(),err);
    }


    vector<int> scores;

    double sensitivity;
    if (mode==1){
        //using the count funtion to calculate the score for each option
        sensitivity=1;
        scores=func_count(data, ignoring,  lower,upper);
    }


    vector<double> noisy_scores;
    for(int i=0; i<(int)scores.size();i++){
        double value=(double)scores[i];
        double noisy=laplace_mech(value,sensitivity, epsilon);
        noisy_scores.push_back(noisy);
    }

    int index_min=std::distance(noisy_scores.begin(), std::min_element(noisy_scores.end(), noisy_scores.end()));

    db_t result=db_t(lower+index_min);

    return make_tuple(result,Error());

}
