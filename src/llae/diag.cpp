#include "diag.h"
#include <iostream>


namespace llae {

	void report_diag_error(const char* msg,const char* file,int line) {
		std::cerr << "Error: " << msg << " at " << file << ":" << line << std::endl;
	}
}