// -*- mode: c++ -*-
#ifndef string_joiner_h__
#define string_joiner_h__

#include <deque>
#include <string>
#include <sstream>

/**
 * \brief Helper subclass of std::deque<std::string> for convenient
 *  concatenating strings with a separator.
 *
 * The behaviour is similar to the python list().join(",") construct.
 * 
 */
struct StringJoiner : public std::deque<std::string> {
    /**
     * \brief join all strings in the container,
     *
     * Join all the collected strings to one string. The separator is
     * inserted between each element.
     */
    std::string join(const char *j) {
        std::stringstream ss;
        if (size() == 0)
            return "";

        ss << front();

        std::deque<std::string>::const_iterator i = begin() + 1;

        while (i != end()) {
            ss << j << *i;
            i++;
        }
        return ss.str();
    }

    /**
     * \brief append strings to list.
     *
     * Appends the given value to the list of values if it isn't the
     * empty string. "" will be ignored.
     */
    void push_back(const value_type &x) {
        if (x.compare("") == 0)
            return;
        std::deque<value_type>::push_back(x);
    }
};

#endif
