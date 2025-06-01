# BASE IMAGE
FROM debian:bullseye

# Install PACKAGES
RUN apt-get update && apt-get install -y \
    build-essential \
    gdb \                    
    valgrind \
    binutils \      
    man \  
    less \    
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Default command
CMD [ "bash" ]
