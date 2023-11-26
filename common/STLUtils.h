#pragma once

#include <algorithm>
#include <vector>

template <typename T> void RemoveElementFromArray(std::vector<T> &vct, T const &element)
{
    vct.erase(std::remove(vct.begin(), vct.end(), element), vct.end());
}
