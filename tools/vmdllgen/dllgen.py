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
        
    print('��ʼ�����ļ�', file_path)
    subprocess.call(args)

def main():
    
    parser = argparse.ArgumentParser(description='�����DLL������')
    parser.add_argument('--symbol_path', type=str, help='''��������·����Ĭ��ΪSRV*.\\symbols*http://msdl.microsoft.com/download/symbols''')
    parser.add_argument('--output_dir', type=str, help='���Ŀ¼')
    parser.add_argument('input_file_or_dir', nargs=1, type=str, help='�����ļ���Ŀ¼')
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
