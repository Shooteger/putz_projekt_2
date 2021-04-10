project('connectionsim', 'cpp',
        license : ['Boost'],
        version : '0.1',
        default_options : ['cpp_std=c++2a', 'warning_level=3', 'werror=true'])

thread_dep = dependency('threads')

inc_dir = include_directories(['include',
                               get_option('cli11_include_dir'),
                               get_option('spdlog_include_dir'),
                               get_option('json_include_dir'),
                               get_option('asio_include_dir'),
                               get_option('tabulate_include_dir'),
                               get_option('rang_include_dir')                             
                              ])

#subdir('XXX')

src = ['src/main.cpp']

executable('connectionsim',
            sources : src,
            include_directories : inc_dir,
            dependencies : [thread_dep]
            #link_with : XXX_lib
           )

executable('server',
            sources : ['src/server_new.cpp'],
            include_directories : inc_dir,
            dependencies : [thread_dep]
            #link_with : XXX_lib
           )