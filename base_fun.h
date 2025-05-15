#ifndef BASEFUN_H
#define BASEFUN_H

#include <string>
#include <fcntl.h>
#include <iostream>
#include <vector>
#include <string.h>
class BaseFun
{
public:
    BaseFun();
    static BaseFun* getInstance();
    std::string getSelfProcessName();
    std::string getFileContent(const std::string &file_path);
    std::string trimString(const std::string& str);
    std::vector<std::string> sepString(const std::string &sStr, const std::string &sSep="|", bool withEmpty=true);
private:
    static BaseFun*   instance_;
};

#endif // BASEFUN_H
