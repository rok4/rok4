#include "Interpolation.h"

namespace interpolation {
    
const char *einterpolation_name[] = {
    "UNKNOWN",
    "nn",
    "linear",
    "bicubic",
    "lanczos_2",
    "lanczos_3",
    "lanczos_4"
};

const int einterpolation_size = 6;


Kernel::KernelType fromString(std::string strInterpolation)
{
    int i;
    for (i=einterpolation_size; i ; --i) {
        if (strInterpolation.compare(einterpolation_name[i])==0)
            break;
    }
    return static_cast<Kernel::KernelType>(i);
}

std::string toString(Kernel::KernelType KT)
{
    return std::string(einterpolation_name[KT]);
}

}
