project('mace', 'c')

add_global_arguments('-std=c11', language : 'c')

cc = meson.get_compiler('c')

x11        = dependency('x11')
freetype   = dependency('freetype2')
lua        = dependency('lua52')
utf8proc   = cc.find_library('utf8proc', dirs : ['/usr/local/lib'])

executable('mace',
	sources: [
		  'main.c',
		  'draw.c',
		  'font.c',
		  'mouse.c',
		  'tab.c',
		  'pane.c',
		  'panedraw.c',
		  'textbox.c',
		  'textboxdraw.c',
		  'selection.c',
		  'keys.c',
		  'piece.c',
		  'lua.c',
		  'utf8.c',
		  'command.c',
		  'xmain.c'
		],

	include_directories: [
		  include_directories('/usr/local/include'),
	],

	dependencies: [
	          x11,
		  freetype,
		  utf8proc,
		  lua,
		]
	)
