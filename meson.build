project('tutorial', 'cpp', version: '0.1.0', default_options: ['cpp_std=c++20'] )

buildtype = get_option('buildtype')

if buildtype == 'debug'
  add_project_arguments('-DDEBUG', language: 'cpp')
else
  add_project_arguments('-DRELEASE', language: 'cpp')
endif

gtest_dep = dependency('gtest', main : true, required : false)

incdir = include_directories('common')
common_srcs = ['common/SocketUtils.cpp', 'common/TCPSocket.cpp', 'common/TCPServer.cpp']

test_srcs = ['common/tests/basic.cpp']

exchange_srcs = ['exchange/main.cpp', 'exchange/matcher/MEOrder.cpp', 'exchange/matcher/MatchingEngine.cpp', 'exchange/matcher/MEOrderBook.cpp']

lib = static_library('common', common_srcs)

tester = executable('common_tests', sources: test_srcs, include_directories : incdir, dependencies : gtest_dep, link_with : lib)
test('gtest test', tester)

executable('exchange', sources: exchange_srcs, include_directories : incdir, link_with : lib)
