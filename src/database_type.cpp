#include "database_type.hpp"

#include <stdlib.h>     /* abs */
#include <cmath>
#include <numeric>
#include <codecvt>
#include <locale>
#include <iostream>

using namespace std;

/**
 * @brief This class defines a database type based on unions. Currently, this allows to store both floats and integers in the same object without the need to adapt code for each type.
 * This class can be extended at a later point to encompass other types such as double, strings and so on.
 * General operators such as +,-,/,=,*,<,>,<=,=>,!= and == are defined for this database type. 
 * An underlying assumption of this class is that elements from one column all have the same type and computations on db_t elements only happens between elements from the same column (and therefore the same underlying type).
 * 
 */


namespace DBT{


    string aTypeToString(AType type){
        if(type==AType::INT){
            return "INT";
        }else{
            return "FLOAT";
        }
    }

    AType detectType(db_t a){
        if (a.isFloat) {
            return AType::FLOAT;
        }else{
            return AType::INT;
        }        
    }

    wstring toWString(db_t a){
        wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(toString(a));
    }

    string toString(db_t a){

        ostringstream oss;
        if(a.isFloat){
            oss<<(boost::format("%f") % a.val.f);
        }else{
            oss<<a.val.i;
        }
        return oss.str();
    }

    db_t fromString(string s, int type){
        if(type == 0){
            return fromString(s, AType::INT);
        }else if (type == 1){
            return fromString(s, AType::FLOAT);
        }else{
            throw std::invalid_argument("Unrecognized type encountered while typecasting db_t to number.");
        }
    }

    db_t fromString(string s, AType type){
        if(type == AType::INT){
            int i= stoi(s);
            db_t d(i);
            return d;
        }else if (type == AType::FLOAT){
            float f=stof(s);
            db_t d(f);
            return d;
        }else{
            throw std::invalid_argument("Unrecognized type encountered while typecasting db_t to number.");
        }
    }

    size_t getMaxSizeDBT(){
        size_t size_int =  sizeof(int) ;
        size_t size_float =  sizeof(float);
        size_t max_size=std::max(size_int,size_float);
        return max_size;
    }

    //TODO: This interface can be change so type is not used anymore. requires some more rewrite
    vector<unsigned char> serialize(db_t a, AType type){
        size_t size_int =  sizeof(int);
        size_t size_float =  sizeof(float);

        vector<unsigned char> data;
        if(type == AType::INT){
            int value =a.val.i;          
            void* vp =(void*)&value;
            unsigned char* uBytes =(unsigned char*)vp;
            for(size_t i=0; i<size_int;i++){
                data.push_back(uBytes[i]);
            }

        }else if (type == AType::FLOAT){
            float value=a.val.f;
            void* vp =(void*)&value;
            unsigned char* uBytes =(unsigned char*)vp;
            for(size_t i=0; i<size_float;i++){
                data.push_back(uBytes[i]);
            }
        }else{
            throw std::invalid_argument("Unrecognized type encountered while typecasting db_t to number.");
        }
        return data;
    }

    db_t deserialize(vector<unsigned char> data, AType type){
        size_t size_int =  sizeof(int);
        size_t size_float =  sizeof(float);

        if(type == AType::INT){
            unsigned char v[size_int];
            for (size_t i = 0; i < size_int; i++){
                v[i]=data[i];
            } 
            int * vp=(int*) (&v[0]);
            int value=*vp;
            return db_t(value);

        }else if (type == AType::FLOAT){
            unsigned char v[size_float];
            for (size_t i = 0; i < size_int; i++){
                v[i]=data[i];
            } 
            float * vp=(float*) (&v[0]);
            float value=*vp;
            return db_t(value);

        }else{
            throw std::invalid_argument("Unrecognized type encountered while typecasting db_t to number.");
        }
    }


    db_t sum_vector(vector<db_t> vec){

        db_t sum = vec[0];
        for (db_t el: vec) {
            sum += el;
        }
        return sum-vec[0];
    }

    db_t min_vector(vector<db_t> vec){

        db_t min = vec[0];
        for (db_t el: vec) {
            if (el<min) min = el;
        }
        return min;
    }

    db_t max_vector(vector<db_t> vec){

        db_t max = vec[0];
        for (db_t el: vec)
            if (el>max) max =el;
        return max;
    }

    db_t min_diff_vector(vector<db_t> vec){
        AType type= detectType(vec[0]);
        db_t zero=getDBTZero(type);
        vector <db_t> diffs;

        for (db_t el1: vec){
            for (db_t el2: vec){
                db_t diff = DBT::abs(el1-el2);
                if (diff != zero) diffs.push_back(diff);
            }
        }
        db_t min =min_vector(diffs);
        return min;
    }

    db_t mean_vector(vector<db_t> vec){
        AType type=detectType(vec[0]);

        if (type==AType::INT){
            db_t temp ((int) vec.size());
            db_t result(sum_vector(vec)/temp);
            return result;

        }else if(type==AType::FLOAT){
            db_t temp ((float) vec.size());
            db_t result(sum_vector(vec)/temp);
            return result;

        }else{
            throw std::invalid_argument("Unrecognized type encountered while typecasting db_t to number.");
        }
    }

    double mean_vector_double(vector<db_t> vec){
        double size = (double) vec.size();	
        double mean = toDouble(sum_vector(vec))/size;
        return mean;
    }


    //uncorrected standard deviation for ddof =0
    //corrected standard deviation for ddof= 1
    // 95% of values are expected to fall in the confidence interval mean+- 2*std 
    double std(vector<db_t> vec, int ddof){
        double s2=DBT::var(vec,ddof);
        return std::sqrt(s2);
    }

    //uncorrected for ddof=0, corrected for ddof=1
    double var(vector<db_t> vec, int ddof){

        double mean_v = toDouble(mean_vector(vec));
        vector<double> all_x;
        for(db_t a: vec){
            double da=toDouble(a);
            double x=std::fabs((da-mean_v)*(da-mean_v));
            all_x.push_back(x);
        }
        double sum_d=std::accumulate(all_x.begin(), all_x.end(), 0.0);
        double s2 = sum_d/ ((double) all_x.size()-ddof);
        return s2;
    }

    db_t abs(db_t a){
        if(a.isFloat){
            a.val.f=std::fabs(a.val.f);
        }else{
            a.val.i=std::abs(a.val.i); 
        }
        return a;
    }


    double toDouble(db_t a){
        if(a.isFloat){
            return (double) a.val.f;
        }else{
            return ((double) a.val.i);
        }
    }

    db_t fromDouble(double d, AType type){
        if(type == AType::INT){
            int i= std::round(d);
            db_t dbt(i);
            return dbt;
        }else if (type == AType::FLOAT){
            db_t dbt((float) d);
            return dbt;
        }else{
            throw std::invalid_argument("Unrecognized type encountered while typecasting db_t to number.");
        }
    }

    vector<double> vecToDouble(vector<db_t> vec){
        vector<double> data_d;
        for(db_t el: vec){
            data_d.push_back(toDouble(el));
        }
        return data_d;
    }

    db_t getDBTZero(AType type){

        if(type==AType::INT){ 
            db_t zero(0);
            return zero;
        }else if(type == AType::FLOAT){
            db_t zero(0.0f);
			return zero;
        }else{
            throw std::invalid_argument("Unrecognized type encountered while casting db_t to string.");
        }
    }

}