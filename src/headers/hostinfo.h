#pragma once

#include <string>
#include <unistd.h>
#include <iostream>

struct HostInfo {
    std::string hostName;
    std::string loginName;


    friend std::ostream & operator<<(std::ostream & os, const HostInfo & info){
        os<<"HostInfo: {\n";
        os<<"host: "<<info.hostName;
        os<<",\n";
        os<<"login: "<<info.loginName;
        os<<"\n";
        os<<"}\n";
        return os;

    }   
    
};


HostInfo getHostInfo(const int HOSTNAME_MAX = 150){

	char hostnameBuffer[HOSTNAME_MAX];
	auto hres = gethostname(hostnameBuffer, HOSTNAME_MAX);
    if(hres){
        throw std::runtime_error{"Unable to get hostname"};
    }

	char hostLoginBuffer[HOSTNAME_MAX];
	auto loginRes = getlogin_r(hostLoginBuffer, HOSTNAME_MAX);

    if (loginRes){
        throw std::runtime_error{"Unable to get login name"};

    }

    HostInfo info;
    info.hostName = hostnameBuffer;
    info.loginName = hostLoginBuffer;

    return info;


}
