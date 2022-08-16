project(
    'pasimulations',
    'cpp',
    default_options : ['warning_level=3', 'werror=true', 'optimization=3', 'cpp_std=c++20']
)


####################################################################################################
# Variables tracking sources and libraries
####################################################################################################
# .cpp files (This might be used later for example clang-format)
project_sources = []
# Directories where compiler is able to include from
project_include_directories = []

# Currently unused:
# .hpp files (This might be used later for example clang-format)
project_header_files = [] 
# This might replace external object files if useds 
library_path = [] 


####################################################################################################
# Dependencies
####################################################################################################
project_dependencies = []
# For some reason these are needed for kompute
project_dependencies += dependency('vulkan')
project_dependencies += dependency('fmt')
project_dependencies += dependency('threads')


####################################################################################################
# Conan
####################################################################################################
conan_pkgs= {
    # Names must match the package that cmake looks for
    'mp-units': 'mp-units/0.7.0@', # <- Must contain @, otherwise Conan will think it is a path
    'fmt': 'fmt/9.0.0@',
    'cxxopts': 'cxxopts/3.0.0@',
    'Microsoft.GSL': 'ms-gsl/4.0.0@'
    
    # ... the dependency list goes on
}
# Adding new dependencies to this dict is error-free, but if you 
# change an existing entry, e.g. the pakage version, you probably need
# `meson configure --clearcache` to let new setting takes effect.

conan_program = find_program(['conan'], required: true, native: true)

foreach pkg_name, conan_ref : conan_pkgs
    module_path = meson.current_build_dir() / 'conan-cmake' / pkg_name
    run_command(conan_program, 'install', conan_ref, '-if', module_path,
        '-g', 'cmake_find_package', check: true)
    project_dependencies += dependency(pkg_name, method: 'cmake', cmake_module_path: module_path)
endforeach




####################################################################################################
# External object files
####################################################################################################
object_files = ['/home/pamiro/koodaus/cpp/libs/kompute/build/src/libkompute.a']
####################################################################################################
# Implementation of pasimulations
####################################################################################################
subdir('src')
subdir('include')

project_include_directories += include_directories('include')

############]########################################################################################
# Shaders
####################################################################################################
subdir('shaders')

####################################################################################################
# Executable
####################################################################################################
executable(
    'pasimulations',
    project_sources,
    include_directories: project_include_directories,
    dependencies: project_dependencies,
    objects: object_files
)

# Command to generate release build dir
'''
CC=gcc-11 CXX=g++-11 meson setup build_release --buildtype=release
'''
