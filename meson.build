project('mace', 'c')

add_global_arguments('-std=c11', language : 'c')

cc = meson.get_compiler('c')

x11        = dependency('x11')
cairo      = dependency('cairo')
ft         = dependency('freetype2')
lua        = dependency('lua53')
utf8proc   = cc.find_library('utf8proc', dirs : ['/usr/local/lib'])

executable('mace',
	sources: [
		  'main.c',
		  'font.c',
		  'utf8.c',
		  'keys.c',
		  'mouse.c',
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
		  lua,
		  utf8proc,
		]
	)
