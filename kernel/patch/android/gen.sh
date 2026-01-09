#!/bin/bash

# Shell脚本到C代码生成器 - 将user_init.sh转换为C字符串常量

in_file="user_init.sh"
out_file="gen/user_init.c"

# 初始化C字符串常量定义
c_string='static const char user_init[] = "'

temp_string=""

# 逐行读取输入文件并进行转义处理
while IFS= read -r line || [[ -n "$line" ]]; do
    # 转义反斜杠和双引号，添加换行符
    escaped_line=$(echo "$line" | sed 's/\\/\\\\/g; s/"/\\"/g')
    temp_string+="$escaped_line\\n"
done <"$in_file"

# 完成C字符串定义
c_string+="${temp_string}\";"

# 输出到目标文件
echo "$c_string" >$out_file

# 创建空的userd.c文件（如果不存在）
touch userd.c
