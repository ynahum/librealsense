// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "json.h"


namespace utilities {
namespace string {


// Given an outside block, e.g.:
//        012345678901234567890123456789012345678901234567890123456789012345678901234567890
//        0         1         2         3         4         5         6         7         8
//        {"one":1,"two":[1,2,3],"three":{"longassblock":{"insideblock":89012}},"four":4}
// Find the first inside block, including the enclosing delimiters (parenthesis or square brackets):
//                       ^______^        ^_____________________________________^
//
static stringref find_inside_block( stringref const & outside )
{
    // find an opening
    stringref::const_iterator it = outside.begin() + 1;  // opening {, separating comma, etc.
    bool in_quote = false;
    while( it < outside.end() && ( in_quote || *it != '[' && *it != '{' ) )
    {
        if( *it == '"' )
            in_quote = ! in_quote;
        ++it;
    }
    if( it >= outside.end() )
        return stringref();
    assert( ! in_quote );
    auto const begin = it;
    char const open = *begin;

    // find the closing
    char const close = ( *it == '[' ) ? ']' : '}';
    int nesting = 0;
    while( 1 )
    {
        if( ++it == outside.end() )
            // Something is invalid
            return stringref();

        if( *it == '"' )
            in_quote = ! in_quote;
        else if( ! in_quote )
        {
            if( *it == close )
            {
                if( 0 == nesting )
                    break;
                --nesting;
            }
            else if( *it == open )
                ++nesting;
        }
    }
    return stringref( begin, it + 1 );  // including the enclosing {} or []
}


// max-length output
// ---------- ---------------------------------------------------
//          7 { ... }
//          8 {" ... }
//          9 {"a ... }
//         22 {"a[1]":1,"b[2": ... }
//         29 {"a[1]":1,"b[2":3,"d":[ ... }
//         41 {"a[1]":1,"b[2":3,"d":[1,2,{3,4,5, ... ]}
//         42 {"a[1]":1,"b[2":3,"d":[1,2,{ ... },6,7,8]}
//         43 {"a[1]":1,"b[2":3,"d":[1,2,{3 ... },6,7,8]}
//         49 {"a[1]":1,"b[2":3,"d":[1,2,{3,4,5,6 ... },6,7,8]}
//         50 {"a[1]":1,"b[2":3,"d":[1,2,{3,4,5,6,7,8,9},6,7,8]}   <--   original json string
//
ellipsis shorten_json_string( stringref const & str, size_t max_length )
{
    if( str.length() <= max_length )
        // Already short enough
        return ellipsis( str, stringref() );
    if( max_length < 7 )
        return ellipsis( stringref(), str );  // impossible: "{ ... }" is the minimum

    // Try to find an inside block that can be taken out
    ellipsis final;
    stringref range = str;
    while( stringref block = find_inside_block( range ) )
    {
        // Removing the whole block is one option:
        //        0123456789012345678901234567890123456789012345678
        //        0         1         2         3         4
        //        {"one":1,"two":[1,2,3],"three":{ ... },"four":4}
        //        ^_______________________________^    ^__________^
        ellipsis candidate( stringref( str.begin(), block.begin() + 1 ), stringref( block.end() - 1, str.end() ) );
        if( candidate.length() <= max_length && candidate.length() > final.length() )
            final = candidate;

        // But we might be able to remove only an inside block and get a better result:
        auto inside_max_length = int(max_length) - ( block.begin() - str.begin() ) - ( str.end() - block.end() );
        if( inside_max_length > 6 )
        {
            auto inside = shorten_json_string( block, inside_max_length );
            if( inside )
            {
                // See if shortening the inside block gives a better result than shortening
                // the outside...
                //        {"one":1,"two":[1,2,3],"three":{"longassblock":{ ... }},"four":4}
                //                                       ^________________^    ^_^
                ellipsis inside_candidate( stringref( str.begin(), inside.first.end() ),
                                           stringref( inside.second.begin(), str.end() ) );
                if( inside_candidate.length() <= max_length  &&  inside_candidate.length() > final.length() )
                    final = inside_candidate;
            }
        }

        // Next iteration, continue right after this block
        range = stringref( block.end(), str.end() );
    }

    // The minimal solution is to shorten the current block at the end (if we can't find anything else)
    if( ! final )
        final = ellipsis( stringref( str.begin(), str.begin() + max_length - 6 ),
                          stringref( str.end() - 1, str.end() ) );
    return final;
}


}  // namespace string
}  // namespace utilities
