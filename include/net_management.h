#pragma once

int create_tap_device(char *dev_name);
int get_interface_details(const char *interface_name, char *ip_address, char *netmask_address);
int get_mac_address(const char *interface_name, char *mac_address);
int modify_interface_details(const char *interface_name, const char *new_ip_address, const char *new_netmask);
int delete_interface_details(const char *interface_name);
int modify_mac_address(const char *interface_name, const char *new_mac_address);
void print_raw_packet(const char *pkt);
