project('mace', 'c', default_options : ['c_std=c11'])

x11        = dependency('x11')
ft         = dependency('freetype2')
fc         = dependency('fontconfig')
cairo      = dependency('cairo')

conf_data = configuration_data()
conf_data.set('version', '0.2')

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
		'cursel.c',
		'sequence.c',
		'font.c',
		'utf8.c',
		'cmd.c',
		'main.c',
		
		'resources/tomlc99/toml.c',

		'xmain.c'
	],

	dependencies: [
		x11,
		ft,
		fc,
		cairo,
	])

subdir('tests')
