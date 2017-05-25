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
lua        = dependency('lua53', required: false)
if not lua.found()
	lua = dependency('lua', version : '>=5.2.4')
endif

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
