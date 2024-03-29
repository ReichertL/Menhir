#pragma once 

#include <climits>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

/**
 * @brief This class defines a database type based on unions. Currently, this allows to store both floats and integers in the same object without the need to adapt code for each type.
 * This class can be extended at a later point to encompass other types such as double, strings and so on.
 * General operators such as +,-,/,=,*,<,>,<=,=>,!= and == are defined for this database type. 
 * An underlying assumption of this class is that elements from one column all have the same type and computations on db_t elements only happens between elements from the same column (and therefore the same underlying type).
 * 
 */


enum class AType { INT, FLOAT};

using namespace std;


union Union{
    int i;
    float f;
    ~Union(){}
};



class db_t {
        public:

            bool isFloat;
            Union val;

            db_t(){
            }

            db_t(int a){
                val={a};
                isFloat=false;
            }
            db_t(float a){
                val.f=a;
                isFloat=true;
            }
            // copy constructor
            db_t(const db_t& a){
                val=a.val;
                isFloat=a.isFloat;
            }

            // destructor
            ~db_t() {
                //delete val;
            }

            // copy assignment constructor
            db_t operator=(const db_t& b) {
                val = b.val;
                isFloat=b.isFloat;
                return *this;

            }


            db_t operator+(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return db_t(val.f+b.val.f);
                    }else{
                        return db_t(val.i+b.val.i);
                    }
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }

            db_t operator+=(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        val.f=(val.f+b.val.f);
                    }else{
                        val.i=(val.i+b.val.i);
                    }
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
                return *this;
            }


            db_t operator-(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return db_t(val.f-b.val.f);
                    }else{
                        return db_t(val.i-b.val.i);
                    }                
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }

            db_t operator/(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return db_t(val.f/b.val.f);
                    }else{
                        return db_t(val.i/b.val.i);
                    }
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }

            db_t operator*(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return db_t(val.f*b.val.f);
                    }else{
                        return db_t(val.i*b.val.i);
                    }
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }

            db_t operator*(const bool& b) {
                if(isFloat){
                    return db_t(val.f* (float) b); 
                }else{
                    return db_t(val.i * (int)b);
                }
            }

            bool operator<(const db_t& b) const {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return val.f<b.val.f;
                    }else{
                        return val.i<b.val.i;
                    }
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }


            bool operator>(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return val.f>b.val.f;
                    }else{
                        return val.i>b.val.i;
                    }                
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }


            bool operator<=(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return val.f<=b.val.f;
                    }else{
                        return val.i<=b.val.i;
                    }
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }

            bool operator>=(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return val.f>=b.val.f;
                    }else{
                        return val.i>=b.val.i;
                    }
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }

            bool operator!=(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return val.f!=b.val.f;
                    }else{
                        return val.i!=b.val.i;
                    }                
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }

            bool operator==(const db_t& b) {
                if(isFloat==b.isFloat){
                    if(isFloat){
                        return val.f==b.val.f;
                    }else{
                        return val.i==b.val.i;
                    }
                }else{
                    throw invalid_argument("Operation on two db_t using different types.");
                }
            }



};


namespace DBT{


    using namespace std;
    using number = unsigned long long;
    using dbResponse=vector<tuple<vector<db_t>,bool>>;


    string aTypeToString(AType type);
    AType detectType(db_t a);
    string toString(db_t a);
    wstring toWString(db_t a);
    db_t fromString(string s, AType type);
    db_t fromString(string s, int type);
    size_t getMaxSizeDBT();
    vector<unsigned char> serialize(db_t a, AType type);
    db_t deserialize(vector<unsigned char> data, AType type);


    db_t sum_vector(vector<db_t> vec);
    db_t min_vector(vector<db_t> vec);
    db_t max_vector(vector<db_t> vec);
    db_t min_diff_vector(vector<db_t> vec);
    db_t mean_vector(vector<db_t>  vec);

    db_t abs(db_t a);

    double mean_vector_double(vector<db_t> vec);
    double std(vector<db_t> vec, int ddof=0);
    double var(vector<db_t> vec, int ddof=0);
    //double sqrt(db_t a);
    double toDouble(db_t a);
    db_t fromDouble(double d, AType type);
    vector<double> vecToDouble(vector<db_t> vec);
    db_t getDBTZero(AType type);

}