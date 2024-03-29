#pragma once

#include "definitions.h"

/**
 * @brief Functions for managing the state machine of the server when collecting data from the crowd. 
 *  State 0 -> server inactive
 * State 1 -> Data Collection Phase /Data generation Phase
 * State 2 -> Querying Phase
 * State 3 -> Cleaning up and Writing Output
*/

namespace STATE_MACHINE{


    enum STATE{
            INACTIVE , 
            DATA_COLLECTION_CROWD , 
            QUERYING_INTERACTIV , 
            DATA_COLLECTION_OTHER , 
            QUERYING_AUTOMATED , 
            CLEANUP
    };

    struct Table_Entry
    {
        STATE  current_state;
        unsigned int transition_letter;
        STATE  next_state;
    };

    string toString(STATE state);
    Table_Entry const * tableBegin();
    Table_Entry const * tableEnd();
    STATE transitionServerState(unsigned int transition);


    //Transition 0 is a catch all transition
    static const Table_Entry serverState[] =
    {
        //  Current   Transition     Next
        //  State ID    Letter     State ID
        {    INACTIVE,                101,    DATA_COLLECTION_OTHER}, //starting to generate data synthetically or read from file
        {    INACTIVE,                102,    DATA_COLLECTION_CROWD}, 
        {    DATA_COLLECTION_OTHER,   201,    QUERYING_AUTOMATED}, // starting to run QUERIES(generated or read from file) on data (generated of read from file)
        {    DATA_COLLECTION_OTHER,   203,    QUERYING_INTERACTIV}, // Interactive querying on synthetic or from-file data
        {    DATA_COLLECTION_CROWD,   301,    QUERYING_INTERACTIV}, // Interactive querying on synthetic or from-file data
        {    QUERYING_AUTOMATED,      401,    CLEANUP}, // finishing query phase after using generated  QUERIES
        {    QUERYING_INTERACTIV,     501,    CLEANUP}, 

    };

    static const unsigned int  TABLE_SIZE =  sizeof(serverState) / sizeof(serverState[0]);

}
