#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <climits>
#include <codecvt>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <istream>

#define _IF_THEN(S,A,B) (A*S+B*(not S));

#define KEYSIZE 32

// WARNING: this is supposed to be greater than the absolute value of the smallest element in the dataset
#define OFFSET 20000000

#define RPC_PORT 8787

// COLORS
// https://stackoverflow.com/a/9158263/1644554

#define RESET L"\033[0m"
#define BLACK L"\033[30m"			   /* Black */
#define RED L"\033[31m"				   /* Red */
#define GREEN L"\033[32m"			   /* Green */
#define YELLOW L"\033[33m"			   /* Yellow */
#define BLUE L"\033[34m"			   /* Blue */
#define MAGENTA L"\033[35m"			   /* Magenta */
#define CYAN L"\033[36m"			   /* Cyan */
#define WHITE L"\033[37m"			   /* White */
#define BOLDBLACK L"\033[1m\033[30m"   /* Bold Black */
#define BOLDRED L"\033[1m\033[31m"	   /* Bold Red */
#define BOLDGREEN L"\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW L"\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE L"\033[1m\033[34m"	   /* Bold Blue */
#define BOLDMAGENTA L"\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN L"\033[1m\033[36m"	   /* Bold Cyan */
#define BOLDWHITE L"\033[1m\033[37m"   /* Bold White */

using namespace std;


namespace MENHIR
{

	// defines the integer type block ID
	// change (e.g. to unsigned int) if needed
	using number = unsigned long long;
	using uchar	 = unsigned char;
	using uint	 = unsigned int;
	using bytes	 = vector<uchar>;
	using npmap	 = map<pair<number,number>,number>;
    using profile = tuple<bool, number, number, number>;
    using measurement = tuple<number, number, number,number, number, number, number, number>;
    using queryResult = tuple<vector<string>,number, chrono::steady_clock::rep, number>;
    using rpcReturnType	  = vector<tuple<vector<string>, chrono::steady_clock::rep, number>>;
    using treeIndexType = vector<pair<number, bytes>> ;
	using queryReturnType = tuple<vector<bytes>, chrono::steady_clock::rep, number>;



	/**
	 * @brief Primitive exception class that passes along the excpetion message
	 *
	 * Can consume std::string, C-string and boost::format
	 */
	class Exception : public exception
	{
		public:
		explicit Exception(const char* message) :
			msg_(message)
		{
		}

		explicit Exception(const string& message) :
			msg_(message)
		{
		}

		explicit Exception(const boost::format& message) :
			msg_(boost::str(message))
		{
		}

		virtual ~Exception() throw() {}

		virtual const char* what() const throw()
		{
			return msg_.c_str();
		}

		protected:
		string msg_;
	};

}

#define LOG_PARAMETER(parameter) LOG(INFO, boost::wformat(L"%1% = %2%") % #parameter % parameter)
#define PUT_PARAMETER(parameter) root.put(#parameter, parameter);

enum LOG_LEVEL{
		ALL,
		TRACE,
		DEBUG,
		INFO,
		WARNING,
		ERROR,
		CRITICAL,
		LOG_LEVEL_INVALID
};

enum DATASOURCE_T{
		GENERATED,
		FROM_REAL,
		FROM_FILE,
		CROWD,
		DATASOURCE_T_INVALID
};


