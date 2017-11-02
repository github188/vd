#include "/home/bitcom/IPNC_RDK_V3.8.0/Source/ipnc_rdk_kk/ipnc_app/vd/lib/Json/include/json/json.h"
#include <string>
#include <iostream>
int main()
{
	Json::Value root;
	Json::FastWriter fast_writer;	
	root["REGION_ID"]="600901";
	root["DATA_TOTAL_NUM"]="222222";
	std::cout<<fast_writer.write(root)<<std::endl;
	return 0;
}
