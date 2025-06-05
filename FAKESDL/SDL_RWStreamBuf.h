#ifndef SDL_RWSTREAMBUF_H
#define SDL_RWSTREAMBUF_H

#include <SDL.h>
#include <streambuf>
#include <istream>
#include <ostream>

// Stream buffer wrapping SDL_RWops
class SDL_RWStreamBuf : public std::streambuf {
    static const size_t bufferSize = 4096;
    SDL_RWops* rwops;
    char inputBuffer[bufferSize];
    char outputBuffer[bufferSize];

public:
    explicit SDL_RWStreamBuf(SDL_RWops* rw) : rwops(rw) {
        // Set up the get area pointers for reading
        setg(inputBuffer, inputBuffer, inputBuffer);
        // Set up the put area pointers for writing
        setp(outputBuffer, outputBuffer + bufferSize);
    }

    ~SDL_RWStreamBuf() {
        sync();  // flush output buffer on destruction
    }

protected:
    // Reading: underflow is called when the input buffer is empty
    int_type underflow() override {
        if (gptr() < egptr())
            return traits_type::to_int_type(*gptr());

        // Read from SDL_RWops into inputBuffer
        size_t n = SDL_RWread(rwops, inputBuffer, 1, bufferSize);
        if (n == 0)
            return traits_type::eof();

        // Set buffer pointers
        setg(inputBuffer, inputBuffer, inputBuffer + n);
        return traits_type::to_int_type(*gptr());
    }

    // Writing: overflow is called when output buffer is full or on flush
    int_type overflow(int_type ch) override {
        if (ch != traits_type::eof()) {
            // Add the character to the buffer
            *pptr() = traits_type::to_char_type(ch);
            pbump(1);
        }
        // Write buffer to SDL_RWops
        if (flushBuffer() == -1)
            return traits_type::eof();

        return traits_type::not_eof(ch);
    }

    // Sync: flush output buffer
    int sync() override {
        return flushBuffer();
    }

    // Seek: override seekoff and seekpos to handle seeking with SDL_RWseek
    std::streampos seekoff(std::streamoff off,
                           std::ios_base::seekdir way,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        // Flush output buffer before seeking
        if (sync() == -1)
            return -1;

        int whence;
        switch (way) {
            case std::ios_base::beg: whence = RW_SEEK_SET; break;
            case std::ios_base::cur: whence = RW_SEEK_CUR; break;
            case std::ios_base::end: whence = RW_SEEK_END; break;
            default: return -1;
        }

        Sint64 pos = SDL_RWseek(rwops, off, whence);
        if (pos < 0)
            return -1;

        // Reset get area pointers on seek
        setg(inputBuffer, inputBuffer, inputBuffer);
        return pos;
    }

    std::streampos seekpos(std::streampos sp,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override {
        return seekoff(sp, std::ios_base::beg, which);
    }

private:
    int flushBuffer() {
        ptrdiff_t n = pptr() - pbase();
        if (n > 0) {
            size_t written = SDL_RWwrite(rwops, pbase(), 1, n);
            if (written != static_cast<size_t>(n))
                return -1;
            pbump(-n); // reset put pointer
        }
        return 0;
    }
};

#endif // SDL_RWSTREAMBUF_H