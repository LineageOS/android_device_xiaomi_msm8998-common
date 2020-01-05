#!/vendor/bin/sh

WLAN_MAC="/mnt/vendor/persist/wlan_bt/wlan.mac"

# data format:
# Intf0MacAddress=00AA00BB00CC
# Intf1MacAddress=00AA00BB00CD
# END
WLAN_MAC_BIN="/mnt/vendor/persist/wlan_mac.bin"

if [ ! -s "${WLAN_MAC}" ]; then
    exit
fi

# Read the mac from persist
raw_mac=$(od -An -t x1 $WLAN_MAC | tr -d '\n ')

# Convert to decimal
dec_mac=$(printf "%d" "0x$raw_mac")

# The MAC of the first interface is the decimal mac,
# converted to uppercase
first_mac=$(printf "%08X" "$dec_mac")

# Increment the decimal mac by one
dec_mac=$(echo "$dec_mac + 1" | bc)

# The MAC of the first interface is the decimal mac
# plus one, converted to uppercase
second_mac=$(printf "%08X" "$dec_mac")

# Write the MACs
echo "Intf0MacAddress=${first_mac}" > "${WLAN_MAC_BIN}"
echo "Intf1MacAddress=${second_mac}" >> "${WLAN_MAC_BIN}"
echo "END" >> "${WLAN_MAC_BIN}"
