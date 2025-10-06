# LiraSearch

**Ultrafast shape and electrostatic ligand similarity search**

LiraSearch is a tool / library for performing high-performance ligand similarity searches based on molecular shape and electrostatic potentials. It is designed for use in computational chemistry / drug discovery pipelines.

## Installation

### Galaxy webserver

First of all, you can avoid installation completely by using the publicly available webserver at https://usegalaxy.eu/?tool_id=lirasearch. This also allows you access to the precomputed compound databases used for similarity search.

### Docker container

If you want to run via the command-line, the simplest way is to use the Docker container: `docker pull sb17/lirasearch`. This is used by the webserver above.

Or if you want to build the container yourself:

```bash
# Clone the repo
git clone https://github.com/gaintherapeutics/LiraSearch.git
cd LiraSearch/docker
docker build -t lirasearch:latest .
```

After entering the container interactively with `docker run -it sb17/lirasearch` you can run the following commands:

```bash
python -m esp_dnn.predict -m ligand -i . -o .           # predict ESP charges using the DNN method by Rathi et al., 2019, J. Med. Chem
esp-surface-generator ligand.pdb.pqr ligand.pdb.tmesh 	# calculate a tmesh file representing the ESP surface, code from the authors above
sh-coeff-calculator .                                   # calculate spherical harmonics coefficients
```

### Installation without container

You can also run the LiraSearch code without using the container. First install Julia; you will also need to install the required dependencies with

```
julia -e 'using Pkg; Pkg.add.(["Glob", "Printf", "ArgParse", "TimerOutputs", "GeometryBasics", "ImplicitBVH", "Distributions"])'
```

For installation and running the ESP-DNN code, please see the repos of the original authors from Astex:
* https://github.com/AstexUK/ESP_DNN
* https://github.com/AstexUK/esp-surface-generator
* https://pubs.acs.org/doi/full/10.1021/acs.jmedchem.9b01129 (paper from J. Med. Chem.)

## Usage

Starting from a directory containing tmesh files, run `julia sh_coeffs.jl .`. You can test in the example directory:

```
cd example
julia ../sh_coeffs.jl .

```

## Contributing

1. Fork the repository
2. Create a new branch (git checkout -b feature/foo)
3. Write code 
4. Submit a pull request

Please raise issues on this GitHub repository.

## License

This project is released under the MIT License.
