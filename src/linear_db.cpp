#include "linear_db.hpp"
#include "utility.hpp"
#include "globals.hpp"

#include <stdexcept>
#include <numeric>

/**
 * @brief This file contains a code for a naive database based on linear scanning. It does not use an ORAM.
 * This exists solely for comparing runtimes of Menhir against the naive approach.
 * 
 */

namespace LinearDB{

/**
 * @brief Construct a new LinearOblivDB object
 * 
 * @param cF : column format of the database
 */
LinearOblivDB::LinearOblivDB(vector<AType> cF){
    this->columnFormat=cF;
    this->numColumns=this->columnFormat.size();
}

/**
 * @brief Construct a new LinearOblivDB  object
 * 
 * @param cF : column format of the database
 * @param inputData : input data for storing in the database
 * @param numDatapointsAtStart : index from which onward input data should be stored in the database
 */
LinearOblivDB::LinearOblivDB(vector<AType> cF, vector<vector<db_t>> *inputData, size_t numDatapointsAtStart){
    this->columnFormat=cF;
    this->numColumns=this->columnFormat.size();
	for(size_t i=0;i<numDatapointsAtStart;i++){
		if(CURRENT_LEVEL==DEBUG) LOG(DEBUG, boost::wformat(L"Inserting %d/%d")%i %numDatapointsAtStart);
		vector<db_t> record=INPUT_DATA.back();
		INPUT_DATA.pop_back();
		#ifdef NDEBUG 
			this->insert(record);
		#else
			this->insert(record,(size_t) i);
		#endif
	}
}

/**
 * @brief Inserts new data point into the naive database using keys and a value. This operation is very fast, as the data point only needs to be added to the end of the list.
 * 
 * @param key  : vector of len(columnFormat) containing a data point to be stored.
 * @param value : Value associated with the node (unindexed data).
 * @return size_t : Hash of the node
 */
size_t LinearOblivDB::insert(vector<db_t> key,MENHIR::bytes value ){
    size_t nodeHash=listOfNodes.size()+1;//getNodeHash(key, value,nodeID);
    if(CURRENT_LEVEL<=DEBUG){
        std::ostringstream oss;
        for (size_t i = 0; i < key.size(); i++){
            oss<<DBT::toString(key[i])<<",";
        }
        LOG( DEBUG,boost::wformat( L"LinearDB insert: [ %s ]- %d") % MENHIR::toWString(oss.str()) % nodeHash);
    }
    listOfNodes.push_back(make_tuple(key,nodeHash,value));
    return nodeHash;
}

/**
 * @brief Insert new data point in the naive database by only using keys.   
 * 
 * @param key : vector of len(columnFormat) containing a data point to be stored. 
 * @return size_t : hash of the node
 */
size_t LinearOblivDB::insert(vector<db_t> key ){
    if(key.size()!=(size_t)(numColumns)){
        throw std::invalid_argument("The number of Keys passed is not equal to the number of columns.");
    }

    MENHIR::bytes value=vector<uchar>( MENHIR::VALUE_SIZE, 0);
    return insert(key,value);
}

/**
 * @brief Inserts new data point into the naive database using keys and a hash. This operation is very fast, as the data point only needs to be added to the end of the list.
 * 
 * @param key  : vector of len(columnFormat) containing a data point to be stored.
 * @param nodeHash : node Hash to be set
 * @return size_t : returns nodeHash
 */
size_t LinearOblivDB::insert(vector<db_t> key, size_t nodeHash){
    if(key.size()!=(size_t)(numColumns)){
        throw std::invalid_argument("The number of Keys passed is not equal to the number of columns.");
    }

    if(CURRENT_LEVEL<=DEBUG){
        std::ostringstream oss;
        for (size_t i = 0; i < numColumns; i++){
            oss<<DBT::toString(key[i])<<",";
        }
        LOG( DEBUG,boost::wformat( L"LinearDB insert: [ %s ]- %d") % MENHIR::toWString(oss.str()) % nodeHash);
    }
    MENHIR::bytes value=vector<uchar>( MENHIR::VALUE_SIZE, 0);
    listOfNodes.push_back(make_tuple(key,nodeHash,value));
    return nodeHash;
}

/**
 * @brief For finding an interval while also ensuring volume patters are not leaked, the naive approach scans the whole database for each query.
 * Additionally, so it is not leaked wether a new data point was added, a (dummy) write to each of the positions of the output array is made.
 * Evaluations have shown, that although no ORAM is used here, this approach is slower by orders of magnitude than Menhir.
 * 
 * @param startKey 
 * @param endKey 
 * @param column 
 * @param estimate 
 * @return dbResponse 
 */
dbResponse LinearOblivDB::findInterval(db_t startKey, db_t endKey, ushort column, number estimate){

    dbResponse results;
    vector<bytes> files;

    if(MENHIR::RETRIEVE_EXACTLY.size()!=0){
        auto[dummy_key,dummy_hash, dummy_file]=listOfNodes[0];

        //filling result array with dummy values.
        for(size_t i=0; i<MENHIR::RETRIEVE_EXACTLY_NOW;i++){
            results.push_back(make_tuple(dummy_key,true));
            files.push_back(dummy_file);

        }
    }else{
        LOG(CRITICAL, L"Not implemented");
    }

    for(size_t i=0;i<listOfNodes.size();i++){
        auto[key,nodeHash,value]=listOfNodes[i];
        if(CURRENT_LEVEL<=DEBUG) LOG(DEBUG, boost::wformat(L"%d/%d \n ")%i % listOfNodes.size());
        bool inInterval=(key[column]>=startKey and key[column]<=endKey);

        bool written=false;
        for(size_t el=0;el<results.size();el++){
            auto[elInResults, isDummy] =results[el];
            bytes elInFiles=files[el];
            bool overwrite=isDummy and (not written) and inInterval;
            for(size_t att=0;att<MENHIR::NUM_ATTRIBUTES;att++){
                elInResults[att]=_IF_THEN(overwrite, key[att],elInResults[att]);
            }
            for(size_t indexByte=0;indexByte<MENHIR::VALUE_SIZE;indexByte++){
                elInFiles[indexByte]=(char)_IF_THEN(overwrite, (int) value[indexByte],(int)elInFiles[indexByte]);
            }
            written=_IF_THEN(overwrite,true,written);
            results[el]=make_tuple(elInResults,overwrite);
            files[el]=elInFiles;
        }
    }
    if(CURRENT_LEVEL<=DEBUG) LOG(DEBUG, boost::wformat(L"len(resuts)=%d, len(files)=%d")%results.size() %files.size());
    return results;

}


size_t LinearOblivDB::size(){
    return listOfNodes.size();
}

}