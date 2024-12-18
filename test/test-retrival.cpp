#define DEF_DEBUG
#include "definitions.h"
#include "utility.hpp"
#include "avl_multiset.hpp"
#include "database_type.hpp"
#include "get_data_and_queries.hpp"
#include "avl_loadtree.hpp"
#include "path-oram/definitions.h"
#include "path-oram/oram.hpp"
//#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include <regex>
#include <boost/variant2/variant.hpp>

using namespace boost::variant2;
using namespace std;
using namespace MENHIR;
using namespace DOSM;
using namespace testing;




TEST(AVLTreeTests, InitTree){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=ERROR;

    vector<AType> thisFormat {AType::INT, AType::FLOAT};
    number capacity=100;
    size_t sizeValue=0;    
    AVLTree *tree=new AVLTree(thisFormat,sizeValue,capacity);
    ASSERT_EQ(tree->size(),0);
    ASSERT_EQ(tree->empty(),true);
}





TEST(AVLTreeTests, InsertHead){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=ERROR;
    
    vector<AType> thisFormat {AType::INT, AType::FLOAT};
    number capacity=100;
    size_t sizeValue=0;    
    AVLTree *tree=new AVLTree(thisFormat,sizeValue,capacity);
    vector<db_t> keys {db_t(1), db_t(5.3f)};
    size_t nodeHash=tree->insert(keys);

    std::ostringstream oss1;
    oss1 << "Node[key:1-" << nodeHash<<", LH:0, RH:0]\n";
    string expected_c1=oss1.str();

    std::ostringstream oss2;
    oss2 << "Node[key:5.300000-" << nodeHash<<", LH:0, RH:0]\n";
    string expected_c2=oss2.str();

    ASSERT_EQ(tree->size(), 1);
    ASSERT_EQ(tree->toString(false,0),expected_c1);
    ASSERT_EQ(tree->toString(false,1),expected_c2);
   
}


TEST(AVLTreeTests, putTreeInORAM){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=ERROR;   

    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    //key, valueHAsh, data, lC, rC, lH,rH,lS,rS
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{75},vector<ulong>{65},vector<int>{2},vector<int>{2}, thisFormat, 55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{25},vector<ulong>{45},vector<ulong>{45},vector<int>{1},vector<int>{1},thisFormat, 35),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{65},vector<ulong>{85},vector<ulong>{85},vector<int>{1},vector<int>{1},thisFormat, 75),
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat, 25),
        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{55},vector<int>{0},vector<int>{0}, thisFormat, 45),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{75},vector<int>{0},vector<int>{0}, thisFormat, 65),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0}, thisFormat,85),
    };
    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);

    ASSERT_EQ(tree->size(), 7);

    string expected= "Node[key:5-5, LH:2, RH:2, left:35, right:75, next:65, B:0, H:3, Data: '5'], ptr:55\n"\
                     "Node[key:3-5, LH:1, RH:1, left:25, right:45, next:45, B:0, H:2, Data: '3'], ptr:35\n"\
                     "Node[key:7-5, LH:1, RH:1, left:65, right:85, next:85, B:0, H:2, Data: '7'], ptr:75\n"\
                     "Node[key:2-5, LH:0, RH:0, left:0, right:0, next:35, B:0, H:1, Data: '2'], ptr:25\n"\
                     "Node[key:4-5, LH:0, RH:0, left:0, right:0, next:55, B:0, H:1, Data: '4'], ptr:45\n"\
                     "Node[key:6-5, LH:0, RH:0, left:0, right:0, next:75, B:0, H:1, Data: '6'], ptr:65\n"\
                     "Node[key:8-5, LH:0, RH:0, left:0, right:0, next:0, B:0, H:1, Data: '8'], ptr:85\n";   
    ASSERT_EQ(tree->toString(true,0),expected);

}

TEST(AVLTreeTests, createTreeFromDatavector_unevenNumberOfNodes){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number logcapacity=7;
    size_t sizeValue=0;    


    vector<vector<db_t>> inputData;
    inputData.push_back(vector<db_t>{3});
    inputData.push_back(vector<db_t>{7});
    inputData.push_back(vector<db_t>{4});
    inputData.push_back(vector<db_t>{2});
    inputData.push_back(vector<db_t>{6});
    inputData.push_back(vector<db_t>{5});
    inputData.push_back(vector<db_t>{1});

    INPUT_DATA=inputData;
	//loadTreeBulkNonObliv( inputData.size(),tree);
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue, logcapacity, ORAM_Z,  STASH_FACTOR, BATCH_SIZE, &INPUT_DATA, INPUT_DATA.size(),USE_ORAM);


    string expected="Node[key:4-3, LH:2, RH:2, left:4, right:5, next:6, B:0, H:3, Data: '4'], ptr:3\n"\
                    "Node[key:2-4, LH:1, RH:1, left:7, right:1, next:1, B:0, H:2, Data: '2'], ptr:4\n"\
                    "Node[key:6-5, LH:1, RH:1, left:6, right:2, next:2, B:0, H:2, Data: '6'], ptr:5\n"\
                    "Node[key:1-7, LH:0, RH:0, left:0, right:0, next:4, B:0, H:1, Data: '1'], ptr:7\n"\
                    "Node[key:3-1, LH:0, RH:0, left:0, right:0, next:3, B:0, H:1, Data: '3'], ptr:1\n"\
                    "Node[key:5-6, LH:0, RH:0, left:0, right:0, next:5, B:0, H:1, Data: '5'], ptr:6\n"\
                    "Node[key:7-2, LH:0, RH:0, left:0, right:0, next:0, B:0, H:1, Data: '7'], ptr:2\n";
    ASSERT_EQ(tree->toString(true,0),expected);

}

TEST(AVLTreeTests, createTreeFromDatavector_evenNumberOfNodes){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=INFO;
    
    vector<AType> thisFormat {AType::INT};
    number logcapacity=7;
    size_t sizeValue=0;    


    vector<vector<db_t>> inputData;
    inputData.push_back(vector<db_t>{3});
    inputData.push_back(vector<db_t>{7});
    inputData.push_back(vector<db_t>{4});
    inputData.push_back(vector<db_t>{2});
    inputData.push_back(vector<db_t>{6});
    inputData.push_back(vector<db_t>{5});
    inputData.push_back(vector<db_t>{1});
    inputData.push_back(vector<db_t>{8});

    INPUT_DATA=inputData;
	//loadTreeBulkNonObliv(inputData.size(),tree);
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue, logcapacity, ORAM_Z,  STASH_FACTOR, BATCH_SIZE, &INPUT_DATA, INPUT_DATA.size(),USE_ORAM);



    string expected="Node[key:5-6, LH:3, RH:2, left:1, right:2, next:5, B:1, H:4, Data: '5'], ptr:6\n"\
                    "Node[key:3-1, LH:2, RH:1, left:7, right:3, next:3, B:1, H:3, Data: '3'], ptr:1\n"\
                    "Node[key:7-2, LH:1, RH:1, left:5, right:8, next:8, B:0, H:2, Data: '7'], ptr:2\n"\
                    "Node[key:1-7, LH:0, RH:1, left:0, right:4, next:4, B:-1, H:2, Data: '1'], ptr:7\n"\
                    "Node[key:4-3, LH:0, RH:0, left:0, right:0, next:6, B:0, H:1, Data: '4'], ptr:3\n"\
                    "Node[key:6-5, LH:0, RH:0, left:0, right:0, next:2, B:0, H:1, Data: '6'], ptr:5\n"\
                    "Node[key:8-8, LH:0, RH:0, left:0, right:0, next:0, B:0, H:1, Data: '8'], ptr:8\n"\
                    "Node[key:2-4, LH:0, RH:0, left:0, right:0, next:1, B:0, H:1, Data: '2'], ptr:4\n";
    ASSERT_EQ(tree->toString(true,0),expected);

}




TEST(AVLTreeTests, InsertDuplicate){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<db_t> key={db_t(5)};
    size_t nodeHash1=tree->insert(key);
    size_t nodeHash2=tree->insert(key);
    std::ostringstream oss;
    if(nodeHash2>nodeHash1){
        oss <<  "Node[key:5-"<<nodeHash1<<", LH:0, RH:1]\n";
        oss <<  "Node[key:5-"<<nodeHash2<<", LH:0, RH:0]\n";
    }else{
        oss <<  "Node[key:5-"<<nodeHash1<<", LH:1, RH:0]\n";
        oss <<  "Node[key:5-"<<nodeHash2<<", LH:0, RH:0]\n";
    }
    string expected=oss.str();

    ASSERT_EQ(tree->size(), 2);
    ASSERT_EQ(tree->toString(),expected);
   
}

TEST(AVLTreeTests, RetrieveNULLNODE){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{75},vector<ulong>{65},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{25},vector<ulong>{45},vector<ulong>{45},vector<int>{1},vector<int>{1},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{65},vector<ulong>{85},vector<ulong>{85},vector<int>{1},vector<int>{1},thisFormat,75),
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat,25 ),
        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{45},vector<int>{0},vector<int>{0},thisFormat,45),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{75},vector<int>{0},vector<int>{0},thisFormat,65),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,85),

    };

    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);
    AVLTreeNode NULL_NODE=AVLTreeNode(thisFormat, sizeValue);

    bytes response;
    tree->getORAM()->get(NULL_PTR+1,response);       
    ASSERT_NE(response.size(),0);
    
    AVLTreeNode nodeRetrieved= tree->getNodeORAM(NULL_PTR, true);       
    ASSERT_EQ(NULL_NODE.toStringFull(true),nodeRetrieved.toStringFull(true));
    

}

TEST(AVLTreeTests, InsertIndirectDuplicate){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{75},vector<ulong>{65},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{25},vector<ulong>{45},vector<ulong>{45},vector<int>{1},vector<int>{1},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{65},vector<ulong>{85},vector<ulong>{85},vector<int>{1},vector<int>{1},thisFormat,75),
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat,25 ),
        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{55},vector<int>{0},vector<int>{0},thisFormat,45),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{75},vector<int>{0},vector<int>{0},thisFormat,65),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,85),

    };

    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);

    number nodeHash= tree->insert(vector<db_t>{db_t(7)});
    ASSERT_EQ(tree->size(), nodes.size()+1);
    ostringstream oss;
    oss<<            "Node[key:5-5, LH:2, RH:3]\n"\
                     "Node[key:3-5, LH:1, RH:1]\n"\
                     "Node[key:7-5, LH:1, RH:2]\n"\
                     "Node[key:2-5, LH:0, RH:0]\n"\
                     "Node[key:4-5, LH:0, RH:0]\n"\
                     "Node[key:6-5, LH:0, RH:0]\n"\
                     "Node[key:8-5, LH:1, RH:0]\n"\
                     "Node[key:7-"<<nodeHash<<", LH:0, RH:0]\n";   
    string expected=oss.str();             
    ASSERT_EQ(tree->toString(),expected);
   
}
 



TEST(RotationTests, RightRotateNoDuplicates){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{45},vector<ulong>{75},vector<ulong>{65},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{35},vector<ulong>{0},vector<ulong>{55},vector<int>{1},vector<int>{0},thisFormat,45),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{65},vector<ulong>{85},vector<ulong>{85},vector<int>{1},vector<int>{1},thisFormat,75),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{45},vector<int>{0},vector<int>{0},thisFormat,35 ),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{75},vector<int>{0},vector<int>{0},thisFormat,65),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,85)
    };
    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);
    number nodeHash=tree->insert(vector<db_t>{db_t(2)});

    ASSERT_EQ(tree->size(), nodes.size()+1);
    ostringstream oss;
    oss<<"Node[key:5-5, LH:2, RH:2]\n"\
                     "Node[key:3-5, LH:1, RH:1]\n"\
                     "Node[key:7-5, LH:1, RH:1]\n"\
                     "Node[key:2-"<<nodeHash<<", LH:0, RH:0]\n"\
                     "Node[key:4-5, LH:0, RH:0]\n"\
                     "Node[key:6-5, LH:0, RH:0]\n"\
                     "Node[key:8-5, LH:0, RH:0]\n";
    string expected=oss.str();                             
    ASSERT_EQ(tree->toString(),expected);
   
}



TEST(RotationTests, RightRotateWithDuplicates){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{75},vector<ulong>{65},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{34},vector<ulong>{0},vector<ulong>{55},vector<int>{1},vector<int>{0},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{65},vector<ulong>{85},vector<ulong>{85},vector<int>{1},vector<int>{1},thisFormat,75),
        AVLTreeNode(vector<db_t>{db_t(3)},4,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat,34 ),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{75},vector<int>{0},vector<int>{0},thisFormat,65),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,85)
    };
    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);
    number nodeHash=3;
    tree->insert(vector<db_t>{db_t(3)},nodeHash);

    ASSERT_EQ(tree->size(), nodes.size()+1);
    string expected= "Node[key:5-5, LH:2, RH:2]\n"\
                     "Node[key:3-4, LH:1, RH:1]\n"\
                     "Node[key:7-5, LH:1, RH:1]\n"\
                     "Node[key:3-3, LH:0, RH:0]\n"\
                     "Node[key:3-5, LH:0, RH:0]\n"\
                     "Node[key:6-5, LH:0, RH:0]\n"\
                     "Node[key:8-5, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
   
}


TEST(RotationTests, LeftRotateNoDuplicates){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{65},vector<ulong>{65},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{25},vector<ulong>{45},vector<ulong>{45},vector<int>{1},vector<int>{1},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{75},vector<ulong>{75},vector<int>{0},vector<int>{1},thisFormat,65),
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat,25 ),
        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{55},vector<int>{0},vector<int>{0},thisFormat,45),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{0},vector<ulong>{0}, vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,75)
    };
    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);
    number nodeHash=5;
    tree->insert(vector<db_t>{db_t(8)},nodeHash);

    ASSERT_EQ(tree->size(), nodes.size()+1);
    string expected= "Node[key:5-5, LH:2, RH:2]\n"\
                     "Node[key:3-5, LH:1, RH:1]\n"\
                     "Node[key:7-5, LH:1, RH:1]\n"\
                     "Node[key:2-5, LH:0, RH:0]\n"\
                     "Node[key:4-5, LH:0, RH:0]\n"\
                     "Node[key:6-5, LH:0, RH:0]\n"\
                     "Node[key:8-5, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
   
}

TEST(RotationTests, LeftRotateWithDuplicates){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{56},vector<ulong>{56},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{25},vector<ulong>{45},vector<ulong>{45},vector<int>{1},vector<int>{1},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(5)},6,vector<ulong>{0},vector<ulong>{57},vector<ulong>{57},vector<int>{0},vector<int>{1},thisFormat,56),
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat,25 ),
        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{55},vector<int>{0},vector<int>{0},thisFormat,45),
        AVLTreeNode(vector<db_t>{db_t(5)},7,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,57)
    };
    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);
    number nodeHash=8;
    tree->insert(vector<db_t>{db_t(5)},nodeHash);

    ASSERT_EQ(tree->size(), nodes.size()+1);
    string expected= "Node[key:5-5, LH:2, RH:2]\n"\
                     "Node[key:3-5, LH:1, RH:1]\n"\
                     "Node[key:5-7, LH:1, RH:1]\n"\
                     "Node[key:2-5, LH:0, RH:0]\n"\
                     "Node[key:4-5, LH:0, RH:0]\n"\
                     "Node[key:5-6, LH:0, RH:0]\n"\
                     "Node[key:5-8, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
   
}



TEST(RotationTests, RightLeftRotateNoDuplicates){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{15},vector<ulong>{45},vector<ulong>{35},vector<int>{3},vector<int>{3},thisFormat,25),
        AVLTreeNode(vector<db_t>{db_t(1)},5,vector<ulong>{13},vector<ulong>{17},vector<ulong>{16},vector<int>{2},vector<int>{2},thisFormat,15),
        AVLTreeNode(vector<db_t>{db_t(1)},3,vector<ulong>{12},vector<ulong>{14},vector<ulong>{14},vector<int>{1},vector<int>{1},thisFormat,13),
        AVLTreeNode(vector<db_t>{db_t(1)},7,vector<ulong>{16},vector<ulong>{18},vector<ulong>{18},vector<int>{1},vector<int>{1},thisFormat,17 ),
        AVLTreeNode(vector<db_t>{db_t(1)},2,vector<ulong>{0},vector<ulong>{0},vector<ulong>{13},vector<int>{0},vector<int>{0},thisFormat,12),
        AVLTreeNode(vector<db_t>{db_t(1)},4,vector<ulong>{0},vector<ulong>{0},vector<ulong>{15},vector<int>{0},vector<int>{0},thisFormat,14),
        AVLTreeNode(vector<db_t>{db_t(1)},6,vector<ulong>{0},vector<ulong>{0},vector<ulong>{17},vector<int>{0},vector<int>{0},thisFormat,16),
        AVLTreeNode(vector<db_t>{db_t(1)},8,vector<ulong>{0},vector<ulong>{0},vector<ulong>{25},vector<int>{0},vector<int>{0},thisFormat,18),

        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{35},vector<ulong>{75},vector<ulong>{65},vector<int>{1},vector<int>{1},thisFormat,45 ),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{45},vector<int>{0},vector<int>{0},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{65},vector<ulong>{85},vector<ulong>{85},vector<int>{1},vector<int>{1},thisFormat,75),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{75},vector<int>{0},vector<int>{0},thisFormat,65),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,85),
    };
    vector<ulong> root=vector<ulong>{25};
    tree->putTreeInORAM(nodes,root);
    number nodeHash=5;
    tree->insert(vector<db_t>{db_t(5)},nodeHash);

    ASSERT_EQ(tree->size(), nodes.size()+1);
    string expected= "Node[key:2-5, LH:3, RH:3]\n"\
                     "Node[key:1-5, LH:2, RH:2]\n"\
                     "Node[key:6-5, LH:2, RH:2]\n"\
                     "Node[key:1-3, LH:1, RH:1]\n"\
                     "Node[key:1-7, LH:1, RH:1]\n"\
                     "Node[key:4-5, LH:1, RH:1]\n"\
                     "Node[key:7-5, LH:0, RH:1]\n"\
                     "Node[key:1-2, LH:0, RH:0]\n"\
                     "Node[key:1-4, LH:0, RH:0]\n"\
                     "Node[key:1-6, LH:0, RH:0]\n"\
                     "Node[key:1-8, LH:0, RH:0]\n"\
                     "Node[key:3-5, LH:0, RH:0]\n"\
                     "Node[key:5-5, LH:0, RH:0]\n"\
                     "Node[key:8-5, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
   
}

TEST(RotationTests, RightLeftRotateWithDuplicates){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(2)},0,vector<ulong>{15},vector<ulong>{24},vector<ulong>{23},vector<int>{3},vector<int>{3},thisFormat,20),
        AVLTreeNode(vector<db_t>{db_t(1)},5,vector<ulong>{13},vector<ulong>{17},vector<ulong>{16},vector<int>{2},vector<int>{2},thisFormat,15),
        AVLTreeNode(vector<db_t>{db_t(1)},3,vector<ulong>{12},vector<ulong>{14},vector<ulong>{14},vector<int>{1},vector<int>{1},thisFormat,13),
        AVLTreeNode(vector<db_t>{db_t(1)},7,vector<ulong>{16},vector<ulong>{18},vector<ulong>{18},vector<int>{1},vector<int>{1},thisFormat,17 ),
        AVLTreeNode(vector<db_t>{db_t(1)},2,vector<ulong>{0},vector<ulong>{0},vector<ulong>{13},vector<int>{0},vector<int>{0},thisFormat,12),
        AVLTreeNode(vector<db_t>{db_t(1)},4,vector<ulong>{0},vector<ulong>{0},vector<ulong>{15},vector<int>{0},vector<int>{0},thisFormat,14),
        AVLTreeNode(vector<db_t>{db_t(1)},6,vector<ulong>{0},vector<ulong>{0},vector<ulong>{17},vector<int>{0},vector<int>{0},thisFormat,16),
        AVLTreeNode(vector<db_t>{db_t(1)},8,vector<ulong>{0},vector<ulong>{0},vector<ulong>{20},vector<int>{0},vector<int>{0},thisFormat,18),

        AVLTreeNode(vector<db_t>{db_t(2)},4,vector<ulong>{23},vector<ulong>{27},vector<ulong>{26},vector<int>{1},vector<int>{2},thisFormat,24 ),
        AVLTreeNode(vector<db_t>{db_t(2)},3,vector<ulong>{0},vector<ulong>{0},vector<ulong>{24},vector<int>{0},vector<int>{0},thisFormat,23),
        AVLTreeNode(vector<db_t>{db_t(2)},7,vector<ulong>{26},vector<ulong>{28},vector<ulong>{28},vector<int>{1},vector<int>{1},thisFormat,27),
        AVLTreeNode(vector<db_t>{db_t(2)},6,vector<ulong>{0},vector<ulong>{0},vector<ulong>{27},vector<int>{0},vector<int>{0},thisFormat,26),
        AVLTreeNode(vector<db_t>{db_t(2)},8,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,28),
    };
    vector<ulong> root=vector<ulong>{20};
    tree->putTreeInORAM(nodes,root);
    number nodeHash=5;
    tree->insert(vector<db_t>{db_t(2)},nodeHash);

    ASSERT_EQ(tree->size(), nodes.size()+1);
    string expected= "Node[key:2-0, LH:3, RH:3]\n"\
                     "Node[key:1-5, LH:2, RH:2]\n"\
                     "Node[key:2-6, LH:2, RH:2]\n"\
                     "Node[key:1-3, LH:1, RH:1]\n"\
                     "Node[key:1-7, LH:1, RH:1]\n"\
                     "Node[key:2-4, LH:1, RH:1]\n"\
                     "Node[key:2-7, LH:0, RH:1]\n"\
                     "Node[key:1-2, LH:0, RH:0]\n"\
                     "Node[key:1-4, LH:0, RH:0]\n"\
                     "Node[key:1-6, LH:0, RH:0]\n"\
                     "Node[key:1-8, LH:0, RH:0]\n"\
                     "Node[key:2-3, LH:0, RH:0]\n"\
                     "Node[key:2-5, LH:0, RH:0]\n"\
                     "Node[key:2-8, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
   
}




TEST(RotationTests, LeftRightRotateNoDuplicates){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(20)},5,{75},{215},{35},{3},{3},thisFormat,205), 
        AVLTreeNode({db_t(21)},5,{213},{217},{216},{2},{2},thisFormat,215), 
        AVLTreeNode({db_t(21)},3, {212},{214},{214},{1},{1},thisFormat,213), 
        AVLTreeNode({db_t(21)},7,{216},{218},{218},{1},{1},thisFormat,217),
        AVLTreeNode({db_t(21)},2, {0},{0},{213},{0},{0},thisFormat,212),
        AVLTreeNode({db_t(21)},4, {0},{0},{215},{0},{0},thisFormat,214),
        AVLTreeNode({db_t(21)},6, {0},{0},{217},{0},{0},thisFormat,216),
        AVLTreeNode({db_t(21)},8, {0},{0},{0},{0},{0},thisFormat,218),

        AVLTreeNode({db_t(7)},5, {45},{85},{85},{2},{1},thisFormat,75),
        AVLTreeNode({db_t(8)},5, {0},{0},{205},{0},{0},thisFormat,85),
        AVLTreeNode({db_t(4)},5, {35},{55},{55},{1},{1},thisFormat,45),
        AVLTreeNode({db_t(3)},5, {0},{0},{45},{0},{0},thisFormat,35),
        AVLTreeNode({db_t(5)},5, {0},{0},{75},{0},{0},thisFormat,55),

    };
    vector<ulong> root={205};
    tree->putTreeInORAM(nodes,root);
    number nodeHash=5;
    tree->insert(vector<db_t>{db_t(6)},nodeHash);


    ASSERT_EQ(tree->size(), nodes.size()+1);
    string expected= "Node[key:20-5, LH:3, RH:3]\n"\
                     "Node[key:5-5, LH:2, RH:2]\n"\
                     "Node[key:21-5, LH:2, RH:2]\n"\
                     "Node[key:4-5, LH:1, RH:0]\n"\
                     "Node[key:7-5, LH:1, RH:1]\n"\
                     "Node[key:21-3, LH:1, RH:1]\n"\
                     "Node[key:21-7, LH:1, RH:1]\n"\
                     "Node[key:3-5, LH:0, RH:0]\n"\
                     "Node[key:6-5, LH:0, RH:0]\n"\
                     "Node[key:8-5, LH:0, RH:0]\n"\
                     "Node[key:21-2, LH:0, RH:0]\n"\
                     "Node[key:21-4, LH:0, RH:0]\n"\
                     "Node[key:21-6, LH:0, RH:0]\n"\
                     "Node[key:21-8, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
   
}


TEST(RotationTests, LeftRightRotateWithDuplicates){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(2)},10,{27},{215},{212},{3},{3},thisFormat,210), 
        AVLTreeNode({db_t(2)},7,{24},{28},{28},{2},{1},thisFormat,27),
        AVLTreeNode({db_t(2)},8,{0},{0},{210},{0},{0},thisFormat,28),
        AVLTreeNode({db_t(2)},4,{23},{25},{25},{1},{1},thisFormat,24),
        AVLTreeNode({db_t(2)},3,{0},{0},{24},{0},{0},thisFormat,23),
        AVLTreeNode({db_t(2)},5,{0},{0},{27},{0},{0},thisFormat,25),

        AVLTreeNode({db_t(2)},15,{213},{217},{216},{2},{2},thisFormat,215), 
        AVLTreeNode({db_t(2)},13,{212},{214},{214},{1},{1},thisFormat,213), 
        AVLTreeNode({db_t(2)},17,{216},{218},{218},{1},{1},thisFormat,217),
        AVLTreeNode({db_t(2)},12,{0},{0},{213},{0},{0},thisFormat,212),
        AVLTreeNode({db_t(2)},14,{0},{0},{215},{0},{0},thisFormat,214),
        AVLTreeNode({db_t(2)},16,{0},{0},{217},{0},{0},thisFormat,216),
        AVLTreeNode({db_t(2)},18,{0},{0},{0},{0},{0},thisFormat,218),

    };
    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);
    number nodeHash=6;
    tree->insert(vector<db_t>{db_t(2)},nodeHash);

    ASSERT_EQ(tree->size(), nodes.size()+1);
    string expected= "Node[key:2-10, LH:3, RH:3]\n"\
                     "Node[key:2-5, LH:2, RH:2]\n"\
                     "Node[key:2-15, LH:2, RH:2]\n"\
                     "Node[key:2-4, LH:1, RH:0]\n"\
                     "Node[key:2-7, LH:1, RH:1]\n"\
                     "Node[key:2-13, LH:1, RH:1]\n"\
                     "Node[key:2-17, LH:1, RH:1]\n"\
                     "Node[key:2-3, LH:0, RH:0]\n"\
                     "Node[key:2-6, LH:0, RH:0]\n"\
                     "Node[key:2-8, LH:0, RH:0]\n"\
                     "Node[key:2-12, LH:0, RH:0]\n"\
                     "Node[key:2-14, LH:0, RH:0]\n"\
                     "Node[key:2-16, LH:0, RH:0]\n"\
                     "Node[key:2-18, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
   
}



TEST(InsertionTests, NewFirst){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{75},vector<ulong>{65},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{25},vector<ulong>{45},vector<ulong>{45},vector<int>{1},vector<int>{1},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{65},vector<ulong>{85},vector<ulong>{85},vector<int>{1},vector<int>{1},thisFormat,75),
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat,25 ),
        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{55},vector<int>{0},vector<int>{0},thisFormat,45),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{75},vector<int>{0},vector<int>{0},thisFormat,65),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,85),

    };

    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);

    number nodeHash= tree->insert(vector<db_t>{db_t(1)});
    ASSERT_EQ(tree->size(), nodes.size()+1);
    ostringstream oss;
    oss<<   "Node[key:5-5, LH:3, RH:2, left:35, right:75, next:65, B:1, H:4, Data: '5'], ptr:55\n"\
            "Node[key:3-5, LH:2, RH:1, left:25, right:45, next:45, B:1, H:3, Data: '3'], ptr:35\n"\
            "Node[key:7-5, LH:1, RH:1, left:65, right:85, next:85, B:0, H:2, Data: '7'], ptr:75\n"\
            "Node[key:2-5, LH:1, RH:0, left:1, right:0, next:35, B:1, H:2, Data: '2'], ptr:25\n"\
            "Node[key:4-5, LH:0, RH:0, left:0, right:0, next:55, B:0, H:1, Data: '4'], ptr:45\n"\
            "Node[key:6-5, LH:0, RH:0, left:0, right:0, next:75, B:0, H:1, Data: '6'], ptr:65\n"\
            "Node[key:8-5, LH:0, RH:0, left:0, right:0, next:0, B:0, H:1, Data: '8'], ptr:85\n"\
            "Node[key:1-"<<nodeHash<<", LH:0, RH:0, left:0, right:0, next:25, B:0, H:1, Data: '1'], ptr:1\n";

    string expected=oss.str();             
    ASSERT_EQ(tree->toString(true,0),expected);
}


TEST(InsertionTests, NewLast){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{75},vector<ulong>{65},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{25},vector<ulong>{45},vector<ulong>{45},vector<int>{1},vector<int>{1},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{65},vector<ulong>{85},vector<ulong>{85},vector<int>{1},vector<int>{1},thisFormat,75),
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat,25 ),
        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{55},vector<int>{0},vector<int>{0},thisFormat,45),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{75},vector<int>{0},vector<int>{0},thisFormat,65),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,85),

    };

    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);

    number nodeHash= tree->insert(vector<db_t>{db_t(9)});
    ASSERT_EQ(tree->size(), nodes.size()+1);
    ostringstream oss;
    oss<<   "Node[key:5-5, LH:2, RH:3, left:35, right:75, next:65, B:-1, H:4, Data: '5'], ptr:55\n"\
            "Node[key:3-5, LH:1, RH:1, left:25, right:45, next:45, B:0, H:2, Data: '3'], ptr:35\n"\
            "Node[key:7-5, LH:1, RH:2, left:65, right:85, next:85, B:-1, H:3, Data: '7'], ptr:75\n"\
            "Node[key:2-5, LH:0, RH:0, left:0, right:0, next:35, B:0, H:1, Data: '2'], ptr:25\n"\
            "Node[key:4-5, LH:0, RH:0, left:0, right:0, next:55, B:0, H:1, Data: '4'], ptr:45\n"\
            "Node[key:6-5, LH:0, RH:0, left:0, right:0, next:75, B:0, H:1, Data: '6'], ptr:65\n"\
            "Node[key:8-5, LH:0, RH:1, left:0, right:1, next:1, B:-1, H:2, Data: '8'], ptr:85\n"\
            "Node[key:9-"<<nodeHash<<", LH:0, RH:0, left:0, right:0, next:0, B:0, H:1, Data: '9'], ptr:1\n";


    string expected=oss.str();             
    ASSERT_EQ(tree->toString(true,0),expected);
}


TEST(InsertionTests, LeafNode_RightChild){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{75},vector<ulong>{75},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{25},vector<ulong>{45},vector<ulong>{45},vector<int>{1},vector<int>{1},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{0},vector<ulong>{85},vector<ulong>{85},vector<int>{0},vector<int>{1},thisFormat,75),
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat,25 ),
        AVLTreeNode(vector<db_t>{db_t(4)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{55},vector<int>{0},vector<int>{0},thisFormat,45),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,85),
    };

    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);

    number nodeHash= tree->insert(vector<db_t>{db_t(6)});
    ASSERT_EQ(tree->size(), nodes.size()+1);
    ostringstream oss;


    oss<<   "Node[key:5-5, LH:2, RH:2, left:35, right:75, next:1, B:0, H:3, Data: '5'], ptr:55\n"\
            "Node[key:3-5, LH:1, RH:1, left:25, right:45, next:45, B:0, H:2, Data: '3'], ptr:35\n"\
            "Node[key:7-5, LH:1, RH:1, left:1, right:85, next:85, B:0, H:2, Data: '7'], ptr:75\n"\
            "Node[key:2-5, LH:0, RH:0, left:0, right:0, next:35, B:0, H:1, Data: '2'], ptr:25\n"\
            "Node[key:4-5, LH:0, RH:0, left:0, right:0, next:55, B:0, H:1, Data: '4'], ptr:45\n"\
            "Node[key:6-"<<nodeHash<<", LH:0, RH:0, left:0, right:0, next:75, B:0, H:1, Data: '6'], ptr:1\n"\
            "Node[key:8-5, LH:0, RH:0, left:0, right:0, next:0, B:0, H:1, Data: '8'], ptr:85\n";

    string expected=oss.str();             
    ASSERT_EQ(tree->toString(true,0),expected);
}

TEST(InsertionTests, LeafNode_LeftChild){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{75},vector<ulong>{65},vector<int>{2},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{25},vector<ulong>{0},vector<ulong>{55},vector<int>{1},vector<int>{1},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(7)},5,vector<ulong>{65},vector<ulong>{85},vector<ulong>{85},vector<int>{1},vector<int>{1},thisFormat,75),
        AVLTreeNode(vector<db_t>{db_t(2)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{35},vector<int>{0},vector<int>{0},thisFormat,25 ),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{75},vector<int>{0},vector<int>{0},thisFormat,65),
        AVLTreeNode(vector<db_t>{db_t(8)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{0},vector<int>{0},vector<int>{0},thisFormat,85),
    };

    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);

    number nodeHash= tree->insert(vector<db_t>{db_t(4)});
    ASSERT_EQ(tree->size(), nodes.size()+1);
    ostringstream oss;


    oss<<   "Node[key:5-5, LH:2, RH:2, left:35, right:75, next:65, B:0, H:3, Data: '5'], ptr:55\n"\
            "Node[key:3-5, LH:1, RH:1, left:25, right:1, next:1, B:0, H:2, Data: '3'], ptr:35\n"\
            "Node[key:7-5, LH:1, RH:1, left:65, right:85, next:85, B:0, H:2, Data: '7'], ptr:75\n"\
            "Node[key:2-5, LH:0, RH:0, left:0, right:0, next:35, B:0, H:1, Data: '2'], ptr:25\n"\
            "Node[key:4-"<<nodeHash<<", LH:0, RH:0, left:0, right:0, next:55, B:0, H:1, Data: '4'], ptr:1\n"\
            "Node[key:6-5, LH:0, RH:0, left:0, right:0, next:75, B:0, H:1, Data: '6'], ptr:65\n"\
            "Node[key:8-5, LH:0, RH:0, left:0, right:0, next:0, B:0, H:1, Data: '8'], ptr:85\n";

    string expected=oss.str();             
    ASSERT_EQ(tree->toString(true,0),expected);
}

TEST(InsertionTests, CenterNode_newRoot){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    vector<AType> thisFormat {AType::INT};
    number capacity=100;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode(vector<db_t>{db_t(5)},5,vector<ulong>{35},vector<ulong>{95},vector<ulong>{65},vector<int>{1},vector<int>{2},thisFormat,55),
        AVLTreeNode(vector<db_t>{db_t(3)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{55},vector<int>{0},vector<int>{0},thisFormat,35),
        AVLTreeNode(vector<db_t>{db_t(9)},5,vector<ulong>{65},vector<ulong>{0},vector<ulong>{0},vector<int>{1},vector<int>{1},thisFormat,95),
        AVLTreeNode(vector<db_t>{db_t(6)},5,vector<ulong>{0},vector<ulong>{0},vector<ulong>{95},vector<int>{0},vector<int>{0},thisFormat,65),
    };

    vector<ulong> root=vector<ulong>{55};
    tree->putTreeInORAM(nodes,root);

    number nodeHash= tree->insert(vector<db_t>{db_t(7)});

    ASSERT_EQ(tree->size(), nodes.size()+1);
    ostringstream oss;


    oss<<   "Node[key:6-5, LH:2, RH:2, left:55, right:95, next:1, B:0, H:3, Data: '6'], ptr:65\n"\
            "Node[key:5-5, LH:1, RH:0, left:35, right:0, next:65, B:1, H:2, Data: '5'], ptr:55\n"\
            "Node[key:9-5, LH:1, RH:1, left:1, right:0, next:0, B:0, H:2, Data: '9'], ptr:95\n"\
            "Node[key:3-5, LH:0, RH:0, left:0, right:0, next:55, B:0, H:1, Data: '3'], ptr:35\n"\
            "Node[key:7-"<<nodeHash<<", LH:0, RH:0, left:0, right:0, next:95, B:0, H:1, Data: '7'], ptr:1\n";
    string expected=oss.str();             
    ASSERT_EQ(tree->toString(true,0),expected);
}

TEST(DeletionTests, deletionOfNonexistentNode){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(2)},10,{26},{215},{212},{3},{3},thisFormat,210), 
        AVLTreeNode({db_t(2)},6,{24},{28},{27},{2},{2},thisFormat,26),
        AVLTreeNode({db_t(2)},8,{27},{29},{29},{1},{1},thisFormat,28),
        AVLTreeNode({db_t(2)},4,{23},{25},{25},{1},{1},thisFormat,24),
        AVLTreeNode({db_t(2)},3,{0},{0},{24},{0},{0},thisFormat,23),
        AVLTreeNode({db_t(2)},5,{0},{0},{26},{0},{0},thisFormat,25),
        AVLTreeNode({db_t(2)},7,{0},{0},{28},{0},{0},thisFormat,27),
        AVLTreeNode({db_t(2)},9,{0},{0},{210},{0},{0},thisFormat,29),

        AVLTreeNode({db_t(2)},15,{213},{217},{216},{2},{2},thisFormat,215), 
        AVLTreeNode({db_t(2)},13,{212},{214},{214},{1},{1},thisFormat,213), 
        AVLTreeNode({db_t(2)},17,{216},{218},{218},{1},{1},thisFormat,217),
        AVLTreeNode({db_t(2)},12,{0},{0},{213},{0},{0},thisFormat,212),
        AVLTreeNode({db_t(2)},14,{0},{0},{215},{0},{0},thisFormat,214),
        AVLTreeNode({db_t(2)},16,{0},{0},{217},{0},{0},thisFormat,216),
        AVLTreeNode({db_t(2)},18,{0},{0},{0},{0},{0},thisFormat,218),
    };
    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);
    tree->deleteEntry(2,19,0);


    ASSERT_EQ(tree->size(), nodes.size()-1);
    string expected= "Node[key:2-10, LH:3, RH:3]\n"\
                        "Node[key:2-6, LH:2, RH:2]\n"\
                        "Node[key:2-15, LH:2, RH:2]\n"\
                        "Node[key:2-4, LH:1, RH:1]\n"\
                        "Node[key:2-8, LH:1, RH:1]\n"\
                        "Node[key:2-13, LH:1, RH:1]\n"\
                        "Node[key:2-17, LH:1, RH:1]\n"\
                        "Node[key:2-3, LH:0, RH:0]\n"\
                        "Node[key:2-5, LH:0, RH:0]\n"\
                        "Node[key:2-7, LH:0, RH:0]\n"\
                        "Node[key:2-9, LH:0, RH:0]\n"\
                        "Node[key:2-12, LH:0, RH:0]\n"\
                        "Node[key:2-14, LH:0, RH:0]\n"\
                        "Node[key:2-16, LH:0, RH:0]\n"\
                        "Node[key:2-18, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
}





TEST(DeletionTests, LeafDeletion){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(2)},10,{26},{215},{212},{3},{3},thisFormat,210), 
        AVLTreeNode({db_t(2)},6,{24},{28},{27},{2},{2},thisFormat,26),
        AVLTreeNode({db_t(2)},8,{27},{29},{29},{1},{1},thisFormat,28),
        AVLTreeNode({db_t(2)},4,{23},{25},{25},{1},{1},thisFormat,24),
        AVLTreeNode({db_t(2)},3,{0},{0},{24},{0},{0},thisFormat,23),
        AVLTreeNode({db_t(2)},5,{0},{0},{26},{0},{0},thisFormat,25),
        AVLTreeNode({db_t(2)},7,{0},{0},{28},{0},{0},thisFormat,27),
        AVLTreeNode({db_t(2)},9,{0},{0},{210},{0},{0},thisFormat,29),

        AVLTreeNode({db_t(2)},15,{213},{217},{216},{2},{2},thisFormat,215), 
        AVLTreeNode({db_t(2)},13,{212},{214},{214},{1},{1},thisFormat,213), 
        AVLTreeNode({db_t(2)},17,{216},{218},{218},{1},{1},thisFormat,217),
        AVLTreeNode({db_t(2)},12,{0},{0},{213},{0},{0},thisFormat,212),
        AVLTreeNode({db_t(2)},14,{0},{0},{215},{0},{0},thisFormat,214),
        AVLTreeNode({db_t(2)},16,{0},{0},{217},{0},{0},thisFormat,216),
        AVLTreeNode({db_t(2)},18,{0},{0},{0},{0},{0},thisFormat,218),
    };
    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);
    tree->deleteEntry(2,3,0);


    ASSERT_EQ(tree->size(), nodes.size()-1);
    string expected= "Node[key:2-10, LH:3, RH:3]\n"\
                        "Node[key:2-6, LH:2, RH:2]\n"\
                        "Node[key:2-15, LH:2, RH:2]\n"\
                        "Node[key:2-4, LH:0, RH:1]\n"\
                        "Node[key:2-8, LH:1, RH:1]\n"\
                        "Node[key:2-13, LH:1, RH:1]\n"\
                        "Node[key:2-17, LH:1, RH:1]\n"\
                        "Node[key:2-5, LH:0, RH:0]\n"\
                        "Node[key:2-7, LH:0, RH:0]\n"\
                        "Node[key:2-9, LH:0, RH:0]\n"\
                        "Node[key:2-12, LH:0, RH:0]\n"\
                        "Node[key:2-14, LH:0, RH:0]\n"\
                        "Node[key:2-16, LH:0, RH:0]\n"\
                        "Node[key:2-18, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
}



TEST(DeletionTests, SingelNodeTree){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    tree->insert(vector<db_t>{db_t(5)},5);
    tree->deleteEntry(5,5,0);
    string expected= ""; 

    ASSERT_EQ(tree->size(), 0);
    ASSERT_EQ(tree->toString(true),expected);
   
}


TEST(DeletionTests, DeletionWithLeftChild){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(2)},10,{26},{215},{212},{3},{3},thisFormat,210), 
        AVLTreeNode({db_t(2)},6,{24},{28},{27},{2},{2},thisFormat,26),
        AVLTreeNode({db_t(2)},8,{27},{29},{29},{1},{1},thisFormat,28),
        AVLTreeNode({db_t(2)},4,{23},{0},{26},{1},{0},thisFormat,24),
        AVLTreeNode({db_t(2)},3,{0},{0},{24},{0},{0},thisFormat,23),
        AVLTreeNode({db_t(2)},7,{0},{0},{29},{0},{0},thisFormat,27),
        AVLTreeNode({db_t(2)},9,{0},{0},{210},{0},{0},thisFormat,29),

        AVLTreeNode({db_t(2)},15,{213},{217},{216},{2},{2},thisFormat,215), 
        AVLTreeNode({db_t(2)},13,{212},{214},{214},{1},{1},thisFormat,213), 
        AVLTreeNode({db_t(2)},17,{216},{218},{218},{1},{1},thisFormat,217),
        AVLTreeNode({db_t(2)},12,{0},{0},{213},{0},{0},thisFormat,212),
        AVLTreeNode({db_t(2)},14,{0},{0},{215},{0},{0},thisFormat,214),
        AVLTreeNode({db_t(2)},16,{0},{0},{217},{0},{0},thisFormat,216),
        AVLTreeNode({db_t(2)},18,{0},{0},{0},{0},{0},thisFormat,218),

    };

    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);
    tree->deleteEntry(2,4,0);


    ASSERT_EQ(tree->size(), nodes.size()-1);
    string expected= "Node[key:2-10, LH:3, RH:3]\n"\
                        "Node[key:2-6, LH:1, RH:2]\n"\
                        "Node[key:2-15, LH:2, RH:2]\n"\
                        "Node[key:2-3, LH:0, RH:0]\n"\
                        "Node[key:2-8, LH:1, RH:1]\n"\
                        "Node[key:2-13, LH:1, RH:1]\n"\
                        "Node[key:2-17, LH:1, RH:1]\n"\
                        "Node[key:2-7, LH:0, RH:0]\n"\
                        "Node[key:2-9, LH:0, RH:0]\n"\
                        "Node[key:2-12, LH:0, RH:0]\n"\
                        "Node[key:2-14, LH:0, RH:0]\n"\
                        "Node[key:2-16, LH:0, RH:0]\n"\
                        "Node[key:2-18, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
}


TEST(DeletionTests, DeletionWithRightChild){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);
    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(2)},10,{26},{215},{212},{3},{3},thisFormat, 210), 
        AVLTreeNode({db_t(2)},6,{24},{28},{27},{2},{2}, thisFormat, 26),
        AVLTreeNode({db_t(2)},8,{27},{29},{29},{1},{1},thisFormat, 28),
        AVLTreeNode({db_t(2)},4,{25},{0},{25},{1},{0}, thisFormat, 24),
        AVLTreeNode({db_t(2)},5,{0},{0},{26},{0}, {0},thisFormat, 25),
        AVLTreeNode({db_t(2)},7,{0},{0},{28},{0},{0}, thisFormat, 27),
        AVLTreeNode({db_t(2)},9,{0},{0},{210},{0},{0}, thisFormat, 29),

        AVLTreeNode({db_t(2)},15,{213},{217},{216},{2},{2},  thisFormat,215 ), 
        AVLTreeNode({db_t(2)},13,{212},{214},{214},{1},{1}, thisFormat, 213), 
        AVLTreeNode({db_t(2)},17,{216},{218},{218},{1},{1}, thisFormat, 217),
        AVLTreeNode({db_t(2)},12,{0},{0},{213}, {0},{0}, thisFormat,212 ),
        AVLTreeNode({db_t(2)},14,{0},{0},{215},{0},{0}, thisFormat, 214),
        AVLTreeNode({db_t(2)},16,{0},{0},{217},{0},{0}, thisFormat, 216),
        AVLTreeNode({db_t(2)},18,{0},{0},{0},{0},{0}, thisFormat, 218),

    };

    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);
    tree->deleteEntry(2,4,0);



    ASSERT_EQ(tree->size(), nodes.size()-1);
    string expected= "Node[key:2-10, LH:3, RH:3]\n"\
                        "Node[key:2-6, LH:1, RH:2]\n"\
                        "Node[key:2-15, LH:2, RH:2]\n"\
                        "Node[key:2-5, LH:0, RH:0]\n"\
                        "Node[key:2-8, LH:1, RH:1]\n"\
                        "Node[key:2-13, LH:1, RH:1]\n"\
                        "Node[key:2-17, LH:1, RH:1]\n"\
                        "Node[key:2-7, LH:0, RH:0]\n"\
                        "Node[key:2-9, LH:0, RH:0]\n"\
                        "Node[key:2-12, LH:0, RH:0]\n"\
                        "Node[key:2-14, LH:0, RH:0]\n"\
                        "Node[key:2-16, LH:0, RH:0]\n"\
                        "Node[key:2-18, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
}


TEST(DeletionTests, DeletionWithTwoChildren_LargeLeftSubTree){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(2)},10,{26},{215},{212},{3},{3},thisFormat,210 ), 
        AVLTreeNode({db_t(2)},6,{24},{28},{27},{2},{2},thisFormat, 26),
        AVLTreeNode({db_t(2)},8,{27},{29},{29},{1},{1},thisFormat, 28),
        AVLTreeNode({db_t(2)},4,{23},{25},{25},{1},{1},thisFormat, 24),
        AVLTreeNode({db_t(2)},3,{0},{0},{24},{0},{0},thisFormat, 23),
        AVLTreeNode({db_t(2)},5,{0},{0},{26},{0},{0},thisFormat, 25),
        AVLTreeNode({db_t(2)},7,{0},{0},{28},{0},{0},thisFormat, 27),
        AVLTreeNode({db_t(2)},9,{0},{0},{210},{0},{0},thisFormat, 29),

        AVLTreeNode({db_t(2)},15,{213},{217},{216},{2},{2},thisFormat,215 ), 
        AVLTreeNode({db_t(2)},13,{212},{214},{214},{1},{1},thisFormat, 213), 
        AVLTreeNode({db_t(2)},17,{216},{218},{218},{1},{1},thisFormat, 217),
        AVLTreeNode({db_t(2)},12,{0},{0},{213},{0},{0},thisFormat, 212),
        AVLTreeNode({db_t(2)},14,{0},{0},{215},{0},{0},thisFormat, 214),
        AVLTreeNode({db_t(2)},16,{0},{0},{217},{0},{0},thisFormat, 216),
        AVLTreeNode({db_t(2)},18,{0},{0},{0},{0},{0},thisFormat, 218),

    };

    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);
    tree->deleteEntry(2,6,0);


    ASSERT_EQ(tree->size(), nodes.size()-1);
    string expected= "Node[key:2-10, LH:3, RH:3]\n"\
                        "Node[key:2-7, LH:2, RH:2]\n"\
                        "Node[key:2-15, LH:2, RH:2]\n"\
                        "Node[key:2-4, LH:1, RH:1]\n"\
                        "Node[key:2-8, LH:0, RH:1]\n"\
                        "Node[key:2-13, LH:1, RH:1]\n"\
                        "Node[key:2-17, LH:1, RH:1]\n"\
                        "Node[key:2-3, LH:0, RH:0]\n"\
                        "Node[key:2-5, LH:0, RH:0]\n"\
                        "Node[key:2-9, LH:0, RH:0]\n"\
                        "Node[key:2-12, LH:0, RH:0]\n"\
                        "Node[key:2-14, LH:0, RH:0]\n"\
                        "Node[key:2-16, LH:0, RH:0]\n"\
                        "Node[key:2-18, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
}



TEST(DeletionTests, DeletionWithBothChildren_ReplacementWithRightSubtree){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(2)},11,{26},{215},{212},{4},{3},thisFormat,211 ), 
        AVLTreeNode({db_t(2)},6,{24},{29},{27},{2},{3},thisFormat, 26),
        AVLTreeNode({db_t(2)},9,{27},{210},{210},{2},{1},thisFormat, 29 ),
        AVLTreeNode({db_t(2)},4,{23},{25},{25},{1},{1},thisFormat, 24),
        AVLTreeNode({db_t(2)},3,{0},{0},{24},{0},{0},thisFormat, 23),
        AVLTreeNode({db_t(2)},5,{0},{0},{26},{0},{0},thisFormat, 25),
        AVLTreeNode({db_t(2)},7,{0},{28},{27},{0},{1},thisFormat,27 ),
        AVLTreeNode({db_t(2)},10,{0},{0},{211},{0},{0},thisFormat, 210),
        AVLTreeNode({db_t(2)},8,{0},{0},{29},{0},{0},thisFormat, 28),

        AVLTreeNode({db_t(2)},15,{213},{217},{216},{2},{2},thisFormat,215 ), 
        AVLTreeNode({db_t(2)},13,{212},{214},{214},{1},{1},thisFormat, 213), 
        AVLTreeNode({db_t(2)},17,{216},{218},{218},{1},{1},thisFormat, 217),
        AVLTreeNode({db_t(2)},12,{0},{0},{213},{0},{0},thisFormat,212 ),
        AVLTreeNode({db_t(2)},14,{0},{0},{215},{0},{0},thisFormat, 214),
        AVLTreeNode({db_t(2)},16,{0},{0},{217},{0},{0},thisFormat, 216),
        AVLTreeNode({db_t(2)},18,{0},{0},{0},{0},{0},thisFormat, 218),

    };

    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);
    tree->deleteEntry(2,6,0);


    ASSERT_EQ(tree->size(), nodes.size()-1);
    string expected= "Node[key:2-11, LH:3, RH:3]\n"\
                        "Node[key:2-7, LH:2, RH:2]\n"\
                        "Node[key:2-15, LH:2, RH:2]\n"\
                        "Node[key:2-4, LH:1, RH:1]\n"\
                        "Node[key:2-9, LH:1, RH:1]\n"\
                        "Node[key:2-13, LH:1, RH:1]\n"\
                        "Node[key:2-17, LH:1, RH:1]\n"\
                        "Node[key:2-3, LH:0, RH:0]\n"\
                        "Node[key:2-5, LH:0, RH:0]\n"\
                        "Node[key:2-8, LH:0, RH:0]\n"\
                        "Node[key:2-10, LH:0, RH:0]\n"\
                        "Node[key:2-12, LH:0, RH:0]\n"\
                        "Node[key:2-14, LH:0, RH:0]\n"\
                        "Node[key:2-16, LH:0, RH:0]\n"\
                        "Node[key:2-18, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
}


TEST(DeletionTests, DeletionWithBothChildren_ShortLeftSubTree){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(2)},10,{26},{215},{212},{3},{3},thisFormat, 210), 
        AVLTreeNode({db_t(2)},6,{24},{28},{27},{2},{2},thisFormat, 26),
        AVLTreeNode({db_t(2)},8,{27},{29},{29},{1},{1},thisFormat, 28),
        AVLTreeNode({db_t(2)},4,{23},{25},{25},{1},{1},thisFormat, 24),
        AVLTreeNode({db_t(2)},3,{0},{0},{24},{0},{0},thisFormat, 23),
        AVLTreeNode({db_t(2)},5,{0},{0},{26},{0},{0},thisFormat, 25),
        AVLTreeNode({db_t(2)},7,{0},{0},{28},{0},{0},thisFormat, 27),
        AVLTreeNode({db_t(2)},9,{0},{0},{210},{0},{0},thisFormat, 29),

        AVLTreeNode({db_t(2)},15,{213},{217},{216},{2},{2},thisFormat,215 ), 
        AVLTreeNode({db_t(2)},13,{212},{214},{214},{1},{1},thisFormat, 213), 
        AVLTreeNode({db_t(2)},17,{216},{218},{218},{1},{1},thisFormat, 217),
        AVLTreeNode({db_t(2)},12,{0},{0},{213},{0},{0},thisFormat, 212),
        AVLTreeNode({db_t(2)},14,{0},{0},{215},{0},{0},thisFormat, 214),
        AVLTreeNode({db_t(2)},16,{0},{0},{217},{0},{0},thisFormat, 216),
        AVLTreeNode({db_t(2)},18,{0},{0},{0},{0},{0},thisFormat, 218),

    };

    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);
    tree->deleteEntry(2,4,0);

    ASSERT_EQ(tree->size(), nodes.size()-1);
    string expected= "Node[key:2-10, LH:3, RH:3]\n"\
                        "Node[key:2-6, LH:2, RH:2]\n"\
                        "Node[key:2-15, LH:2, RH:2]\n"\
                        "Node[key:2-5, LH:1, RH:0]\n"\
                        "Node[key:2-8, LH:1, RH:1]\n"\
                        "Node[key:2-13, LH:1, RH:1]\n"\
                        "Node[key:2-17, LH:1, RH:1]\n"\
                        "Node[key:2-3, LH:0, RH:0]\n"\
                        "Node[key:2-7, LH:0, RH:0]\n"\
                        "Node[key:2-9, LH:0, RH:0]\n"\
                        "Node[key:2-12, LH:0, RH:0]\n"\
                        "Node[key:2-14, LH:0, RH:0]\n"\
                        "Node[key:2-16, LH:0, RH:0]\n"\
                        "Node[key:2-18, LH:0, RH:0]\n";
    ASSERT_EQ(tree->toString(),expected);
}



TEST(Find, FindNodeNoDuplicates){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    //key, valueHAsh, data, lC, rC, lH,rH,
    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(5)},5,{35},{75},{65},{2},{2}, thisFormat,55),
        AVLTreeNode({db_t(3)},5,{25},{45},{45},{1},{1}, thisFormat,35),
        AVLTreeNode({db_t(7)},5,{65},{85},{85},{1},{1}, thisFormat,75),
        AVLTreeNode({db_t(2)},5,{0},{0},{25},{0},{0}, thisFormat,25),
        AVLTreeNode({db_t(4)},5,{0},{0},{55},{0},{0}, thisFormat,45),
        AVLTreeNode({db_t(6)},5,{0},{0},{65},{0},{0}, thisFormat,65),
        AVLTreeNode({db_t(8)},5,{0},{0},{0},{0},{0}, thisFormat,85)
    };

    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);

    auto[r_keys, r_dummy]=tree->findNode(3,5,0);
    auto[e_keys,e_dummy]=make_tuple<vector<db_t>,bool>({db_t(3)},false);

    ASSERT_TRUE(r_keys.size()==thisFormat.size());
    ostringstream oss_retrieved;
    ostringstream oss_expected;

    for(size_t i=0;i<thisFormat.size();i++){
        oss_retrieved<<DBT::toString(r_keys[i])<<" ";
        oss_expected<<DBT::toString(e_keys[i])<<" ";
    }
    ASSERT_EQ(oss_retrieved.str(), oss_expected.str());
    ASSERT_EQ(r_dummy,e_dummy);

}

TEST(Find, FindNodeNoDuplicates_MultiCols){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT,AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    //key, valueHAsh, data, lC, rC, lH,rH,
    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(5),db_t(10)}  ,5,{35,35},{75,75}, {65,65},{2,2},{2,2}, thisFormat,55),
        AVLTreeNode({db_t(3),db_t(6)}   ,5,{25,25},{45,45},{45,45},{1,1},{1,1},thisFormat,35),
        AVLTreeNode({db_t(7),db_t(14)}  ,5,{65,65},{85,85},{85,85},{1,1},{1,1}, thisFormat,75),
        AVLTreeNode({db_t(2),db_t(4)}   ,5,{0,0},{0,0},{35,35},{0,0},{0,0}, thisFormat,25),
        AVLTreeNode({db_t(4),db_t(8)}   ,5,{0,0},{0,0},{55,55},{0,0},{0,0}, thisFormat,45),
        AVLTreeNode({db_t(6),db_t(12)}  ,5,{0,0},{0,0},{75,75},{0,0},{0,0}, thisFormat,65),
        AVLTreeNode({db_t(8),db_t(16)}  ,5,{0,0},{0,0},{0,0},{0,0},{0,0}, thisFormat,85)
    };

    vector<ulong> root={nodes[0].nodeID,nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);

    auto[r_keys, r_dummy]=tree->findNode(3,5,0);
    auto[e_keys,e_dummy]=make_tuple<vector<db_t>,bool>({db_t(3), db_t(6)},false);

    ASSERT_TRUE(r_keys.size()==thisFormat.size());
    ostringstream oss_retrieved;
    ostringstream oss_expected;

    for(size_t i=0;i<thisFormat.size();i++){
        oss_retrieved<<DBT::toString(r_keys[i])<<" ";
        oss_expected<<DBT::toString(e_keys[i])<<" ";
    }
    ASSERT_EQ(oss_retrieved.str(), oss_expected.str());
    ASSERT_EQ(r_dummy,e_dummy);

}



TEST(Find, FindNodeWithDuplicates_MultiCols){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT,AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    //key, valueHAsh, data, lC, rC, lH,rH,lS,rS
    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(5),db_t(10)}  ,5,{53,53},{75,75}, {65,65},{2,2},{2,2}, thisFormat,55),
        AVLTreeNode({db_t(5),db_t(6)}   ,3,{52,52},{54,54},{54,54},{1,1},{1,1},thisFormat,53),
        AVLTreeNode({db_t(7),db_t(14)}  ,5,{65,65},{85,85},{85,85},{1,1},{1,1}, thisFormat,75),
        AVLTreeNode({db_t(5),db_t(4)}   ,2,{0,0},{0,0},{53,53},{0,0},{0,0},thisFormat,52),
        AVLTreeNode({db_t(5),db_t(8)}   ,4,{0,0},{0,0},{55,55},{0,0},{0,0}, thisFormat,54),
        AVLTreeNode({db_t(6),db_t(12)}  ,5,{0,0},{0,0},{75,75},{0,0},{0,0}, thisFormat,65),
        AVLTreeNode({db_t(8),db_t(16)}  ,5,{0,0},{0,0},{0,0},{0,0},{0,0}, thisFormat,85)
    };

    vector<ulong> root={nodes[0].nodeID,nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);

    auto[r_keys, r_dummy]=tree->findNode(5,2,0);
    auto[e_keys,e_dummy]=make_tuple(vector<db_t>{db_t(5),db_t(4)},false);

    ASSERT_TRUE(r_keys.size()==thisFormat.size());
    ostringstream oss_retrieved;
    ostringstream oss_expected;

    for(size_t i=0;i<thisFormat.size();i++){
        oss_retrieved<<DBT::toString(r_keys[i])<<" ";
        oss_expected<<DBT::toString(e_keys[i])<<" ";
    }
    ASSERT_EQ(oss_retrieved.str(), oss_expected.str());
    ASSERT_EQ(r_dummy,e_dummy);

}


TEST(Find, FindInterval){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT,AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    //key, valueHAsh, data, lC, rC, lH,rH,lS,rS
    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(5),db_t(55)}   ,5,{33,33},{77,77},{56,56},{2,2},{2,2},thisFormat,55),
        AVLTreeNode({db_t(3),db_t(33)}   ,3,{11,11},{54,54},{54,54},{1,1},{1,1}, thisFormat,33),
        AVLTreeNode({db_t(7),db_t(77)}   ,7,{56,56},{88,88},{88,88},{1,1},{1,1}, thisFormat,77),
        AVLTreeNode({db_t(1),db_t(11)}   ,1,{0,0},{0,0},{33,33},{0,0},{0,0},thisFormat,11),
        AVLTreeNode({db_t(5),db_t(54)}   ,4,{0,0},{0,0},{55,55},{0,0},{0,0}, thisFormat,54),
        AVLTreeNode({db_t(5),db_t(56)}   ,6,{0,0},{0,0},{77,77},{0,0},{0,0},thisFormat,56),
        AVLTreeNode({db_t(8),db_t(88)}   ,8,{0,0},{0,0},{0,0},{0,0},{0,0},thisFormat,88)
    };
    vector<ulong> root={nodes[0].nodeID,nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);   
    
    int estimate=2;
    DBT::dbResponse returned=tree->findIntervalMenhir(db_t(5),db_t(5),0ull,estimate);
    
    vector<vector<db_t>> expected;
    expected.push_back(nodes[0].key);
    expected.push_back(nodes[4].key);
    expected.push_back(nodes[5].key);
    size_t expected_real=expected.size(); 

    ASSERT_EQ(returned.size(), (expected.size()+estimate));

    int count_real=0;
    std::ostringstream oss;
    for (int i = 0; i < (int) returned.size(); i++){
        auto[keys, dummy]=returned[i];
        if(!dummy){
            count_real++;
            oss<<"[";
            for(size_t i=0;i<thisFormat.size();i++){
                oss<<DBT::toString(keys[i]);
                if(i!=thisFormat.size()-1) oss<<",";
            }
            oss<<"]\n";
        }
    }
    
    ASSERT_EQ(count_real, expected_real);

    //check if all expected are returned
    for (size_t i = 0; i < returned.size(); i++){
        auto[r,dummy]=returned[i];
        if(dummy)continue;

        bool exists=false;
        for(size_t ii=0;ii<expected.size();ii++){
            bool equals=true;
            for(size_t iii=0;iii<thisFormat.size();iii++){
                if(r[iii]!=expected[ii][iii])equals=false;
            }
            if (equals)exists=true;
        }

        std::ostringstream oss;
        oss<<"[";
        for(size_t j=0;j<thisFormat.size();j++){
            oss<<DBT::toString(r[j]);
            if(j!=thisFormat.size()-1) oss<<",";
        }
        oss<<"]\n";
        ASSERT_TRUE(exists)<<"Following returned value was not expected: "<<oss.str();
    }
}


TEST(Find, FindInterval_LargeEstimate){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT,AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    //key, valueHAsh, data, lC, rC, lH,rH,lS,rS
    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(5),db_t(55)}   ,5,{33,33},{77,77},{56,56},{2,2},{2,2}, thisFormat,55),
        AVLTreeNode({db_t(3),db_t(33)}   ,3,{11,11},{54,54},{54,54},{1,1},{1,1},thisFormat,33),
        AVLTreeNode({db_t(7),db_t(77)}   ,7,{56,56},{88,88},{88,88},{1,1},{1,1},thisFormat,77),
        AVLTreeNode({db_t(1),db_t(11)}   ,1,{0,0},{0,0},{33,33},{0,0},{0,0},thisFormat,11),
        AVLTreeNode({db_t(5),db_t(54)}   ,4,{0,0},{0,0},{55,55},{0,0},{0,0}, thisFormat,54),
        AVLTreeNode({db_t(5),db_t(56)}   ,6,{0,0},{0,0},{77,77},{0,0},{0,0}, thisFormat,56),
        AVLTreeNode({db_t(8),db_t(88)}   ,8,{0,0},{0,0},{0,0},{0,0},{0,0},thisFormat,88)
    };
    vector<ulong> root={nodes[0].nodeID,nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);   
    
    int estimate=7;
    DBT::dbResponse returned=tree->findIntervalMenhir(db_t(5),db_t(5),0ull,estimate);
    
    vector<vector<db_t>> expected;
    expected.push_back(nodes[0].key);
    expected.push_back(nodes[4].key);
    expected.push_back(nodes[5].key);
    size_t expected_real=expected.size(); 


    int count_real=0;
    std::ostringstream oss;
    for (int i = 0; i < (int) returned.size(); i++){
        auto[keys, dummy]=returned[i];
        if(!dummy){
            count_real++;
        }
        oss<<"[";
        for(size_t i=0;i<thisFormat.size();i++){
            oss<<DBT::toString(keys[i]);
            oss<<"(dummy:"<<dummy<<")";
            if(i!=thisFormat.size()-1) oss<<",";
        }
        oss<<"]\n";
        
    }

    ASSERT_EQ(returned.size(), (expected.size()+estimate))<<"Returned:\n"<<oss.str();
    ASSERT_EQ(count_real, expected_real)<<"Returned:\n"<<oss.str();

    //check if all expected are returned
    for (size_t i = 0; i < returned.size(); i++){
        auto[r,dummy]=returned[i];
        if(dummy)continue;

        bool exists=false;
        for(size_t ii=0;ii<expected.size();ii++){
            bool equals=true;
            for(size_t iii=0;iii<thisFormat.size();iii++){
                if(r[iii]!=expected[ii][iii])equals=false;
            }
            if (equals)exists=true;
        }

        std::ostringstream oss;
        oss<<"[";
        for(size_t j=0;j<thisFormat.size();j++){
            oss<<DBT::toString(r[j]);
            if(j!=thisFormat.size()-1) oss<<",";
        }
        oss<<"]\n";
        ASSERT_TRUE(exists)<<"Following returned value was not expected: "<<oss.str();
    }
}


/*                                  7-7
                    7-1                                         8-5

            6-4              7-4                        8-2                     8-8
        
    6-2         6-5       7-2       7-5             7-9        8-3          8-6           8-9 

6-1               6-6       7-3       7-6        7-8   8-1        8-4           8-7

*/
TEST(Find, FindInterval_IntervalTwoSubtrees){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{
        AVLTreeNode({db_t(7)},7,{71},{85},{78},{4},{4},  thisFormat, 77),

        AVLTreeNode({db_t(7)},1,{64},{74},{72},{3},{3}, thisFormat, 71),
        AVLTreeNode({db_t(8)},5,{82},{88},{86},{3},{3},  thisFormat, 85),
        
        AVLTreeNode({db_t(6)},4,{62},{65},{65},{2},{2},  thisFormat, 64),
        AVLTreeNode({db_t(7)},4,{72},{75},{75},{2},{2},  thisFormat, 74),
        AVLTreeNode({db_t(8)},2,{79},{83},{83},{2},{2},  thisFormat, 82),
        AVLTreeNode({db_t(8)},8,{86},{89},{89},{2},{1},thisFormat, 88),

        AVLTreeNode({db_t(6)},2,{61},{0},{64},{1},{0},  thisFormat, 62),
        AVLTreeNode({db_t(6)},5,{0},{66},{66},{0},{1},thisFormat, 65),
        AVLTreeNode({db_t(7)},2,{0},{73},{73},{0},{1},thisFormat, 72), 
        AVLTreeNode({db_t(7)},5,{0},{76},{76},{0},{1},thisFormat, 75), 
        AVLTreeNode({db_t(7)},9,{78},{81},{81},{1},{1}, thisFormat, 79),
        AVLTreeNode({db_t(8)},3,{0},{84},{84},{0},{1},thisFormat, 83),
        AVLTreeNode({db_t(8)},6,{0},{87},{87},{0},{1},thisFormat, 86),
        AVLTreeNode({db_t(8)},9,{0},{0},{0},{0},{0},thisFormat, 89),


        AVLTreeNode({db_t(6)},1,{0},{0},{62},{0},{0},thisFormat, 61),
        AVLTreeNode({db_t(6)},6,{0},{0},{71},{0},{0},thisFormat, 66),
        AVLTreeNode({db_t(7)},3,{0},{0},{74},{0},{0},thisFormat, 73),        
        AVLTreeNode({db_t(7)},6,{0},{0},{77},{0},{0},thisFormat, 76),
        AVLTreeNode({db_t(7)},8,{0},{0},{79},{0},{0},thisFormat, 78),
        AVLTreeNode({db_t(8)},1,{0},{0},{82},{0},{0},thisFormat, 81),
        AVLTreeNode({db_t(8)},4,{0},{0},{85},{0},{0},thisFormat, 84),
        AVLTreeNode({db_t(8)},7,{0},{0},{88},{0},{0},thisFormat, 87),
    };
    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);   
    
    int estimate=2;
    db_t lower=db_t(7);
    db_t upper=db_t(7);
    DBT::dbResponse returned=tree->findIntervalMenhir(lower,upper,0ull,estimate);
    
    vector<vector<db_t>> expected;
    for(size_t j=0;j<nodes.size();j++){
        if(nodes[j].key[0]>=lower and nodes[j].key[0] <= upper){
            expected.push_back(nodes[j].key);
        }
    }
    size_t expected_real=expected.size(); 

    ASSERT_EQ(returned.size(), (expected.size()+estimate));

    int count_real=0;
    std::ostringstream oss;
    for (int i = 0; i < (int) returned.size(); i++){
        auto[keys, dummy]=returned[i];
        if(!dummy){
            count_real++;
            oss<<"[";
            for(size_t i=0;i<thisFormat.size();i++){
                oss<<DBT::toString(keys[i]);
                if(i!=thisFormat.size()-1) oss<<",";
            }
            oss<<"]\n";
        }
    }
    
    ASSERT_EQ(count_real, expected_real);

    //check if all expected are returned
    for (size_t i = 0; i < returned.size(); i++){
        auto[r,dummy]=returned[i];
        if(dummy)continue;

        bool exists=false;
        for(size_t ii=0;ii<expected.size();ii++){
            bool equals=true;
            for(size_t iii=0;iii<thisFormat.size();iii++){
                if(r[iii]!=expected[ii][iii])equals=false;
            }
            if (equals)exists=true;
        }

        std::ostringstream oss;
        oss<<"[";
        for(size_t j=0;j<thisFormat.size();j++){
            oss<<DBT::toString(r[j]);
            if(j!=thisFormat.size()-1) oss<<",";
        }
        oss<<"]\n";
        ASSERT_TRUE(exists)<<"Following returned value was not expected: "<<oss.str();
    }
}


/*                                 
                    7-0                                   

            6-4              8-4                      
        
    6-2         6-5       7-2       8-5           

6-1               6-6    7-1   7-3       8-6      
*/
TEST(Find, FindInterval_StartWithInternalNode){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;
    vector<AType> thisFormat {AType::INT};
    number capacity=300;
    size_t sizeValue=0;    
	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue,capacity);

    vector<AVLTreeNode> nodes{

        AVLTreeNode({db_t(7)},0,{64},{84},{71},{3},{3},thisFormat, 70),
        
        AVLTreeNode({db_t(6)},4,{62},{65},{65},{2},{2},  thisFormat, 64),
        AVLTreeNode({db_t(8)},4,{72},{85},{85},{2},{2},  thisFormat, 84),

        AVLTreeNode({db_t(6)},2,{61},{0},{64},{1},{0}, thisFormat, 62),
        AVLTreeNode({db_t(6)},5,{0},{66},{66},{0},{1},thisFormat, 65),
        AVLTreeNode({db_t(7)},2,{71},{73},{73},{1},{1},thisFormat, 72), 
        AVLTreeNode({db_t(8)},5,{0},{86},{86},{0},{1},thisFormat, 85), 

        AVLTreeNode({db_t(6)},1,{0},{0},{62},{0},{0},thisFormat, 61),
        AVLTreeNode({db_t(6)},6,{0},{0},{70},{0},{0},thisFormat, 66),
        AVLTreeNode({db_t(7)},1,{0},{0},{72},{0},{0},thisFormat, 71),        
        AVLTreeNode({db_t(7)},3,{0},{0},{84},{0},{0},thisFormat, 73),        
        AVLTreeNode({db_t(8)},6,{0},{0},{0},{0},{0},thisFormat, 86),

    };
    vector<ulong> root={nodes[0].nodeID};
    tree->putTreeInORAM(nodes,root);   
    
    int estimate=2;
    db_t lower=db_t(7);
    db_t upper=db_t(7);
    DBT::dbResponse returned=tree->findIntervalMenhir(lower,upper,0ull,estimate);
    
    vector<vector<db_t>> expected;
    for(size_t j=0;j<nodes.size();j++){
        if(nodes[j].key[0]>=lower and nodes[j].key[0] <= upper){
            expected.push_back(nodes[j].key);
        }
    }
    size_t expected_real=expected.size(); 

    ASSERT_EQ(returned.size(), (expected.size()+estimate));

    int count_real=0;
    std::ostringstream oss;
    for (int i = 0; i < (int) returned.size(); i++){
        auto[keys, dummy]=returned[i];
        if(!dummy){
            count_real++;
            oss<<"[";
            for(size_t i=0;i<thisFormat.size();i++){
                oss<<DBT::toString(keys[i]);
                if(i!=thisFormat.size()-1) oss<<",";
            }
            oss<<"]\n";
        }
    }
    
    ASSERT_EQ(count_real, expected_real);

    //check if all expected are returned
    for (size_t i = 0; i < returned.size(); i++){
        auto[r,dummy]=returned[i];
        if(dummy)continue;

        bool exists=false;
        for(size_t ii=0;ii<expected.size();ii++){
            bool equals=true;
            for(size_t iii=0;iii<thisFormat.size();iii++){
                if(r[iii]!=expected[ii][iii])equals=false;
            }
            if (equals)exists=true;
        }

        std::ostringstream oss;
        oss<<"[";
        for(size_t j=0;j<thisFormat.size();j++){
            oss<<DBT::toString(r[j]);
            if(j!=thisFormat.size()-1) oss<<",";
        }
        oss<<"]\n";
        ASSERT_TRUE(exists)<<"Following returned value was not expected: "<<oss.str();
    }
}



TEST(Find, FindInterval_Randomized){
    extern LOG_LEVEL CURRENT_LEVEL;
    CURRENT_LEVEL=WARNING;

    number capacity=30;
    number logcapacity=10;
    number lower=120;
    size_t repetitions= 3;
    number numQueries=5;
    vector<AType> thisFormat {AType::INT};
    size_t sizeValue=0;

    for(size_t seed=0;seed<repetitions;seed++){
        LOG(INFO,boost::wformat(L"repetition %d/%d") %seed %repetitions);
        std::mt19937 rng(seed);
        std::uniform_int_distribution<std::mt19937::result_type> dist1(lower,capacity);
        int numDatapoints=dist1(rng)%(capacity-10);
        LOG(INFO,boost::wformat(L"numDatapoints %d") %numDatapoints);


        std::uniform_int_distribution<std::mt19937::result_type> dist2(0,100);
        vector<int> inputData_temp;
        for(int i=0; i<numDatapoints;i++){
            inputData_temp.push_back(dist2(rng));
        }
        std::sort (inputData_temp.begin(), inputData_temp.end());           
        vector<vector<db_t>> inputData;
        for(int i=0; i<numDatapoints;i++){
            inputData.push_back({db_t(inputData_temp[i])});
        }

        INPUT_DATA=inputData;
	    //loadTreeBulkNonObliv(inputData.size(),tree);
    	AVLTree *tree=new DOSM::AVLTree(thisFormat,sizeValue, logcapacity, ORAM_Z,  STASH_FACTOR, BATCH_SIZE, &INPUT_DATA, INPUT_DATA.size(),USE_ORAM);


        std::uniform_int_distribution<std::mt19937::result_type> dist3(0,numDatapoints-1);
        std::uniform_int_distribution<std::mt19937::result_type> dist4(1,numDatapoints/2); //estimate can not be smaller than 1

        for(size_t i=0;i<numQueries;i++){
            LOG(INFO,boost::wformat(L"query %d/%d") %i %numQueries);

            db_t lower=inputData[dist3(rng)][0];  
            db_t upper=inputData[dist3(rng)][0];  
            if(upper<lower)swap(lower,upper);
            int estimate=dist4(rng);
            LOG_LEVEL before=CURRENT_LEVEL;
            if(seed==repetitions+1 and i==0) CURRENT_LEVEL=TRACE;
            DBT::dbResponse returned=tree->findIntervalMenhir(lower,upper,0ull,estimate);
            CURRENT_LEVEL=before;


            vector<vector<db_t>> expected;
            for(int j=0;j<numDatapoints;j++){
                if(inputData[j][0]>=lower and inputData[j][0] <= upper){
                    expected.push_back(inputData[j]);
                }
            }


            int count_real=0;
            std::ostringstream oss;
            oss<<"Seed: "<<seed<<", Query Number:"<<i<<endl;
            oss<<"Returned (num:"<<returned.size()<<"): ";
            for (int ii = 0; ii < (int) returned.size(); ii++){
                auto[keys, dummy]=returned[ii];
                if(!dummy){
                    count_real++;
                }
                oss<<"[";
                for(size_t iii=0;iii<thisFormat.size();iii++){
                    oss<<DBT::toString(keys[iii]);
                    if(dummy) oss<<" (dummy:"<<dummy<<")";
                    if(iii!=thisFormat.size()-1) oss<<",";
                    
                    oss<<"]";
                    if(ii!=returned.size()-1) oss<<",";
                }
            }
            oss<< "\nExpected (num:"<<expected.size()<<"): ";
            for (int ii = 0; ii < (int) expected.size(); ii++){
                auto keys=expected[ii];
                oss<<"[";
                for(size_t iii=0;iii<thisFormat.size();iii++){
                    oss<<DBT::toString(keys[iii]);
                    if(iii!=thisFormat.size()-1) oss<<",";
                    oss<<"]";
                    if(ii!=expected.size()-1) oss<<",";

                }
            }

        
            ASSERT_EQ(returned.size(), (expected.size()+estimate))<<oss.str()<< tree->print(CURRENT_LEVEL,tree->getRoots()[0],false,0);
            ASSERT_EQ(count_real, expected.size())<<oss.str()<< tree->print(CURRENT_LEVEL,tree->getRoots()[0],false,0);

        }
    }
}



int main(int argc, char ** argv) {
    //testing::InitGoogleMock(&__argc, __argv);
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();

}