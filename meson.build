project('mace', 'c')

add_global_arguments('-std=c11', language : 'c')

cc = meson.get_compiler('c')

x11        = dependency('x11')
cairo      = dependency('cairo')
ft         = dependency('freetype2')
fc         = dependency('fontconfig')
lua        = dependency('lua53')
utf8proc   = cc.find_library('utf8proc', dirs : ['/usr/local/lib'])

executable('mace',
	sources: [
		  'mace.c',
		  'font.c',
		  'utf8.c',
		  'tab.c',
		  'textbox.c',
		  'textboxdraw.c',
		  'selection.c',
		  'sequence.c',
		  'lua.c',
		  'xmain.c'
		],

	include_directories: [
		  include_directories('/usr/local/include'),
	],

	dependencies: [
	          x11,
		  cairo,
		  ft,
		  fc,
		  lua,
		  utf8proc,
		]
	)
