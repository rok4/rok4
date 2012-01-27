#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <string>
#include <string.h>
#include "Kernel.h"

//To declare a new interpolation change the implementation too

namespace Interpolation {
    
    Kernel::KernelType fromString(std::string strInterpolation);
    std::string toString(Kernel::KernelType interpolation);

}

#endif //INTERPOLATION_H
