#pragma once

#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <string>
#include <cmath>
#include <sstream>
#include <cstring>
#include <iterator>

#include "database_type.hpp"
#include "path-oram/definitions.h"


/**
 * @brief This class defines AVLTreeNodes. 
 * If len(columnFormat)==1 this can be considered a normal AVL tree with one key (that is indexed) and one value (unindexed byte vector).
 * If len(columnFormat)>1, for each column a new tree is constructed. These multiple trees are constructed on the same objects. Only the pointers differ for each column.
 * Example:
 * Node a (key:[2,2], value:000, nodeHash:1), Node b (key[3,1], value: 100, nodeHash:2), Node c (key: [2,3], value: 111, nodeHash: 0)
 * 
 * Logical Tree for Column 0:                  Logical Tree for Column 1: 
 *      a                                                   a
 *    /   \                                               /   \
 *   c     b                                             b     c           
 * 
 * The full data for Node a looks something like this: 
 * Node a (key:[2,2], value:000, nodeHash:1, ptrLeftChild:[b,c], ptrRightChild:[c,b],lHeight:[1,0],rHeight:[0,0], next:[b,c], nodeID:a)
 * 
 */



#define NULL_PTR (ulong)0
#define DSizeDefault 64
 
namespace DOSM{

using namespace PathORAM;


struct AVLTreeNode {
        
    ulong nodeID;
    vector<ulong> ptrLeftChild;
    vector<ulong>ptrRightChild;
    vector<ulong> next;
    
    vector<int>lHeight;
    vector<int> rHeight;

    vector<db_t> key;
    bytes value;
    size_t nodeHash;
    bool empty;

    ulong numColumns;
    vector<AType> columnFormat;    
    AVLTreeNode();
    AVLTreeNode(vector<AType> columnFormat, size_t sizeValue);
    AVLTreeNode(vector<db_t> k, size_t vHash, bytes v,vector<AType> columnFormat, ulong nodeID);
    AVLTreeNode(vector<db_t> k, size_t vHash, vector<AType> columnFormat, ulong nodeID);
    AVLTreeNode(vector<db_t> k,size_t vHash,
                vector<ulong> lC, vector<ulong> rC, vector<ulong> nN, 
                vector<int> lH, vector<int> rH, 
                vector<AType> columnFormat,ulong nodeID);
    AVLTreeNode(vector<db_t> k,size_t vHash, bytes v, 
                vector<ulong> lC, vector<ulong> rC, vector<ulong> nN, 
                vector<int> lH, vector<int> rH,
                vector<AType> columnFormat,ulong nodeID);

    
    AVLTreeNode(bytes serializedNode,bool dummy, vector<AType> columnFormat, size_t sizeValue);

    uint height(ulong column=0);
    int balanceFactor(ulong column=0);
    bytes serialize();
    bytes padToBlockSize(number blockSize);


    string singleKeyToString(ulong column);
    string keysToString();
    string toString( bool expressiv=false, ulong column=0);
    string toStringFull( bool expressiv=false);

};

number getNumBytesWhenSerialized(vector<AType> columnFormat, size_t sizeValue);

size_t getNodeHash(vector<db_t> key, bytes value, ulong r);

}