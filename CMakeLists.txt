cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(Improved-SQL-plus)
enable_testing()
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Werror -std=gnu99")

#####
# Détection de la bibliothèque Readline
#####
find_library(READLINE_LIBRARY NAMES readline libreadline history libhistory) 

add_executable(im-sql-plus src/readcmd.c src/main.c)
target_link_libraries(im-sql-plus readline history)


