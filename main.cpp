
/* must be first header, due to msvc stuff */
#include "glue.h"

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
#include "ext/find.hpp"
#include "ext/optionparser.hpp"

#if defined(_MSC_VER)
    #define fileno _fileno
    #define isatty _isatty
#endif

enum class SortKind
{
    Extension,
    Stem,
    Filename,
};

struct Config
{
    // what to sort for - default is SortKind::Extension, i.e., file extensions.
    SortKind sortkind = SortKind::Extension;

    // whether to store file extension case-insensitively
    bool icase = false;

    // whether to read from standard input.
    // this is handled by '-i' - default behaviour is to read the current directory
    bool readstdin = false;

    // whether to read input from file(s) that contain filenames and/or file paths
    // this is handled via '-f'
    bool readlistings = false;

    // whether to ignore files that lack a file extension
    bool reject_noext = false;

    // whether to sort values by count. defaults to true
    bool sortvals = true;

    // if '-o' is specified, then the file handle must be closed (deleted, actually)
    bool mustclose = false;

    // where the output is written to. default is std::cout; handled by '-o' flag
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

        auto byhash(size_t hash)
        {
            return std::find_if(m_items.begin(), m_items.end(),  [&](const Item& item) 
            { 
                return (item.hash == hash);
            });
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

        // this function will attempt to remove '\r\n'.
        // this only applies to listing files.
        // it will effectively do nothing if the string does not contain a '\r'
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

        // this function is where post-processing (like turning strings lowercase)
        // happens. new options and/or functionality that directly operate
        // on the input string should be added here.
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

        void modeExtension(const std::filesystem::path& item)
        {
            std::string strext;
            std::string bnamestr;
            std::filesystem::path bname;
            bname = item.filename();
            bnamestr = bname.string();
            /*
            * if the item path is something like "foo/bar/", then
            * bname is just an empty string, and doesn't contain anything to work with.
            * theoretically, this shouldn't happen, though.
            */
            if(bnamestr.size() > 0)
            {
                strext = bname.extension().string();
                /*
                * in some super funky cases, the extension might be something
                * like "." (i.e., "foo."). i don't who or why someone would
                * name a file like this, but still.
                */
                if(strext.size() > 1)
                {
                    increase(strext);
                }
                else
                {
                    if(m_options.reject_noext == false)
                    {
                        increase(bname.string());
                    }
                }
            }
        }

        void modeStem(const std::filesystem::path& item)
        {
            std::string stemstr;
            std::filesystem::path stem;
            stem = item.stem();
            stemstr = stem.string();
            increase(stemstr);
        }

        void handleItem(const std::filesystem::path& item)
        {
            switch(m_options.sortkind)
            {
                case SortKind::Extension:
                    return modeExtension(item);
                case SortKind::Stem:
                    return modeStem(item);
                default:
                    std::cerr << "unimplemented sort kind" << std::endl;
                    std::exit(1);
                    break;
            }
        }

        void walkFilestream(std::istream& infh)
        {
            std::string line;
            while(std::getline(infh, line))
            {
                fixCR(line);
                try
                {
                    handleItem(line);
                }
                catch(std::exception& ex)
                {
                    std::cerr << "in walkFilestream: " << ex.what() << std::endl;
                }
            }
        }

        void walkDirectory(const std::string& dir)
        {
            Find::Finder fi(dir);
            fi.skipItemIf([&](const std::filesystem::path& path, bool isdir, bool isfile)
            {
                (void)path;
                (void)isfile;
                return (isdir == true);
            });
            try
            {
                fi.walk([&](const std::filesystem::path& path)
                {
                    handleItem(path);
                });
            }
            catch(std::exception& ex)
            {
                std::cerr << "*ERROR*: directory_iterator exception: " << ex.what() << std::endl;
            }
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
    prs.on({"-m?", "--mode=?"}, "which sort kind to use ('e': extension, 's': stem, 'f': filename. default: 'e')", [&](const std::string& s)
    {
        char modech;
        modech = std::tolower(s[0]);
        switch(modech)
        {
            case 'x':
            case 'e':
                opts.sortkind = SortKind::Extension;
                break;
            case 'f': // 'filename'
            case 'b': // 'basename'
                opts.sortkind = SortKind::Filename;
                break;
            case 'n': // 'name'
            case 's': // 'stem'
                opts.sortkind = SortKind::Stem;
                break;
            default:
                std::cerr << "unknown mode '" << modech << "'" << std::endl;
                std::exit(1);
                break;
        }
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
                //std::cerr << "reading from stdin" << std::endl;
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
        //std::cerr << "after printOutput" << std::endl;
    }
    catch(std::exception& e)
    {
        std::cerr << "error: " << e.what() << '\n';
    }
    if(opts.mustclose)
    {
        delete fhptr;
    }
    return 0;
}
