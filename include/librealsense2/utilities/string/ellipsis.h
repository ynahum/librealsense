// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "slice.h"
#include <sstream>


namespace utilities {
namespace string {


// Pair of slices, from one or two different sources.
// These can be put together using constructs like ellipsis...
//
struct twoslice
{
    typedef slice::size_type size_type;

    slice first, second;

    twoslice( slice const & f, slice const & s )
        : first( f )
        , second( s )
    {
    }
    twoslice() {}

    size_type length() const { return first.length() + second.length(); }

    bool empty() const { return ! length(); }

    // empty is also invalid, but invalid does not necessarily mean empty!
    bool is_valid() const { return ! first; }
    operator bool() const { return is_valid(); }
};


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
struct ellipsis : twoslice
{
    static constexpr size_type const extra_length = 5;  // " ... "

    size_type length() const
    {
        size_type l = twoslice::length();
        if( l )
            l += extra_length;
        return l;
    }

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
