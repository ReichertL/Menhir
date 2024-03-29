#include "struct_querying.hpp"		

namespace MENHIR{

    string toString(AggregateFunc func){
        switch (func)
        {
            case AggregateFunc::SUM : return "SUM" ;
            case AggregateFunc::MEAN: return "MEAN";
            case AggregateFunc::VARIANCE: return "VARIANCE";
            case AggregateFunc::COUNT: return "COUNT";
            case AggregateFunc::MAX_COUNT:return "MAX_COUNT";
            case AggregateFunc::MIN_COUNT: return "MIN_COUNT";
            // omit default case to trigger compiler warning for missing cases
        };
        return "";
    }

    int toInt(AggregateFunc func){
        switch (func)
        {
            case AggregateFunc::SUM : return 0 ;
            case AggregateFunc::MEAN: return 1;
            case AggregateFunc::VARIANCE: return 2;
            case AggregateFunc::COUNT: return 3;
            case AggregateFunc::MAX_COUNT:return 4;
            case AggregateFunc::MIN_COUNT: return 5;
            // omit default case to trigger compiler warning for missing cases
        };
        return 6;
    }

    AggregateFunc aggregateFuncFromString(string aggregateFuncString){
        AggregateFunc selected=AggregateFunc::AggregateFunc_INVALID; 
    	if (aggregateFuncString=="SUM"){ selected=AggregateFunc::SUM;
        }else if(aggregateFuncString =="MEAN"){ selected=AggregateFunc::MEAN;
        }else if(aggregateFuncString =="VARIANCE"){ selected=AggregateFunc::VARIANCE;
        }else if(aggregateFuncString =="COUNT"){ selected=AggregateFunc::COUNT;
        }else if(aggregateFuncString =="MAX_COUNT"){ selected=AggregateFunc::MAX_COUNT;
        }else if(aggregateFuncString =="MIN_COUNT"){ selected=AggregateFunc::MIN_COUNT;}
		return selected;
    }
}
    