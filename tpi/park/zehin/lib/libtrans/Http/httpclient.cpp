#include "httpclient.h"

#include <string>
#include <stdexcept>
#include"curl/curl.h"

#ifdef WIN32
#pragma comment(lib,"./lib/libcurl.lib")
#endif


static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid)
{
    if (NULL == lpVoid) {
        return -1;
    }

    std::string* str = dynamic_cast<std::string*>((std::string *)lpVoid);
    if( NULL == str || NULL == buffer )
    {
        return -1;
    }

    char* pData = (char*)buffer;
    str->append(pData, size * nmemb);
    return nmemb;
}


int CHttpClient::http_post_data(const char *url, const char* filename, const char *data, size_t size, std::string & strResponse)
{
	CURL *curl = NULL;
	CURLcode res;

	struct curl_httppost *post=NULL;
	struct curl_httppost *last=NULL;

	try
	{
		printf("URL: %s\n", url);

        CURLFORMcode ret = curl_formadd(&post, &last,
                             CURLFORM_COPYNAME, "file[upfile]",
                             CURLFORM_BUFFER, filename,
                             CURLFORM_CONTENTTYPE, "image/jpeg",
                             CURLFORM_BUFFERPTR, data,
                             CURLFORM_BUFFERLENGTH, size,
                             CURLFORM_END);
		/* Add simple file section */
        if(ret != CURL_FORMADD_OK)
		{
			fprintf(stderr, "curl_formadd error.\n");
			return ret;
		}

		/* Fill in the submit field too, even if this is rarely needed */
		//curl_formadd(&post, &last,
		//	CURLFORM_COPYNAME, "submit",
		//	CURLFORM_COPYCONTENTS, "OK",
		//	CURLFORM_END);

		curl = curl_easy_init();
		if(curl == NULL)
		{
			fprintf(stderr, "curl_easy_init() error.\n");
			return CURLE_FAILED_INIT;
		}

		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl, CURLOPT_URL, url); /*Set URL*/
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		int timeout = 10;
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        // disable Expection header
#if 0
        struct curl_slist * headers = 0;
        headers = curl_slist_append(headers, "Expect:");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
#endif

		res = curl_easy_perform(curl);
#if 0
        curl_slist_free_all(headers);
#endif
		if(res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform[%d] error.\n", res);
            curl_easy_cleanup(curl);
            curl_formfree(post);

			return res;
		}

        curl_easy_cleanup(curl);
        curl_formfree(post);
		return CURLE_OK;
	}
    catch (exception &e)
    {
        printf("%s\n", e.what());
		curl_easy_cleanup(curl);
		curl_formfree(post);
        return 99;
    }
}

int CHttpClient::http_post_file(const char *url, const char *filename, std::string & strResponse)
{
	CURL *curl = NULL;
	CURLcode res;

	struct curl_httppost *post=NULL;
	struct curl_httppost *last=NULL;

	try
	{
		if(filename == NULL || url == NULL)
			return -1;

		printf("URL: %s\n", url);
		printf("filename: %s\n", filename);

		/* Add simple file section */
		CURLFORMcode ret = curl_formadd(&post, &last, CURLFORM_COPYNAME, "files[upfile]",//files[upfile]
			CURLFORM_FILE, filename, CURLFORM_END);
			if(ret != CURL_FORMADD_OK)
		{
			fprintf(stderr, "curl_formadd error.\n");
			return ret;
		}

		/* Fill in the submit field too, even if this is rarely needed */
		//curl_formadd(&post, &last,
		//	CURLFORM_COPYNAME, "submit",
		//	CURLFORM_COPYCONTENTS, "OK",
		//	CURLFORM_END);

		curl = curl_easy_init();
		if(curl == NULL)
		{
			fprintf(stderr, "curl_easy_init() error.\n");
			return CURLE_FAILED_INIT;
		}

		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl, CURLOPT_URL, url); /*Set URL*/
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		int timeout = 10;
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);


		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
        // disable Expection header
        struct curl_slist * headers = 0;
        headers = curl_slist_append(headers, "Expect:");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
		if(res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform[%d] error.\n", res);
            curl_easy_cleanup(curl);
            curl_formfree(post);

			return res;
		}

        curl_easy_cleanup(curl);
        curl_formfree(post);
		return CURLE_OK;
	}
    catch (exception &e)
    {
        printf("%s\n", e.what());
		curl_easy_cleanup(curl);
		curl_formfree(post);
        return 99;
    }
}
//-------------------------------------------------------


CHttpClient::CHttpClient():
    m_bDebug(false)
{
}

int CHttpClient::Init()
{
	CURLcode code;
	code = curl_global_init(CURL_GLOBAL_ALL);
	if (code != CURLE_OK)
	{
		return 0;
	}
	return 1;
}
int CHttpClient::UnInit()
{
	curl_global_cleanup();
	return 1;
}



static int OnDebug(CURL *, curl_infotype itype, char * pData, size_t size, void *)
{
    if(itype == CURLINFO_TEXT)
    {
        //printf("[TEXT]%s\n", pData);
    }
    else if(itype == CURLINFO_HEADER_IN)
    {
        printf("[HEADER_IN]%s\n", pData);
    }
    else if(itype == CURLINFO_HEADER_OUT)
    {
        printf("[HEADER_OUT]%s\n", pData);
    }
    else if(itype == CURLINFO_DATA_IN)
    {
        printf("[DATA_IN]%s\n", pData);
    }
    else if(itype == CURLINFO_DATA_OUT)
    {
        printf("[DATA_OUT]%s\n", pData);
    }
    return 0;
}



int CHttpClient::Post(const std::string & strUrl, const std::string & strPost, std::string & strResponse)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    if(NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }
    if(m_bDebug)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }
    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPost.c_str());
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}


size_t read_data(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	return fread(buffer, size, nmemb, (FILE *)user_p);
}

int CHttpClient::Get(const std::string & strUrl, std::string & strResponse)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    if(NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }
    if(m_bDebug)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }
    //<pre name="code" class="cpp">
    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
    /**
    * 当多个线程都使用超时处理的时候，同时主线程中有sleep或是wait等操作。
    * 如果不设置这个选项，libcurl将会发信号打断这个wait从而导致程序退出。
    */
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}

int CHttpClient::Posts(const std::string & strUrl, const std::string & strPost, std::string & strResponse, const char * pCaPath)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    if(NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }
    if(m_bDebug)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }
    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strPost.c_str());
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    if(NULL == pCaPath)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    }
    else
    {
        //缺省情况就是PEM，所以无需设置，另外支持DER
        //curl_easy_setopt(curl,CURLOPT_SSLCERTTYPE,"PEM");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
        curl_easy_setopt(curl, CURLOPT_CAINFO, pCaPath);
    }
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}

int CHttpClient::Gets(const std::string & strUrl, std::string & strResponse, const char * pCaPath)
{
    CURLcode res;
    CURL* curl = curl_easy_init();
    if(NULL == curl)
    {
        return CURLE_FAILED_INIT;
    }
    if(m_bDebug)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }
    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&strResponse);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    if(NULL == pCaPath)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
    }
    else
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
        curl_easy_setopt(curl, CURLOPT_CAINFO, pCaPath);
    }
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return res;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CHttpClient::SetDebug(bool bDebug)
{
    m_bDebug = bDebug;
}
