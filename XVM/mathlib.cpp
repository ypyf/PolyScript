#include "mathlib.h"

namespace math 
{
    int IntPow(int base, int index)
    {
        int r = base;
        index--;
        while(index--)
            r *= base;
        return r;
    }

}	// namespace math