#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tvpnetposa_protocol.h"
#include "tvpnetposa_register.h"
#include "commonfuncs.h"

//tvpnetposa heartbeate protocol decode
int tvpnetposa_heartbeate_decode(char *ac_data, int ai_lens)
{
	int li_i = 0;
	int li_lens = 0;
	int li_ret = 0;
	char lc_tmp[15] = {0};

	while(ai_lens--)
	{
		li_lens = strlen("<Message CurrentDateTime=\"");
		if(strncmp(ac_data+li_i, "<Message CurrentDateTime=\"", li_lens) == 0)
		{
			strncpy(lc_tmp, ac_data+li_i+li_lens, 14);

			TRACE_LOG_SYSTEM("gstr_tvpnetposa_register.current_time=%s", lc_tmp);

			li_ret = 1;

			break;
		}

		li_i++;
	}

	return li_ret;
}


//tvpnetposa register protocol decode
int tvpnetposa_register_decode(char *ac_data, int ai_lens)
{
	int li_i = 0;
	int li_lens = 0;
	int li_ret = 0;
	char lc_tmp[12] = {0};

	while(ai_lens--)
	{
		li_lens = strlen("<Message Code=");
		if(strncmp(ac_data+li_i, "<Message Code=", li_lens) == 0)
		{
			if(strncmp(ac_data+li_i+li_lens, "\"0\"", strlen("\"0\"")) == 0)
			{
				li_ret = 1;
			}
		}

		li_lens = strlen("<heartCycle>");
		if(strncmp(ac_data+li_i, "<HeartCycle>", li_lens) == 0)
		{
			strncpy(lc_tmp, ac_data+li_i+li_lens, 2);
			
			gstr_tvpnetposa_register.heart_cycle = atoi(lc_tmp);

			TRACE_LOG_SYSTEM("gstr_tvpnetposa_register.heart_cycle=%s", lc_tmp);

			break;
		}

		li_i++;
	}

	return li_ret;
}


//tvpnetposa register protocol encode
int tvpnetposa_register_encode(char *oc_data, int ai_type)
{
	char lc_comment[512] = {0};
	int  li_comment_lens = 0;
	
	sprintf(lc_comment, "deviceIp=%s&msgPort=%d&company=%s&videoRegFlag=False", gstr_tvpnetposa_register.ip, \
		    gstr_tvpnetposa_register.port, gstr_tvpnetposa_register.company);
	
	li_comment_lens = strlen(lc_comment);
	
	sprintf(oc_data, "POST /toll-gate/home/regist?deviceId=12345678 HTTP/1.1\r\n"\
			"Content-Length: %d\r\n"\
			"Pragma: no-cache\r\n"\
			"Cache-Control: no-cache\r\n"\
			"Accept: text/html\r\n"\
			"Connection : Keep-alive\r\n\r\n", li_comment_lens);

	strcat(oc_data, lc_comment);
	
	return 0;
}

//tvpnetposa heartbeate protocol encode
int tvpnetposa_heartbeate_encode(char *oc_data, int ai_type)
{
	char lc_comment[512] = {0};
	int  li_comment_lens = 0;

	sprintf(lc_comment, "<Message>\r\n"\
					"<Device StateCode=\"1\">\r\n"\
					"<Status Name=\"Detector\" Value=\"1\" />\r\n"\
					"<Status Name=\"Flash\" Value=\"1\" />\r\n"\
					"<Status Name=\"Video\"  Value=\"1\" />\r\n"\
					"</Device>\r\n"\
					"</Message>\r\n");
	
	li_comment_lens = strlen(lc_comment);
	
	sprintf(oc_data, "POST /toll-gate/home/heartBeate?deviceId=12345678 HTTP/1.1\r\n"\
			"Content-Length: %d\r\n"\
			"Pragma: no-cache\r\n"\
			"Cache-Control: no-cache\r\n"\
			"Accept: text/html\r\n"\
			"Connection : Keep-alive\r\n\r\n", li_comment_lens);

	strcat(oc_data, lc_comment);
	
	return 0;
}





 
