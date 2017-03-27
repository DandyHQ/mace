project('mace', 'c')

cc = meson.get_compiler('c')

inc = include_directories('/usr/local/include')

x11        = dependency('x11')
freetype   = dependency('freetype2')
utf8proc   = cc.find_library('utf8proc', dirs : '/usr/local/lib')

executable('mace',
	sources:
		[ 'main.c',
		  'draw.c',
		  'font.c',
		  'mouse.c',
		  'tab.c',
		  'pane.c',
		  'focus.c',
		  'piece.c',
		  'xmain.c'
		],

	include_directories: inc,

	dependencies:
		[ x11, freetype, utf8proc ]
	)
