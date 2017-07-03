project('mace', 'c')

add_global_arguments('-std=c11',
		     '-Wpointer-sign',
		     '-Werror',
		     '-O3',
		     language : 'c')

x11        = dependency('x11')
ft         = dependency('freetype2')
fc         = dependency('fontconfig')
cairo      = dependency('cairo')

conf_data = configuration_data()
conf_data.set('version', '0.1.0')

configure_file(input : 'config.h.in',
	       output : 'config.h',
	       configuration : conf_data)

executable('mace', install: true,
	sources: [
		'mace.c',
		'pane.c',
		'tab.c',
		'textbox.c',
		'textboxdraw.c',
		'selection.c',
		'cursor.c',
		'sequence.c',
		'font.c',
		'utf8.c',
		'cmd.c',
		'resources/xlib-keysyms.c',
		'xmain.c'
	],

	dependencies: [
		x11,
		ft,
		fc,
		cairo,
	])


seqtest = executable('tests/sequence',
	sources: [
		'tests/sequence.c',
		'sequence.c',
		'utf8.c',
		'font.c'
	],

	dependencies: [
		cairo,
		fc,
		ft,
	]
)

test('Sequence Test', seqtest)
