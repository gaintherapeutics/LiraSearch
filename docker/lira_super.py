import os
import glob
import argparse
import time
import numpy as np

from tqdm import tqdm

from rdkit import Chem
from rdkit.Chem import AllChem, SDWriter


def load_rotation_matrix(file_path):
    matrix = np.loadtxt(file_path)
    if matrix.shape != (3, 3):
        raise ValueError("Rotation matrix must be 3x3.")
    return matrix


def compute_centroid(mol):
    conf = mol.GetConformer()
    coords = np.array([conf.GetAtomPosition(i) for i in range(mol.GetNumAtoms())])
    return coords.mean(axis=0)


def center_molecule(mol, centroid):
    conf = mol.GetConformer()
    for i in range(mol.GetNumAtoms()):
        pos = np.array(conf.GetAtomPosition(i))
        conf.SetAtomPosition(i, pos - centroid)


def rotate_molecule(mol, rotation_matrix):
    conf = mol.GetConformer()
    for i in range(mol.GetNumAtoms()):
        pos = np.array(conf.GetAtomPosition(i))
        conf.SetAtomPosition(i, rotation_matrix @ pos)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", help="Target PDB file for superposition")
    parser.add_argument("--folder", default="search_results", help="Folder containing SDF files to align")
    args = parser.parse_args()

    lead_file = args.target
    results_folder = args.folder

    name, _ = os.path.splitext(lead_file)

    # Load and center the target molecule
    #suppl = Chem.SDMolSupplier(lead_file, removeHs=False)
    #mol = suppl[0] if suppl and suppl[0] is not None else None
    mol = Chem.MolFromPDBFile(lead_file)
    if mol is None:
        raise ValueError(f"Failed to load PDB file: {lead_file}")

    lead_centroid = compute_centroid(mol)
    center_molecule(mol, lead_centroid)

    with open(f"{name}.origin.sdf", 'w') as f:
        f.write(Chem.MolToMolBlock(mol))

    os.system(f"obabel -isdf {name}.origin.sdf -ocif -O{name}.origin.cif > /dev/null 2>&1")
    t = time.time()
    os.system(f"cif2ply {name}.origin.cif {results_folder}/target.ply > /dev/null 2>&1")
    print("target cifply", time.time() - t)
    os.remove(f"{name}.origin.cif")

    # Process search results
    files = [file for file in glob.glob(f"{results_folder}/*.sdf") if "rotated" not in file]
    for file in tqdm(files):
        name, _ = os.path.splitext(file)

        os.system(f"obabel -isdf {file} -ocif -O{name}.cif > /dev/null 2>&1")
        t = time.time()
        os.system(f"cif2ply {name}.cif {name}.ply > /dev/null 2>&1")
        print("sdf cifply", time.time() - t)
        os.remove(f"{name}.cif")

        t = time.time()
        os.system(f"ShapeAlign --in1 {name}.ply --in2 {results_folder}/target.ply --out {name}.sup.ply > {name}.rot")
        print("sdf shapealign", time.time() - t)

        suppl = Chem.SDMolSupplier(file, removeHs=True, sanitize=False)
        mol = suppl[0] if suppl and suppl[0] is not None else None
        if mol is None:
            print(f"Failed to load SDF file {file}.")
            continue

        rot_matrix = load_rotation_matrix(f"{name}.rot")

        centroid = compute_centroid(mol)
        center_molecule(mol, centroid)
        rotate_molecule(mol, rot_matrix)
        center_molecule(mol, -lead_centroid)

        with open(f"{name}.rotated.sdf", 'w') as f:
            f.write(Chem.MolToMolBlock(mol))

        writer = SDWriter(f"{name}.rotated.sdf")
        writer.write(mol)
        writer.close()

        os.remove(f"{name}.sdf")
        os.remove(f"{name}.rot")
        os.remove(f"{name}.ply")
        os.remove(f"{name}.sup.ply")

    # final_directory = f"{results_folder}/tmp"
    # if not os.path.isdir(final_directory):
    #     os.mkdir(final_directory)

    # os.system(f"cp {results_folder}/*.rotated.sdf {results_folder}/tmp")
    # os.system(f"for f in {results_folder}/tmp/*.sdf; do mv \"$f\" \"${{f%.sdf}}.mol\"; done")
    # os.system(f"mamba run -n esp-dnn-env python -m esp_dnn.predict -m ligand -i {results_folder}/tmp")
    os.remove(f"{results_folder}/target.ply")


if __name__ == "__main__":
    main()


