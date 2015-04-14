# bluepi

sudo hciconfig hci0 leadv 3
sudo hcitool lescan --duplicates > /dev/null &
hcidump -d 0.0.0.0 &
