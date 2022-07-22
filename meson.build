project('Newton particle simulator', 'cpp', default_options : ['warning_level=3', 'werror=true', 'optimization=3', 'cpp_std=c++20'])


conan_pkgs= {
    'mp-units':'mp-units/0.7.0@', # <- Must contain @, otherwise Conan will think it is a path
    'fmt':'fmt/9.0.0@'    
    
    # ... the dependency list goes on
}
# Adding new dependencies to this dict is error-free, but if you 
# change an existing entry, e.g. the pakage version, you probably need
# `meson configure --clearcache` to let new setting takes effect.

deps=[]
deps+=dependency('threads')
foreach pkg_name, conan_ref : conan_pkgs
    module_path = meson.current_build_dir() / 'conan-cmake' / pkg_name
    run_command('conan','install',conan_ref, '-if',module_path,
        '-g','cmake_find_package', check: true)
    deps += dependency(pkg_name, method: 'cmake', cmake_module_path: module_path)
endforeach

src = ['main.cpp']


executable('nps', src, dependencies: deps)

# Command to generate release build dir
#CC=gcc-11 CXX=g++-11 meson setup build_release --buildtype=release
