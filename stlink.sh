# Install pre-requisities
sudo apt-get install git make cmake libusb-1.0-0-dev
sudo apt-get install gcc build-essential
# Build st-link and install from source
git clone https://github.com/stlink-org/stlink
cd stlink
cmake .
sudo make install
sudo ldconfig
cd ..
rm -R stlink

