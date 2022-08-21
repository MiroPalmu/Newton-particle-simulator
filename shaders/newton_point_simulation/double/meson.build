
shader_compiler = find_program(['glslangValidator'], required: true, native: true)

# Array to store all shaders to be compiled:
shader_sources = files('gpu_1.comp', 'gpu_2.comp')

shader_targets = []
foreach shader_source : shader_sources

    shader_targets += custom_target(
	# We have to replace / with _ because path separators are not supported in target names
        'SPIR-V from @0@'.format(shader_source).replace('/', '_'),
        input: shader_source,
        output: '@PLAINNAME@.spv',
        command: [shader_compiler, '-V', '@INPUT@', '-o', '@OUTPUT@']
)
endforeach

project_sources += shader_targets
