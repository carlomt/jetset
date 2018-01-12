#!/usr/bin/env python

import  glob
from distutils.core import setup,Extension

version='1.2.0'

src_files=['multiJet/jetkernel/jetkernel.i']
src_files.extend(glob.glob ('jetkernel_src/src/*.c'))
_module=Extension('_jetkernel',
                  sources=src_files,
                  extra_compile_options='-fPIC  -v  -c -m64 -I',
                  extra_link_options='-suppress',
                  swig_opts=['-v',],
                  include_dirs=['jetkernel_src/include'])

setup(name='multiJet',
      version=version,
      author='Andrea Tramacere',
      author_email='andrea.tramacere@gmail.com',
      packages=['multiJet','leastsqbound','multiJet.jetkernel'],
      package_data={'multiJet':['Spectral_Templates_Repo/*.dat','test_data/SEDs_data/*dat']},
      scripts=['bin/test_interactive.py'],
      requires=['scipy','numpy'],
      ext_modules = [_module],
      py_modules=['jetkernel'], )