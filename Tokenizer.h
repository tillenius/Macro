#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include <string>

using std::string;

class Tokenizer {
protected:
  string str;
  string delimiters;
  size_t pos;
  size_t max;
  char   quotechar;

public:
  Tokenizer(string &s, string &delimiters) {
    pos = 0;
    quotechar = '\"';
    this->delimiters = delimiters;

    this->str = removeComments(s);
    max = str.size();
  }

  bool hasNext(void) {
    skipDelimiters();
    if (pos == max)
      return false;
    return true;
  }

  string next(void) {
    skipDelimiters();
    if (pos == max)
      return "";
    return readWord();
  }

protected:
  void skipDelimiters(void) {
    while( pos < max && delimiters.find(str[pos]) != string::npos )
      pos++;
  }

  string readQuote(void) {
    pos++;
    size_t start = pos;
    while (pos < max && str[pos] != quotechar)
      pos++;
    if (str[pos] == quotechar) {
      pos++;
      return str.substr(start, pos-start-1);
    }
    return str.substr(start, pos-start);
  }

  string readWord(void) {
    if (str[pos] == quotechar)
      return readQuote();

    size_t start = pos;
    while (pos < max && delimiters.find(str[pos]) == string::npos && str[pos] != quotechar)
      pos++;

    return str.substr(start, pos-start);
  }

  string removeComments(string &s) {
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
