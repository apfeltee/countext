
/*
* this is the old, naive, std::map version.
* the new version is a tad faster.
*/

#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <vector>
#include <set>
#include <functional>
#include <string>
#include <cstdio>
#include "pathtools.h"
#include "optionparser.hpp"

struct Options
{
    bool icase = false;
    bool readstdin = false;
    bool sortvals = true;
};


class CountFiles
{
    public:
        enum
        {
            kMaxCStringLength = 128,
        };

        using CString = std::string;
        using CLength = size_t;
        using ExtMap  = std::map<CString, CLength>;
        using Pair    = std::pair<ExtMap::key_type, ExtMap::mapped_type>;
        using List    = std::vector<Pair>;

    private:
        ExtMap m_map;
        Options& m_options;
        CLength m_padding;

    private:
        void checkPadding(CLength slen)
        {
            if(slen > m_padding)
            {
                m_padding = slen;
            }
        }

        void push(const std::string& val)
        {
            auto iter = m_map.find(val);
            if(iter == m_map.end())
            {
                m_map[val] = 1;
            }
            else
            {
                m_map[val]++;
            }
        }

    public:
        CountFiles(Options& opts): m_options(opts)
        {
        }

        void increase(const std::string& val)
        {
            std::string copy;
            checkPadding(val.size());
            if(m_options.icase)
            {
                copy = val;
                std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
                push(copy);
            }
            else
            {
                push(val);
            }
        }

        void handleItem(const std::string& item)
        {
            std::string ext;
            std::string bname;
            bname = Path::GetBasename(item);
            if(Path::FileExtension(bname, ext))
            {
                increase(ext);
            }
            else
            {
                increase(bname);
            }
        }

        void walkFilestream(std::istream& infh)
        {
            std::string line;
            while(std::getline(infh, line))
            {
                //std::cerr << "<stdin> " << line << std::endl;
                handleItem(line);
            }
        }

        void walkDirectory(const std::string& dir)
        {
            std::string ext;
            auto onerr = [&](std::runtime_error& ex)
            {
                std::cerr << "*ERROR* " << ex.what() << std::endl;
            };
            Path::WalkDirectory(dir, [&](const Path::DirectoryEntry& ent) -> bool
            {
                if(ent.isfile())
                {
                    handleItem(ent.name());
                }
                return true;
            }, onerr);
        }

        ExtMap get()
        {
            return m_map;
        }

        List getSorted()
        {
            List vals;
            vals.reserve(m_map.size());
            for(auto it=m_map.begin(); it!=m_map.end(); it++)
            {
                vals.push_back(*it);
            }
            std::sort(vals.begin(), vals.end(), [](const Pair& lhs, const Pair& rhs)
            {
                return lhs.second < rhs.second;
            });
            return vals;
        }

        void printVals(std::ostream& out, const CString& ext, const CLength& count)
        {
            size_t realpad;
            realpad = (m_padding + 2);
            out << std::setw(realpad) << ext << " " << count << std::endl;
        }

        template<typename IterAT, typename IterBT>
        void printIter(std::ostream& out, IterAT ibegin, IterBT iend)
        {
            for(auto it=ibegin; it!=iend; it++)
            {
                printVals(out, it->first, it->second);
            }
        }

        void printTo(std::ostream& out)
        {
            if(m_options.sortvals)
            {
                auto vals = getSorted();
                printIter(out, vals.begin(), vals.end());
            }
            else
            {
                printIter(out, m_map.begin(), m_map.end());
            }
        }
};


int main(int argc, char* argv[])
{
    OptionParser prs;
    Options opts;
    prs.on("-i", "--stdin", "read input from stdin", [&]
    {
        opts.readstdin = true;
    });
    prs.on("-c", "--nocase", "turn extensions/names to lowercase", [&]
    {
        opts.icase = true;
    });
    prs.on("-n", "--nosort", "do not sort", [&]
    {
        opts.sortvals = false;
    });
    try
    {
        prs.parse(argc, argv);
        CountFiles cf(opts);
        if((opts.readstdin == false) && (prs.size() == 0))
        {
            cf.walkDirectory(".");
        }
        else
        {
            if(opts.readstdin == true)
            {
                cf.walkFilestream(std::cin);
            }
            else
            {
                auto dirs = prs.positional();
                for(auto dir: dirs)
                {
                    cf.walkDirectory(dir);
                }
            }
        }
        cf.printTo(std::cout);
    }
    catch(std::runtime_error& e)
    {
        std::cerr << "parsing error: " << e.what() << std::endl;
    }
    return 0;
}
