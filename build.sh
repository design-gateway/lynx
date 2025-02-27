#-----------------------------------------------------------------------
# Build custom Lynx on KR260
#-----------------------------------------------------------------------
# Define the path for DG.cfg
CONFIG_PATH="/home/ubuntu/tool/lynx/etc/DG.cfg"

# Ensure the target directory exists
mkdir -p /home/ubuntu/tool/lynx/etc

# Create DG.cfg with the required network parameter
echo "GATEWAY_IP:192.168.11.2" > "$CONFIG_PATH"
echo "FPGA_IP:192.168.11.84" >> "$CONFIG_PATH"
echo "FPGA_MAC:80.00.11.22.33.44" >> "$CONFIG_PATH"

# Clean previous installations
sudo make uninstall
sudo make distclean

# Configure and build Lynx
./configure --with-ssl=/home/ubuntu/tool/openssl/lib --prefix=/home/ubuntu/tool/lynx
sudo make install