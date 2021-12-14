from sys import argv, exit
import os
import re

def make_script(num_threads_list = [1, 2, 4, 8], 
        sizes_list = [10, 12, 14]
        ) :

    for size in sizes_list :
        for num_threads in num_threads_list :
            for i in range(1, 4, 1) :
                filename = f'run_{size}_{num_threads}_{i}'
                ex_str = f'mpisubmit.pl -p {num_threads} --stdout {filename}.out --stderr {filename}.err ./run -- {size} \n'
                # print(ex_str)
                os.system(ex_str)


if __name__ == '__main__':

    num_threads_list = None
    sizes_list = None

    if len(argv) > 3 :
        print('*****Enter no more than two lines like \"[number, number]\"*****')
        exit(0)
    elif len(argv) == 3 :
        # print(re.findall(r'\w+', re.sub(r'[^ \d]', ' ', argv[1])))
        sizes_list = [int(elem) for elem in re.findall(r'\w+', re.sub(r'[^ \d]', ' ', argv[1]))]
        # print(re.findall(r'\w+', re.sub(r'[^ \d]', ' ', argv[2])))
        num_threads_list = [int(elem) for elem in re.findall(r'\w+', re.sub(r'[^ \d]', ' ', argv[2]))]
        make_script(num_threads_list, sizes_list)
    else :
        make_script()

    # os.system('mpicc -O3 main.c -o main')
    # os.system('bsub < script')
