#ifndef __WILDCARDS_H__
#define __WILDCARDS_H__

#include <string>
#include <stack>

class Wildcards {
public:
    static bool match(const std::string &pattern, const std::string &str) {
        size_t strPos = 0;
        size_t ptnPos = 0;
        std::stack< std::pair<size_t, size_t> > backTrack;
        for (;;) {
            if (strPos == str.size() && ptnPos == pattern.size())
                return true;
            const char ptn = pattern[ptnPos];
            if (strPos < str.size() && (str[strPos] == ptn || ptn == '?')) {
                ++strPos;
                ++ptnPos;
                continue;
            }
            if (ptnPos < pattern.size() && ptn == '*') {
                ++ptnPos;
                backTrack.push(std::pair<size_t, size_t>(strPos, ptnPos));
                continue;
            }
            if (backTrack.size() == 0)
                return false;
            strPos = ++backTrack.top().first;
            ptnPos = backTrack.top().second;
            if (strPos >= str.size())
                backTrack.pop();
        }
        return strPos == str.size() && ptnPos == pattern.size();
    }
};

#endif // __WILDCARDS_H__
