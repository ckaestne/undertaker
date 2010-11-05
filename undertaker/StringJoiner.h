// -*- mode: c++ -*-
#ifndef string_joiner_h__
#define string_joiner_h__

#include <deque>
#include <string>
#include <sstream>

struct StringJoiner : public std::deque<std::string> {
    std::string join(const char *j) {
        std::stringstream ss;
        if (size() == 0)
            return "";

        ss << front();
        pop_front();

        while (size() > 0) {
            ss << j << front();
            pop_front();
        }
        return ss.str();
    }

    void push_back(const value_type &x) {
        if (x.compare("") == 0)
            return;
        std::deque<value_type>::push_back(x);
    }
};

#endif
