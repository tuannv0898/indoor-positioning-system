HTTP Server
```
sudo python3 manage.py runserver 0.0.0.0:80
```

Beagle Bone Green Wi-fi config
```
sudo connmanctl
scan wifi
services
agent on
connect wifi_884aea625a7c_52544c53_managed_psk
quit
```

/etc/network/interfaces
```
auto wlan0
iface wlan0 inet dhcp
```

/etc/wpa_supplicant/wpa_supplicant.conf
```
network={
    ssid="RTLS"
    scan_ssid=1
    proto=RSN
    key_mgmt=WPA-PSK
    pairwise=CCMP
    group=CCMP
    psk="payitforward"
}
```

```
sudo systemctl disable connman.service
```