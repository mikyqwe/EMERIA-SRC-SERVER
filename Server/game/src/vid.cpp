#include "stdafx.h"
#include "vid.h"
#include <boost/functional/hash.hpp>

// 16.06.2016 Soluzione dell'Hash della classe VID || Arves100 ||
std::size_t hash_value(VID const& v) {
    boost::hash<DWORD> hasher;
    return hasher(v.getID());
}