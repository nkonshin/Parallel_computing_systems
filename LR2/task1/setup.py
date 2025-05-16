from setuptools import setup
from setuptools.extension import Extension
from Cython.Build import cythonize
import numpy

ext_modules = [
    Extension(
        name="sum_omp",
        sources=["sum_omp.pyx"],
        extra_compile_args=["-fopenmp"],
        extra_link_args=["-fopenmp"],
        include_dirs=[numpy.get_include()]
    )
]

setup(
    name="sum_omp",
    ext_modules=cythonize(ext_modules, compiler_directives={'language_level': "3"}),
)