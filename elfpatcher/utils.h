//
// Created by uwe on 13.03.18.
//

#ifndef ELFPATCHER_CP_H
#define ELFPATCHER_CP_H
#include <string>
#include <vector>
extern int copyFile(const char *from, const char *to);
void hexDump (void *addr, size_t len);
bool mayBeString(uint32_t num);
void sbr(uint8_t *buf, uint32_t data);
std::vector<std::string> split(const std::string &s, char delim);

void ltrim(std::string &s);
void rtrim(std::string &s);
void trim(std::string &s);
bool match(const char *pattern, const char *candidate, int p, int c);

#endif //ELFPATCHER_CP_H
