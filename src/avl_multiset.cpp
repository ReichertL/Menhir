#include "avl_multiset.hpp"
#include "utility.hpp"
#include "globals.hpp"
#include <stdexcept>
#include <sys/time.h>
#include <sys/resource.h>

namespace DOSM{

/**
 * @brief Construct a new AVLTree::AVLTree object but letting the ORAM library set important parameters.
 * 
 * @param cF 
 * @param capacity 
 * @param USE_ORAM 
 */
AVLTree::AVLTree(vector<AType> cF, size_t vSize, number capacity, bool USE_ORAM):AVLTree::AVLTree(
            cF, vSize, capacity, ORAM_Z=3ull, STASH_FACTOR=4ull, BATCH_SIZE=1ull, USE_ORAM){

    }



/**
 * @brief Construct a new AVLTree::AVLTree object. 
 * The tree is filled with data from input data.
 * 
 * @param cF : Column format
 * @param vSize :value size, so size of the value associated with each data tuple 
 * @param ORAM_LOG_CAPACITY : Size of the Path ORAM used for storing the AVL Tree.
 * @param ORAM_Z 
 * @param STASH_FACTOR 
 * @param BATCH_SIZE 
 * @param inputData : vector containing data tuples. 
 * @param numDatapointsAtStart : number of data points from inputData to be used for constructing the tree
 * @param USE_ORAM : For testing purposes. Wether or not data should be stored in an ORAM. 
 */
AVLTree::AVLTree(vector<AType> cF, size_t vSize,  number ORAM_LOG_CAPACITY,number ORAM_Z, number STASH_FACTOR, number BATCH_SIZE,vector<vector<db_t>> *inputData, size_t numDatapointsAtStart,  bool USE_ORAM){

    this->columnFormat=cF;
    this->sizeValue=vSize;

    this->numColumns=this->columnFormat.size();
    this->USE_ORAM=USE_ORAM;
    this->maxCapacity=pow(2,ORAM_LOG_CAPACITY); 
    LOG(INFO, boost::wformat(L"AVLTree.maxCapacity =%d ")%this->maxCapacity);

    this->ptrRoot=vector<ulong>(numColumns,NULL_PTR);
    this->ORAM_Z=ORAM_Z;
    this->STASH_FACTOR=STASH_FACTOR;
    this->ORAM_LOG_CAPACITY=ORAM_LOG_CAPACITY;
    this->BATCH_SIZE=BATCH_SIZE;
    this->ORAM_BLOCK_SIZE=getNumBytesWhenSerialized(this->columnFormat,this->sizeValue);    

    LOG_PARAMETER(ORAM_Z);
    LOG_PARAMETER(STASH_FACTOR);
    LOG_PARAMETER(this->ORAM_LOG_CAPACITY);
    LOG_PARAMETER(BATCH_SIZE);
    LOG_PARAMETER(ORAM_BLOCK_SIZE);
    
    size_t oramParameter= (1 << this->ORAM_LOG_CAPACITY) * this->ORAM_Z+this->ORAM_Z;
    LOG_PARAMETER(oramParameter);
    size_t stashSize=this->STASH_FACTOR * this->ORAM_LOG_CAPACITY * this->ORAM_Z;
    LOG(INFO,  boost::wformat(L"ORAM Stash Size is: %d") %(stashSize));


    //prepare Data as tree
    vector<pair<number, bytes>> data;
    vector<ulong> thisRoots;
    //numDatapointsAtStart has to be at least one less than total number of 2^ORAM_LOG_CAPACITY as NULL_NODE has to fit into the ORAM

    if(numDatapointsAtStart>0){
        tie(data,thisRoots)=createTreeStructureNonObliv(inputData, numDatapointsAtStart, this->sizeValue,this->columnFormat, this->ORAM_BLOCK_SIZE);
        availableBlockNumbers=queue<ulong>();
        this->ptrRoot=thisRoots;

    }
    for (size_t i = numDatapointsAtStart+1; i<this->maxCapacity;i++){
        availableBlockNumbers.push(i);
    }
    this->treeSize=numDatapointsAtStart;


    //Constructing an ORAM is very memory intensive for a short period of time. If multiple processes run in parallel, this can cause the OS to freeze. Therefore, this rudimentary locking mechanism was implemented.
    unsigned long int sec= time(NULL)%INT_MAX;
    int lockID=(int) sec;
    int ramEstimation=(this->ORAM_BLOCK_SIZE*this->treeSize* pow(10,-9))*(4.1+4.4+3.9+0.22); //in GB
    ramEstimation=max(ramEstimation, 1);
    ramEstimation=min(ramEstimation, 240);
    LOG(INFO, boost::wformat(L"Attempting to lock %ld GB of RAM. ")%ramEstimation);

    while(!isRAMAvailable(lockID,(-1)*ramEstimation)){
        int sleepTime= 60; //in seconds
        LOG(INFO, boost::wformat(L"Waiting for %d seconds until enough RAM is freed.")%sleepTime);
        sleep(sleepTime);
    }

    //creates in Memory Position, Storage and Stash adapters 
    LOG(INFO, boost::wformat(L"Creating ORAM"));
    
    this->oram = make_shared<PathORAM::ORAM>(
            this->ORAM_LOG_CAPACITY,
            this->ORAM_BLOCK_SIZE,
            this->ORAM_Z,
            make_shared<InMemoryStorageAdapter>(oramParameter, this->ORAM_BLOCK_SIZE, bytes(), this->ORAM_Z),
			make_shared<InMemoryPositionMapAdapter>(oramParameter),
			make_shared<InMemoryStashAdapter>(stashSize),
			true,
			this->BATCH_SIZE);
    LOG(INFO, boost::wformat(L"Loading Data into ORAM (%d datapoints) ") %data.size());

    if(numDatapointsAtStart>0){
        this->oram->load(data);
    }
    AVLTreeNode NULL_NODE= AVLTreeNode(this->columnFormat,this->sizeValue);
    this->nullNodeBytes= NULL_NODE.serialize();

    this->oram->put(NULL_PTR+1, this->nullNodeBytes);

    LOG(INFO, boost::wformat(L"Finished loading data into ORAM"));

    freeRAM(lockID);
    LOG(INFO, boost::wformat(L"Removed Lock for %ld GB of RAM.")%ramEstimation);
    
}



/**
 * @brief Construct a new AVLTree::AVLTree object using the parameters for ORAM_Z and BATCH_SIZE that are passed.
 * @param cF 
 * @param capacity 
 * @param ORAM_Z 
 * @param BATCH_SIZE 
 * @param USE_ORAM 
 */
AVLTree::AVLTree(vector<AType> cF, size_t vSize, number capacity, number ORAM_Z, number STASH_FACTOR, number BATCH_SIZE, bool USE_ORAM){

    this->columnFormat=cF;
    this->sizeValue=vSize;

    this->numColumns=this->columnFormat.size();
    this->USE_ORAM=USE_ORAM;
    this->maxCapacity=capacity+1; //+1 is for reserving space for NULL_NODE
    LOG(INFO, boost::wformat(L"AVLTree.maxCapacity =%d ")%this->maxCapacity);

    this->ptrRoot=vector<ulong>(numColumns,NULL_PTR);
    this->treeSize = 0;
    this->ORAM_Z=ORAM_Z;
    this->STASH_FACTOR=STASH_FACTOR;

    this->ORAM_LOG_CAPACITY=std::max((number) ceil(log((double)this->maxCapacity/(double)this->ORAM_Z)/log(2.0)),3ull);  
    this->BATCH_SIZE=BATCH_SIZE;

    this->ORAM_BLOCK_SIZE=getNumBytesWhenSerialized(this->columnFormat,this->sizeValue);    
    /*if(this->ORAM_BLOCK_SIZE<32 ){
        LOG(WARNING, L"The Nodes stored in the AVL Tree must be at least 2 AES block sizes when serialized, so at least 32 bytes (equals rS=32).");
        ORAM_BLOCK_SIZE=32;
    }
    if(this->ORAM_BLOCK_SIZE%16!=0){
        this->LEN_PADDING=(16-this->ORAM_BLOCK_SIZE%16);
        auto oldBlockSize=this->ORAM_BLOCK_SIZE;
        this->ORAM_BLOCK_SIZE=this->ORAM_BLOCK_SIZE+this->LEN_PADDING;
        LOG(INFO, boost::wformat(L"The size of the stored data has to be devisable by 16. The unpadded block size is %d. We therefore pad the block size to: %d") %oldBlockSize %ORAM_BLOCK_SIZE);
    }*/

    LOG_PARAMETER(capacity);
    LOG_PARAMETER(ORAM_Z);
    LOG_PARAMETER(STASH_FACTOR);
    LOG_PARAMETER(this->ORAM_LOG_CAPACITY);
    LOG_PARAMETER(BATCH_SIZE);
    LOG_PARAMETER(ORAM_BLOCK_SIZE);
    
    size_t oramParameter= (1 << this->ORAM_LOG_CAPACITY) * this->ORAM_Z+this->ORAM_Z;
    LOG_PARAMETER(oramParameter);
    size_t stashSize=4 * this->ORAM_LOG_CAPACITY * this->ORAM_Z;
    LOG(INFO,  boost::wformat(L"ORAM Stash Size is: %d") %(stashSize));


    AVLTreeNode NULL_NODE= AVLTreeNode(this->columnFormat,this->sizeValue);
    this->nullNodeBytes= NULL_NODE.serialize();

    //creates inMemory Position, Storage and Stash adapters 
    
    this->oram = make_shared<PathORAM::ORAM>(
            this->ORAM_LOG_CAPACITY,
            this->ORAM_BLOCK_SIZE,
            this->ORAM_Z,
            make_shared<InMemoryStorageAdapter>(oramParameter, this->ORAM_BLOCK_SIZE, bytes(), this->ORAM_Z),
			make_shared<InMemoryPositionMapAdapter>(oramParameter),
			make_shared<InMemoryStashAdapter>(stashSize),
			true,
			this->BATCH_SIZE);


    oram->put(NULL_PTR+1, this->nullNodeBytes);
    
    //Node pointer 0 is reserved
    for(size_t i=1;i<this->maxCapacity;i++){
        this->availableBlockNumbers.push(i);
    }
}




shared_ptr<PathORAM::ORAM> AVLTree::getORAM(){
    return this->oram;
}


number AVLTree::getORAMBLOCKSIZE(){
    return ORAM_BLOCK_SIZE;
}

vector<ulong>  AVLTree::getRoots(){
    return this->ptrRoot;
}

size_t  AVLTree::size(){
    return this->treeSize;
}

AVLTree::~AVLTree(){

}



/**
 * @brief Worst Case tree hight of an AVL tree with n nodes according to https://en.wikipedia.org/wiki/AVL_tree in section "Comparison to other structures".
 * n=  treesize
 * @return int 
 */
int AVLTree::getPad(){
    int n=this->treeSize;
    double phi=(1+sqrt(5))/2;
    double c=1/(log(phi)/log(2));
    double b=c/2*log(5)/log(2)-2;
    double d=1+1/(pow(phi,4)*sqrt(5));
    int result=ceil(c*log(n+d)/log(2)+b);
    //Plus one for root node
    if(this->treeSize==1){
        result=1;
    }
    return result;

    //return (int) ceil(1.44*log(treeSize)+1);
}


bool AVLTree::empty() const{
    return treeSize == 0;
}




#pragma region ORAM_FUNCTIONS


/**
 * @brief Get an AVLTreeNode from the ORAM based on the ID. If dummy is true, then the node with ID 0 is retrieved as dummy operation.
 * 
 * @param nodePtr 
 * @param dummy 
 * @return AVLTreeNode 
 */
AVLTreeNode AVLTree::getNodeORAM(ulong nodePtr, bool dummy){

    //is a dummy read operation if nodePtr=NULL_PTR OR if dummy is true
    dummy= (not (bool) nodePtr) or dummy;
    const ulong ptr=NULL_PTR*dummy+nodePtr*(not dummy);
    bytes response;
    oram->get(ptr+1, response);
    if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"Get Node with nodeID %d from ORAM.")%ptr);

    if(response.size()==0){
        LOG(ERROR, boost::wformat(L"Node with nodeID %d was not found at %d in ORAM.")%ptr %(ptr+1));
        exit(1);
    }
    AVLTreeNode node=AVLTreeNode(response, dummy, columnFormat, sizeValue); 
    return node;
    
}

/**
 * @brief Returns a number that can be used as ID for a new AVLTreeNode.
 * 
 * @return ulong 
 */
ulong AVLTree::getNewORAMID(){
    if(availableBlockNumbers.size()==0){
        LOG(CRITICAL, L"No new numbers can be given to new ORAM nodes. This might be because the ORAM is full.");
    }
    ulong nodePtr=availableBlockNumbers.front();
    availableBlockNumbers.pop();
    return nodePtr;
}

/**
 * @brief Writes an AVL Tree node into the block with the corresponding ID. If dummy is true, an empty block is written to Block with ID 0.
 * 
 * @param node 
 * @param dummy 
 */
void AVLTree::putNodeORAM( AVLTreeNode node, bool dummy){
    
    dummy=dummy or node.empty;

    const ulong ptr=NULL_PTR*dummy+node.nodeID*(not dummy);
    bytes nodeBytes=node.serialize();

    for(size_t i=0;i<nodeBytes.size();i++){
        uint8_t b= _IF_THEN((uint8_t) dummy,(uint8_t) nullNodeBytes[i],(uint8_t) nodeBytes[i]);
        nodeBytes [i]=(uchar) b;
    }

    if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"Putting block %d in ORAM")%ptr);
    oram->put(ptr+1, nodeBytes);
        
}

/**
 * @brief Overwrites the block at ID nodePtr in the ORAM with zeros.
 * 
 * @param nodePtr 
 */
void AVLTree::deleteNodeORAM(ulong nodePtr){
    
    //putting empty node in place of Node  
    oram->put(nodePtr+1, this->nullNodeBytes);
    availableBlockNumbers.push(nodePtr);

}




/**
 * @brief Places the nodes directly in the ORAM. Only the formats is checked but the tree structur is not verified.
 * Only to be used for testing or if a functional tree is restored. 
 * Remember that the last value of keys represents the time since start with an int.
 * 
 * @param nodes 
 * @param nodePtrs 
 * @param thisRoot 
 */
void AVLTree::putTreeInORAM(vector<AVLTreeNode > nodes, vector<ulong> thisRoot){
    LOG(INFO, L"Put Tree Structure in ORAM");

    if(thisRoot.size()!=this->numColumns){
        ostringstream oss;
        oss<<"there must be as many roots as there are columns. Was "<<thisRoot.size()<<" expected "<<this->numColumns;
        __throw_invalid_argument(oss.str().c_str());
    }

    unordered_map<ulong,ulong> nodeIDs;

    vector<pair<number, bytes>> data;
    for(size_t i=0;i<nodes.size();i++){
        AVLTreeNode n=nodes[i];
        if(n.ptrLeftChild.size()!=(size_t)this->numColumns or
        n.ptrRightChild.size()!=(size_t)this->numColumns or
        n.next.size()!=(size_t)this->numColumns or
        n.lHeight.size()!=(size_t)this->numColumns or
        n.rHeight.size()!=(size_t)this->numColumns or
        n.key.size()!=(size_t)this->numColumns){
            __throw_invalid_argument("Vectors stored in an AVLTreeNode must have a lenght equal to the number of columns (numColumns).");
        }
        if(n.numColumns!=this->numColumns){
            __throw_invalid_argument("NumColumns in Node needs to be the same as in the tree.");
        }
   
        bytes nodeBytes=n.padToBlockSize(ORAM_BLOCK_SIZE);
        data.push_back(make_pair(n.nodeID+1,nodeBytes));
        nodeIDs.insert({(ulong)n.nodeID, (ulong)n.nodeID});
    }
    
    for(size_t i=0;i<nodes.size();i++){
        vector<ulong> next=nodes[i].next;
        for(size_t j=0;j<next.size();j++){
            if(not(nodeIDs.count(next[j])==1 or next[j]==0)){
                __throw_invalid_argument("Next pointer does not point to an existing node.");
            }
        }
    }

    oram->load(data);
    //Reinsert 0, because it has been overwritten because of oram->load
    //AVLTreeNode NULL_NODE=AVLTreeNode(this->columnFormat);
    oram->put(NULL_PTR+1, this->nullNodeBytes);
    //putNodeORAM( NULL_NODE, false);

    availableBlockNumbers=queue<ulong>();
    for(size_t i=1;i<=maxCapacity;i++){
        if(nodeIDs.count((ulong)i)==0){
            availableBlockNumbers.push(i);
        }
    }


    ptrRoot=thisRoot;
    treeSize=nodes.size();
    if(CURRENT_LEVEL<=TRACE and treeSize<10000){
        for (size_t i = 0; i < numColumns; i++){
            if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat( L"Column %d ") %i);
            this->print(TRACE, ptrRoot[i],true,i);
        }
    }
}

#pragma endregion


#pragma region INSERTION_FUNCTIONS
//-----------------Insertion----------------------------------------------------------------------------------------

/**
 * @brief  Inserting a new node with a set of keys and a value into the AVL Tree.
 * 
 * @param key : A data point / data tuple (equivalent to a row in a database). These are indexed and can be used for filter operations during querying.
 * @param value : An additional byte array that can be stored with the keys. This is not indexed.
 * @return size_t : Hash corresponding to the AVL Tree Node. Can be used for deleting data.
 */
size_t AVLTree::insert(vector<db_t> key, bytes value){
    ulong nodeID=getNewORAMID();
    size_t nodeHash=getNodeHash(key, value,nodeID);

    if(CURRENT_LEVEL==DEBUG){
        ostringstream oss;
        for (size_t i=0; i<numColumns;i++){
            oss<<DBT::toString(key[i]);
            if(i<numColumns-1) oss<<",";
        }
        LOG( DEBUG,boost::wformat( L"Insert: [ %s ]- %d") % MENHIR::toWString(oss.str()) % nodeHash);
    }

    insertHelper(key, nodeHash,value,nodeID);


    if(CURRENT_LEVEL<=TRACE){
        for (size_t i = 0; i < numColumns; i++){
            this->print(TRACE, ptrRoot[i],true,i);
        }
    }
    return nodeHash;
}


/**
 * @brief Inserts a record consisting of the key vector in the oblivious AVL tree. 
 * Also a unique nodeHash is created which represents the passed key vector and time. 
 * This hash is returned and can later be used to delete this specific record.
 * 
 * @param key 
 * @return size_t 
 */
size_t AVLTree::insert(vector<db_t> key ){
        
    if(key.size()!=(size_t)(numColumns)){
        throw std::invalid_argument("The number of Keys passed is not equal to the number of columns.");
    }

    if(treeSize+1>maxCapacity){
        LOG(ERROR,L"No more new Elements can be inserted.");
        return 0;
    }

    bytes value= vector<uchar> (sizeValue, 0);
    return insert(key,value);

}

#ifndef NDEBUG

/**
 * @brief Inserts node into the (multi-)AVL tree based on the passed keys. Instead of a random node hash, the passed one is used.
 * 
 * @param key 
 * @param nodeHash 
 */
void AVLTree::insert(vector<db_t> key, size_t nodeHash){

    if(key.size()!=(size_t)(numColumns)){
            throw std::invalid_argument("The number of Keys passed is not equal to the number of columns.");
        }

    if(treeSize+1>maxCapacity){
        LOG(ERROR,L"No more new Elements can be inserted.");
        return;
    }

    ulong nodeID=getNewORAMID();

    if(CURRENT_LEVEL==DEBUG){
        std::ostringstream oss;
        for (size_t i = 0; i < numColumns; i++){
            oss<<DBT::toString(key[i])<<",";
        }
        LOG( DEBUG,boost::wformat( L"DOSM insert Node: ID  %d, Key %s, Hash %d") %nodeID % MENHIR::toWString(oss.str()) % nodeHash);
    }

    bytes value =vector<uchar> (sizeValue, 0);
    insertHelper(key, nodeHash,value,nodeID);
    if(CURRENT_LEVEL<=TRACE)
        for (size_t i = 0; i < numColumns; i++){
           this->print(TRACE,ptrRoot[i],true,i);
        }
}

#endif





/**
 * @brief Subfunction for inserting new nodes. It finds the correct leaf location where the new node needs to be placed and returns the required path information.
 * 
 * @param column 
 * @param newNode 
 * @return tuple<vector<AVLTreeNode>,vector<ulong>, vector<bool>,vector<bool>, bool> 
 */
tuple<vector<AVLTreeNode>,vector<bool>, bool, ulong,ulong> AVLTree::insertHelper_findParent( int column,AVLTreeNode newNode){
    int pad=getPad();
    vector<AVLTreeNode> nodes;
    nodes.reserve(pad);
    vector<bool> lORr;
    lORr.reserve(pad);
    bool toInsert=true;



    ulong prePtr=NULL_PTR;
    ulong nextIDIfFirst=NULL_PTR; //the index at which the first non-dummy node appears

    AVLTreeNode curNode(columnFormat, sizeValue);
    ulong ptrCur=ptrRoot[column];

    if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"column %d - Find location for Insertion") % column);
   
    for(size_t i=0; i<(size_t) pad;i++){
        curNode=getNodeORAM(ptrCur);
        nodes.push_back(curNode);
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"column %d - curNode: %s") %column %MENHIR::toWString(curNode.toString(true,column)));    


        bool diffKey= (bool)(newNode.key[column]!=curNode.key[column]);
        bool diffHash= (bool)(newNode.nodeHash-curNode.nodeHash);
        toInsert=(diffKey or diffHash);
        bool takeLeft=(newNode.key[column]<curNode.key[column] or (newNode.key[column]==curNode.key[column] and newNode.nodeHash<curNode.nodeHash));
        lORr.push_back(takeLeft);

        //for determining how to update successor pointers later

        bool isPrior=(not takeLeft) and (ptrCur!=NULL_PTR);
        prePtr=_IF_THEN(isPrior,ptrCur,prePtr);
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"isPrior %d, prePtr: %d") %isPrior %prePtr);    
        
        ulong tmpPtr=ptrCur;
        ptrCur=_IF_THEN(takeLeft, curNode.ptrLeftChild[column],  curNode.ptrRightChild[column] );

        //determining how to update successorpointer in case this is new smallest node
        bool foundLeaf= (ptrCur==NULL_PTR) and (tmpPtr !=NULL_PTR); 
        nextIDIfFirst=_IF_THEN(foundLeaf,tmpPtr,nextIDIfFirst); //if the fist dummy pointer is found, then tmpPtr hold the pointer to a leaf node
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"foundLeaf %d, nextIDIfFirst: %d") %foundLeaf %nextIDIfFirst);    


    }

    return make_tuple(nodes,lORr, toInsert,prePtr,nextIDIfFirst);
}

/**
 * @brief This is a subfunction for inserting a new node. This function takes a the appropriate leaf location of a new node and the relevant path information.
 *         It then updates all parents to reflect new height and size (so how many elements with the same key are in this subtree). It also conducts repalancing procedures. 
 * 
 * @param column 
 * @param newNode 
 * @param nodes 
 * @param lORr 
 * @param toInsert 
 */
tuple<ulong,ulong,ulong, int> AVLTree::insertHelper_updateParents(int column, AVLTreeNode newNode, vector<AVLTreeNode> nodes, vector<bool> lORr, bool toInsert){

    ulong ptr=newNode.nodeID;
    AVLTreeNode curNode(columnFormat, sizeValue);
    ulong ptrCur;

    if(CURRENT_LEVEL<=TRACE)
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"\ncolumn %d - Update parents after insertion.") % column);

    int height=0; //the loop starts at the (dummy) leaves 
    bool foundParent=false; //is true if the parent has been found. Signifies that the current node is not a dummy. 
    int update;
    int balanceType=0;
    ulong balance_n=NULL_PTR;
    ulong balance_child=newNode.nodeID; //in case the new node is part or a rebalancing procedure, however it is not yet in the list of nodes so we set it as default
    ulong balance_childschild=newNode.nodeID;
    bool balanceNodeFound=false;

    for(int i=(int) nodes.size()-1;i>=0;i--){
        curNode=nodes[i];
        ptrCur=curNode.nodeID;
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"\ncurNode: %s") %MENHIR::toWString(curNode.toString(true,column)));    

        bool possibleParent=(ptrCur!=NULL_PTR); //Node is not a dummy
        bool isParent=possibleParent and not foundParent and toInsert; //node is the direct parent where the new node needs to be inserted
        foundParent=isParent or foundParent;
        if(CURRENT_LEVEL<=TRACE){
            LOG(TRACE, boost::wformat(L"possibleParent: %d, toInsert:%d, foundParent: %d, isParent: %d") %possibleParent %toInsert %foundParent %isParent);    
            LOG(TRACE, boost::wformat(L"column %d - curNode: %s") %column %MENHIR::toWString(curNode.toString(true,column)));    
        }

        bool isLeft=lORr[i];//is true is the path from the current node diverges to the left

        //updating the leftchild pointer / rightchild pointer the direct parent of the new node
        update= (int) (isParent and isLeft);
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"update leftChild:%d")%update);    
        curNode.ptrLeftChild[column]=update*ptr+(not update)*curNode.ptrLeftChild[column];
        update= (int) (isParent and (not isLeft));
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"update rightChild:%d")%update);    
        curNode.ptrRightChild[column]=update*ptr+ (not update)*curNode.ptrRightChild[column];


        height=_IF_THEN(foundParent,height+1,height);

        //only collect data for balancing if not the last node in array (i.e. potential leaf node if tree was fully used).
        // This if condition is data oblivious
        if (i<(int) nodes.size()-1){


            //we assume here ,that the rebalancing already happend. So the height of the node which was rebalances is originalHeight+1
            //the height of nodes that are lower than the node for rebalancing are either already correct or will be corrected during rebalancing
            update= (int) (isLeft and foundParent);
            curNode.lHeight[column]=update*height+ (not update)*curNode.lHeight[column];
            if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"update lheight:%d height:%d")%update %height);    
            update=(int) ((not isLeft)and foundParent);
            curNode.rHeight[column]=update*height+(not update)*curNode.rHeight[column]; 
            if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"update rheight:%d height: %d")%update %height);    


            AVLTreeNode child=nodes[i+1];    

            if(CURRENT_LEVEL==TRACE){
                LOG(TRACE, boost::wformat(L"curNode: %s") %MENHIR::toWString(curNode.toString(true,column)));    
                LOG(TRACE, boost::wformat(L"child: %s")  %MENHIR::toWString(child.toString(true,column)));    
            }


            //get balance type
            bool balancingRequired= (curNode.balanceFactor()==2 or curNode.balanceFactor()==-2);// it true if the current node requiers some kind of rotation.
            bool update_balaceData= (balancingRequired and not balanceNodeFound); //is true if the current node requires balancing and is the first to do so
            balanceNodeFound= _IF_THEN(update_balaceData, true, balanceNodeFound);

            /*  will be -3 for left rotation,
                +3 for right rotation
                -2 or -1 for right left rotation
                +2 or +1 for left right rotation
                0 should never occure for b if an rotation is required
            */
            int b= child.balanceFactor()+curNode.balanceFactor(); 
            balanceType= _IF_THEN(update_balaceData, b, balanceType);
            bool update_n=(update_balaceData and curNode.nodeID !=NULL_PTR); //only update nodes if they are not dummy nodes
            balance_n=_IF_THEN(update_n, curNode.nodeID, balance_n);
            bool update_child=(update_balaceData and child.nodeID!=NULL_PTR);
            balance_child=_IF_THEN(update_child, child.nodeID, balance_child);

            if(CURRENT_LEVEL==TRACE){
                LOG(TRACE, boost::wformat(L" balancingRequried %d,b %d , update_balaceData %d, balanceNodeFound %d , balanceType %d")  %balancingRequired %b %update_balaceData %balanceNodeFound %balanceType);    
                LOG(TRACE, boost::wformat(L"update_n %d balance_n %d")%update_n %balance_n  );                
                LOG(TRACE, boost::wformat(L"update_child %d balance_child %d") %update_child %balance_child );   
            }
            if(i<(int) nodes.size()-2){
                // potential tree hight (including the dummies) allows for double rotations
                AVLTreeNode childschild=nodes[i+2]; 
                bool update_childchild= (update_balaceData and childschild.nodeID != NULL_PTR);
                balance_childschild=_IF_THEN(update_childchild, childschild.nodeID, balance_childschild);

                if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"update_childchild %d, balance_childChild %d") %update_childchild %balance_childschild);    
            }

            //so that all nodes above the balance node have the correct height for after balancing
            //balancing decreases the tree height for this subtree by one. 
            // We therefore remove the height added earlier and decrease the height for this node by 1.
            /*int height_n=_IF_THEN(update_balaceData,height-1,height);
            update= (int) (isLeft and foundParent and update_balaceData);
            curNode.lHeight[column]=update*height_n+ (not update)*curNode.lHeight[column];
            if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"update lheight:%d")%update);    
            update=(int) ((not isLeft)and foundParent and update_balaceData);
            curNode.rHeight[column]=update*height_n+(not update)*curNode.rHeight[column]; 
            if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"update rheight:%d")%update);*/  
            height=_IF_THEN(update_balaceData,(height-1),height);


            //if the parent of a balanceNode is found the pointer to one of its children needs to be updated as it changes during rotation.
            bool updateRChild_balancing=(curNode.ptrRightChild[column]==balance_n);
            bool updateLChild_balancing=(curNode.ptrLeftChild[column]==balance_n);

            //update in case of single rotation
            bool singleRotation=(balanceType!=0);
            update=(singleRotation and updateRChild_balancing);
            curNode.ptrRightChild[column]=_IF_THEN(update,balance_child,curNode.ptrRightChild[column]);
            update=(singleRotation and updateLChild_balancing);
            curNode.ptrLeftChild[column]=_IF_THEN(update,balance_child,curNode.ptrLeftChild[column]);
            //update in case of double rotation
            bool doubleRotation=(balanceType==1 or balanceType==2 or balanceType==-1 or balanceType==-2);
            update=(doubleRotation and updateRChild_balancing);
            curNode.ptrRightChild[column]=_IF_THEN(update,balance_childschild,curNode.ptrRightChild[column]);
            update=(doubleRotation and updateLChild_balancing);
            curNode.ptrLeftChild[column]=_IF_THEN(update,balance_childschild,curNode.ptrLeftChild[column]);

        }

        if(CURRENT_LEVEL<=TRACE)
            if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"column %d - curNode(after update %d): %s") %column %update %MENHIR::toWString(curNode.toString(true,column)));    
        putNodeORAM(curNode); //writing nodes back to ORAM
        nodes[i]=curNode;

    }
    return make_tuple(balance_n,balance_child,balance_childschild, balanceType );
}



/**
 * @brief This function updates the pointer to the next node of the new node and the node that needs to point at the new node.
 * 
 * @param nodeID 
 * @param column 
 * @param nodes 
 * @param lORr 
 */
void AVLTree::insertHelper_updateNext(ulong nodeID, int column, ulong prePtr, ulong nextIDIfFirst){


    if(CURRENT_LEVEL==DEBUG) LOG(DEBUG, L"Updating the pointers to the next node for the new node and its prior.");    


    AVLTreeNode newNode=getNodeORAM(nodeID);

    // case1: new node is smallest possible node.
    bool isFirst=(prePtr == NULL_PTR);
    if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"isFirst %d, nextIDIfFirst:%d") %isFirst %nextIDIfFirst);    
    newNode.next[column]=_IF_THEN(isFirst,nextIDIfFirst, newNode.next[column]);
    if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"newNode (update %d): %s") %isFirst %MENHIR::toWString(newNode.toString(true,column)));    
  
    //all other cases
    AVLTreeNode prior=getNodeORAM(prePtr); 
    ulong nextID=prior.next[column];
    prior.next[column]=nodeID;
    putNodeORAM(prior,isFirst); //if isFirst is true, then the prior node is a dummy

    newNode.next[column]=_IF_THEN((not isFirst),nextID, newNode.next[column]);
    putNodeORAM(newNode);
    if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"newNode (update %d): %s") %(not isFirst) %MENHIR::toWString(newNode.toString(true,column)));    

}


/**
 * @brief 
 * 
 * @param balance_n 
 * @param balance_child 
 * @param balance_childschild 
 * @param balanceType 
 */
void AVLTree::insertHelper_rebalance(uint column, ulong balance_n, ulong balance_child, ulong balance_childschild, int balanceType){
    //rebalancing
    AVLTreeNode  n= getNodeORAM(balance_n);
    AVLTreeNode child=getNodeORAM(balance_child);
    AVLTreeNode childChild=getNodeORAM(balance_childschild);
    
    if(CURRENT_LEVEL==DEBUG) LOG(DEBUG, boost::wformat(L"balanceType %d on \nNode %s,\n     Child %s,}\n    childChild %s") 
            %balanceType
            %MENHIR::toWString(n.toString(true,column))
            %MENHIR::toWString(child.toString(true,column))
            %MENHIR::toWString(childChild.toString(true,column))
            );    

    /*  will be -3 for left rotation,
        +3 for right rotation
        -2 or -1 for right left rotation
        +2 or +1 for left right rotation
        0 means that no balancing is required
    */
    // single rotation
    bool singleRotation=(balanceType==-3 or balanceType==3);
    bool leftRotate=(balanceType==-3);
    singleRotate(&n, &child, column,leftRotate,singleRotation);
    if(CURRENT_LEVEL==DEBUG){
        LOG(DEBUG, boost::wformat(L"single Rotate %d, left Rotate %d") %singleRotation %leftRotate); 
        LOG(DEBUG, boost::wformat(L"After Single Rotation\nNode %s,\n     Child %s,}\n    childChild %s") 
            %MENHIR::toWString(n.toString(true,column))
            %MENHIR::toWString(child.toString(true,column))
            %MENHIR::toWString(childChild.toString(true,column))
            );    
    }
    //double rotation (seperatly)
    bool doubleRotation=(balanceType==2 or balanceType==1 or balanceType==-1 or balanceType==-2);
    bool leftRightRotate=(balanceType==2 or balanceType==1);
    singleRotate(&child, &childChild,column,leftRightRotate,doubleRotation); //first rotation of double rotation
    if(CURRENT_LEVEL==DEBUG) LOG(DEBUG, boost::wformat(L" After Double Rotation part 1\nNode %s,\n     Child %s,}\n    childChild %s") 
            %MENHIR::toWString(n.toString(true,column))
            %MENHIR::toWString(child.toString(true,column))
            %MENHIR::toWString(childChild.toString(true,column))
            );   
    singleRotate(&n, &childChild,column,(not leftRightRotate),doubleRotation); //second rotation of double rotation
    if(CURRENT_LEVEL==DEBUG){
        LOG(DEBUG, boost::wformat(L"Double Rotate %d, leftRightRotate %d") %doubleRotation %leftRightRotate); 
        LOG(DEBUG, boost::wformat(L" After Double Rotation \nNode %s,\n     Child %s,}\n    childChild %s") 
            %MENHIR::toWString(n.toString(true,column))
            %MENHIR::toWString(child.toString(true,column))
            %MENHIR::toWString(childChild.toString(true,column))
            );
    }
    putNodeORAM(n);
    putNodeORAM(child);
    putNodeORAM(childChild, (not doubleRotation));

    //update root if rebalancing node n was the root itself
    bool balanceNodeWasRoot=(n.nodeID==ptrRoot[column]);

    bool update= singleRotation and balanceNodeWasRoot;
    ptrRoot[column]=_IF_THEN( update, child.nodeID, ptrRoot[column]);
    update= doubleRotation and  balanceNodeWasRoot;
    ptrRoot[column]=_IF_THEN( update, childChild.nodeID, ptrRoot[column]);

    if(CURRENT_LEVEL==DEBUG) LOG(DEBUG, boost::wformat(L"balanceNodeWasRoot %d, new/current Root %d") %balanceNodeWasRoot %ptrRoot[column]); 
}

/**
 * @brief This is a helper function for creating and then inserting a new node based on the passed keys into the tree.
 * 
 * @param key 
 * @param nodeHash 
 * @param nodeID 
 */
void AVLTree::insertHelper(vector<db_t> key, size_t nodeHash, bytes value, ulong nodeID){
    
    if(treeSize==0){
        AVLTreeNode newNode=AVLTreeNode(key, nodeHash, value,columnFormat,nodeID );
        treeSize=treeSize+1;
        putNodeORAM(newNode); //BlockID in ORAM
        
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, L"\nEmpty Tree->NewNode: "+MENHIR::toWString(newNode.toStringFull(true)));    
        ptrRoot=vector<ulong>(numColumns,newNode.nodeID);
        return;
    }

    AVLTreeNode newNode=AVLTreeNode(key, nodeHash,value, columnFormat,nodeID );
    treeSize=treeSize+1;
    putNodeORAM(newNode); 
    if(CURRENT_LEVEL==TRACE) LOG(TRACE, L"\nNewNode: "+MENHIR::toWString(newNode.toStringFull(true)));    
    for (size_t j = 0; j < numColumns; j++){
        auto[nodes,lORr,toInsert, prePtr,nextIDIfFirst]=insertHelper_findParent(j, newNode);
        auto[n,child,childchild,balancetype]=insertHelper_updateParents(j,newNode, nodes,lORr,toInsert); //
        insertHelper_updateNext(nodeID, j, prePtr,nextIDIfFirst); //1 ORAM READ, 1 ORAM Write 
        insertHelper_rebalance(j,n,child,childchild,balancetype);

    }
}

/**
 * @brief Oblivious Balance operation on AVL tree. Returns the node which takes the place of the node on which the balance operation was called. 
 * Updates the nodes after balancing in the ORAM.
 * 
 * @param node 
 * @param nodePtr 
 * @param column 
 * @return tuple<AVLTreeNode ,ulong,uint> 
 */
tuple<AVLTreeNode ,ulong,uint> AVLTree::balance(AVLTreeNode node, ulong nodePtr, ulong column){
    if(CURRENT_LEVEL<=TRACE)
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"start balance on %d----------------------------------------------------------------------")%nodePtr);
    if(treeSize<=2){
        //no balance needed if there are only two nodes in the tree
        tuple<AVLTreeNode ,ulong,uint>  result= make_tuple(node,nodePtr,node.height(column));
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"end balance (No balance needed)---- ptr:%d height: %d--------") % nodePtr  % node.height(column));
        putNodeORAM(node);
        return result;
    }
    if(CURRENT_LEVEL==TRACE) LOG(TRACE, L"BalanceNode: "+MENHIR::toWString(node.toString(true,column)));

    ulong l_ptr=node.ptrLeftChild[column];
    ulong r_ptr=node.ptrRightChild[column];
    AVLTreeNode L=getNodeORAM(node.ptrLeftChild[column], false);
    AVLTreeNode R=getNodeORAM(node.ptrRightChild[column], false);

    int n_balanceFactor= node.balanceFactor(column);
    int l_balanceFactor= L.balanceFactor(column);
    int r_balanceFactor= R.balanceFactor(column);
    //cout<<"Balance factors of Node n("<<node.key<<" ,"<<node.nodeHash<<"): "<<n_balanceFactor <<" l: "<<l_balanceFactor<<" r:"<<r_balanceFactor<<endl;

    if(n_balanceFactor>= 2 and l_balanceFactor>=1){
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, L"Right Rotate");
        AVLTreeNode LR=getNodeORAM(NULL_PTR, true);
        //right rotate
        rightRotate(&node, nodePtr,&L, column);
        putNodeORAM(node);
        putNodeORAM(L);
        putNodeORAM(node,true);
        
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"end balance ---- ptr:%d height: %d--------") % nodePtr % node.height(column));
        return make_tuple(L, l_ptr,L.height(column));

    }else if(n_balanceFactor>=2){

        if(CURRENT_LEVEL==TRACE) LOG(TRACE, L"Left Right Rotate");
    
        ulong lr_ptr=L.ptrRightChild[column];
        AVLTreeNode LR=getNodeORAM(L.ptrRightChild[column], false);

        //left rotate in left subtree
        leftRotate(&L,l_ptr,&LR,column);

        //change node : leftsubtree
        node.ptrLeftChild[column]=lr_ptr;
        node.lHeight[column]=LR.height();

        //right rotate around node
        rightRotate(&node,nodePtr,&LR,column);

        putNodeORAM(node);
        putNodeORAM(L);
        putNodeORAM(LR);

        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"end balance ---- ptr:%d  height: %d--------") % nodePtr  % node.height(column));
        return make_tuple(LR, lr_ptr,LR.height(column));
        
    }else if(n_balanceFactor<=-2 and r_balanceFactor<=-1){
        if(CURRENT_LEVEL==TRACE) LOG(TRACE,L"Left Rotate");

        //dummy reads
        AVLTreeNode dummy=getNodeORAM(NULL_PTR, true);

        //left rotate
        leftRotate(&node, nodePtr,&R,column);
        
        putNodeORAM(node);
        putNodeORAM(R);
        putNodeORAM(dummy,true);
        
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"end balance ---- ptr:%d  height: %d--------") % nodePtr  % node.height(column));
        return make_tuple(R, r_ptr, R.height(column));

        
    }else if(n_balanceFactor<=-2){
        if(CURRENT_LEVEL==TRACE) LOG(TRACE,L"Right Left Rotate");
        ulong rl_ptr=R.ptrLeftChild[column];
        AVLTreeNode RL=getNodeORAM(R.ptrLeftChild[column], false);

        //left rotate in left subtree
        rightRotate(&R,r_ptr,&RL,column);

        //change node : leftsubtree
        node.ptrRightChild[column]=rl_ptr;
        node.rHeight[column]=RL.height();

        //right rotate around node
        leftRotate(&node,nodePtr,&RL,column);
        putNodeORAM(node);
        putNodeORAM(R);
        putNodeORAM(RL);

        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"end balance ---- ptr:%d  height: %d--------") % nodePtr  % node.height(column));
        return make_tuple(RL, rl_ptr, RL.height(column));        
    }else{
        if(CURRENT_LEVEL<=TRACE)
            if(CURRENT_LEVEL==TRACE) LOG(TRACE,L"Dummy Rotate");

        ulong rl_ptr=R.ptrLeftChild[column];
        AVLTreeNode RL=getNodeORAM(R.ptrLeftChild[column], true);

        //left rotate in left subtree
        rightRotate(&R,r_ptr,&RL,column);

        //change node : leftsubtree
        L.ptrRightChild[column]=rl_ptr;
        L.rHeight[column]=RL.height(column);

        //right rotate around node
        leftRotate(&L,l_ptr,&RL,column);
        putNodeORAM(L,true);
        putNodeORAM(node);
        putNodeORAM(RL,true);

        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"end balance ---- ptr:%d  height: %d--------") % nodePtr % node.height(column));
        return make_tuple(node,nodePtr,node.height(column));
    }

    //this->print(ptrRoot);
    
}



/**
 * @brief Oblivious right  or left rotate
 * 
 * @param node 
 * @param child
 * @param column 
 * @param rotationType
 */
void AVLTree::singleRotate(AVLTreeNode *node, AVLTreeNode *child, ulong column, bool left, bool rotate){

    bool leftRotate=left and rotate;
    node->ptrRightChild[column]=_IF_THEN(leftRotate, child->ptrLeftChild[column], node->ptrRightChild[column]);
    child->ptrLeftChild[column]=_IF_THEN(leftRotate, node->nodeID, child->ptrLeftChild[column]);
    node->rHeight[column]=_IF_THEN(leftRotate, child->lHeight[column],node->rHeight[column]);
    child->lHeight[column]=_IF_THEN(leftRotate, node->height(column),child->lHeight[column] );
    
    bool rightRotate=(not left) and rotate;
    node->ptrLeftChild[column]=_IF_THEN(rightRotate, child->ptrRightChild[column], node->ptrLeftChild[column]);
    child->ptrRightChild[column]=_IF_THEN( rightRotate, node->nodeID, child->ptrRightChild[column]);
    node->lHeight[column]=_IF_THEN(rightRotate,child->rHeight[column],node->lHeight[column]) ;
    child->rHeight[column]=_IF_THEN(rightRotate, node->height(column), child->rHeight[column]);

}


void AVLTree::rightRotate(AVLTreeNode *node,ulong nodePtr, AVLTreeNode *L, ulong column){

    node->ptrLeftChild[column]=L->ptrRightChild[column];
    L->ptrRightChild[column]=nodePtr;
    node->lHeight[column]=L->rHeight[column];
    L->rHeight[column]=node->height(column);
}


void AVLTree::leftRotate(AVLTreeNode *node,ulong nodePtr, AVLTreeNode *R, ulong column){

    node->ptrRightChild[column]=R->ptrLeftChild[column];
    R->ptrLeftChild[column]=nodePtr;
    node->rHeight[column]=R->lHeight[column];
    R->lHeight[column]=node->height(column);
        
}
 
#pragma endregion


#pragma  region DELETION_FUNCTIONS
//---------------DELETION--------------------------------------------------------------

/**
 * @brief Delete an entry (so all data associated with a AVL Tree Node) in the AVL Tree based on its nodeHash and and one key. 
 * TODO: Drop key and column from this call. 
 * 
 * @param key :key for node that is to be deleted.
 * @param nodeHash : hash for node to be deleted.
 * @param column : Column in which key is stored.
 */
void AVLTree::deleteEntry(db_t key, size_t nodeHash, ulong column){
    LOG( DEBUG,boost::wformat( L"DOSM delete: [ %d ]- %d") % DBT::toWString(key) % nodeHash);
    deleteHelper(key, nodeHash,column);
    
    if(CURRENT_LEVEL<=TRACE){
        for (size_t i = 0; i < numColumns; i++){
            this->print(TRACE,ptrRoot[i],true,i);
        }
    }
}

/**
 * @brief Subroutine for deleting a node from the AVL Tree.
 * 
 * @param key 
 * @param nodeHash 
 * @param column 
 */
void AVLTree::deleteHelper(db_t key, size_t nodeHash, ulong column){

    //find the correct node based on the passed column and update this column
    auto[deletePtr,nodes,ptrsNodes,lORr]=deleteHelper_findNode(key,nodeHash, column);
    AVLTreeNode delNode=getNodeORAM(deletePtr);
    auto[repPtr,repHeight,repKey]=deleteHelper_findReplacement(deletePtr,delNode, column);
    deleteHelper_updateNodes(deletePtr,repPtr, repHeight,  repKey, nodes,ptrsNodes, lORr, column);

    //update all remaining columns
    for (size_t i = 0; i < numColumns; i++){
        if(i==column){
            continue;
        }
        auto[repPtr,repHeight,repKey]=deleteHelper_findReplacement(deletePtr,delNode,i);
        auto[deletePtr,nodes,ptrsNodes,lORr]=deleteHelper_findNode(delNode.key[i],delNode.nodeHash, column);
        deleteHelper_updateNodes(deletePtr,repPtr, repHeight,  repKey, nodes,ptrsNodes, lORr,  i);
    }
    deleteNodeORAM(deletePtr); //makes the nodeID available again
    treeSize=treeSize-1;

}


/**
 * @brief Subroutine for deleting AVLTreeNode from AVLTree. Searches for node that is to be deleted.
 * 
 * @param key : key of node to be deleted
 * @param nodeHash : hash of node to be deleted
 * @param column : column for which the key is relevant. 
 * @return tuple<ulong,vector<AVLTreeNode >,vector<ulong>,vector<bool>> : returns ID of node to be deleted, a vector  nodes of AVLTreeNodes from root to leaf that contains the node to be deleted, the vector ptrsNodes containing alls IDs of all nodes in nodes, the vector lORr which contains information wether the path from root to leaf to a left or a right child.
 */
tuple<ulong,vector<AVLTreeNode >,vector<ulong>,vector<bool>>
AVLTree::deleteHelper_findNode(db_t key, size_t nodeHash, ulong column){
    int pad=getPad();
    //cout<<"pad "<< pad<<endl;

    ulong deletePtr=NULL_PTR;
    vector<AVLTreeNode > nodes;
    vector<ulong> ptrsNodes;
    vector<bool> lORr;
    if(pad>0){
        lORr.reserve(pad);
        ptrsNodes.reserve(pad);
        nodes.reserve(pad);

    }

    ulong ptrCur=ptrRoot[column];
    AVLTreeNode curNode(columnFormat, sizeValue);
    for(int i=1; i<=pad;i++){

        curNode=getNodeORAM(ptrCur);
        nodes.push_back(curNode);
        ptrsNodes.push_back(ptrCur);
                
        
        bool sameKey= (bool)(key==curNode.key[column]);
        bool sameHash= (bool)(nodeHash==curNode.nodeHash);
        int toDelete=(int) (sameKey and sameHash);
        deletePtr=toDelete*ptrCur+(not toDelete)*deletePtr;
        //cout<<"toDelete:"<<toDelete<<" delPtr:"<<deletePtr<<endl;
        
        if(key<curNode.key[column] or (key==curNode.key[column] and nodeHash<curNode.nodeHash)){
            //take left branch
            ptrCur=curNode.ptrLeftChild[column];
            lORr.push_back(true);

        }else{
            //take right branch
            ptrCur=curNode.ptrRightChild[column];
            lORr.push_back(false);
        }
    }
    return make_tuple(deletePtr,nodes,ptrsNodes,lORr);
}

/**
 * @brief Subroutine for deleting AVLTreeNode from AVLTree. Searches for replacement of node that is to be deleted.
 * This function is called to cover the case where the node has only one subtree.
 * @param deletePtr : pointer of node to be deleted
 * @param delNode : Node to be deleted
 * @param column : column for which the (multi-)AVLTree is currently updated
 * @return tuple<ulong,int,db_t> : returns tuple consisting of ID, height and key of the replacement node.
 */
tuple<ulong,int,db_t>
AVLTree::deleteHelper_findReplacement(ulong deletePtr,AVLTreeNode delNode, ulong column){
    //cout<<"\nFIND Replacement"<<endl;
    int pad=getPad();
    ulong repPtr;
    int height;
    db_t repKey;
    AVLTreeNode repNode(columnFormat, sizeValue);
    bool rCexists= (delNode.ptrRightChild[column]!=NULL_PTR);
    bool lCexists= (delNode.ptrLeftChild[column]!=NULL_PTR);
    bool twoC=(rCexists and lCexists);
    bool noC =(not rCexists)and (not lCexists);
    
    bool dummyExc= not(rCexists and lCexists);
    repPtr=findReplacement_twoSubtrees(pad,deletePtr,delNode,column,dummyExc );

    ulong temp_repPtr=NULL_PTR;
    temp_repPtr=        _IF_THEN((lCexists and not rCexists),delNode.ptrLeftChild[column],temp_repPtr);
    temp_repPtr=        _IF_THEN((rCexists and not lCexists),delNode.ptrRightChild[column], temp_repPtr);
    temp_repPtr=        _IF_THEN(twoC,repPtr, temp_repPtr);
    repPtr=temp_repPtr;
    AVLTreeNode N= getNodeORAM(repPtr);
    height=_IF_THEN(noC,0,N.height(column)); 
    repKey=_IF_THEN(noC,delNode.key[column], N.key[column]); 
    //cout<<"deleteHelper_findReplacement: "<<repPtr<<" "<<"height "<<height<< " size "<<size<<" repKey "<<DBT::toString(repKey)<<endl;
    //cout<< "Node N :"<<N.toStringFull(true)<<endl;
    return make_tuple(repPtr,height, repKey);
}

/**
 * @brief Subroutine for deleting AVLTreeNode from AVLTree. Searches for replacement of node that is to be deleted.
 * This function is called to cover the case where the node has two subtrees.
 * 
 * @param pad : worst case hight of current AVL tree, used for padding number of  accessed
 * @param deletePtr : pointer of node to be deleted
 * @param delNode : Node to be deleted
 * @param column : column for which the (multi-)AVLTree is currently updated
 * @param dummyExc : wether execution this function  is just a dummy operation or not. 
 * @return ulong : returns pointer of replacement node
 */
ulong AVLTree::findReplacement_twoSubtrees(int pad, ulong deletePtr,AVLTreeNode delNode, ulong column, bool dummyExc){
        bool dummy=false;
        vector<AVLTreeNode > nodes;
            vector<ulong> ptrsNodes;
            ulong ptrParent=deletePtr;
            ulong repPtr= delNode.ptrRightChild[column];
            db_t repKey=delNode.key[column];

            AVLTreeNode curNode(columnFormat,sizeValue);
            ulong ptrCur=repPtr;

            //cout<<"going from del Node to most leftChild of Right Tree"<<endl;
            for(int i=0; i<pad;i++){

                curNode=getNodeORAM(ptrCur);
                nodes.push_back(curNode);
                ptrsNodes.push_back(ptrCur);

                repPtr=_IF_THEN(((bool) curNode.ptrLeftChild[column]),curNode.ptrLeftChild[column],repPtr);//(not(bool) curNode.ptrLeftChild)*repPtr+curNode.ptrLeftChild; 
                repKey=_IF_THEN(((bool)ptrCur),curNode.key[column],repKey);

                //cout<<"i:"<<i<<" cur:" <<ptrCur<<" repPtr: "<<repPtr<<endl;

                ulong temp=ptrCur;
                ptrCur=curNode.ptrLeftChild[column];
                ptrParent=_IF_THEN(((bool)ptrCur),temp,ptrParent);//ptrCur?temp:ptrParent;

            }

            //cout<<"repKey: "<<repKey<<endl;

            bool foundRep=false;
            int newHeight=0;

            //cout<<"going backwards from most left child of right subtree to delNode"<<endl;
            for(int i=pad-1;i>=0;i--){
                curNode=nodes[i];
                ptrCur=ptrsNodes[i];

                bool isRep=not (bool)(ptrCur-repPtr);
                foundRep= isRep or foundRep;

                //update direkt Parent of Replacement: we alter this pointer twice
                bool isParent=not ((bool)(curNode.ptrLeftChild[column]-repPtr));
                curNode.ptrLeftChild[column]=_IF_THEN(isParent,NULL_PTR,curNode.ptrLeftChild[column]);//isParent? NULL_PTR:curNode.ptrLeftChild;
                
                //update size of all other nodes above replacement
                bool notRep= not isRep;

                curNode.lHeight[column]=_IF_THEN((foundRep and notRep), newHeight, curNode.lHeight[column]);//(found and notRep )? newHeight: curNode.lHeight;
                newHeight=_IF_THEN((foundRep and notRep), curNode.height(column), newHeight);//(found and notRep )? curNode.height(): newHeight;

                dummy=_IF_THEN((ptrCur==NULL_PTR), true,dummyExc);        
                putNodeORAM(curNode,dummy);                

            }

            AVLTreeNode repNode=getNodeORAM(repPtr);
            AVLTreeNode repParentNode=getNodeORAM(ptrParent);
            AVLTreeNode repRChildNode=getNodeORAM(repNode.ptrRightChild[column]); //might be a Dummy
            bool notDummy= (bool) repNode.ptrRightChild[column];

            //cout<<"repNode.ptrRightChild "<<repNode.ptrRightChild<<endl;
            //cout<<"repParentNode "<<ptrParent<<endl;

            repParentNode.ptrLeftChild[column]=repNode.ptrRightChild[column];
            repParentNode.lHeight[column]=_IF_THEN(notDummy,repRChildNode.height(column),0); 


            dummy=_IF_THEN((ptrParent==NULL_PTR), true,dummyExc);        
            putNodeORAM(repParentNode,dummy);

            //update Replacement Node
            bool sameNode=((bool) (delNode.ptrRightChild[column] ==repPtr));
            repNode.ptrRightChild[column]=_IF_THEN(sameNode,NULL_PTR,delNode.ptrRightChild[column]);
            repNode.ptrLeftChild[column]= delNode.ptrLeftChild[column];
            repNode.lHeight[column]=delNode.lHeight[column];
            repNode.rHeight[column]=newHeight;             


            dummy=_IF_THEN((repPtr==NULL_PTR), true,dummyExc);        
            putNodeORAM(repNode,dummy);
            
            return  repPtr;

}

/**
 * @brief Subroutine for deleting AVLTreeNode from AVLTree. Updates all nodes so the node to be deleted can be removed without leaving pointers to no where.
 * TODO: so balancing uses new functions.
 *
 * @param deletePtr : ID of node  to be deleted 
 * @param repPtr  : ID of replacement node
 * @param repHeight : Hight of replacement node
 * @param repKey : Key of replacement node for the corresponding column
 * @param nodes : nodes in the path from root to leaf (via the node to be deleted)
 * @param ptrsNodes : pointers of the nodes in nodes
 * @param lORr : wether path from root to leaf took a left or a right turn 
 * @param column : colum for which the (multi-)AVLTree is currently updated
 */
void AVLTree::deleteHelper_updateNodes(ulong deletePtr, ulong repPtr, int repHeight,  db_t repKey, 
        vector<AVLTreeNode > nodes, vector<ulong> ptrsNodes,vector<bool> lORr,  ulong column){
    //cout<<"updating pointers"<<endl;

    int update;
    bool found=false; //we first need to find the node in our list
    bool deleteThis=false;
    int pad=getPad();
    bool nodeExists=(deletePtr!=0);

    AVLTreeNode curNode(columnFormat, sizeValue);
    ulong ptrCur=NULL_PTR;
    for(int i=pad;i>0;i--){
        //cout<<"found "<<found<<endl;
        curNode=nodes[i-1];
        ptrCur=ptrsNodes[i-1];
        bool isLeft=lORr[i-1];

        update= (int) (found and isLeft and nodeExists);
        //cout<<"update left Child:"<<update<<endl;
        curNode.ptrLeftChild[column]=_IF_THEN(update,repPtr,curNode.ptrLeftChild[column]);
        update= (int) (found and (not isLeft)and nodeExists);
        //cout<<"update right Child:"<<update<<endl;
        curNode.ptrRightChild[column]=_IF_THEN(update,repPtr,curNode.ptrRightChild[column]);
        
        update =(int) (found and isLeft and nodeExists);
        curNode.lHeight[column]=_IF_THEN(update,repHeight,curNode.lHeight[column]);
        update =(int) (found and (not isLeft) and nodeExists);
        curNode.rHeight[column]= _IF_THEN(update,repHeight,curNode.rHeight[column]);
        

        bool isReplacement= (curNode.nodeID==repPtr);
        bool dummy=(!(bool)found) or isReplacement;
        curNode.empty=dummy;
        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"updated CurNode %s (dummy %d)")%MENHIR::toWString(curNode.toStringFull(true)) %dummy);
    
        //TODO: Replace this call
        auto[ignore,ptr_temp,height_temp]=balance(curNode,ptrCur,column); //calls putNodeORAM
        
        update=(int)found and nodeExists;
        repPtr=_IF_THEN(update,ptr_temp,repPtr);
        repHeight=_IF_THEN(update,height_temp,repHeight);

        deleteThis=not ((bool)(deletePtr-ptrCur));
        found=found or deleteThis;
    }
        
    //cout<<"deleteThis "<<deleteThis<<endl;
    ptrRoot[column]=_IF_THEN(deleteThis,repPtr,ptrCur);
    //cout<<"ptrRoot["<<column<<"]"<<ptrRoot[column]<<endl;
}

#pragma endregion



#pragma region FIND_FUNCTIONS_MEHNIR

/* 
* 
*/

/**
 * @brief Function to find a specific key with a specific hash. The ulong of operations is padded.
 * 
 * @param key 
 * @param nodeHash 
 * @param column 
 * @return tuple<vector<db_t>,bool> returns  Vector of Keys for this node with the requested hash. A bool value indicates if the returned values are just dummies. If true, the node could not be found.
 */
tuple<vector<db_t>,bool> AVLTree::findNode(db_t key, size_t nodeHash, ulong column){

    tuple<vector<db_t>,bool> result=findNodeHelper(key,nodeHash, column);
    return result;

}



/**
 * @brief Oblivious Algorithm to find a specific key with a specific hash. The number of operations is padded.
 *           Returns a vector of keys that belong to the searched node.
 *           Bool value indicating if these values are simply dummies.
 * 
 * @param key 
 * @param nodeHash 
 * @param column 
 * @return tuple<vector<db_t>,bool> 
 */
tuple<vector<db_t>,bool> AVLTree::findNodeHelper(db_t key, size_t nodeHash, ulong column){

    //cout<<"\n\nFIND: "<<DBT::toString(key)<<"-"<<nodeHash<<endl;
    vector<db_t> thisData; 
    for (size_t i = 0; i < numColumns; i++){
        AType type= columnFormat[i];
        db_t zero=DBT::getDBTZero(type);
        thisData.push_back(zero);
    }

    uint dummy=1;

    int pad=getPad();

    AVLTreeNode curNode(columnFormat, sizeValue);
    ulong curPtr=ptrRoot[column];
    for (int i = 0; i < pad; i++){

        curNode=getNodeORAM(curPtr);

        bool sameKey= (bool)(key==curNode.key[column]);
        bool sameHash= (bool)(nodeHash==curNode.nodeHash);
        bool update=(sameKey and sameHash);

        for (size_t i = 0; i < numColumns; i++){
            thisData[i]=_IF_THEN(update, curNode.key[i], thisData[i]);
        }
        
        dummy=_IF_THEN(update, 0, dummy);

        bool smallerKey= (curNode.key[column]-key)>0;
        bool smallerHash=(curNode.nodeHash-nodeHash)>0;
        curPtr=_IF_THEN(smallerKey,curNode.ptrLeftChild[column], curNode.ptrRightChild[column]);
        curPtr=_IF_THEN((sameKey and smallerHash),curNode.ptrLeftChild[column], curPtr);
    
    }
    return make_tuple(thisData,dummy);
}


/**
 * @brief Oblivious Algorithm to find a all keys that fall into a specific interval. The number of operations is padded.
 *           Returns Vector of key of the searched node.
 *           Bool vector indicating if these values are simply dummies.
 *          Warning: This function does not make use of the nextPointer stored in each Node. Instead it searches the next node everytime. 
 *          This makes the funciton rather inefficient and only suitable if less than ~5% of nodes are retrieved.
 * 
 * @param startKey 
 * @param endKey 
 * @param column 
 * @param estimate 
 * @return DBT::dbResponse 
 */
DBT::dbResponse AVLTree::findIntervalMenhir(db_t startKey, db_t endKey, ulong column, number estimate){


    DBT::dbResponse results=findIntervalHelperMenhir(startKey, endKey, column, estimate);

    if(CURRENT_LEVEL<=DEBUG){
        ostringstream oss;
        for (size_t i = 0; i < results.size(); i++){
            vector<db_t> record;
            bool dummy;

            tie(record,dummy)=results[i];
            db_t val=record[column];
            if(!dummy){
                oss<<"[";
                oss<<DBT::toString(val);
                oss<<"]; ";
            }
        }
        LOG(DEBUG, MENHIR::toWString(oss.str()));
    }

    return results;

}


/**
 * @brief Subroutine for finding all entries for which the key in the passed column falls into [startKey,endKey].
 * 
 * @param startKey 
 * @param endKey 
 * @param column 
 * @param estimate : number of data points to retrieve for hiding the volume pattern. This is effectively the number of dummies returned.
 * @return DBT::dbResponse: Vector of tuples consisting of database entries and bool values indicating wether the entry is a dummy or not.
 */
DBT::dbResponse  AVLTree::findIntervalHelperMenhir(db_t startKey, db_t endKey, ulong column, number estimate){
    int pad=getPad();
    DBT::dbResponse results;

    AVLTreeNode thisRoot(columnFormat, sizeValue);
    
    if(CURRENT_LEVEL==DEBUG) LOG(DEBUG, boost::wformat(L"estimate %d ")% estimate);
    ulong count=estimate;
    bool firstIteration=true;
    ulong nextID=NULL_PTR;
    bool allDummies=false;

    if(MENHIR::RETRIEVE_EXACTLY.size()!=0){
        allDummies=true;
    }


    while(count>0 or firstIteration ){
        if(MENHIR::RETRIEVE_EXACTLY.size()!=0 and results.size()==estimate){
            break;
        }


        bool isDummy=true;
        vector<db_t> thisData;
        AVLTreeNode curNode(columnFormat, sizeValue);

        if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"------------iteration %d - count noisy nodes to add %d ---------- ")% results.size() %count);

        //There are two different cases as way the first node is found during the first iteration differs from the other cases
        if(firstIteration==true){
            firstIteration=false;
            //set nextID and data
            ulong ptr=ptrRoot[column];
            thisData=MENHIR::getEmptyRow(columnFormat);

            for(int i=0;i<pad;i++){
                curNode=getNodeORAM(ptr);
                bool isDummy=(ptr==NULL_PTR);
                bool sameKey= (curNode.key[column]==startKey);

                bool smallerKey= (startKey<curNode.key[column]);
            
                bool takeLeft= (sameKey) or (not sameKey and smallerKey);            
                ptr=_IF_THEN(takeLeft, curNode.ptrLeftChild[column], curNode.ptrRightChild[column]);
                
                bool inInterval= (curNode.key[column]>=startKey) and  (curNode.key[column]<=endKey);
                bool isFirst=(inInterval and not isDummy); //as we always take the left we will find the smallest node in interval and fill the raining slots with dummies or smaller nodes
                nextID=_IF_THEN(isFirst,curNode.next[column],nextID);
                if(CURRENT_LEVEL==TRACE) LOG(TRACE, boost::wformat(L"nextID %d") %nextID );
                for(size_t j=0;j<numColumns;j++){
                    thisData[j]=_IF_THEN(isFirst,curNode.key[j],thisData[j]);
                }
                if(CURRENT_LEVEL==TRACE) LOG(TRACE,boost::wformat(L"curNode (inInterval %d, isFirst %d) :  %s") %inInterval %isFirst %MENHIR::toWString(curNode.toStringFull(true)));
            }

        }else{
            curNode=getNodeORAM(nextID);
            if(CURRENT_LEVEL==TRACE) LOG(TRACE,boost::wformat(L"curNode: %s") %MENHIR::toWString(curNode.toStringFull(true)));
            nextID=curNode.next[column];
            thisData=curNode.key;
        }
        
        bool inInterval= (thisData[column]>=startKey) and  (thisData[column]<=endKey);
        bool noDummiesYet=((ulong)count==estimate);
        isDummy=_IF_THEN((inInterval and noDummiesYet),false, isDummy);
        isDummy=_IF_THEN((allDummies),true, isDummy);



        results.push_back(make_tuple(thisData, isDummy));

        if(CURRENT_LEVEL==TRACE){ 
            ostringstream oss;
            for (size_t ii = 0; ii < results.size(); ii++){
                vector<db_t> record;
                bool dummy;
                tie(record,dummy)=results[ii];
                db_t val=record[column];
                oss<<"[";
                oss<<DBT::toString(val);
                oss<<"]; ";
                        
            }
            LOG(TRACE, MENHIR::toWString(oss.str()));
        }

        if(CURRENT_LEVEL==TRACE){ 
                LOG(TRACE,boost::wformat(L"Node added:  %s (dummy %d)") % DBT::toWString(thisData[column]) %isDummy);
                LOG(TRACE,boost::wformat(L"nextID %d") %nextID);
                LOG(TRACE, boost::wformat(L"Results.size(): %d") %results.size());
        }

        //counting down if isDummy
        int newCount=count-1;
    
        count=_IF_THEN(isDummy,newCount, count);

    }
    return results;

}


#pragma endregion


#pragma region UTILITY_FUNCTIONS


/**
 * @brief Prints the tree as a vertical tree shape. Prints into the LOG at the provided LOG_LEVEL.
 * 
 * @param level 
 * @param nodePtr 
 * @param expressiv 
 * @param column 
 * @param isLeft 
 * @param prefix 
 * @return string 
 */
string AVLTree::print(LOG_LEVEL level,ulong nodePtr,bool expressive, ulong column, bool isLeft, const std::string& prefix){
    

    if(CURRENT_LEVEL>level){
        return "";
    }

    if( nodePtr != NULL_PTR ){
        AVLTreeNode node= getNodeORAM(nodePtr);
        ostringstream oss;
        oss << prefix;
        oss<< (isLeft ? "├──" : "└──" );
        oss << DBT::toString(node.key[column]) <<": ("<<node.toString(expressive, column)<<")"<< std::endl;
        //cout<< oss.str()<<endl;
        //cout.flush();
        LOG(level, MENHIR::toWString(oss.str()));

        // enter the next tree level - left and right branch
        string rstr= this->print( level, node.ptrRightChild[column] ,expressive, column,true, prefix + (isLeft ? "│   " : "    "));
        string lstr=this->print(level, node.ptrLeftChild[column] ,expressive,column, false, prefix + (isLeft ? "│   " : "    "));
        


    }
    return "";

}


 /**
  * @brief Returns the tree as vertical tree shape as string.
  * 
  * @param expressiv 
  * @param column 
  * @return string 
  */
string AVLTree::toString(bool expressive, ulong column){
    queue<ulong> toProcess;
    toProcess.push(ptrRoot[column]);

    ulong ptrCur=NULL_PTR;
    std::ostringstream oss;

    while(not toProcess.empty()){
        ptrCur=toProcess.front();
        toProcess.pop();
        AVLTreeNode curNode=getNodeORAM(ptrCur);

        if(curNode.ptrLeftChild[column]!=NULL_PTR){
            toProcess.push(curNode.ptrLeftChild[column]);
        }
        if(curNode.ptrRightChild[column]!=NULL_PTR){
            toProcess.push(curNode.ptrRightChild[column]);
        }

        if(curNode.empty==false){
            if(! expressive){
                oss<<curNode.toString(false,column)<<"\n";
            }else{
                oss<<curNode.toString(true,column)<<"\n";
            }
        }

    }
    return oss.str();
}


#pragma endregion
}