
# 定义了一个SRC_LIST变量，包含了当前目录所有文件
aux_source_directory(. SRC_LIST)
# 增加已封装的数据库代码
aux_source_directory(./db SRC_LIST_DB)
aux_source_directory(./model SRC_LIST_MODEL)
aux_source_directory(./redis REDIS_LIST)

# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${SRC_LIST_DB} ${SRC_LIST_MODEL} ${REDIS_LIST})

# 指定可执行文件链接时需要依赖的动态库
target_link_libraries(ChatServer  muduo_net muduo_base mysqlclient hiredis pthread)