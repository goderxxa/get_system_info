#pragma once



#include <string>
#include <iostream>
#include <windows.h>
#include <iphlpapi.h>
#include <sstream>

namespace internal
{
	namespace detail
    {
		class Fingerprint
                {
		public:
			std::string getBoardSerial() ;
			std::string getCPUSerial() ;
			std::string getMAC() ;

		};
	}
}
