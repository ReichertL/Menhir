#include "utility.hpp"
#include "definitions.h"
#include "globals.hpp"

#include <string>
#include <math.h>
#include <stdexcept>
#include <sqlite3.h> 

/**
 * @brief This file contains function of general utility.
 */

using namespace std;
using namespace MENHIR;


/**
 * @brief Checks for a lock DB which clarifies if parts of the RAM are locked.
 *  Returns true if such a file does not exist so the program just continues as expected.
 * 	If currently the required ram is not available, false is returned.
 * 
 * @return int 
 */
int isRAMAvailable(int id,int required){

	sqlite3 *db;
	int rc = sqlite3_open(RAM_LOCK_FILE.c_str(), &db);

	if( rc ) {
		LOG(ERROR,  boost::wformat(L"Can't open database: %s\n") % toWString(sqlite3_errmsg(db)));
		return true;
	}
	LOG(DEBUG, boost::wformat(L"Opened database %s successfully\n") %toWString(RAM_LOCK_FILE) );


	auto callback= [](void *data, int argc, char **argv, char **azColName){
   		int i;
   
   		for(i = 0; i<argc; i++){
      		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   		}
   		
		int sum=atoi(argv[0]);
		LOG(DEBUG, boost::wformat(L"Result: %d\n") %sum);


		int *available = (int*)data;
		*available = sum;

   		printf("\n");
   		return 0;
	};


   	int available=0;
	char *zErrMsg = 0;
	char * sql = "Select SUM(USED) from LOCKED;";
   	rc = sqlite3_exec(db, sql, callback, (void*)&available, &zErrMsg);
   	if( rc != SQLITE_OK ) {
		LOG(ERROR, boost::wformat(L"SQL error: %s\n") %toWString(zErrMsg));
      	sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return true;
   } else {
		LOG(DEBUG, boost::wformat(L"Select was successfull."));
   }

	LOG(DEBUG, boost::wformat(L"Result: %d\n") %available);

	if(available<required){
		sqlite3_close(db);
		return false;
	}

	string insert = "Insert into LOCKED (ID,USED) VALUES ("+to_string(id)+", "+to_string(required)+");";
   	rc = sqlite3_exec(db, insert.c_str(), callback, (void*)&available, &zErrMsg);
   	if( rc != SQLITE_OK ) {
		LOG(ERROR, boost::wformat(L"SQL error: %s\n") %toWString(zErrMsg));
      	sqlite3_free(zErrMsg);
		sqlite3_close(db);
		return true;
   } else {
		LOG(DEBUG, boost::wformat(L"Insertion was successfull."));
   }	


	sqlite3_close(db);
	return true;
}

/**
 * @brief Removes entry from RAM_LOCK_FILE which locks part of the RAM.
 * 
 * @param id : id of the entry in the RAM_LOCK_FILE locking the RAM.
 */
void freeRAM(int id){

	sqlite3 *db;
	int rc = sqlite3_open(RAM_LOCK_FILE.c_str(), &db);

	if( rc ) {
		LOG(ERROR,  boost::wformat(L"Can't open database: %s\n") % toWString(sqlite3_errmsg(db)));
		return;
	}
	LOG(DEBUG, boost::wformat(L"Opened database %s successfully\n") %toWString(RAM_LOCK_FILE) );


	auto callback= [](void *data, int argc, char **argv, char **azColName){
   		return 0;
	};


   	int available=0;
	char *zErrMsg = 0;
	string sql = "Delete from LOCKED where ID="+to_string(id)+";";
   	rc = sqlite3_exec(db, sql.c_str(), callback, (void*)&available, &zErrMsg);
   	if( rc != SQLITE_OK ) {
		LOG(ERROR, boost::wformat(L"SQL error: %s\n") %toWString(zErrMsg));
      	sqlite3_free(zErrMsg);
   } else {
		LOG(DEBUG, boost::wformat(L"Remove was successful."));
   }

	sqlite3_close(db);
	LOG(INFO, boost::wformat(L"Removed RAM lock"));

}


/**
 * @brief Returns String for LOG_LEVEL
 * 
 * @param level 
 * @return string 
 */
string toString(LOG_LEVEL level){
        switch (level){
            case LOG_LEVEL::ALL : return "ALL" ;
            case LOG_LEVEL::TRACE: return "TRACE";
            case LOG_LEVEL::DEBUG: return "DEBUG";
            case LOG_LEVEL::INFO: return "INFO";
            case LOG_LEVEL::ERROR:return "ERROR";
            case LOG_LEVEL::WARNING:return "WARNING";
            case LOG_LEVEL::CRITICAL: return "CRITICAL";
            // omit default case to trigger compiler warning for missing cases
        };
        return "";
}


/**
 * @brief Get LOG_LEVEL from string.
 * 
 * @param logLevelString 
 * @return LOG_LEVEL 
 */
LOG_LEVEL loglevelFromString(string logLevelString){
	LOG_LEVEL level=LOG_LEVEL::LOG_LEVEL_INVALID;
	if (logLevelString=="ALL"){ level=LOG_LEVEL::ALL;
        }else if(logLevelString =="TRACE"){ level=LOG_LEVEL::TRACE;
        }else if(logLevelString =="DEBUG"){ level=LOG_LEVEL::DEBUG;
        }else if(logLevelString =="INFO"){ level=LOG_LEVEL::INFO;
        }else if(logLevelString =="WARNING"){ level=LOG_LEVEL::WARNING;
        }else if(logLevelString =="ERROR"){ level=LOG_LEVEL::ERROR;
        }else if(logLevelString =="CRITICAL"){ level=LOG_LEVEL::CRITICAL;
		}//else{ LOG(INFO, L"Option passed with -v was not valid. The Log Level will be set to " +toWString(toString(CURRENT_LEVEL)));}
	return level;
}

/**
 * @brief For Logging. Returns wString for a given LOG_LEVEL.
 * 
 * @param level 
 * @return wstring 
 */
wstring toWString(LOG_LEVEL level){
	return toWString(toString(level));
}


/**
 * @brief Writes a log message. Wrapper. Function adapted from Epsolute (https://github.com/epsolute/Epsolute).
 * 
 * @param level 
 * @param message 
 */
void LOG(LOG_LEVEL level, boost::wformat message){
		LOG(level, boost::str(message));
}

/**
 * @brief Writes a log message. Function adapted from Epsolute (https://github.com/epsolute/Epsolute).
 * 
 * @param level 
 * @param message 
 */
void LOG(LOG_LEVEL level, wstring message){

		vector<wstring> logLevelColors = {
		BOLDWHITE,
		WHITE,
		CYAN,
		GREEN,
		YELLOW,
		RED,
		BOLDRED};

		if (level >= CURRENT_LEVEL){
			auto t = time(nullptr);
			wcout << L"[" << put_time(localtime(&t), L"%d/%m/%Y %H:%M:%S") << L"] " << setw(10) 
			<< logLevelColors[level] << toWString(level) 
			<< L": " << message << RESET << endl;


			if (FILE_LOGGING){
				wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
				LOG_FILE << "[" << put_time(localtime(&t), "%d/%m/%Y %H:%M:%S") << "] "
				<< setw(10) << converter.to_bytes(toWString(level)) << ": "
				<< converter.to_bytes(message) << endl;
			}
		}
		if (level == CRITICAL){
			exit(1);
		}
}

namespace MENHIR{
	/**
	 * @brief Return string for DATASOURCE_T type.
	 * 
	 * @param source 
	 * @return string 
	 */
	string toString(DATASOURCE_T source){
        switch (source){
            case DATASOURCE_T::GENERATED : return "GENERATED" ;
            case DATASOURCE_T::FROM_REAL: return "FROM_REAL";
            case DATASOURCE_T::FROM_FILE: return "FROM_FILE";
            case DATASOURCE_T::CROWD: return "CROWD";

            // omit default case to trigger compiler warning for missing cases
        };
        return "";
	}

	/**
	 * @brief Return int for DATASOURCE_T type.
	 * 
	 * @param source 
	 * @return int 
	 */
	int toInt(DATASOURCE_T source){
        switch (source){
            case DATASOURCE_T::GENERATED : return 0 ;
            case DATASOURCE_T::FROM_REAL: return 1;
            case DATASOURCE_T::FROM_FILE: return 2;
            case DATASOURCE_T::CROWD: return 3;

            // omit default case to trigger compiler warning for missing cases
        };
        return 4;
	}

	/**
	 * @brief Reads a string in the format "int,int,...,int" and returns a vector of ints. Used for parsing the retieveExactly command line argument.
	 * 
	 * @param retrieveExactlyString 
	 * @return vector<number> 
	 */
	vector<number> retieveExactlyfromString(string retrieveExactlyString){
		vector<string> vs;
		vector<number> rs;
		boost::algorithm::split(vs, retrieveExactlyString, boost::is_any_of(","));
		for(size_t i=0;i<vs.size();i++){
			rs.push_back(stoi(vs[i]));
		}
		return rs;

	}

	/**
	 * @brief Get DATASOURCE_T value from string. Used for parsing the datasource command line argument.
	 * 
	 * @param dataSourceString 
	 * @return DATASOURCE_T 
	 */
	DATASOURCE_T datasourcefromString(string dataSourceString){
		DATASOURCE_T selected=DATASOURCE_T::DATASOURCE_T_INVALID;
		if (dataSourceString=="GENERATED"){ selected=DATASOURCE_T::GENERATED;
        }else if(dataSourceString =="FROM_REAL"){ selected=DATASOURCE_T::FROM_REAL;
        }else if(dataSourceString =="FROM_FILE"){ selected=DATASOURCE_T::FROM_FILE;
        }else if(dataSourceString =="CROWD"){ selected=DATASOURCE_T::CROWD;}
		
		return selected;
	}

	/**
	 * @brief For an Error object, return the error code and warning message as string.
	 * 
	 * @param err 
	 * @return string 
	 */
    string errToString(Error err){
		ostringstream oss;
		oss<<"Error "<<err.code;
		oss<<": "<<err.err_string.c_str();
		return oss.str();
	}	

	/**
	 * @brief Logging/printing an Error using the LOG function.
	 * 
	 * @param err 
	 * @param level 
	 */
	void print_err(Error err, LOG_LEVEL level){
		LOG(level, err.err_string);
	}

	/**
	 * @brief Convert time to wstring
	 * 
	 * @param time 
	 * @return wstring 
	 */
	wstring timeToString(long long time){
		wstringstream text;
		vector<wstring> units = {
			L"ns",
			L"μs",
			L"ms",
			L"s"};
		for (number i = 0; i < units.size(); i++)
		{
			if (time < 10000 || i == units.size() - 1)
			{
				text << time << L" " << units[i];
				break;
			}
			else
			{
				time /= 1000;
			}
		}

		return text.str();
	}

	/**
	 * @brief Convert a string to wString. For logging purposes.
	 * 
	 * @param input 
	 * @return wstring 
	 */
	wstring toWString(string input){
		wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(input);
	}
	
	/**
	 * @brief Convert string into array of uchars.
	 * 
	 * @param s 
	 * @return bytes 
	 */
	bytes bytesFromString(string s){
		int len=s.length()+1;
		uchar buf[len];
		memcpy(buf, s.data(), len);
		vector<uchar> out((uchar *)buf, (uchar *)buf + len);	

		return out;	
	}

	/**
	 * @brief Convert array of uchars into wString.
	 * 
	 * @param bytes 
	 * @return wstring 
	 */
	wstring bytesToString(long long bytes){
		wstringstream text;
		vector<wstring> units = {
			L"B",
			L"KB",
			L"MB",
			L"GB"};
		for (number i = 0; i < units.size(); i++)
		{
			if (bytes < (1 << 13) || i == units.size() - 1)
			{
				text << bytes << L" " << units[i];
				break;
			}
			else
			{
				bytes >>= 10;
			}
		}

		return text.str();
	}

	/**
	 * @brief Convert a vector of uchars into a number.
	 * 
	 * @param b 
	 * @return number 
	 */
	number bytesToNumber(vector<uchar> b){
		number i;
		sscanf((char *) b.data(), "%llu", &i);
		return i;
	}

	/**
	 * @brief Convert a query object to a string.
	 * 
	 * @param q 
	 * @return string 
	 */
	string queryToString(Query q){
		std::ostringstream oss;
		oss<<"Query[";
		oss<<", attributeIndex:"<<q.attributeIndex;
		oss<<" from:"<<DBT::toString(q.from);
		oss<<", to:"<<DBT::toString(q.to);
		oss<<", pointQuery:"<<q.pointQuery;
		oss<<", whereIndex:"<<q.whereIndex;
		oss<<", whereFrom:"<<DBT::toString(q.whereFrom);
		oss<<", whereTo:"<<DBT::toString(q.whereTo);
		oss<<", agg:"<<q.agg;
		oss<<", epsilon:"<<q.epsilon;
		oss<<", extra:"<<q.extra;
		oss<<"]";
		return oss.str();
	}

	/**
	 * @brief Convert a query object to a string that follows the CSV format.
	 * 
	 * @param q 
	 * @return string 
	 */
	string queryToCSVString(Query q){
		std::ostringstream oss;
		oss<<q.attributeIndex;
		oss<<","<<DBT::toString(q.from);
		oss<<","<<DBT::toString(q.to);
		oss<<","<<q.pointQuery;
		oss<<","<<q.whereIndex;
		oss<<","<<DBT::toString(q.whereFrom);
		oss<<","<<DBT::toString(q.whereTo);
		oss<<","<<q.agg;
		oss<<","<<q.epsilon;
		oss<<","<<q.extra;
		return oss.str();
	}

	/**
	 * @brief Construct a query object from a string.
	 * 
	 * @param s 
	 * @return Query 
	 */
	Query queryFromCSVString(string s){
		
			vector<string> vs;
			boost::algorithm::split(vs, s, boost::is_any_of(","));
			auto attributeIndex = (uint) stoul(vs[0]);
			AType typeA = COLUMN_FORMAT[attributeIndex];
			auto from  = DBT::fromString(vs[1],typeA);
			auto to = DBT::fromString(vs[2],typeA);
			bool pointQuery = (bool) stoi(vs[3]);

			auto whereIndex = (uint) stoul(vs[4]);
			AType typeW = COLUMN_FORMAT[whereIndex];
			auto whereFrom  = DBT::fromString(vs[5],typeW);
			auto whereTo = DBT::fromString(vs[6],typeW);
			
			bool agg_index=bool(stoi(vs[7]));
			AggregateFunc agg=static_cast<AggregateFunc>(agg_index);

			double epsilon=stod(vs[8]);
			int mcount_width=stoi(vs[9]);
		Query query=Query{attributeIndex,from,to,pointQuery,whereIndex,whereFrom,whereTo, agg, epsilon,mcount_width};
		return query;
	}
	
	/**
	 * @brief Get an aggregate function from  a string 
	 * 
	 * @param s 
	 * @return AggregateFunc 
	 */
	AggregateFunc getAggFromString(string s){
		
		if(s=="SUM"){
			return AggregateFunc::SUM;
		}else if(s=="MEAN"){
			return AggregateFunc::MEAN;
		}else if(s=="VARIANCE"){
			return AggregateFunc::VARIANCE;

		}else if(s=="COUNT"){
			return AggregateFunc::COUNT;

		}else if(s=="MAX_COUNT"){
			return AggregateFunc::MAX_COUNT;

		}else if(s=="MIN_COUNT"){
			return AggregateFunc::MIN_COUNT;
		}
		return AggregateFunc::SUM;
	}

	/**
	 * @brief Get a vector of zeros that follows the column format. 
	 * 
	 * @param columnFormat 
	 * @return vector<db_t> 
	 */
	vector<db_t> getEmptyRow(vector<AType> columnFormat){
		vector<db_t> thisData; 
        for (size_t i = 0; i < columnFormat.size(); i++){
			AType type= columnFormat[i];
			db_t zero=DBT::getDBTZero(type);
            thisData.push_back(zero);    
        }
		return thisData;
	}
}
