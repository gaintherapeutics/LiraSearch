import sys
from rdkit import Chem

mol = Chem.MolFromMolFile(sys.argv[1])
Chem.MolToPDBFile(mol, sys.argv[2])

