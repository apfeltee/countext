
#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <functional>
#include <string>
#include <cstdio>
#include "pathtools.h"
#include "optionparser.hpp"

#if defined(_MSC_VER)
    #define fileno _fileno
    #define isatty _isatty
#endif

struct Config
{
    bool icase = false;
    bool readstdin = false;
    bool readlistings = false;
    bool reject_noext = false;
    bool sortvals = true;
    bool mustclose = false;
    std::ostream* outstream;
};

class ExtList
{
    public:
        template<typename T>
        using ContainerType = std::deque<T>;

        struct Item
        {
            std::string ext;
            size_t count;
            size_t hash;

            bool equals(const std::string& other)
            {
                return (other == ext);
            }
        };

    private:
        ContainerType<size_t> m_seen;
        ContainerType<Item> m_items;
        std::hash<std::string> m_hashfn;

    public:
        ExtList()
        {
        }

        auto begin()
        {
            return m_items.begin();
        }

        auto end()
        {
            return m_items.end();
        }

        Item byhash(size_t hash)
        {
            auto iter = std::find_if(m_items.begin(), m_items.end(),  [&](const Item& item) 
            { 
                return (item.hash == hash);
            });
            return *iter;
        }

        bool contains(size_t hash, size_t& idx)
        {
            auto it = std::find(m_seen.begin(), m_seen.end(), hash);
            if(it != m_seen.end())
            {
                idx = std::distance(m_seen.begin(), it);
                return true;
            }
            return false;
        }

        void increase(const std::string& ext)
        {
            size_t idx;
            size_t hash;
            hash = m_hashfn(ext);
            if(contains(hash, idx))
            {
                m_items[idx].count++;
            }
            else
            {
                m_seen.push_back(hash);
                m_items.push_back(Item{ext, 1, hash});
            }
        }
};

class CountFiles
{
    private:
        ExtList m_map;
        Config& m_options;
        size_t m_padding = 5;

    private:
        void checkPadding(size_t slen)
        {
            if(slen > m_padding)
            {
                m_padding = slen;
            }
        }

        void fixCR(std::string& str)
        {
            int lastchar;
            int belastchar;
            if(str.length() > 2)
            {
                auto eit = str.end();
                auto itlastchar = (eit - 1);
                auto itbelastchar = (eit - 2);
                lastchar = *itlastchar;
                belastchar = *itbelastchar;
                // std::getline is so fucking useful that it just plain stops at '\n' --
                // even though '\r\n' is pretty common! but who the fuck cares, amirite?
                if(lastchar == '\r')
                {
                    str.erase(itlastchar);
                }
                // this seriously shouldn't be happening, actually ...
                else if((lastchar == '\n') && (belastchar == '\r'))
                {
                    str.erase(itbelastchar);
                    str.erase(itlastchar);
                }
            }
        }

        void push(const std::string& val)
        {
            m_map.increase(val);
        }

    public:
        CountFiles(Config& opts): m_options(opts)
        {
        }

        std::ostream& out()
        {
            return *(m_options.outstream);
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
            /*
            * if the item path is something like "foo/bar/", then
            * bname is just an empty string, and doesn't contain anything to work with
            */
            if(bname.size() > 0)
            {
                if(Path::FileExtension(bname, ext))
                {
                    increase(ext);
                }
                else
                {
                    if(m_options.reject_noext == false)
                    {
                        increase(bname);
                    }
                }
            }
        }

        void walkFilestream(std::istream& infh)
        {
            std::string line;
            while(std::getline(infh, line))
            {
                fixCR(line);
                handleItem(line);
            }
        }

        void walkDirectory(const std::string& dir)
        {
            std::string ext;
            auto onerr = [&](std::runtime_error& ex)
            {
                std::cerr << "*ERROR* " << ex.what() << '\n';
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

        void sort()
        {
            std::sort(m_map.begin(), m_map.end(), [](const ExtList::Item& lhs, const ExtList::Item& rhs)
            {
                return (lhs.count < rhs.count);
            });
        }

        ExtList get()
        {
            return m_map;
        }

        void printVals(const std::string& ext, const size_t& count)
        {
            size_t realpad;
            realpad = (m_padding + 2);
            out() << std::setw(realpad) << ext << " " << count << '\n';
        }

        void printOutput()
        {
            if(m_options.sortvals)
            {
                sort();
            }
            for(auto it=m_map.begin(); it!=m_map.end(); it++)
            {
                printVals(it->ext, it->count);
            }
        }
};

/*
* for some reason, isatty() seems to not work... sometimes.
* haven't been able to figure out why, yet.
* works so far with msvc, clang (for msvc), and gcc.
*/
static bool have_filepipe()
{
    return (isatty(fileno(stdin)) == 0);
}

int main(int argc, char* argv[])
{
    OptionParser prs;
    Config opts;
    std::fstream* fhptr;
    opts.outstream = &std::cout;
    prs.on({"-i", "--stdin"}, "read input from stdin", [&]
    {
        opts.readstdin = true;
    });
    prs.on({"-c", "--nocase"}, "turn extensions/names to lowercase", [&]
    {
        opts.icase = true;
    });
    prs.on({"-n", "--nosort"}, "do not sort", [&]
    {
        opts.sortvals = false;
    });
    prs.on({"-f", "--listing"}, "interpret arguments as a list of files containing paths", [&]
    {
        opts.readlistings = true;
    });
    prs.on({"-e", "--rnoext"}, "do not print files without an extension", [&]
    {
        opts.reject_noext = true;
    });
    prs.on({"-o?", "--output=?"}, "write output to file (default: write to stdout)", [&](const std::string& s)
    {
        fhptr = new std::fstream(s, std::ios::out | std::ios::binary);
        if(!fhptr->good())
        {
            std::cerr << "failed to open '" << s << "' for writing" << '\n';
            std::exit(1);
        }
        opts.outstream = fhptr;
        opts.mustclose = true;
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
                if(have_filepipe())
                {
                    cf.walkFilestream(std::cin);
                }
                else
                {
                    std::cerr << "error: no file piped" << '\n';
                    return 1;
                }
            }
            else if(opts.readlistings)
            {
                auto files = prs.positional();
                for(auto file: files)
                {
                    std::fstream fh(file, std::ios::in | std::ios::binary);
                    if(fh.good())
                    {
                        //std::cerr << "reading paths from \"" << file << "\" ..." << '\n';
                        cf.walkFilestream(fh);
                    }
                    else
                    {
                        std::cerr << "failed to open \"" << file << "\" for reading" << '\n';
                    }
                }
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
        cf.printOutput();
    }
    catch(std::runtime_error& e)
    {
        std::cerr << "parsing error: " << e.what() << '\n';
    }
    if(opts.mustclose)
    {
        delete fhptr;
    }
    return 0;
}
