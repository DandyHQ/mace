project('mace', 'c')

add_global_arguments('-std=c11',
		     '-Wpointer-sign',
		     '-Werror',
		     '-O3',
		     language : 'c')

x11        = dependency('x11')
ft         = dependency('freetype2')
fc         = dependency('fontconfig')

hb         = dependency('harfbuzz-icu')

cairo      = dependency('cairo')

lua        = dependency('lua53', required: false)
if not lua.found()
	lua = dependency('lua', version : '>=5.2.4')
endif

#path = join_paths(get_option('datadir'), 'mace')
# Ubuntu has a very old version of meson which doesn't have join_paths.
path = get_option('datadir') + '/mace'

conf_data = configuration_data()
conf_data.set('version', '0.1.0')
#conf_data.set('path', join_paths(get_option('prefix'), path))
conf_data.set('path', get_option('prefix') + '/' + path)

configure_file(input : 'config.h.in',
	       output : 'config.h',
	       configuration : conf_data)


install_data('macerc.lua', install_dir : path)
install_data('mace.lua', install_dir : path)

executable('mace', install: true,
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
	     ft,
	     fc,
             hb,
	     cairo,
	     lua,
	   ],
	  )


seqtest = executable('tests/sequence',
		     sources: [
		       'tests/sequence.c',
		       'sequence.c',
		       'utf8.c',
		     ],

		     dependencies: [
		       cairo,
		       ft,
		       fc,
		       lua,
		     ]
		    )

test('Sequence Test', seqtest)
