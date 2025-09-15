# Debian base image
FROM debian:bookworm-slim

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV MAMBA_ROOT_PREFIX=/opt/micromamba
ENV PATH=$MAMBA_ROOT_PREFIX/bin:$PATH

# Install base dependencies and micromamba
RUN apt-get update && apt-get install -y \
    curl \
    gcc \
    g++ \    
    cmake \
    libfftw3-dev \
    libjpeg-dev \
    libgmp-dev \
    libmpfr-dev \
    libboost-all-dev \
    # libcgal-dev \  # apt version is out of date, we need to compile ourselves
    unzip \
    bzip2 \
    build-essential \
    ca-certificates \
    libglib2.0-0 \
    libxext6 \
    libsm6 \
    libxrender1 \
    software-properties-common \
    librocksdb-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Micromamba
RUN curl -L https://micromamba.snakepit.net/api/micromamba/linux-64/latest | tar -xvj -C /usr/local/bin --strip-components=1 bin/micromamba

# Create Python2 environment for esp-surface-generator and install packages
RUN micromamba create -y -p $MAMBA_ROOT_PREFIX/envs/pyenv \
    python=2.7.15 \
    rdkit=2018.09.3 \
    keras=2.2.4 \
    tensorflow=1.10.0 \
    numpy=1.16.2 \
    xarray=0.11.3 \
    --channel=conda-forge \
    && micromamba clean --all --yes

# Create Python3 environment for LiraSearch and install packages
RUN micromamba create -y -p $MAMBA_ROOT_PREFIX/envs/lirasearch \
    python=3.11 \
    tqdm=4.67.1 \
    scipy=1.15.2 \
    colorama=0.4.6 \
    numpy=2.2.6 \
    rdkit=2025.03.2 \
    openbabel=3.1.1 \
    --channel=conda-forge \
    && micromamba clean --all --yes

# Activate environment by default
ENV PATH=$MAMBA_ROOT_PREFIX/envs/pyenv/bin:$PATH

# Install Node.js and npm v18
RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - && \
    apt-get update && \
    apt-get install -y nodejs

# Install Julia (in /opt/, not as root)
ENV JULIA_VERSION=1.11.4
ENV JULIA_DEPOT_PATH=/opt/.julia
RUN curl -L https://julialang-s3.julialang.org/bin/linux/x64/1.11/julia-$JULIA_VERSION-linux-x86_64.tar.gz -o julia.tar.gz && \
    tar -xvzf julia.tar.gz && \
    mv julia-$JULIA_VERSION /opt/julia && \
    ln -s /opt/julia/bin/julia /usr/local/bin/julia && \
    rm julia.tar.gz

# Install Julia packages
ENV JULIA_CPU_TARGET=x86_64;haswell;skylake;skylake-avx512;tigerlake
RUN julia -e 'using Pkg; Pkg.add.(["Glob", "Printf", "ArgParse", "TimerOutputs", "GeometryBasics", "ImplicitBVH", "Distributions"])'
RUN julia -e 'using Pkg; Pkg.precompile()'

# Install CGAL
RUN curl -L https://github.com/CGAL/cgal/archive/refs/tags/v6.0.1.zip -o cgal.zip && unzip cgal.zip && rm cgal.zip && mkdir cgal-6.0.1/build && cd cgal-6.0.1/build && cmake .. && make install

# Create and switch to GitRepos directory
RUN mkdir /opt/GitRepos
WORKDIR /opt/GitRepos

# Download and unpack repos from GitHub
RUN curl -L https://github.com/AstexUK/ESP_DNN/archive/refs/heads/master.zip -o esp_dnn.zip && unzip esp_dnn.zip && cd ESP_DNN-master && python setup.py install && cd .. && rm -rf esp_dnn.zip ESP_DNN-master/build ESP_DNN-master/dist ESP_DNN-master/esp_dnn/ext
RUN curl -L https://github.com/AstexUK/esp-surface-generator/archive/refs/heads/master.zip -o esp-surface-gen.zip && unzip esp-surface-gen.zip && cd esp-surface-generator-master && npm install && cd .. && ln -s /opt/GitRepos/esp-surface-generator-master/cli.js /usr/bin/esp-surface-generator && rm esp-surface-gen.zip
RUN curl -L https://github.com/simonbray/SHCoeffsDev/archive/refs/heads/main.zip -o SHCoeffs.zip && unzip SHCoeffs.zip && ln -s /opt/GitRepos/SHCoeffsDev-main/sh_coeffs.jl /usr/bin/sh-coeff-calculator && ln -s /opt/GitRepos/SHCoeffsDev-main/Geotools.jl /usr/bin/ && rm SHCoeffs.zip &&  chmod +x /opt/GitRepos/SHCoeffsDev-main/sh_coeffs.jl

# Install from the repos
ADD ShapeSPH.zip /opt/GitRepos
RUN unzip ShapeSPH.zip && cd ShapeSPH/ && make clean && make && mv /opt/GitRepos/ShapeSPH/Bin/Linux/ShapeAlign /usr/bin && cd .. && rm -rf ShapeSPH* 
ADD cif2ply.zip /opt/GitRepos
RUN unzip cif2ply.zip && mkdir cif2ply/build && cd cif2ply/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make && mv /opt/GitRepos/cif2ply/build/cif2ply /usr/bin && cd .. && rm -rf cifply*
WORKDIR /

# Compile rocksdb_to_sdf
COPY restore_sdf_from_rocksdb.cpp /
RUN mkdir /opt/GitRepos/rocksdb_to_sdf && cd /opt/GitRepos/rocksdb_to_sdf && mv /restore_sdf_from_rocksdb.cpp . && g++ -std=c++17 restore_sdf_from_rocksdb.cpp -o /usr/bin/rocksdb_to_sdf -lrocksdb -lz -lbz2 -lsnappy -llz4 -lzstd -pthread

# Add scripts
RUN mkdir /opt/scripts
COPY lira_search_sdf.py /opt/scripts
COPY lira_super.py /opt/scripts
COPY convert_sdf_to_pdb.py /opt/scripts

# Set default shell to bash
SHELL ["/bin/bash", "-c"]

# Default command
CMD ["bash"]

