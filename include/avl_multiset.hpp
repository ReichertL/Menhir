#pragma once 

#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <string>
#include <cmath>
#include <sstream>
//#include <variant>

#include "definitions.h"
#include "database_type.hpp"
#include "avl_treenode.hpp"
#include "avl_loadtree.hpp"
#include "path-oram/definitions.h"
#include "path-oram/oram.hpp"
#include "path-oram/position-map-adapter.hpp"
#include "path-oram/stash-adapter.hpp"
#include "path-oram/storage-adapter.hpp"




namespace DOSM{

#define NULL_PTR (ulong)0

//#define _IF_THEN(S,A,B) (S*A+(not S)*B);

using namespace PathORAM;

class AVLTree {
    uint treeSize;
    vector<ulong> ptrRoot;
    uint maxCapacity;
    std::queue<ulong> availableBlockNumbers;

    vector<AType> columnFormat;    
    size_t sizeValue;
    ulong numColumns;
    bool USE_ORAM=true;


    //for padding and dummy operations
    bytes nullNodeBytes;

    //if USE_ORAM is true
    shared_ptr<PathORAM::ORAM> oram;  //make_shared ensures that the object is allocated on the heap
    number ORAM_BLOCK_SIZE; 
    number ORAM_LOG_CAPACITY=16ull;
    number ORAM_Z= 3uLL;
    number STASH_FACTOR=4ull;
    number BATCH_SIZE= 1ull;    				
    number LEN_PADDING=0ull;

    //ORAM operations
    ulong getNewORAMID();
    void deleteNodeORAM(ulong nodePtr); 
    
    //Inserstion and Rebalancing
    void rightRotate(AVLTreeNode *node,ulong nodePtr, AVLTreeNode *L, ulong column);
    void leftRotate(AVLTreeNode *node,ulong nodePtr, AVLTreeNode *R, ulong column); 
    void singleRotate(AVLTreeNode *node, AVLTreeNode *child, ulong column, bool left, bool rotate);

    /*tuple<AVLTreeNode ,ulong,uint,uint,uint> 
            insertHelper(db_t key, size_t nodeHash, uchar* data, ulong nodePtr);*/
    void insertHelper(vector<db_t> key, size_t nodeHash, bytes value, ulong nodeID);
    tuple<vector<AVLTreeNode>,vector<bool>, bool, ulong,ulong> insertHelper_findParent( int column, AVLTreeNode newNode);
    tuple<ulong,ulong,ulong, int>  insertHelper_updateParents(int column, AVLTreeNode newNode, vector<AVLTreeNode> nodes, vector<bool> lORr, bool toInsert);
    void insertHelper_updateNext(ulong nodeID, int column, ulong prePtr,ulong nextIDIfFirst);
    void insertHelper_rebalance(uint column, ulong balance_n, ulong balance_child, ulong balance_childschild, int balanceType); 
    tuple<AVLTreeNode ,ulong,uint>  balance(AVLTreeNode node, ulong nodePtr, ulong column);
    
    //Deletion
    void  deleteHelper(db_t key, size_t nodeHash, ulong column);
    tuple<ulong, vector<AVLTreeNode >, vector<ulong>,vector<bool>>  deleteHelper_findNode(db_t key, size_t nodeHash, ulong column);
    tuple<ulong,int,db_t> deleteHelper_findReplacement(ulong deletePtr,AVLTreeNode delNode, ulong column);
    ulong findReplacement_twoSubtrees(int pad, ulong deletePtr,AVLTreeNode delNode, ulong column, bool dummyExc);
    void deleteHelper_updateNodes(  ulong deletePtr, ulong repPtr, int repHeight, db_t repKey, 
                                     vector<AVLTreeNode > nodes, vector<ulong> ptrsNodes,vector<bool> lORr,ulong column); 

    //Find Functions
    tuple<vector<db_t>,bool> findNodeHelper(db_t key, size_t nodeHash, ulong column);

    vector<AVLTreeNode > findIntervalHelperOblix(db_t key,int i, int j,ulong column);
    vector<AVLTreeNode > findIntervalHelperOblix_volumePadded(db_t startKey,  int si,  db_t endKey,int ei,ulong estimate, ulong column);
    
    DBT::dbResponse  findIntervalHelperMenhir(db_t startKey, db_t endKey,  ulong column,number estimate);


    // Util
    int getPad();
    #ifdef NDEBUG
        shared_ptr<PathORAM::ORAM> getORAM();
        AVLTreeNode getNodeORAM(ulong nodeptr, bool dummy=false);
        void putNodeORAM( AVLTreeNode node, bool dummy=false);    
        vector<ulong> getRoots();
    #endif



public:
    AVLTree(vector<AType> columnFormat, size_t sizeValue, number capacity, bool USE_ORAM=true);
    AVLTree(vector<AType> columnFormat,  size_t sizeValue, number ORAM_LOG_CAPACITY,number ORAM_Z, number STASH_FACTOR, number BATCH_SIZE, vector<vector<db_t>> *inputData, size_t numDatapointsAtStart, bool USE_ORAM=true );
    AVLTree(vector<AType> cF, size_t vSize, number capacity, number ORAM_Z, number STASH_FACTOR, number BATCH_SIZE, bool USE_ORAM);

    ~AVLTree();

    number getORAMBLOCKSIZE();




    void putTreeInORAM(vector<AVLTreeNode > nodes, vector<ulong> thisRoot);
    
    size_t insert(vector<db_t> key, bytes value);
    size_t insert(vector<db_t> key);
    #ifndef NDEBUG
        void insert(vector<db_t> key, size_t valueHash);
    #endif

    
    //the whole node will be deleted
    void deleteEntry(db_t key, size_t nodeHash, ulong column);

    bool empty() const;
    size_t size();
    
    
    tuple<vector<db_t>,bool> findNode(db_t key, size_t nodeHash, ulong column);


    DBT::dbResponse findIntervalMenhir(db_t startKey, db_t endKey, ulong column, number estimate);
    
    

    #ifndef NDEBUG
        shared_ptr<PathORAM::ORAM> getORAM();
        AVLTreeNode getNodeORAM(ulong nodeptr, bool dummy=false);
        void putNodeORAM( AVLTreeNode node, bool dummy=false); 
        vector<ulong> getRoots();
    #endif

    string print( LOG_LEVEL level, ulong nodePtr,bool expressiv=false, ulong column=0,bool isLeft=false, const std::string& prefix="");
    string toString(bool expressiv=false, ulong column=0);


};

}