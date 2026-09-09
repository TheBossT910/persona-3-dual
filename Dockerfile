# Persona 3 Dual - Developer Environment
#
# Based on the official BlocksDS image
# Adds build dependencies

# ==========================================
# Base stage - only for GitHub ACTIONS
# GitHub Actions targets this stage
# ==========================================

# Use the dev-latest because it contains the source code of blocksds, so we can step in on the debugger
FROM skylyrac/blocksds:dev-v1.22.2 AS base

LABEL maintainer="The P3D Project"
LABEL description="Full build environment for Persona 3 Dual (NDS homebrew)"

# Suppress interactive apt prompts
ENV DEBIAN_FRONTEND=noninteractive

# System packages
# ffmpeg        – video/audio asset conversion (used by the asset pipeline)
# mtools        – FAT image creation (sdcard.img)
# libblas3      – required by ffmpeg at runtime (update-alternatives symlink)
# liblapack3    – same as above
# python3 / pip – asset pipeline scripts
# zip / gzip    – packaging release artifacts
# git-lfs       – large file storage (LFS pointers resolved during CI checkout)
# ccache        – compiler cache for faster rebuilds (CI manages cache via actions/cache)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential=12.10ubuntu1 \
    cmake=3.28.3-1build7 \
    ffmpeg=7:6.1.1-3ubuntu5 \
    mtools=4.0.43-1build1 \
    libblas3=3.12.0-3build1.1 \
    liblapack3=3.12.0-3build1.1 \
    python3=3.12.3-0ubuntu2.1 \
    python3-pip=24.0+dfsg-1ubuntu1.3 \
    python3-venv=3.12.3-0ubuntu2.1 \
    zip=3.0-13ubuntu0.2 \
    gzip=1.12-1ubuntu3.2 \
    git-lfs=3.4.1-1ubuntu0.4 \
    ccache=4.9.1-1 \
    && git lfs install --system \
    && rm -rf /var/lib/apt/lists/*

# Python virtual environment for CI (GitHub Actions runs as root)
# The Makefile calls $(HOME)/.venv/bin/python3 directly, so this only needs
# to exist at /root/.venv
# No need to touch PATH.
COPY tools/requirements.txt /tmp/requirements.txt
RUN python3 -m venv /root/.venv \
    && /root/.venv/bin/pip install --no-cache-dir --upgrade pip \
    && /root/.venv/bin/pip install --no-cache-dir -r /tmp/requirements.txt

# ==========================================
# Dev Container stage - only for developers
# GitHub Actions targets base only
# ==========================================
FROM base AS devcontainer

# Developers specific packages
# sudo          – just for the case aigis need root access
# gdb-multiarch – debugger
RUN apt-get update && apt-get install -y --no-install-recommends \
    sudo \
    gdb-multiarch \
    && rm -rf /var/lib/apt/lists/*

# Add aigis user so we don't run as root on dev container
# Also gives access to opt/wonderful (Wonderful Toolchain)
RUN userdel -r ubuntu \
    && useradd -m aigis \
    && chown -R aigis:aigis /opt/wonderful

# Give sudo access to aigis
RUN echo "aigis ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/aigis \
    && chmod 0440 /etc/sudoers.d/aigis \
    && visudo -c -f /etc/sudoers.d/aigis

USER aigis

# venv for aigis (same requirements.txt as CI, resolved via $(HOME) in Makefile)
RUN python3 -m venv "$HOME/.venv" \
    && "$HOME/.venv/bin/pip" install --no-cache-dir --upgrade pip \
    && "$HOME/.venv/bin/pip" install --no-cache-dir -r /tmp/requirements.txt

# Default: drop into a shell so developers can run make, explore, debug, etc.
CMD ["/bin/bash"]
