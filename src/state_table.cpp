#include "state_table.hpp"
#include "definitions.h"
#include "globals.hpp"
#include "utility.hpp"

/**
 * @brief Functions for managing the state machine of the server when collecting data from the crowd. 
 * 
 */
using namespace std; 
namespace STATE_MACHINE{

    STATE SERVER_STATE = STATE::INACTIVE;

    string toString(STATE state){
        switch (state)
        {
            case STATE::INACTIVE : return "INACTIVE" ;
            case STATE::DATA_COLLECTION_CROWD: return "DATA_COLLECTION_CROWD";
            case STATE::QUERYING_INTERACTIV: return "QUERYING_INTERACTIV";
            case STATE::DATA_COLLECTION_OTHER: return "DATA_COLLECTION_OTHER";
            case STATE::QUERYING_AUTOMATED:return "QUERYING_AUTOMATED";
            case STATE::CLEANUP: return "CLEANUP";
            // omit default case to trigger compiler warning for missing cases
        };
        return "";
    }

    STATE transitionServerState(unsigned int transition){

        Table_Entry const *  p_entry = tableBegin(); // pointer to constant table entry
        Table_Entry const * const  p_table_end =  tableEnd(); //constant pointer to constant table entry
        bool state_found = false;
        while ((!state_found) && (p_entry != p_table_end))
        {
            if (p_entry->current_state == SERVER_STATE)
            {
                //Tranistion 0 is a catch all
                if (p_entry->transition_letter == transition or p_entry->transition_letter==0)
                {
                    LOG(INFO, boost::wformat(L"Server  transitioned from old state %d via transition edge %d to new state %d" ) 
                        % SERVER_STATE 
                        % transition
                        % p_entry->next_state);  
                    SERVER_STATE = p_entry->next_state;
                    state_found = true;
                    break;
                }
            }
            ++p_entry;
        }
        if (!state_found)
        {
            LOG(DEBUG, boost::wformat(L"Did NOT transition server state for current state %d and transition edge %d") 
                % SERVER_STATE 
                % transition);    
        }
        return SERVER_STATE;
    }

    Table_Entry const * tableBegin(){
        return &serverState[0];
    }


    Table_Entry const * tableEnd(){
        return &serverState[TABLE_SIZE];
    }  
}