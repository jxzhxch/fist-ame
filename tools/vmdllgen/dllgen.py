# -*- coding: gbk -*-
import os
import os.path
import subprocess
import argparse

exe_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'dll2desc.exe')

def proc_file(file_path, symbol_path, output_dir):

    args = [exe_path]
    if symbol_path:
        args.append('-s')
        args.append(symbol_path)
    if output_dir:
        args.append('-o')
        args.append(output_dir)
    args.append(file_path)
        
    print('开始处理文件', file_path)
    subprocess.call(args)

def main():
    
    parser = argparse.ArgumentParser(description='虚拟机DLL生产器')
    parser.add_argument('--symbol_path', type=str, help='''符号搜索路径，默认为SRV*.\\symbols*http://msdl.microsoft.com/download/symbols''')
    parser.add_argument('--output_dir', type=str, help='输出目录')
    parser.add_argument('input_file_or_dir', nargs=1, type=str, help='输入文件或目录')
    args = parser.parse_args()

    input_file_or_dir = args.input_file_or_dir[0]

    if os.path.isdir(input_file_or_dir):
        for root, dirs, files in os.walk(input_file_or_dir):
            for file_name in files:
                
                file_path = os.path.join(root, file_name)
                
                with open(file_path, 'rb') as ifile:
                    first_bytes = ifile.read(2)
                    if len(first_bytes) != 2 or first_bytes[0] != ord('M') or first_bytes[1] != ord('Z'):
                        continue

                proc_file(file_path, args.symbol_path, args.output_dir)

    else:
        
        proc_file(input_file_or_dir, args.symbol_path, args.output_dir)

if __name__ == '__main__':
    main()
