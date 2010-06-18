#ifndef ZIZLER_H_
#define ZIZLER_H_

#include "Ziz.h"

#include <string>

typedef enum {
    Short,
    Medium,
    Long
} Mode;

std::ostream & operator+ (std::ostream &stream, Ziz::File             const &);
std::ostream & operator+ (std::ostream &stream, Ziz::Block            const &);
std::ostream & operator+ (std::ostream &stream, Ziz::ConditionalBlock const &);

std::ostream & operator<<(std::ostream &stream, Ziz::File             const &);
std::ostream & operator<<(std::ostream &stream, Ziz::Block            const &);
std::ostream & operator<<(std::ostream &stream, Ziz::CodeBlock        const &);
std::ostream & operator<<(std::ostream &stream, Ziz::ConditionalBlock const &);

std::ostream & operator>>(std::ostream &stream, Ziz::File             const &);
std::ostream & operator>>(std::ostream &stream, Ziz::Block            const &);
std::ostream & operator>>(std::ostream &stream, Ziz::CodeBlock        const &);
std::ostream & operator>>(std::ostream &stream, Ziz::ConditionalBlock const &);

std::string Indent(int depth);


bool ziztest(std::string file, Mode mode);

#endif /* ZIZLER_H_ */
