project('mace', 'c')

add_global_arguments('-std=c11',
		     '-Wpointer-sign',
		     '-Werror',
		     '-O3',
		     language : 'c')

x11        = dependency('x11')
cairo      = dependency('cairo')
ft         = dependency('freetype2')
fc         = dependency('fontconfig')
lua        = dependency('lua52')

executable('mace',
	   sources: [
	     'mace.c',
	     'pane.c',
	     'tab.c',
	     'textbox.c',
	     'textboxdraw.c',
	     'selection.c',
	     'sequence.c',
	     'font.c',
	     'utf8.c',
	     'lua.c',
	     'xmain.c'
	   ],

	   dependencies: [
	     x11,
	     cairo,
	     ft,
	     fc,
	     lua,
	   ]
	  )
