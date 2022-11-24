// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <ostream>
#include <cassert>
#include <cstring>


namespace utilities {
namespace string {


// Same as std::string, except not null-terminated, and does not manage its own memory! Similar to the C++17
// std::string_view, except it can be expanded to be non-const, too.
// 
// This is meant to point into an existing string and have a short life-time. If the underlying memory is removed, e.g.:
//      stringref foo()
//      {
//          std::string bar = "haha";
//          return stringref(bar);
//      }
// Not good!
// 
// Most of the string functionality (that does not change the range) can be implemented here as needed, mostly inline...
// 
// This can come in very useful when wanting to break a string into parts but without incurring allocations or copying,
// or changing of the original string contents (like strtok does), or when the original memory is not null-terminated.
//
class stringref
{
public:
    //typedef std::string::const_iterator const_iterator;
    typedef char const * const_iterator;
    typedef std::string::size_type size_type;
    typedef std::string::value_type value_type;

private:
	const_iterator _begin, _end;

public:
    stringref( const_iterator begin, const_iterator end )
        : _begin( begin )
        , _end( end )
    {
        assert( begin <= end );
    }
    stringref( char const * str, size_type length )
        : stringref( str, str + length )
    {
    }
    explicit stringref( char const * str )
        : stringref( str, std::strlen( str ) )
    {
    }
    stringref( std::string const & str )
        : stringref( str.data(), str.length() )
    {
    }
    stringref()
        : _begin( nullptr )
        , _end( nullptr )
    {
    }

    bool empty() const { return begin() == end(); }
    size_type length() const { return end() - begin(); }
    operator bool() const { return ! empty(); }

    void clear() { _end = _begin; }

    const_iterator begin() const { return _begin; }
    const_iterator end() const { return _end; }

    value_type front() const { return *begin(); }
    value_type back() const { return end()[-1]; }
};


inline std::ostream & operator<<( std::ostream & os, stringref const & str )
{
    return os.write( str.begin(), str.length() );
}


}  // namespace string
}  // namespace utilities
