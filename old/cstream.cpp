
#include <iostream>
#include <functional>
#include <cstdio>

template<typename CharT>
class basic_cstream
{
    protected:
        FILE* m_handle = nullptr;
        bool m_isclosed = false;
        bool m_isopen = false;

    private:
        void sanitycheck()
        {
            m_isopen = (m_handle != nullptr);
        }

        size_t writeChunk(const void* data, size_t chunksize, size_t objsizeof)
        {
            return std::fwrite(data, objsizeof, chunksize, m_handle);
        }

        template<typename... ArgsT>
        size_t writeFormatted(const CharT* fmtstr, ArgsT&&... args)
        {
            return std::fprintf(m_handle, fmtstr, args...);
        }

        template<typename NumT, size_t fmtlen>
        size_t writeNumeric(NumT nval, const CharT(&fmt)[fmtlen])
        {
            return writeFormatted(fmt, nval);
        }

        size_t writeStdString(const std::basic_string<CharT>& s)
        {
            return writeChunk(s.c_str(), s.size(), sizeof(CharT));
        }

        size_t writeLiteralString(const CharT* str, size_t litlength)
        {
            return writeChunk(str, litlength, sizeof(CharT));
        }

        size_t writeChar(CharT ch)
        {
            std::fputc(ch, m_handle);
            return 1;
        }

        size_t writeInteger(int nv)
        {
            return writeNumeric(nv, "%d");
        }

        size_t writeFloat(double nv)
        {
            return writeNumeric(nv, "%f");
        }

        size_t writeSizeType(size_t nv)
        {
            return writeNumeric(nv, "%lu");
        }

    public:
        basic_cstream()
        {
            sanitycheck();
        }

        basic_cstream(FILE* hnd): m_handle(hnd)
        {
            sanitycheck();
        }

        basic_cstream(const std::basic_string<CharT>& fpath, const std::basic_string<CharT>& mode)
        {
            this->open(fpath, mode);
            sanitycheck();
        }

        ~basic_cstream()
        {
            this->close();
        }

        basic_cstream& open(const std::basic_string<CharT>& fpath, const std::basic_string<CharT>& mode)
        {
            m_handle = std::fopen(fpath.c_str(), mode.c_str());
            return *this;
        }

        void close()
        {
            if((m_isclosed == false) && (m_handle != nullptr))
            {
                std::fclose(m_handle);
                m_isclosed = true;
                m_isopen = false;
                m_handle = nullptr;
            }
        }

        bool good() const
        {
            return m_isopen;
        }

        basic_cstream& flush()
        {
            std::fflush(m_handle);
            return *this;
        }

        size_t write(const void* data, size_t len, size_t objsizeof)
        {
            return writeChunk(data, len, objsizeof);
        }

        size_t write(const CharT* data, size_t len)
        {
            return writeChunk(data, len, sizeof(CharT));
        }

        template<size_t litlength, typename... ArgsT>
        basic_cstream& format(const CharT*(&str)[litlength], ArgsT&&... args)
        {
            writeFormatted(str, args...);
            return *this;
        }

        template<typename... ArgsT>
        basic_cstream& format(const std::string& str, ArgsT&&... args)
        {
            writeFormatted(str.c_str(), args...);
            return *this;
        }

        basic_cstream& put(CharT ch)
        {
            writeChar(ch);
            return *this;
        }

        size_t read(void* destbuffer, size_t readhowmuch, size_t objsizeof)
        {
            return std::fread(destbuffer, objsizeof, readhowmuch, m_handle);
        }

        size_t read(void* destbuffer, size_t readhowmuch)
        {
            return this->read(destbuffer, readhowmuch, sizeof(CharT));
        }

        size_t read(std::basic_string<CharT>& dest, size_t howmuch, size_t objsizeof)
        {
            size_t realrd;
            CharT* tmpbuf;
            tmpbuf = new CharT[howmuch];
            dest.clear();
            dest.reserve(howmuch);
            realrd = this->read(tmpbuf, howmuch, objsizeof);
            if(realrd > 0)
            {
                dest.append(tmpbuf, realrd);
            }
            delete[] tmpbuf;
            return realrd;
        }

        size_t read(std::basic_string<CharT>& dest, size_t howmuch)
        {
            return this->read(dest, howmuch, sizeof(CharT));
        }

        bool read_line(std::basic_string<CharT>& dest, CharT sep=CharT('\n'))
        {
            CharT ch;
            
        }

        basic_cstream& operator<<(CharT c)
        {
            writeChar(c);
            return *this;
        }

        basic_cstream& operator<<(int nv)
        {
            writeInteger(nv);
            return *this;
        }

        basic_cstream& operator<<(double nv)
        {
            writeFloat(nv);
            return *this;
        }

        basic_cstream& operator<<(size_t nv)
        {
            writeSizeType(nv);
            return *this;
        }

        basic_cstream& operator<<(const std::basic_string<CharT>& str)
        {
            writeStdString(str);
            return *this;
        }

        template<size_t litlength>
        basic_cstream& operator<<(const CharT*(&str)[litlength])
        {
            writeLiteralString(str, litlength);
            return *this;
        }
};

using cstream = basic_cstream<char>;
cstream ccin(stdin);
cstream ccout(stdout);
cstream ccerr(stderr);

int main()
{
    ccout << "hello world!";
    /*
    auto n = 17.4431e1;
    ccout << "n = " << n << '\n';
    float c = 55.32142;
    ccout.format("c = %f\n", c);
    */
}
