project('tutorial', 'cpp', version: '0.1.0', default_options: ['cpp_std=c++20'] )

buildtype = get_option('buildtype')

if buildtype == 'debug'
  add_project_arguments('-DDEBUG', language: 'cpp')
else
  add_project_arguments('-DRELEASE', language: 'cpp')
endif

gtest_dep = dependency('gtest', main : true, required : false)

common_srcs = ['common/SocketUtils.cpp', 'common/TCPSocket.cpp']

test_srcs = ['common/tests/basic.cpp'] + common_srcs
server_srcs = ['server/TCPServer.cpp', 'server/tests/basic.cpp'] + common_srcs

incdir = include_directories('common')

tester = executable('common_tests', sources: test_srcs, include_directories : incdir, dependencies : gtest_dep)
server = executable('server', sources: server_srcs, include_directories : incdir, dependencies : gtest_dep)
test('gtest test', tester)
test('gtest test', server)
# executable('tutorial', sources: srcs, include_directories : incdir)
