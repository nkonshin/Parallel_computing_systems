# cython: language_level=3, boundscheck=False, wraparound=False
# distutils: extra_compile_args = -fopenmp
# distutils: extra_link_args = -fopenmp

import numpy as np
cimport numpy as np
from cython.parallel import prange
cimport cython
from libc.stdlib cimport malloc, free

@cython.boundscheck(False)
@cython.wraparound(False)
def parallel_array_sum(np.ndarray[np.float64_t, ndim=1] arr):
    cdef Py_ssize_t i, n = arr.shape[0]
    cdef double total = 0.0

    # Используем параллельный цикл
    # Cython сам разобьёт работу между потоками
    with nogil:
        for i in prange(n, schedule='static'):
            total += arr[i]
    return total