#include "diag.h"
#include <iostream>
#include "logger.h"


namespace llae {

	void report_diag_error(const char* msg,const char* file,int line) {
		std::string message = "Error: " + std::string(msg) + " at " + std::string(file) + ":" + std::to_string(line);
		log::write(log::level::error,message);
	}
}