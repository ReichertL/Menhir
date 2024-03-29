#include "avl_treenode.hpp"

#include <openssl/sha.h>
#include <iostream>

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


namespace DOSM{


/**
 * @brief Construct a new AVLTreeNode::AVLTreeNode object without any data. Each node is stored in the ORAM as one ORAM block and can be access via its ID.
 * 
 * @param columnFormat : Column format of the database. Relevant for parsing data.
 * @param sizeValue : The size of the value to be stored in the node (so the value associated with the keys).
 */
AVLTreeNode::AVLTreeNode(vector<AType> columnFormat, size_t sizeValue){
    this->columnFormat= columnFormat;
    numColumns=columnFormat.size();
    empty=true;

    key=vector<db_t>();
    for (size_t i = 0; i < numColumns; i++){
        AType type= columnFormat[i];
        db_t zero=DBT::getDBTZero(type);
        key.push_back(zero);
    }
    
    nodeID=0;
    nodeHash=0;
    value=vector<uchar>(sizeValue,0);
    ptrLeftChild=vector<ulong>(numColumns,NULL_PTR);
    ptrRightChild=vector<ulong>(numColumns,NULL_PTR);
    next=vector<ulong>(numColumns,NULL_PTR);
    lHeight=vector<int>(numColumns,0);
    rHeight=vector<int>(numColumns,0);

}

/**
 * @brief Construct a new AVLTreeNode::AVLTreeNode object. Constructs a node with empty value. For legacy reasons.
 * 
 * @param k : vector of len(columnFormat) containing a data point to be stored. This data is indexed and can be used by queries for filtering.
 * @param nHash : hash of the node
 * @param columnFormat : Column format of the database. Relevant for parsing data.
 * @param nodeID : ID of the node, location where it is stored in the ORAM
 */
AVLTreeNode::AVLTreeNode(vector<db_t> k, size_t nHash, vector<AType> columnFormat, ulong nodeID){
    this->nodeID=nodeID;
    empty=false;
    this->columnFormat=columnFormat;
    numColumns=columnFormat.size();

    key=k;
    nodeHash=nHash;
    value=vector<uchar>();
    ptrLeftChild=vector<ulong>(numColumns,NULL_PTR);
    ptrRightChild=vector<ulong>(numColumns,NULL_PTR);
    next=vector<ulong>(numColumns,NULL_PTR);
    lHeight=vector<int>(numColumns,0);
    rHeight=vector<int>(numColumns,0);
    
}

/**
 * @brief Construct a new AVLTreeNode::AVLTreeNode object given a set of key, a value, its hash and a pre-set nodeID.
 * The new node has no children and no hight.
 * 
 * @param k : vector of len(columnFormat) containing a data point to be stored. This data is indexed and can be used by queries for filtering.
 * @param nHash : hash of the node
 * @param v : Value associated with the node (unindexed data).
 * @param columnFormat : Column format of the database. Relevant for parsing data.
 * @param nodeID : ID of the node, location where it is stored in the ORAM
 */
AVLTreeNode::AVLTreeNode(vector<db_t> k, size_t nHash, bytes v, vector<AType> columnFormat, ulong nodeID){
    this->nodeID=nodeID;
    empty=false;
    this->columnFormat=columnFormat;
    numColumns=columnFormat.size();

    key=k;
    nodeHash=nHash;
    value=v;
    ptrLeftChild=vector<ulong>(numColumns,NULL_PTR);
    ptrRightChild=vector<ulong>(numColumns,NULL_PTR);
    next=vector<ulong>(numColumns,NULL_PTR);
    lHeight=vector<int>(numColumns,0);
    rHeight=vector<int>(numColumns,0);
    
}

/**
 * @brief Construct a new AVLTreeNode::AVLTreeNode object based on keys, value, hash, nodeID as well as information about left and right children.
 * This new node has left and right children for each column. The corresponding height values hold the information, how many nodes exist in each subtree with the same key. 
 * 
 * @param k : vector of len(columnFormat) containing a data point to be stored. This data is indexed and can be used by queries for filtering.
 * @param nHash : hash of the node
 * @param v : Value associated with the node (unindexed data).
 * @param lC : vector of len(columFormat) containing IDs of left children for each tree
 * @param rC : vector of len(columFormat) containing IDs of right children for each tree
 * @param nN : vector of len(columFormat) containing IDs of the next node (in order) for each tree
 * @param lH : vector of len(columFormat) containing the left height of the node for each tree (so how nodes with the same key exist in the left subtree)
 * @param rH : vector of len(columFormat) containing the right height of the node for each tree (so how nodes with the same key exist in the right subtree)
 * @param columnFormat : Column format of the database. Relevant for parsing data.
 * @param nodeID : ID of the node, location where it is stored in the ORAM
 */
AVLTreeNode::AVLTreeNode(vector<db_t> k, size_t nHash, bytes v, vector<ulong> lC, vector<ulong> rC, vector<ulong> nN, vector<int> lH, vector<int> rH, vector<AType> columnFormat,ulong nodeID){
    this->nodeID=nodeID;

    this->columnFormat=columnFormat;
    numColumns=columnFormat.size();
    empty=false;
   
    key=k;
    nodeHash=nHash;
    value=v;
    ptrLeftChild=lC;
    ptrRightChild=rC;
    next=nN;
    lHeight=lH;
    rHeight=rH;
}

/**
 * @brief Construct a new AVLTreeNode::AVLTreeNode object based on keys,  hash, nodeID as well as information about left and right children.
 * This new node has left and right children for each column. The corresponding height values hold the information, how many nodes exist in each subtree with the same key. 
 * Here, no value is passed.
 * 
 * @param k : vector of len(columnFormat) containing a data point to be stored. This data is indexed and can be used by queries for filtering.
 * @param nHash : hash of the node
 * @param lC : vector of len(columFormat) containing IDs of left children for each tree
 * @param rC : vector of len(columFormat) containing IDs of right children for each tree
 * @param nN : vector of len(columFormat) containing IDs of the next node (in order) for each tree
 * @param lH : vector of len(columFormat) containing the left height of the node for each tree (so how nodes with the same key exist in the left subtree)
 * @param rH : vector of len(columFormat) containing the right height of the node for each tree (so how nodes with the same key exist in the right subtree)
 * @param columnFormat : Column format of the database. Relevant for parsing data.
 * @param nodeID :ID of the node, location where it is stored in the ORAM
 */
AVLTreeNode::AVLTreeNode(vector<db_t> k, size_t nHash, vector<ulong> lC, vector<ulong> rC, vector<ulong> nN, vector<int> lH, vector<int> rH, vector<AType> columnFormat,ulong nodeID){
    this->nodeID=nodeID;

    this->columnFormat=columnFormat;
    numColumns=columnFormat.size();
    empty=false;
   
    key=k;
    nodeHash=nHash;
    value=vector<uchar>();
    ptrLeftChild=lC;
    ptrRightChild=rC;
    next=nN;
    lHeight=lH;
    rHeight=rH;
}

/**
 * @brief Construct a new AVLTreeNode::AVLTreeNode object from a serialized object.
 * 
 * 
 * @param serializedNode : array of  unsinged chars created by serializing an AVLTreeNode
 * @param dummy : Bool value indicating wether this is a dummy operation
 * @param columnFormat : Column format of the database. Relevant for parsing data.
 * @param sizeValue : The size of the value to be stored in the node (so the value associated with the keys).
 */
AVLTreeNode::AVLTreeNode(bytes serializedNode, bool dummy, vector<AType> columnFormat, size_t sizeValue){
    this->nodeID=nodeID;
    this->columnFormat=columnFormat;
    numColumns=columnFormat.size();
    empty=dummy;

    size_t sizeDBT=DBT::getMaxSizeDBT();
    int len=numColumns*sizeDBT;
    size_t s=0;
    key=vector<db_t>();


    for (size_t i = 0; i < numColumns; i++){
        bytes k;
        for(size_t j=0;j<sizeDBT;j++){
            int pos=i*sizeDBT+j;
            uchar vj=serializedNode[pos];
            k.push_back(vj);
        }
        key.push_back(DBT::deserialize(k,columnFormat[i]));
    }


    s=s+len;
    len=sizeof(size_t);
    uchar vH[sizeValue];
    for (size_t i = s; i < s+len; i++){
        vH[i-s]=serializedNode[i];
    } 
    size_t * vp=(size_t*) (&vH[0]);
    nodeHash=*vp;

    s=s+len;
    len=sizeValue;
    for (size_t i = s; i < s+len; i++){
        value.push_back(serializedNode[i]);
    } 

    s=s+len;
    len=sizeof(ulong)*numColumns;
    uchar lC[len];
    for (size_t i = s; i < s+len; i++){
        lC[i-s]=serializedNode[i];
    } 
    ptrLeftChild=vector<ulong>();
    ptrLeftChild.insert(ptrLeftChild.end(), (ulong*)&lC[0], (ulong*)&lC[len]);



    s=s+len;
    len=sizeof(ulong)*numColumns;
    uchar rC[len];
    for (size_t i = s; i < s+len; i++){
        rC[i-s]=serializedNode[i];
    } 
    ptrRightChild=vector<ulong>();
    ptrRightChild.insert(ptrRightChild.end(), (ulong*)&rC[0], (ulong*)&rC[len]);

    s=s+len;
    len=sizeof(ulong)*numColumns;
    uchar nN[len];
    for (size_t i = s; i < s+len; i++){
        nN[i-s]=serializedNode[i];
    } 
    next=vector<ulong>();
    next.insert(next.end(), (ulong*)&nN[0], (ulong*)&nN[len]);


    s=s+len;
    len=sizeof(int)*numColumns;
    uchar lH[len];
    for (size_t i = s; i < s+len; i++){
        lH[i-s]=serializedNode[i];
    }
    lHeight=vector<int>();
    lHeight.insert(lHeight.end(), (int*)&lH[0], (int*)&lH[len]);


    s=s+len;
    len=sizeof(int)*numColumns;
    uchar rH[len];
    for (size_t i = s; i < s+len; i++){
        rH[i-s]=serializedNode[i];
    } 
    rHeight=vector<int>();
    rHeight.insert(rHeight.end(), (int*)&rH[0], (int*)&rH[len]);

    s=s+len;
    len=sizeof(ulong);
    uchar id[len];
    for (size_t i = s; i < s+len; i++){
        id[i-s]=serializedNode[i];
    } 
    ulong * vid=(ulong*) (&id[0]);
    nodeID=*vid;

}

/**
 * @brief Serializes the current AVLTreeNode. It is converted to a vector of unsinged char. 
 * 
 * @return bytes 
 */
bytes AVLTreeNode::serialize(){
    bytes serialized;
    int numBytes=sizeof(db_t)*numColumns+sizeof(size_t)+sizeof(ulong)*2*numColumns+sizeof(uint)*2*numColumns+sizeof(int)*2*numColumns+sizeof(ulong);
    serialized.reserve(numBytes);

    for (size_t i = 0; i < numColumns; i++){
        vector<uchar> data_vec = DBT::serialize(key[i],columnFormat[i]);
        uchar* uBytes = data_vec.data();
        for (size_t i = 0; i <DBT::getMaxSizeDBT(); i++){
            serialized.push_back(uBytes[i]);
        }
    }


    void* vp =(void*)&nodeHash;
    uchar* uBytes =(uchar*)vp;
    for (size_t i = 0; i < sizeof(size_t); i++){
        serialized.push_back(uBytes[i]);
    }

    for (size_t i = 0; i < value.size(); i++){
        serialized.push_back(value[i]);
    }
    
    for (size_t i = 0; i < numColumns; i++){
        void* vp =(void*)&ptrLeftChild[i];
        uchar* uBytes =(uchar*)vp;
        for (size_t i = 0; i < sizeof(ulong); i++){
            serialized.push_back(uBytes[i]);
        }
    }


    for (size_t i = 0; i < numColumns; i++){
        void* vp =(void*)&ptrRightChild[i];
        uchar* uBytes =(uchar*)vp;
        for (size_t i = 0; i < sizeof(ulong); i++){
            serialized.push_back(uBytes[i]);
        }
    }

    for (size_t i = 0; i < numColumns; i++){
        void* vp =(void*)&next[i];
        uchar* uBytes =(uchar*)vp;
        for (size_t i = 0; i < sizeof(ulong); i++){
            serialized.push_back(uBytes[i]);
        }
    }




    for (size_t i = 0; i < numColumns; i++){
        void* vp =(void*)&lHeight[i];
        uchar* uBytes =(uchar*)vp;
        for (size_t i = 0; i < sizeof(int); i++){
            serialized.push_back(uBytes[i]);
        }
    }

    for (size_t i = 0; i < numColumns; i++){
        void* vp =(void*)&rHeight[i];
        uchar* uBytes =(uchar*)vp;
        for (size_t i = 0; i < sizeof(int); i++){
            serialized.push_back(uBytes[i]);
        }
    }

    void* vid =(void*)&nodeID;
    uchar* uBytes_ID =(uchar*)vid;
    for (size_t i = 0; i < sizeof(ulong); i++){
        serialized.push_back(uBytes_ID[i]);
    }


    serialized.shrink_to_fit();
    return serialized;
}

/**
 * @brief Gets the size of bytes for a node with the  given format when serialized. 
 * 
 * @param columnFormat : Column format of the database. Relevant for parsing data.
 * @param sizeValue :
 * @return number : number of bytes of the serialized AVLTreeNode with the passed parameters
 */
number getNumBytesWhenSerialized(vector<AType> columnFormat, size_t sizeValue){
    AVLTreeNode NULL_NODE=AVLTreeNode(columnFormat, sizeValue);
    bytes serialized=NULL_NODE.serialize();
    return serialized.size();
}

/**
 * @brief Computes the height of the node for a certain column.
 * 
 * @param column 
 * @return uint 
 */
uint AVLTreeNode::height(ulong column){
    return max(lHeight[column],rHeight[column])+1;
}

/**
 * @brief Computes the AVL balance factor for a column. 
 * 
 * @param column 
 * @return int 
 */
int AVLTreeNode::balanceFactor(ulong column){
    return (int)lHeight[column]-(int)rHeight[column];
}

/**
 * @brief An ORAM can only contain data of a fixed Blocksize. 
 * First the node is serialized. For nodes which are smaller than that block size after serialization, padding is added.
 * The padded byte array is returned.
 * 
 * @param blockSize 
 * @return bytes 
 */
bytes AVLTreeNode::padToBlockSize(number blockSize){
    bytes nodeBytes=this->serialize();
    number diff=blockSize- nodeBytes.size();
    for (size_t i = 0; i < diff; i++){
        nodeBytes.push_back('0');
    }
    return nodeBytes;
    
}

/**
 * @brief Compute the value hash for a tuple. To distinguish between identical tuples a value r is factored in (this can be the nodeID)
 * 
 * @param key : vector of len(columnFormat) containing a data point to be stored. This data is indexed and can be used by queries for filtering.
 * @param r : potentially the nodeID, but does not have to be
 * @return size_t : a Hash 
 */
size_t getNodeHash(vector<db_t> key,bytes value, ulong r){
    std::ostringstream oss;
    for (size_t i = 0; i < key.size(); i++){
        oss<<DBT::toString(key[i])<<",";
    }

    for (size_t i = 0; i < value.size(); i++){
        oss<<value[i]<<",";
    }
    oss<<r;
    //non cryptographic hash
    //std::hash<std::string> hashingFunction;
    //size_t nodeHash= (size_t) hashingFunction(oss.str());

    unsigned char hash[SHA_DIGEST_LENGTH]; // == 20
    const char* cstring=oss.str().c_str();
    void * bits_void= (void*) &cstring[0];
    const unsigned char * bits=(const unsigned char *) bits_void;
    SHA1(bits, oss.str().size() , hash);
    size_t * vp=(size_t*) (&hash[0]);
    size_t nodeHash=*vp;
    return nodeHash;
 }

/**
 * @brief Returns a string containing all keys of the current AVLTreeNode.
 * 
 * @return string 
 */
string AVLTreeNode::keysToString(){
    string sKeys= "";
    for(size_t i=0; i<numColumns;i++){
        string s= DBT::toString(key[i]);
        sKeys=sKeys+s;
        if(i!=numColumns-1){
            sKeys=sKeys+",";
        }
    }
    return sKeys;
}

/**
 * @brief Returns a string of the current AVLTreeNode for a given column. 
 * This contains the nodeHash as well as key, left height and right height for the passed column.
 * If expressive is set left children,right children,next nodes, balance factor, height of the node, ID of the node, and all keys are incorporated.
 * 
 * @param expressive
 * @param column 
 * @return string 
 */
string AVLTreeNode::toString(bool expressive,ulong column){
    std::ostringstream oss;

    oss<<"Node[key:"<<DBT::toString(key[column]) <<"-"<<nodeHash;
    oss <<", LH:" <<lHeight[column]     <<", RH:" <<rHeight[column];
    
    if (expressive){
        oss<<", left:"    <<ptrLeftChild[column]<<", right:"   << ptrRightChild[column];
        oss<<", next:"<< next[column];
        oss<<", B:" <<this->balanceFactor();
        oss<<", H:"  <<this->height();
        oss<<", Data: '"<<keysToString()<<"'";
    }
    oss<<"]";
    
    if(expressive){
        oss<< ", ptr:"<<nodeID;
    }
    return oss.str();

}

/**
 * @brief Returns a string of the current AVLTreeNode with data from all columns. 
 * This contains  nodeHash, as well as  keys, right height and left height for all columns. 
 * If expressive ist set left children,right children,next nodes, balance factor, height of the node and the ID of the node are incorporated.
 * 
 * @param expressive 
 * @return string 
 */
string AVLTreeNode::toStringFull(bool expressive){
    std::ostringstream oss;

    oss<<"Node[key:'"<<keysToString();
    
    oss<<"''; v:"<<nodeHash;
    /*for(size_t i=0;i<len(value);i++){
        oss<<value[i];
    }*/

    oss<<"; LH:'";
    std::copy(lHeight.begin(), lHeight.end(), std::ostream_iterator<int>(oss, ","));

    oss<<"'; RH:'" ;
    std::copy(rHeight.begin(), rHeight.end(), std::ostream_iterator<int>(oss, ","));

        
    if (expressive){
        oss<<"'; left:'";
        std::copy(ptrLeftChild.begin(), ptrLeftChild.end(), std::ostream_iterator<ulong>(oss, ",")); 

        oss<<"'; right:'" ;
        std::copy(ptrRightChild.begin(), ptrRightChild.end(), std::ostream_iterator<ulong>(oss, ",")); 

        oss<<"'; next:'" ;
        std::copy(next.begin(), next.end(), std::ostream_iterator<ulong>(oss, ",")); 

        oss<<"', B:'" <<this->balanceFactor();
        oss<<"', H:'"  <<this->height();
    }
    oss<<"']";
    
    if(this->nodeID!=NULL_PTR ){
        oss<< ", ptr:"<<this->nodeID;
    }
    return oss.str();

}

}

