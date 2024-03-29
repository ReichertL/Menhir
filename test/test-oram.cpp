#define DEF_DEBUG
#include "definitions.h"
#include "avl_multiset.hpp"
#include "utility.hpp"
#include "path-oram/definitions.h"
#include "path-oram/oram.hpp"
#include "path-oram/utility.hpp"
#include "gtest/gtest.h"
//#include <variant>
#include <random>
#include <boost/variant2/variant.hpp>

using namespace boost::variant2;
using namespace std;
using namespace MENHIR;
using namespace DOSM;
using namespace testing;



shared_ptr<ORAM> getORAM(number CAPACITY, number BLOCK_SIZE, number Z, number BATCH_SIZE){

    number LOG_CAPACITY=std::max((number) ceil(log((double) CAPACITY/(double)Z)/log(2.0)),3ull);  
    //number LOG_CAPACITY = std::max((number) ceil(log(CAPACITY+1)/log(2))-1,2ull); 

    size_t oramParameter= (1 << LOG_CAPACITY) * Z;
    size_t stashSize=4 * LOG_CAPACITY * Z;
	shared_ptr<ORAM> oram  = make_shared<PathORAM::ORAM>(
            LOG_CAPACITY,
            BLOCK_SIZE,
            Z,
            make_shared<InMemoryStorageAdapter>(oramParameter, BLOCK_SIZE, bytes(), Z),
			make_shared<InMemoryPositionMapAdapter>(oramParameter),
			make_shared<InMemoryStashAdapter>(stashSize),
			true,
			BATCH_SIZE);
    LOG_PARAMETER(LOG_CAPACITY);
    LOG_PARAMETER(oramParameter);
    LOG_PARAMETER(stashSize);
    return oram;

}

TEST(ORAMTest, Setup){
    number CAPACITY = 20;
    number Z = 3;		
	number BLOCK_SIZE   = 64;
	number BATCH_SIZE   = 1;
    shared_ptr<ORAM> oram =getORAM(CAPACITY, BLOCK_SIZE, Z, BATCH_SIZE);

	ulong id=(ulong) 0;
	auto data = fromText(to_string(id), BLOCK_SIZE);
	oram->put(id+1, data);  
}


TEST(ORAMTest,Retrieve){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;

    size_t sizeValue=0;
    vector<AType> thisFormat {AType::INT};
    number BLOCK_SIZE=getNumBytesWhenSerialized(thisFormat,sizeValue);  

    number CAPACITY = 20;
    number Z = 3;		
    number BATCH_SIZE=1ull;

    //creates inMemory Position, Storage and Stash adapters 
    shared_ptr<ORAM> oram =getORAM(CAPACITY, BLOCK_SIZE, Z, BATCH_SIZE);

    LOG_PARAMETER(CAPACITY);
    LOG_PARAMETER(Z);
    LOG_PARAMETER(BATCH_SIZE);
    LOG_PARAMETER(BLOCK_SIZE);


    bytes value=vector<uchar>(sizeValue, 0);
    AVLTreeNode node= AVLTreeNode(vector<db_t>{db_t(11111)},0,value,thisFormat,7);

    bytes b=node.serialize();
    ostringstream oss;
    oss<<"Length of Serialized node:"<<b.size()<<endl;
    for(size_t i=0;i<b.size();i++){
        oss<<(int)b[i]<<" ";
    }
    oss<<endl;
    LOG(DEBUG, toWString(oss.str()));


    oram->put(1,b);

    bytes response;
    oram->get(1,response); 

    
    ostringstream oss2;
    oss2<<"response\n";
    for(size_t i=0;i<b.size();i++){
        oss2<<(int)response[i]<<" ";
    }
    oss2<<endl;
    LOG(DEBUG, toWString(oss2.str()));

    AVLTreeNode nodeRetrieved=AVLTreeNode(response,false,thisFormat,sizeValue);
    ASSERT_EQ(node.toStringFull(true),nodeRetrieved.toStringFull(true));

}


TEST(ORAMTest,Retrieve_NULLNODE){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;

    size_t sizeValue=0;
    vector<AType> thisFormat {AType::INT};
    number BLOCK_SIZE=getNumBytesWhenSerialized(thisFormat,sizeValue);  

    number CAPACITY = 20;
    number Z = 3;		
	number BATCH_SIZE   = 1;
    shared_ptr<ORAM> oram =getORAM(CAPACITY, BLOCK_SIZE, Z, BATCH_SIZE);

    LOG_PARAMETER(CAPACITY);
    LOG_PARAMETER(Z);
    LOG_PARAMETER(BATCH_SIZE);
    LOG_PARAMETER(BLOCK_SIZE);


    AVLTreeNode node= AVLTreeNode(thisFormat,sizeValue);
    bytes b=node.serialize();
    ostringstream oss;
    oss<<"Length of Serialized node:"<<b.size()<<endl;
    for(size_t i=0;i<b.size();i++){
        oss<<(int)b[i]<<" ";
    }
    oss<<endl;
    LOG(DEBUG, toWString(oss.str()));

    oram->put(NULL_PTR+1,b);

    bytes response;
    oram->get(NULL_PTR+1,response); 


    ostringstream oss2;
    oss2<<"response size "<<response.size()<<"\n";
    for(size_t i=0;i<response.size();i++){
        oss2<<(int)response[i]<<" ";
    }
    oss2<<endl;
    LOG(DEBUG, toWString(oss2.str()));

    AVLTreeNode nodeRetrieved=AVLTreeNode(response,false,thisFormat,sizeValue);
    ASSERT_EQ(node.toStringFull(true),nodeRetrieved.toStringFull(true));

}



TEST(ORAMTest,Retrieve_MultipleCols){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;

    size_t sizeValue=0;
    vector<AType> thisFormat {AType::INT, AType::INT};
    number BLOCK_SIZE=getNumBytesWhenSerialized(thisFormat,sizeValue);  

    number CAPACITY = 20;
    number Z = 3;		
	number BATCH_SIZE   = 1;
    shared_ptr<ORAM> oram =getORAM(CAPACITY, BLOCK_SIZE, Z, BATCH_SIZE);


    LOG_PARAMETER(CAPACITY);
    LOG_PARAMETER(Z);
    LOG_PARAMETER(BATCH_SIZE);
    LOG_PARAMETER(BLOCK_SIZE);

    bytes value=vector<uchar>(sizeValue,0);
    AVLTreeNode node= AVLTreeNode(vector<db_t>{db_t(11111),db_t(10)},0,value, thisFormat,7);

    bytes b=node.serialize();
    ostringstream oss;
    oss<<"Length of Serialized node:"<<b.size()<<endl;
    for(size_t i=0;i<b.size();i++){
        oss<<(int)b[i]<<" ";
    }
    oss<<endl;
    LOG(DEBUG, toWString(oss.str()));

    oram->put(1,b);

    bytes response;
    oram->get(1,response);    
    AVLTreeNode nodeRetrieved=AVLTreeNode(response,false,thisFormat,sizeValue);
    ASSERT_EQ(node.toStringFull(true), nodeRetrieved.toStringFull(true));
    

}

TEST(ORAMTest,Retrieve_MultipleColsMixed){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;

    size_t sizeValue=0;
    vector<AType> thisFormat {AType::INT, AType::INT,AType::FLOAT};
    number BLOCK_SIZE=getNumBytesWhenSerialized(thisFormat,sizeValue);  

    number CAPACITY = 20;
    number Z = 3;		
	number BATCH_SIZE   = 1;
    shared_ptr<ORAM> oram =getORAM(CAPACITY, BLOCK_SIZE, Z, BATCH_SIZE);


    LOG_PARAMETER(CAPACITY);
    LOG_PARAMETER(Z);
    LOG_PARAMETER(BATCH_SIZE);
    LOG_PARAMETER(BLOCK_SIZE);

    bytes value=vector<uchar>(sizeValue,0);
    AVLTreeNode node= AVLTreeNode(vector<db_t>{db_t(11111),db_t(10), db_t((float)M_PI)},0,value,thisFormat,7);

    bytes b=node.serialize();
    ostringstream oss;
    oss<<"Length of Serialized node:"<<b.size()<<endl;
    for(size_t i=0;i<b.size();i++){
        oss<<(int)b[i]<<" ";
    }
    oss<<endl;
    LOG(DEBUG, toWString(oss.str()));

    oram->put(1,b);

    bytes response;
    oram->get(1,response);    
    AVLTreeNode nodeRetrieved=AVLTreeNode(response,false,thisFormat, sizeValue);
    ASSERT_EQ(node.toStringFull(true), nodeRetrieved.toStringFull(true));

}


TEST(ORAMTest, NumLevelsCalculation){
 extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;

    size_t sizeValue=0;
    vector<AType> thisFormat {AType::INT};
    number BLOCK_SIZE=getNumBytesWhenSerialized(thisFormat,sizeValue);  

    for(size_t i=5;i<100;i++){
        number CAPACITY = i+1;//+1 for the NULL NODE

        LOG(INFO, boost::wformat(L"-----------Capacity: %d------------") %CAPACITY);
        number Z = 3;		
        number BATCH_SIZE   = 1;
        shared_ptr<ORAM> oram =getORAM(CAPACITY, BLOCK_SIZE, Z, BATCH_SIZE);


        LOG_PARAMETER(CAPACITY);
        LOG_PARAMETER(Z);
        LOG_PARAMETER(BATCH_SIZE);
        LOG_PARAMETER(BLOCK_SIZE);

        bytes value=vector<uchar>(sizeValue,0);
        AVLTreeNode tree_node= AVLTreeNode(vector<db_t>{db_t(5)},9, value,vector<ulong>{10},vector<ulong>{11},vector<ulong>{11},vector<int>{2},vector<int>{3},thisFormat, 0);
        //put and check that insertion succeeded
        for(size_t j=0;j<CAPACITY;j++){

            ulong  id =j; //NULLNODE is 0
            AVLTreeNode this_node=tree_node;
            tree_node.nodeID=id;
            tree_node.key[0]=db_t((int)j);
            bytes b = tree_node.serialize();

            LOG(DEBUG, boost::wformat(L"Inserting  %d / %d - ID %d") %(j+1) %CAPACITY %id );

            oram->put(id,b);
            bytes response;
            oram->get(id,response);    
            AVLTreeNode nodeRetrieved=AVLTreeNode(response,false,thisFormat, sizeValue);
            ASSERT_EQ(tree_node.toStringFull(true), nodeRetrieved.toStringFull(true));
        }
    }
}

TEST(ORAMTest, QueryAndUpdate){
 extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;

    size_t sizeValue=0;
    vector<AType> thisFormat {AType::INT};
    number BLOCK_SIZE=getNumBytesWhenSerialized(thisFormat, sizeValue);  
    if(BLOCK_SIZE<32 ){
        LOG(INFO, L"The Nodes stored in the AVL Tree must be at least 2 AES block sizes when serialized, so at least 32 bytes (equals rS=32).");
        BLOCK_SIZE=32;
    }
    if(BLOCK_SIZE%16!=0){
        BLOCK_SIZE=BLOCK_SIZE+(16-BLOCK_SIZE%16);
        LOG(INFO, boost::wformat(L"The size of the stored data has to be devisable by 16. We therefore pad the block size to: %d") %BLOCK_SIZE);
    }

        number CAPACITY = 100;
        number Z = 3;		
        number BATCH_SIZE   = 1;
        shared_ptr<ORAM> oram =getORAM(CAPACITY, BLOCK_SIZE, Z, BATCH_SIZE);


        LOG_PARAMETER(CAPACITY);
        LOG_PARAMETER(Z);
        LOG_PARAMETER(BATCH_SIZE);
        LOG_PARAMETER(BLOCK_SIZE);

        bytes value=vector<uchar>(sizeValue,0);
        AVLTreeNode tree_node= AVLTreeNode(vector<db_t>{db_t(5)},9,value,vector<ulong>{10},vector<ulong>{11},vector<ulong>{11},vector<int>{2},vector<int>{3},thisFormat, 0);
        size_t max_size=CAPACITY+1; //+1 for the NULL NODE
        //put and check that insertion succeeded
        for(size_t i=0;i<max_size;i++){
            ulong  id =i+1;
            AVLTreeNode this_node=tree_node;
            tree_node.nodeID=id;
            tree_node.key[0]=db_t((int)i);
            bytes b = tree_node.serialize();
            /*ostringstream oss;
            oss<<"Length of Serialized node:"<<b.size()<<endl;
            for(size_t i=0;i<b.size();i++){
                oss<<(int)b[i]<<" ";
            }
            oss<<endl;
            LOG(DEBUG, toWString(oss.str()));*/

            oram->put(id,b);
            bytes response;
            oram->get(id,response);    
            AVLTreeNode nodeRetrieved=AVLTreeNode(response,false,thisFormat, sizeValue);
            ASSERT_EQ(tree_node.toStringFull(true), nodeRetrieved.toStringFull(true));
        }

        size_t newValue=99;
        //update and put back
        for(size_t i=0;i<max_size;i++){
            ulong  id =i+1;
            bytes response;
            oram->get(id,response);    
            AVLTreeNode nodeRetrieved=AVLTreeNode(response,false,thisFormat, sizeValue);
            AVLTreeNode expected=tree_node;
            expected.nodeID=id;
            expected.key[0]=db_t((int)i);
            ASSERT_EQ(expected.toStringFull(true), nodeRetrieved.toStringFull(true));
            nodeRetrieved.ptrRightChild[0]=newValue;
            oram->put(id,nodeRetrieved.serialize());
        }

        //verify updates were successful
        for(size_t i=0;i<max_size;i++){
            ulong  id =i+1;
            bytes response;
            oram->get(id,response);    
            AVLTreeNode nodeRetrieved=AVLTreeNode(response,false,thisFormat,sizeValue);
            AVLTreeNode expected=tree_node;
            expected.nodeID=id;
            expected.key[0]=db_t((int)i);
            expected.ptrRightChild[0]=newValue;
            ASSERT_EQ(expected.toStringFull(true), nodeRetrieved.toStringFull(true));
        }
    
}


TEST(ORAMTest, QueryAndUpdateRandom){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=DEBUG;

    size_t sizeValue=0;
    vector<AType> thisFormat {AType::INT};
    number BLOCK_SIZE=getNumBytesWhenSerialized(thisFormat,sizeValue);  
    if(BLOCK_SIZE<32 ){
        LOG(INFO, L"The Nodes stored in the AVL Tree must be at least 2 AES block sizes when serialized, so at least 32 bytes (equals rS=32).");
        BLOCK_SIZE=32;
    }
    if(BLOCK_SIZE%16!=0){
        BLOCK_SIZE=BLOCK_SIZE+(16-BLOCK_SIZE%16);
        LOG(INFO, boost::wformat(L"The size of the stored data has to be devisable by 16. We therefore pad the block size to: %d") %BLOCK_SIZE);
    }
    size_t numOperations=10000;
    number CAPACITY = numOperations;
    number Z = 3;		
    number LOG_CAPACITY = std::max((number) ceil(log(CAPACITY+1)/log(2))-1,2ull); //+1 for NULLNODE
    number BATCH_SIZE   = 1;
    shared_ptr<ORAM> oram = make_shared<PathORAM::ORAM>(
                        LOG_CAPACITY,
                        BLOCK_SIZE,
                        Z,
                        make_shared<InMemoryStorageAdapter>((1 << LOG_CAPACITY), BLOCK_SIZE, bytes(), Z),
                        make_shared<InMemoryPositionMapAdapter>(((1 << LOG_CAPACITY) * Z) + Z),
                        make_shared<InMemoryStashAdapter>(3 * LOG_CAPACITY * Z),
                        true,
                        BATCH_SIZE);

    LOG_PARAMETER(CAPACITY);
    LOG_PARAMETER(Z);
    LOG_PARAMETER(LOG_CAPACITY);
    LOG_PARAMETER(BATCH_SIZE);
    LOG_PARAMETER(BLOCK_SIZE);

    bytes value=vector<uchar>(sizeValue,0);

    AVLTreeNode tree_node= AVLTreeNode(vector<db_t>{db_t(5)},9,value,vector<ulong>{10},vector<ulong>{11},vector<ulong>{11},vector<int>{2},vector<int>{3}, thisFormat, 0);
    vector<ulong> existingNodes;
    int numRepetitions=1;
    for(int seed=0;seed<numRepetitions;seed++){
        LOG(INFO, boost::wformat(L"Seed: %d") %seed);

        //std::random_device dev;
        std::mt19937 rng(seed);//dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist6(0,numOperations);

        for(size_t i=0;i<numOperations;i++){
            //LOG(INFO, boost::wformat(L"- i: %d") %i);

            int mode=dist6(rng)%10;

            if(mode==0 or  i==0){
                //put Node
                ulong  id =i+1;
                AVLTreeNode this_node=AVLTreeNode(tree_node);
                this_node.nodeID=id;
                if(id%4!=0){
                    this_node.key[0]=db_t((int)i);
                }
                bytes b = this_node.serialize();   
                oram->put(id,b);
                existingNodes.push_back(id);
            }else{
                //get Node
                ulong id;
                if(mode%2==0){
                    id=existingNodes[0];
                }else{
                    int id_index=dist6(rng)%existingNodes.size();
                    id=existingNodes[id_index];
                }
                bytes response;
                oram->get(id,response); 
                ASSERT_NE(response.size(),0);   
                AVLTreeNode nodeRetrieved=AVLTreeNode(response,false,thisFormat,sizeValue);
                AVLTreeNode expected=AVLTreeNode(tree_node);;
                expected.nodeID=id;
                if(id%4!=0){
                    expected.key[0]=db_t((int)id-1);
                }
                ASSERT_EQ(expected.toStringFull(true), nodeRetrieved.toStringFull(true));   
            }
        }
    }
}

int main(int argc, char ** argv) {
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();

}