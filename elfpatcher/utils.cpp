// https://stackoverflow.com/questions/2180079/how-can-i-copy-a-file-on-unix-using-c
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <locale>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include "utils.h"

int copyFile(const char *from, const char *to)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

    out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}


void hexDump (void *addr, size_t len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n", len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}


bool mayBeString(uint32_t num) {
    uint8_t *chars = (uint8_t *)&num;
    for(int i=0;i<4;i++) {
        uint8_t n = chars[i];
        if(!(
                n >= '!' && n <= '~' ||
                n == ' ' || n == '\t' ||
                n == '\n' || n == '\r'
        )) return false;
    }
    return true;
}

void sbr(uint8_t *buf, uint32_t data) {
    buf[0] = (uint8_t)(data>>24);
    buf[1] = (uint8_t)(data>>16);
    buf[2] = (uint8_t)(data>>8);
    buf[3] = (uint8_t)(data);
}

// https://stackoverflow.com/questions/236129/the-most-elegant-way-to-iterate-the-words-of-a-string
template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring

// trim from start (in place)
void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// https://stackoverflow.com/questions/23457305/compare-strings-with-wildcard/23457543#23457543
bool match(const char *pattern, const char *candidate, int p, int c) {
    if (pattern[p] == '\0') {
        return candidate[c] == '\0';
    } else if (pattern[p] == '*') {
        for (; candidate[c] != '\0'; c++) {
            if (match(pattern, candidate, p+1, c))
                return true;
        }
        return match(pattern, candidate, p+1, c);
    } else if (pattern[p] != '?' && pattern[p] != candidate[c]) {
        return false;
    }  else {
        return match(pattern, candidate, p+1, c+1);
    }
}
