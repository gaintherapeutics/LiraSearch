import csv
import os
import sys
import argparse

import numpy as np

from tqdm import tqdm
from colorama import Fore, Style
from scipy.spatial import distance

#from pyignite import Client
#from pyignite.datatypes.cluster_state import ClusterState


def _make_gen(reader):
    b = reader(1024 * 1024)
    while b:
        yield b
        b = reader(1024*1024)


def rawgencount(filename):
    f = open(filename, 'rb')
    f_gen = _make_gen(reader=f.raw.read)
    return sum(buf.count(b'\n') for buf in f_gen)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--in_file", help="input file for the search")
    parser.add_argument("--out_file", help="output CSV file for the results")
    parser.add_argument("--out_keys", help="output list of keys for the results")
    parser.add_argument("--out_folder", help="output folder containing PDB files")
    parser.add_argument("--database", help="database file to search")
    #parser.add_argument("--ignite_address", help="IP address and port of Ignite server. Default is 127.0.0.1:10800")
    parser.add_argument("--num_results", type=int, default=50, help="number of selected results")
    args = parser.parse_args()

    lead_file = args.in_file
    results_file = args.out_file
    keys_file = args.out_keys
    limit = args.num_results
    data_file = args.database
    #ignite_address = args.ignite_address or '127.0.0.1:10800'
    #ignite_ip, ignite_port = ignite_address.split(':')
    print(Style.RESET_ALL)

    # Section 1 - Load the ligand database
    # Database file and line count
    #data_file = '/home/magellan/Lira_Server/database/fingerprints/rid_coeffs.csv'
    count = rawgencount(filename=data_file)

    codes = []
    vectors_shape = []
    vectors_electrostatics = []

    print()
    print(Fore.LIGHTCYAN_EX + 'Loading in-memory database.' + Fore.LIGHTMAGENTA_EX)
    progress_bar = tqdm(total=count)
    error = 0
    prev_code = ""
    with open(data_file) as csv_file:
        for line in csv.reader(csv_file, delimiter=','):
            if "ligand" in line[0]:
                progress_bar.update(1)
                continue

            code, id = line.pop(0).rsplit('_', 1)
            vector = np.square(np.float32(line))

            progress_bar.update(1)

            if np.isnan(vector).any():
                error += 1
                vector = np.nan_to_num(vector, nan=0.0, posinf=0.0, neginf=0.0)

            if id == 'shape':
                codes.append(code.replace("pdb", "sdf"))
                vectors_shape.append(vector)
                prev_code = code
            else:
                if code != prev_code:
                    print(line)
                    raise Exception(f'Error on {code} != {prev_code}')

                vectors_electrostatics.append(vector)

    progress_bar.close()

    print()
    print(f'Found {error} errors loading the dataset')
    # Weights for the shape descriptor
    print()
    print('Computing metric weights.')
    ws = np.reciprocal(np.square(np.std(np.stack(vectors_shape, axis=0), axis=0)))
    #print(ws)
    #print()

    we = np.reciprocal(np.square(np.std(np.stack(vectors_electrostatics, axis=0), axis=0)))
    #print(we)
    #print()

    # Section 2 - Load the search RID
    with open(lead_file, 'r') as in_file:
        lines = in_file.readlines()

        shape = lines[1].split(',')
        tag = shape.pop(0)

        esp = lines[2].split(',')
        tag = esp.pop(0)

        rid_shape = np.square(np.float32(shape))
        rid_electrostatic = np.square(np.float32(esp))

    tag = tag.split('_')[0]
    print(Style.RESET_ALL)
    print(Fore.LIGHTCYAN_EX + f'Searching database for similarity using file: {lead_file} - {tag}' + Fore.LIGHTMAGENTA_EX)

    vector_count = len(vectors_shape)
    distances_shape = np.zeros(vector_count)
    distances_electrostatic = np.zeros(vector_count)
    for i in tqdm(range(vector_count)):
        distances_shape[i] = distance.euclidean(rid_shape, vectors_shape[i], w=ws)
        distances_electrostatic[i] = distance.euclidean(rid_electrostatic, vectors_electrostatics[i], w=we)

    sorted_distances_shape = np.argsort(distances_shape)

    top = sorted_distances_shape[:limit]

    print(Fore.LIGHTCYAN_EX)
    print(top)
    print([codes[i] for i in top])

    #client = Client()
    #with client.connect(ignite_ip, int(ignite_port)):
    #    client.get_cluster().set_state(ClusterState.ACTIVE)
    #    my_cache = client.get_cache('zincdb')
#
    with open(results_file+'.csv', 'w') as out_file:
        out_file.write('code,shape_score,elec_score\n')
        for i in top:
            out_file.write(f'{codes[i]},{distances_shape[i]:.4f},{distances_electrostatic[i]:.4f}\n')
    with open(keys_file, 'w') as keys_f:
        for i in top:
            keys_f.write(codes[i])
            keys_f.write('\n')


#                with open(f'./{results_folder}/{codes[i]}', 'w') as sdf_file:
#                    molecule = my_cache.get(codes[i])
#                    print(molecule)
#                    print(type(molecule))
#                    sdf_file.write(''.join(molecule))

    print()
    print(np.max(distances_shape))
    print(Style.RESET_ALL)


if __name__ == '__main__':
    main()

