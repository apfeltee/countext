
/*
* this file is part of countext, and FALLS UNDER THE SAME LICENSE.
* IT IS **NOT** PUBLIC DOMAIN.
*/

#pragma once
#include <sstream>
#include <functional>
#include <cstring>

namespace Path
{
    char DetectSlashType(const std::string& path)
    {
        size_t i;
        size_t pos;
        static const char slashes[] = {'/', '\\', 0};
        for(i=0; slashes[i]!=0; i++)
        {
            pos = path.find_first_of(slashes[i]);
            if(pos != std::string::npos)
            {
                return slashes[i];
            }
        }
        return 0;
    }

    std::string GetBasename(const std::string& path)
    {
        size_t pos;
        char slash;
        std::string npath;
        slash = DetectSlashType(path);
        if(slash == 0)
        {
            return path;
        }
        pos = path.find_last_of(slash);
        npath = path.substr(pos + 1, path.size());
        return npath;
    }

    bool FileExtension(const std::string& name, std::string& dest)
    {
        size_t pos;
        std::string tmp;
        pos = name.find_last_of(".");
        if(pos != std::string::npos)
        {
            tmp = name.substr(pos, name.size());
            if(tmp.length() > 0)
            {
                dest = tmp;
                return true;
            }
        }
        return false;
    }
}
