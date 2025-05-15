#include "base_fun.h"
#include <string.h>
#include <string>
#include <limits.h>
#include <fstream>
#include <algorithm>
#include <cctype>
#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#include <psapi.h>
#endif

BaseFun*   BaseFun::instance_;
BaseFun::BaseFun()
{
    instance_ = NULL;
}

BaseFun *BaseFun::getInstance()
{
    if(instance_ == NULL)
        instance_ = new BaseFun();
    return instance_;
}

std::string BaseFun::getSelfProcessName()
{
#ifndef WIN32
    char path[PATH_MAX] = {0};
    int nret = readlink("/proc/self/exe",path,PATH_MAX);
    if(nret < 0)
    {
        return "";
    }
    path[nret] = '\0';

    std::string process_name = path;
    int npos = process_name.find_last_of("/");
    if(npos != std::string::npos)
        process_name = process_name.substr(npos+1);
    return process_name;
#else
    std::string str_name = "";
    HANDLE h_process = GetCurrentProcess();
    char exe_name[MAX_PATH] = {0};
    if(GetModuleFileNameExA(h_process,NULL,exe_name,MAX_PATH) > 0)
    {
        str_name = std::string(exe_name,strlen(exe_name));
    }
    return str_name;
#endif
}

std::string BaseFun::getFileContent(const std::string& file_path)
{
    std::string ret_cont = "";
    std::ifstream ifs(file_path.c_str());
    ret_cont = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    return trimString(ret_cont);
}

std::string BaseFun::trimString(const std::string &str)
{
    auto start = std::find_if_not(str.begin(),str.end(),[](unsigned char ch){
        return std::isspace(ch) || ch == '\r' || ch == '\n' || ch == '\t';
    });
    auto end = std::find_if_not(str.rbegin(),str.rend(),[](unsigned char ch){
        return std::isspace(ch) || ch == '\r' || ch == '\n' || ch == '\t';
    }).base();
    return (start < end ? std::string(start,end) : std::string());
}

std::vector<std::string> BaseFun::sepString(const std::string &sStr, const std::string &sSep, bool withEmpty)
{
    std::vector<std::string> vt;

    std::string::size_type pos = 0;
    std::string::size_type pos1 = 0;

    if(sStr.length()==0)
        return vt;

    while(true)
    {
        std::string s;
        pos1 = sStr.find(sSep, pos);
        if(pos1 == std::string::npos)
        {
            if(pos + 1 <= sStr.length())
            {
                s = sStr.substr(pos);
            }
        }
        else if(pos1 == pos)
        {
            s = "";
        }
        else
        {
            s = sStr.substr(pos, pos1 - pos);
            pos = pos1;
        }

        if(withEmpty)
        {
            vt.push_back(s);
        }
        else
        {
            if(!s.empty())
            {
                vt.push_back(s);
            }
        }

        if(pos1 == std::string::npos)
        {
            break;
        }

        pos += sSep.length();
    }

    return vt;
}
