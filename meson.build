project(
    'Newton particle simulator',
    'cpp',
    default_options : ['warning_level=3', 'werror=true', 'optimization=3', 'cpp_std=c++20']
)
####################################################################################################
# Dependencies
####################################################################################################
deps = []
# For some reason these are needed for kompute
deps += dependency('vulkan')
deps += dependency('fmt')

####################################################################################################
# Source files
####################################################################################################
src = ['main.cpp', 'pasimulations.cpp', 'NewtonPointSimulation.cpp']
####################################################################################################
# Conan
####################################################################################################
conan_pkgs= {
    'mp-units':'mp-units/0.7.0@', # <- Must contain @, otherwise Conan will think it is a path
    'fmt':'fmt/9.0.0@',
    'cxxopts' : 'cxxopts/3.0.0@'   
    
    # ... the dependency list goes on
}
# Adding new dependencies to this dict is error-free, but if you 
# change an existing entry, e.g. the pakage version, you probably need
# `meson configure --clearcache` to let new setting takes effect.

conan_program = find_program(['conan'], required: true, native: true)

deps+=dependency('threads')
foreach pkg_name, conan_ref : conan_pkgs
    module_path = meson.current_build_dir() / 'conan-cmake' / pkg_name
    run_command(conan_program,'install',conan_ref, '-if',module_path,
        '-g','cmake_find_package', check: true)
    deps += dependency(pkg_name, method: 'cmake', cmake_module_path: module_path)
endforeach

############]########################################################################################
# Shaders
####################################################################################################
shader_compiler = find_program(['glslangValidator'], required: true, native: true)
shader_sources = ['testing.comp']
shader_targets = []
foreach shader_source : shader_sources
    shader_targets += custom_target(
        'SPIR-V from @0@'.format(shader_source),
        input: shader_source,
        output: '@PLAINNAME@.spv',
        command: [shader_compiler, '-V', '@INPUT@', '-o', '@OUTPUT@']
    )
endforeach
src += shader_targets
####################################################################################################
# External object files
####################################################################################################
object_files = ['/home/pamiro/koodaus/cpp/libs/kompute/build/src/libkompute.a']

####################################################################################################
# Executable
####################################################################################################
executable('nps', src, dependencies: deps, objects: object_files)

# Command to generate release build dir
'''
CC=gcc-11 CXX=g++-11 meson setup build_release --buildtype=release
'''
