cmake_minimum_required(VERSION 3.0)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 设置可执行文件的最终存储目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件搜索路径
# include_directories()
# 配置库文件搜索路径
# link_directories()

# 指定源文件
#set(SRC_LIST muduo.cpp)
aux_source_directory(. SRC_LIST)

# 生成目标文件
add_executable(server ${SRC_LIST})

# 链接库
target_link_libraries(server muduo_net muduo_base pthread)
