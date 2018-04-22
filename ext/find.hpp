/*
* Copyright 2017 apfeltee (github.com/apfeltee)
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in the
* Software without restriction, including without limitation the rights to use, copy, modify,
* merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <iostream>
#include <sstream>
#include <exception>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <cwchar>

#if defined(_MSC_VER)
    #include <experimental/filesystem> 
    #include <filesystem>
    namespace std
    {
        namespace filesystem
        {
            using namespace std::experimental::filesystem::v1;
        }
    }        
#else
    #include <boost/filesystem.hpp>
    namespace std
    {
        namespace filesystem
        {
            using namespace boost::filesystem;
        }
    }
#endif

namespace Find
{
    class Finder
    {
        public:
            using PruneIfFunc     = std::function<bool(const std::filesystem::path&)>;
            using SkipFunc        = std::function<bool(const std::filesystem::path&,bool,bool)>;
            using IgnoreFileFunc  = std::function<bool(const std::filesystem::path&)>;
            using EachFunc        = std::function<void(const std::filesystem::path&)>;
            using ExceptionFunc   = std::function<void(std::runtime_error&)>;

        public:
            static bool DirectoryIs(const std::filesystem::path& input, const std::filesystem::path& findme)
            {
                auto filename = input.filename();
                if(filename == findme)
                {
                    return true;
                }
                return false;
            }

            static bool DirectoryIs(const std::filesystem::path& input, const std::vector<std::filesystem::path>& items)
            {
                for(auto item: items)
                {
                    if(DirectoryIs(input, item))
                    {
                        return true;
                    }
                }
                return false;
            }

        private:
            std::vector<std::filesystem::path> m_startdirs;
            std::vector<PruneIfFunc> m_prunefuncs;
            std::vector<SkipFunc> m_skipfuncs;
            std::vector<IgnoreFileFunc> m_ignfilefuncs;
            ExceptionFunc m_exceptionfunc;

        protected:
            void setup(const std::vector<std::filesystem::path>& dirs)
            {
                m_startdirs = dirs;
            }

            bool skipItem(const std::filesystem::path& path, bool isdir, bool isfile)
            {
                for(auto fn: m_skipfuncs)
                {
                    if(fn(path, isdir, isfile) == false)
                    {
                        return true;
                    }
                }
                return false;
            }

            void doWalk(const std::filesystem::path& dir, EachFunc eachfn)
            {
                bool isdir;
                bool isfile;
                bool ispruned;
                bool emitme;
                size_t level;
                level = 0;
                std::filesystem::recursive_directory_iterator end;
                std::filesystem::recursive_directory_iterator iter(dir);
                #if (!defined(__CYGWIN__)) && (!defined(__CYGWIN32__))
                /*
                * this check is necessary because in certain circumstances, passing
                * a non-existant path to recursive_directory_iterator, even without error_code,
                * will not throw an exception ...
                * the solution: check if $iter equals $end, and if it does, also
                * check if $dir exists at all.
                * if those are true, manually throw an exception!
                * as std::filesystem* isn't available for cygwin (yet?), this
                * won't apply for the boost::filesystem wrapper, and in fact, won't compile
                * (due to constructor diffs - boost::system::error_code vs std::error_code).
                *
                * mind you, this is a hack meant to fix something that i really shouldn't
                * have to fix...
                */
                if(iter == end)
                {
                    std::error_code ecode;
                    if(std::filesystem::is_directory(dir, ecode) == false)
                    {
                        throw std::filesystem::filesystem_error("not a directory", dir, ecode);
                    }
                }
                #endif
                while(iter != end)
                {
                    try
                    {
                        auto entry = *iter;
                        emitme = true;
                        isdir = std::filesystem::is_directory(entry.status());
                        isfile = std::filesystem::is_regular_file(entry.status());
                        emitme = skipItem(entry.path(), isdir, isfile);
                        if(isdir)
                        {
                            if(std::filesystem::is_symlink(entry.path()) /* && do not follow links? */)
                            {
                                iter.disable_recursion_pending();
                            }
                            else
                            {
                                ispruned = false;
                                for(auto prunefn: m_prunefuncs)
                                {
                                    if(prunefn(entry.path()) == true)
                                    {
                                        //iter.pop();
                                        //iter.no_push();
                                        iter.disable_recursion_pending();
                                        ispruned = true;
                                        emitme = false;
                                    }
                                }
                                if(ispruned == false)
                                {
                                    level++;
                                }
                            }
                        }
                        else if(isfile)
                        {
                            ispruned = false;
                            for(auto filefn: m_ignfilefuncs)
                            {
                                if(filefn(entry.path()) == true)
                                {
                                    //std::cout << "-- filefn returned false for " << entry << std::endl; 
                                    ispruned = true;
                                    emitme = false;
                                }
                            }
                        }
                        //std::cout << "-- emitme=" << emitme << ", ispruned=" << ispruned << std::endl;
                        if(emitme == true)
                        {
                            eachfn(entry.path());
                        }
                    }
                    catch(std::runtime_error& ex)
                    {
                        std::cerr << "exception? what=" << ex.what() << std::endl;
                        if(m_exceptionfunc)
                        {
                            m_exceptionfunc(ex);
                        }
                        else
                        {
                            throw ex;
                        }
                    }
                    iter++;
                }
            }

        public:
            // no explicit directories specified - use CWD as starting point
            Finder()
            {
                setup({});
            }

            // single directory specified
            Finder(const std::filesystem::path& dir)
            {
                setup({dir});
            }

            // list of dirs specified
            Finder(const std::vector<std::filesystem::path>& dirs)
            {
                setup(dirs);
            }

            void addDirectory(const std::filesystem::path& path)
            {
                m_startdirs.push_back(path);
            }

            void pruneIf(PruneIfFunc fn)
            {
                m_prunefuncs.push_back(fn);
            }

            void skipItemIf(SkipFunc fn)
            {
                m_skipfuncs.push_back(fn);
            }

            void ignoreIf(IgnoreFileFunc fn)
            {
                m_ignfilefuncs.push_back(fn);
            }

            void onException(ExceptionFunc fn)
            {
                m_exceptionfunc = fn;
            }

            void walk(EachFunc fn)
            {
                if(m_startdirs.size() == 0)
                {
                    m_startdirs.push_back(std::filesystem::current_path());
                }
                for(auto dir: m_startdirs)
                {
                    doWalk(dir, fn);
                }
            }
    };
}

