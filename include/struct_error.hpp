#pragma once 

#include "definitions.h"

/**
 * @brief Class defining Error object. For passing errors between functions.
 * 
 */

class Error{
	public:
		int code;
		wstring err_string;

	Error(){
		code=0;
		err_string=L"";
	}

	Error(wstring err){
		code=1;
		err_string= err;
	}
		
	Error(boost::wformat message){
		err_string= boost::str(message);
	}

	Error(int c, wstring err){
		code=c;
		err_string=err;
	}

	bool is_err(){
		if (code>0)return true;
		return false;
	}

};
	

