#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include <string>

class Tokenizer {
private:
    std::string str;
    std::string delimiters;
    size_t pos = 0;
    size_t max;
    char   quotechar = '\"';

public:
    Tokenizer(const std::string & s, const std::string & delimiters) : delimiters(delimiters) {
        str = removeComments(s);
        max = str.size();
    }

    bool hasNext() {
        skipDelimiters();
        if (pos == max)
            return false;
        return true;
    }

    std::string next() {
        skipDelimiters();
        if (pos == max)
            return "";
        return readWord();
    }

private:
    void skipDelimiters() {
        while (pos < max && delimiters.find(str[pos]) != std::string::npos)
            pos++;
    }

    std::string readQuote() {
        pos++;
        size_t start = pos;
        while (pos < max && str[pos] != quotechar)
            pos++;
        if (str[pos] == quotechar) {
            pos++;
            return str.substr(start, pos - start - 1);
        }
        return str.substr(start, pos - start);
    }

    std::string readWord() {
        if (str[pos] == quotechar)
            return readQuote();

        size_t start = pos;
        while (pos < max && delimiters.find(str[pos]) == std::string::npos && str[pos] != quotechar)
            pos++;

        return str.substr(start, pos - start);
    }

    std::string removeComments(const std::string & s) {
        bool quote = false;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == quotechar)
                quote = !quote;
            else if (!quote && s[i] == '%')
                return s.substr(0, i);
        }
        return s;
    }
};

#endif // __TOKENIZER_H__
