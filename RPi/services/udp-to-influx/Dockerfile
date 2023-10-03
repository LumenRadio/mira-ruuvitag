FROM python:3.8-slim

# Set the working directory in Docker WORKDIR /usr/src/app

# Copy the dependencies file to the working directory
COPY requirements.txt .

# Install any dependencies
RUN pip install --no-cache-dir -r requirements.txt

# Copy the content of the local src directory to the working directory
COPY udp-to-influx.py .

# Specify the command to run on container start
CMD [ "python", "-u", "udp-to-influx.py" ]
