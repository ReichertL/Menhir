

#include "path-oram/definitions.h"
#include "path-oram/oram.hpp"
#include "path-oram/position-map-adapter.hpp"
#include "path-oram/stash-adapter.hpp"
#include "path-oram/storage-adapter.hpp"
#include "avl_treenode.hpp"

#include <vector>
#include <numeric>
#include <iostream>
#include<chrono>

//using namespace MENHIR;
using namespace std;
using namespace DOSM;
using namespace DBT;



pair<double,double> benchmarkORAM(int numOps, number capacity, number ORAM_BLOCK_SIZE){


    number ORAM_Z= 3uLL;
    number BATCH_SIZE=1;
    number maxCapacity=capacity+1;
    number ORAM_LOG_CAPACITY=ceil(log2(maxCapacity  / ORAM_Z)) + 1;

    if(ORAM_BLOCK_SIZE<32 ){
        ostringstream oss;
        oss<<"The Nodes stored in the AVL Tree must be at least 2 AES block sizes, but was ";
        oss << ORAM_BLOCK_SIZE<<"\n";
        throw std::invalid_argument(oss.str());
    }

    shared_ptr<PathORAM::ORAM> oram = make_shared<PathORAM::ORAM>(
            ORAM_LOG_CAPACITY,
            ORAM_BLOCK_SIZE,
            ORAM_Z,
            make_shared<InMemoryStorageAdapter>((1 << ORAM_LOG_CAPACITY),ORAM_BLOCK_SIZE, bytes(),ORAM_Z),
			 make_shared<InMemoryPositionMapAdapter>(((1 <<ORAM_LOG_CAPACITY) *ORAM_Z) +ORAM_Z),
			 make_shared<InMemoryStashAdapter>(3 *ORAM_LOG_CAPACITY *ORAM_Z),
			true,
			BATCH_SIZE);


    bytes b(ORAM_BLOCK_SIZE, 0);

	chrono::steady_clock::time_point start = chrono::steady_clock::now();
    for(int i=0;i<numOps;i++){
        oram->put(i,b);
    }
	chrono::steady_clock::time_point end = chrono::steady_clock::now();
	double durInsert = chrono::duration_cast<chrono::microseconds>(end - start).count()/numOps;
	
    chrono::steady_clock::time_point startG = chrono::steady_clock::now();
    for(int i=0;i<numOps;i++){
        bytes response;
        oram->get(i,response);   
        if(response.size()==0){
            //LOG(ERROR, boost::wformat(L"Node with nodeID %d was not found in ORAM.")%ptr);
        } 
    }
    chrono::steady_clock::time_point endG = chrono::steady_clock::now();
	double durGet = chrono::duration_cast<chrono::microseconds>(endG - startG).count()/numOps;


    return make_pair(durInsert,durGet);

}


int main(){
    vector<number> blockSize;
    blockSize.push_back(64ull);
    blockSize.push_back(512ull);
    blockSize.push_back(4800ull);

    for(int i=0;i<(int)blockSize.size();i++){
        int numMeasures=10;
        int numOps=10000;
        int capacity=pow(10,5);
        vector<double> insert;
        vector<double> get;
        cout<<"Measured "<<numOps<<" operations for ORAM with BlockSize "<<blockSize[i]<<" Capacity:" << capacity<<endl; 
        cout.flush();
        for(int j=0; j<numMeasures;j++){
            pair<double,double> res=benchmarkORAM(numOps,capacity,blockSize[i]);
            insert.push_back(res.first);
            get.push_back(res.second);
        }
        float mean_i = reduce(insert.begin(), insert.end()) / insert.size();
        float mean_g = reduce(get.begin(), get.end()) / get.size();

        cout<<"Insert: "<< mean_i<<"MikroS Get:"<<mean_g<<" MikroS "<<endl;
        cout.flush();
    }
}