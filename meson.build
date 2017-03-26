project('mace', 'c')

freetype   = dependency('freetype2')
x11        = dependency('x11')

executable(
	'mace',
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
	dependencies:
		[ freetype, x11 ]
	)
