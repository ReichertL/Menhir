#pragma once 

#include "definitions.h"
#include "database_type.hpp"

/**
 * @brief Class defining VolumeSanitizer object. Contains volume sanitizer tree.
 * 
 */

namespace MENHIR{


class VolumeSanitizer{
	private:
		number oram_number;
	public:
		double dp_buckets;
		double dp_levels;
		double dp_domain;
		double dp_alpha;
		//map<pair<number, number>, number>
		npmap  noises;



		VolumeSanitizer(number a, double b, double c, double d, double h){
			oram_number=a;
			dp_buckets =b;
			dp_levels =c;
			dp_domain =d;
			dp_alpha=h;
		}
};
}