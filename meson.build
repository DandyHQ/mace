project('mace', 'c', default_options : ['c_std=c11'])

cc = meson.get_compiler('c')

x11        = dependency('x11')
freetype   = dependency('freetype2')
lua        = dependency('lua53')
utf8proc   = cc.find_library('utf8proc', dirs : '/usr/local/lib')

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
		  'xmain.c'
		],

	dependencies: [
	          x11,
		  freetype,
		  utf8proc,
		  lua,
		]
	)
