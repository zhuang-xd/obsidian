#!/usr/bin/env python3
import subprocess
import sys
import os
import re

def replace_array_content(source_c_file, target_file):
    """
    将生成的C文件中的十六进制数据替换到目标文件的数组中

    Args:
        source_c_file: 生成的C文件路径
        target_file: 目标文件路径(sensorParam_sc4336_mipi_attr.c)
    """
    if not os.path.exists(source_c_file):
        print(f"错误: 源C文件不存在: {source_c_file}")
        return False

    if not os.path.exists(target_file):
        print(f"错误: 目标文件不存在: {target_file}")
        return False

    try:
        with open(source_c_file, 'r') as f:
            source_lines = f.readlines()

        # 提取所有十六进制数据
        hex_data = []
        for line in source_lines:
            matches = re.findall(r'0x[0-9a-fA-F]{2}', line)
            hex_data.extend(matches)

        with open(target_file, 'r') as f:
            target_lines = f.readlines()

        # 找到数组的开始和结束
        start_idx = None
        end_idx = None

        for i, line in enumerate(target_lines):
            if 'char NAME(isp_param_buff)[] = {' in line:
                start_idx = i
            if start_idx is not None and line.strip() == '};':
                end_idx = i
                break

        if start_idx is None or end_idx is None:
            print("错误: 无法找到数组的开始或结束位置")
            return False

        # 格式化新的数组内容
        new_array_lines = []
        for i in range(0, len(hex_data), 16):
            chunk = hex_data[i:i+16]
            line_content = ', '.join(chunk)
            if i + 16 < len(hex_data):
                line_content += ','
            new_array_lines.append(line_content)

        # 构建新的文件内容
        new_content = []
        new_content.extend(target_lines[:start_idx+1])  # 保留声明行
        new_content.extend([line + '\n' for line in new_array_lines])  # 添加新数据
        new_content.extend(target_lines[end_idx:])  # 保留结束行及之后的内容

        # 写回文件
        with open(target_file, 'w') as f:
            f.writelines(new_content)

        return True

    except Exception as e:
        print(f"替换过程中发生错误: {e}")
        return False

def convert_hex_to_c(hex_file_path, bin2array_path=None, auto_replace=True):
    """
    使用bin2array工具将hex文件转换为C文件，并可选地替换到目标文件

    Args:
        hex_file_path: hex文件路径
        bin2array_path: bin2array工具路径(可选,默认为./tools/bin2array)
        auto_replace: 是否自动替换到sensorParam_sc4336_mipi_attr.c(默认为True)
    """
    if not os.path.exists(hex_file_path):
        print(f"错误: hex文件不存在: {hex_file_path}")
        return False

    if bin2array_path is None:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        bin2array_path = "./bin2array"

    if not os.path.exists(bin2array_path):
        print(f"错误: bin2array工具不存在: {bin2array_path}")
        return False

    try:
        hex_file_abs = os.path.abspath(hex_file_path)
        c_file_abs = os.path.abspath(hex_file_path) + ".c"
        bin2array_abs = os.path.abspath(bin2array_path)

        if os.path.exists(c_file_abs):
            os.remove(c_file_abs)

        result = subprocess.run(
            [bin2array_abs, hex_file_abs],
            capture_output=True,
            text=True,
            check=True
        )

        if result.returncode == 0:
            print(result.stdout)

            output_c_file = hex_file_path + ".c"
            if os.path.exists(output_c_file):
                os.chmod(output_c_file, 0o644)
                file_size = os.path.getsize(output_c_file)

                # 自动替换到目标文件
                if auto_replace:
                    # 获取目标文件路径
                    script_dir = os.path.dirname(os.path.abspath(__file__))
                    target_file = os.path.join(script_dir, "../make_arcfirmware/app/arc_rpc_demo/media/isp/src/sensor_param/sensorParam_ov04c10_mipi_attr.c")

                    if os.path.exists(target_file):
                        _ = replace_array_content(output_c_file, target_file)
                        os.remove(output_c_file)
                    else:
                        print(f"\n提示: 目标文件不存在，跳过自动替换: {target_file}")

            return True
        else:
            print(f"转换失败: {result.stderr}")
            return False

    except subprocess.CalledProcessError as e:
        print(f"执行bin2array时出错: {e}")
        print(f"错误输出: {e.stderr}")
        return False
    except Exception as e:
        print(f"发生错误: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("用法: python convert_hex_to_c.py <hex文件路径> [bin2array路径] [--no-replace]")
        print("示例: python convert_hex_to_c.py tools/YQ_mc3303_sc4336p_20260107_v2.hex")
        print("      python convert_hex_to_c.py tools/YQ_mc3303_sc4336p_20260107_v2.hex --no-replace")
        sys.exit(1)

    hex_file = sys.argv[1]
    bin2array_path = None
    auto_replace = True

    # 解析参数
    for i in range(2, len(sys.argv)):
        if sys.argv[i] == "--no-replace":
            auto_replace = False
        else:
            bin2array_path = sys.argv[i]

    success = convert_hex_to_c(hex_file, bin2array_path, auto_replace)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
