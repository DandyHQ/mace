project('mace', 'c')

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
		'resources/Unity/src/unity.c',
		'tests/sequence.c',
		'sequence.c',
		'utf8.c',
	],
)

test('Sequence Test', seqtest)

utf8test = executable('tests/utf8',
	sources: [
		'resources/Unity/src/unity.c',
		'tests/utf8.c',
		'utf8.c',
	],
)

test('utf8 Test', utf8test)
