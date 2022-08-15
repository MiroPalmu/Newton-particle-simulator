shader_compiler = find_program(['glslangValidator'], required: true, native: true)
# Array to store all shaders to be compiled
shader_sources = ['test.comp']

# Add subdirs of different simulations here

shader_targets = []
foreach shader_source : shader_sources
    shader_targets += custom_target(
        'SPIR-V from @0@'.format(shader_source),
        input: shader_source,
        output: '@PLAINNAME@.spv',
        command: [shader_compiler, '-V', '@INPUT@', '-o', '@OUTPUT@']
)
endforeach

project_sources += shader_targets
