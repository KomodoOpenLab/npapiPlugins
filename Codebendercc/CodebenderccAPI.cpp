/**********************************************************\

  Auto-generated CodebenderccAPI.cpp

\**********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef ISWIN
#include <SDKDDKVer.h>
#include "dirent.h"
#include <windows.h>
#include <tchar.h>
#else
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string/predicate.hpp>
#include "JSObject.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "global/config.h"
#include "CodebenderccAPI.h"
#include "src/3rdParty/boost/boost/thread/detail/thread.hpp"

FB::variant CodebenderccAPI::download() {
    std::string os = getPlugin().get()->getOS();
    std::string url = sitebase+os+".avrdude";
	std::string confurl = sitebase+os+".avrdude.conf?1";
		
	if (os == "Windows") {	
		std::string dllurl = sitebase+"libusb0.dll";
		if (!fileExists(libusb)) {
			getURL(dllurl, libusb);
		}
	}
	if (os == "X11") {		
		url = sitebase+os+".32.avrdude";
		confurl = sitebase+os+".32.avrdude.conf";			
	}
	
    if (!fileExists(avrdude)) {
        getURL(url, avrdude);
	}    
    if (!fileExists(avrdudeConf)) {
        getURL(confurl, avrdudeConf);
	}
    return sitebase;
}

FB::variant CodebenderccAPI::flash(const std::string& device, const std::string& code, const std::string& maxsize, const std::string& protocol, const std::string& speed, const std::string& mcu) {
 #ifdef ISWIN
	//_chmod(avrdude.c_str(), _S_IREAD|_S_IWRITE|0x00400000);	
#else
	chmod(avrdude.c_str(), S_IRWXU);
#endif
    unsigned char buffer [50000];
    size_t size = base64_decode(code.c_str(), buffer, 50000);
    saveToBin(buffer, size);

    std::string command = "\"" + avrdude + "\" "
            + " -C\"" + avrdudeConf + "\""
            + " -P " + device
            + " -p " + mcu
            + " -u -U flash:w:\"" + binFile + "\":a"
            + " -c " + protocol
            + " -b " + speed
            + " -F 2> "
            + "\"" + outfile + "\"";
	
	if (getPlugin().get()->getOS()=="Windows"){
		command="\""+command+"\"";
	}

    int retVal = system(command.c_str());
    perror(command.c_str());
    return retVal;
}

FB::variant CodebenderccAPI::probeUSB() {
#ifdef ISWIN
	return probeUSB_win();
#else
    DIR *dp;
    std::string dirs = "";
    struct dirent *ep;
    dp = opendir("/dev/");
    if (dp != NULL) {
        while (ep = readdir(dp)) {
            //UNIX ARDUINO PORTS
            if (boost::contains(ep->d_name, "ttyACM") || boost::contains(ep->d_name, "ttyUSB")) {
                dirs += "/dev/";
                dirs += ep->d_name;
                dirs += ",";
            } else if (boost::contains(ep->d_name, "cu.")) {
                dirs += "/dev/";
                dirs += ep->d_name;
                dirs += ",";
            }
        }
        (void) closedir(dp);
    } else
        perror("Couldn't open the directory");

    return dirs;
#endif
}

bool CodebenderccAPI::fileExists(const std::string& filename) {
	return false;
    FILE *pFile;
    pFile = fopen(filename.c_str(), "r");
    if (pFile == NULL) {
        return false;
    } else {
        fclose(pFile);
        return true;
    }
}

void CodebenderccAPI::getURL(const std::string& url, const std::string& destination) {
    FB::SimpleStreamHelper::AsyncGet(
            m_host,
            FB::URI::fromString(url),
            boost::bind(&CodebenderccAPI::getURLCallback, this, _1, _2, _3, _4, destination)
            ,false);
}


///////////////////////////////////////////////////////////////////////////////
/// @fn CodebenderccPtr CodebenderccAPI::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////

CodebenderccPtr CodebenderccAPI::getPlugin() {
    CodebenderccPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
    return plugin;
}

// Read-only property version

std::string CodebenderccAPI::get_version() {
    return FBSTRING_PLUGIN_VERSION;
}

FB::variant CodebenderccAPI::getFlashResult() {
    FILE *pFile;
    pFile = fopen(outfile.c_str(), "r");
    char buffer[128];
    std::string result = "";
    while (!feof(pFile)) {
        if (fgets(buffer, 128, pFile) != NULL)
            result += buffer;
    }
    fclose(pFile);
    return result;
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////PRIVATE////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void CodebenderccAPI::getURLCallback(bool success,
        const FB::HeaderMap& headers, const boost::shared_array<uint8_t>& data, const size_t size, const std::string& destination) {
    FB::VariantMap outHeaders;
    for (FB::HeaderMap::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        if (headers.count(it->first) > 1) {
            if (outHeaders.find(it->first) != outHeaders.end()) {
                outHeaders[it->first].cast<FB::VariantList > ().push_back(it->second);
            } else {
                outHeaders[it->first] = FB::VariantList(FB::variant_list_of(it->second));
            }
        } else {
            outHeaders[it->first] = it->second;
        }
    }
    if (success) {
        std::ofstream myfile;
        myfile.open(destination.c_str(),std::fstream::binary);
        for (size_t i = 0; i < size; i++) {
            myfile << data[i];
        }
        myfile.close();
    } else {

    }
}

/**
 * Runs the given command and returns its output.
 */
/*std::string CodebenderccAPI::exec(const char * cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}
*/

/**
 * Saven the binary data to the binary file specified in the constructor.
 */
void CodebenderccAPI::saveToBin(unsigned char * data, size_t size) {
    std::ofstream myfile;
	myfile.open(binFile.c_str(),std::fstream::binary);
    for (size_t i = 0; i < size; i++) {
        myfile << data[i];
    } 
    myfile.close();
}

/**
 * decode base64 encoded data
 *
 * @param source the encoded data (zero terminated)
 * @param target pointer to the target buffer
 * @param targetlen length of the target buffer
 * @return length of converted data on success, -1 otherwise
 */
size_t CodebenderccAPI::base64_decode(const char *source, unsigned char *target, size_t targetlen) {
    char *src, *tmpptr;
    char quadruple[4];
    unsigned char tmpresult[3];
    int i;
	size_t tmplen = 3;
    int converted = 0;

    /* concatinate '===' to the source to handle unpadded base64 data */
    src = (char *) malloc(strlen(source) + 5);
    if (src == NULL)
        return -1;
    strcpy(src, source);
    strcat(src, "====");
    tmpptr = src;

    /* convert as long as we get a full result */
    while (tmplen == 3) {
        /* get 4 characters to convert */
        for (i = 0; i < 4; i++) {
            /* skip invalid characters - we won't reach the end */
            while (*tmpptr != '=' && _base64_char_value(*tmpptr) < 0)
                tmpptr++;

            quadruple[i] = *(tmpptr++);
        }

        /* convert the characters */
        tmplen = _base64_decode_triple(quadruple, tmpresult);

        /* check if the fit in the result buffer */
        if (targetlen < tmplen) {
            free(src);
            return -1;
        }

        /* put the partial result in the result buffer */
        memcpy(target, tmpresult, tmplen);
        target += tmplen;
        targetlen -= tmplen;
        converted += tmplen;
    }

    free(src);
    return converted;
}

/**
 * determine the value of a base64 encoding character
 *
 * @param base64char the character of which the value is searched
 * @return the value in case of success (0-63), -1 on failure
 */
int CodebenderccAPI::_base64_char_value(char base64char) {
    if (base64char >= 'A' && base64char <= 'Z')
        return base64char - 'A';
    if (base64char >= 'a' && base64char <= 'z')
        return base64char - 'a' + 26;
    if (base64char >= '0' && base64char <= '9')
        return base64char - '0' + 2 * 26;
    if (base64char == '+')
        return 2 * 26 + 10;
    if (base64char == '/')
        return 2 * 26 + 11;
    return -1;
}

/**
 * decode a 4 char base64 encoded byte triple
 *
 * @param quadruple the 4 characters that should be decoded
 * @param result the decoded data
 * @return lenth of the result (1, 2 or 3), 0 on failure
 */
int CodebenderccAPI::_base64_decode_triple(char quadruple[4], unsigned char *result) {
    int i, triple_value, bytes_to_decode = 3, only_equals_yet = 1;
    int char_value[4];

    for (i = 0; i < 4; i++)
        char_value[i] = _base64_char_value(quadruple[i]);

    /* check if the characters are valid */
    for (i = 3; i >= 0; i--) {
        if (char_value[i] < 0) {
            if (only_equals_yet && quadruple[i] == '=') {
                /* we will ignore this character anyway, make it something
                 * that does not break our calculations */
                char_value[i] = 0;
                bytes_to_decode--;
                continue;
            }
            return 0;
        }
        /* after we got a real character, no other '=' are allowed anymore */
        only_equals_yet = 0;
    }

    /* if we got "====" as input, bytes_to_decode is -1 */
    if (bytes_to_decode < 0)
        bytes_to_decode = 0;

    /* make one big value out of the partial values */
    triple_value = char_value[0];
    triple_value *= 64;
    triple_value += char_value[1];
    triple_value *= 64;
    triple_value += char_value[2];
    triple_value *= 64;
    triple_value += char_value[3];

    /* break the big value into bytes */
    for (i = bytes_to_decode; i < 3; i++)
        triple_value /= 256;
    for (i = bytes_to_decode - 1; i >= 0; i--) {
        result[i] = triple_value % 256;
        triple_value /= 256;
    }

    return bytes_to_decode;
}

#ifdef ISWIN
FB::variant CodebenderccAPI::probeUSB_win(){
  HANDLE hCom;
   std::string ports = "";
   
   for (int i=1;i<10;i++){
	//std::string port ="COM"+i;   //"\\\\.\\"
	
   TCHAR pcCommPort [16]; //  Most systems have a COM1 port
   swprintf(pcCommPort,L"COM%d",i);
   //_tcscpy(pcCommPort ,port.c_str());
   //  Open a handle to the specified com port.
   hCom = CreateFile( pcCommPort,
                      GENERIC_READ | GENERIC_WRITE,
                      1,      //  must be opened with exclusive-access
                      NULL,   //  default security attributes
                      OPEN_EXISTING, //  must use OPEN_EXISTING
                      0,      //  not overlapped I/O
                      NULL ); //  hTemplate must be NULL for comm devices

   if (hCom == INVALID_HANDLE_VALUE) 
   {       
       
   }else{
		CloseHandle(hCom);
		ports.append("COM");
		ports.append(boost::lexical_cast<std::string,int>(i));
		ports.append(",");		
   }
   }

	return ports;

}
#endif