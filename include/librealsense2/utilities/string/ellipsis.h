// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "stringref.h"
#include <sstream>


namespace utilities {
namespace string {


// Output to ostream two strings separated by " ... ". E.g.:
//      This is part ... of a string.
// With:
//      os << ellipsis( part1, part2 );
// 
// If either part is empty, no ellipsis is output. Depending on which part is empty, meaning can be
// conveyed:
//     - if the left is empty, operator bool() will return false
// So, returning an empty ellipsis is like returning an invalid state.
//
struct ellipsis
{
    stringref first, second;

    ellipsis( stringref const & f, stringref const & s )
        : first( f )
        , second( s )
    {
    }
    ellipsis() {}

    stringref::size_type length() const
    {
        stringref::size_type l = first.length() + second.length();
        if( l )
            l += 5;
        return l;
    }

    bool empty() const { return ! first; }
    bool is_valid() const { return ! empty(); }
    operator bool() const { return is_valid(); }

    std::string to_string() const {
        std::ostringstream os;
        os << *this;
        return os.str();
    }
};


inline std::ostream & operator<<( std::ostream & os, ellipsis const & el )
{
    if( el.first )
    {
        os << el.first;
        if( el.second )
            os << " ... " << el.second;
    }
    else if( el.second )
        os << el.second;
    return os;
}


}  // namespace string
}  // namespace utilities
