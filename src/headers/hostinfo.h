#pragma once

#include <string>
#include <unistd.h>
#include <iostream>

#include <array>

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


HostInfo getHostInfo(const size_t HOSTNAME_MAX = 150){

	// char hostnameBuffer[HOSTNAME_MAX];
    static constexpr size_t MAX  = 1000;
    std::array<char, MAX> hostnameBuffer;
	auto hres = gethostname(hostnameBuffer.data(), MAX);
    if(hres){
        throw std::runtime_error{"Unable to get hostname"};
    }

    std::array<char, MAX> hostLoginBuffer;
	auto loginRes = getlogin_r(hostLoginBuffer.data(), HOSTNAME_MAX);

    if (loginRes){
        throw std::runtime_error{"Unable to get login name"};

    }

    HostInfo info;
    info.hostName = hostnameBuffer.data(); 
    info.loginName = hostLoginBuffer.data();

    return info;
}
