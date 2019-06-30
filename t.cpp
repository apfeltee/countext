
#include "ext/find.hpp"


std::string _s(const std::string& in)
{
    std::string out;
    if(in.size() > 0)
    {
        out.push_back('"');
        for(auto ch: in)
        {
            out.push_back(ch);
        }
        out.push_back('"');
        return out;
    }
    return "<empty>";
}

std::string _s(const std::filesystem::path& in)
{
    return _s(in.string());
}

int main(int argc, char* argv[])
{
    if(argc > 1)
    {
        auto path = std::filesystem::path(argv[1]);
        std::cout << "root_name():      " << _s(path.root_name()) << std::endl;
        std::cout << "root_directory(): " << _s(path.root_directory()) << std::endl;
        std::cout << "root_path():      " << _s(path.root_path()) << std::endl;
        std::cout << "relative_path():  " << _s(path.relative_path()) << std::endl;
        std::cout << "parent_path():    " << _s(path.parent_path()) << std::endl;
        std::cout << "filename():       " << _s(path.filename()) << std::endl;
        std::cout << "stem():           " << _s(path.stem()) << std::endl;
        std::cout << "extension():      " << _s(path.extension()) << std::endl;
    }
}

