#include <omp.h>
#include <iostream>

int main() {
    #pragma omp parallel
    {
        #pragma omp single
        {
            // Print the OpenMP version used
            std::cout << "OpenMP version " << _OPENMP << std::endl;
        }
    }
    return 0;
}