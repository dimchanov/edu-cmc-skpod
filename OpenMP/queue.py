from sys import argv, exit
import os
import re

def make_script(num_threads_list = [1, 2, 4, 8], 
        sizes_list = [10, 12, 14]
        ) :

    f = open('script', 'w')
    header = '# LSBATCH: User input\n' + \
    '# this file was automaticly created by mpisubmit.pl script for edu-cmc-skpod21-323-11 #\n' + \
    'source /polusfs/setenv/setup.SMPI\n' + \
    '#BSUB -n 1\n' + \
    '#BSUB -W 00:15\n' + \
    '#BSUB -o main.%J.out\n' + \
    '#BSUB -e main.%J.err\n'

    f.write(header)
    for size in sizes_list :
        for num_threads in num_threads_list :
            f.write(f'OMP_NUM_THREADS={num_threads} mpiexec ./main {size} {num_threads}\n')
    f.close()


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

    os.system('xlc -qsmp=omp main.c -o main')
    os.system('bsub < script')
