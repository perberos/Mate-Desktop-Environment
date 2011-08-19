/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2006 Carlos Garnacho.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro <carlosg@mate.org>.
 */

#include "gst-service-role.h"
#include <stdlib.h>
#include <string.h>

typedef struct _ServiceRole ServiceRole;

struct _ServiceRole
{
	const gchar *service;
	GstServiceRole role;
};

static ServiceRole services[] = {
	/* keep this list alphabetically sorted! otherwise bsearch will be funny */
	{ "acct",                   GST_ROLE_SYSTEM_MONITORING },
	{ "acpi-support",           GST_ROLE_POWER_MANAGEMENT },
	{ "acpid",                  GST_ROLE_POWER_MANAGEMENT },
	{ "alsa",                   GST_ROLE_AUDIO_MANAGEMENT },
	{ "alsasound",              GST_ROLE_AUDIO_MANAGEMENT },
	{ "alsa-utils",             GST_ROLE_AUDIO_MANAGEMENT },
	{ "am-utils",               GST_ROLE_AUTOMOUNTER },
	{ "amavis",                 GST_ROLE_ANTIVIRUS },
	{ "amavis-ng",              GST_ROLE_ANTIVIRUS },
	{ "anacron",                GST_ROLE_COMMAND_SCHEDULER },
	{ "apache",                 GST_ROLE_WEB_SERVER },
	{ "apache-perl",            GST_ROLE_WEB_SERVER },
	{ "apache-ssl",             GST_ROLE_WEB_SERVER },
	{ "apache2",                GST_ROLE_WEB_SERVER },
	{ "apmd",                   GST_ROLE_POWER_MANAGEMENT },
	{ "apport",                 GST_ROLE_AUTOMATED_CRASH_REPORTS_MANAGEMENT },
	{ "apt-index-watcher",      GST_ROLE_PACKAGE_INDEX_MONITORING },
	{ "atd",                    GST_ROLE_COMMAND_SCHEDULER },
	{ "atftpd",                 GST_ROLE_FILE_SERVER_FTP },
	{ "atop",                   GST_ROLE_SYSTEM_MONITORING },
	{ "aumix",                  GST_ROLE_AUDIO_MANAGEMENT },
	{ "autofs",                 GST_ROLE_AUTOMOUNTER },
	{ "avahi-daemon",           GST_ROLE_RENDEZVOUS },
	{ "avahi-dnsconfd",         GST_ROLE_RENDEZVOUS },
	{ "backuppc",               GST_ROLE_REMOTE_BACKUP },
	{ "bind",                   GST_ROLE_DNS },
	{ "bind9",                  GST_ROLE_DNS },
	{ "bluetooth",              GST_ROLE_BLUETOOTH_MANAGEMENT },
	{ "bpalogin",               GST_ROLE_TELSTRA_BIGPOND_NETWORK_CLIENT },
	{ "brltty",                 GST_ROLE_BRAILLE_DISPLAY_MANAGEMENT },
	{ "capiutils",              GST_ROLE_ISDN_MANAGEMENT },
	{ "cherokee",               GST_ROLE_WEB_SERVER },
	{ "clamav-daemon",          GST_ROLE_ANTIVIRUS },
	{ "clamav-milter",          GST_ROLE_ANTIVIRUS },
	{ "clamd",                  GST_ROLE_ANTIVIRUS },
	{ "clvm",                   GST_ROLE_CLUSTER_MANAGEMENT },
	{ "cman",                   GST_ROLE_CLUSTER_MANAGEMENT },
	{ "courier",                GST_ROLE_MTA },
	{ "courier-authdaemon",     GST_ROLE_MTA_AUTH },
	{ "courier-mta",            GST_ROLE_MTA },
	{ "cron",                   GST_ROLE_COMMAND_SCHEDULER },
	{ "crond",                  GST_ROLE_COMMAND_SCHEDULER },
	{ "cups",                   GST_ROLE_PRINTER_SERVICE },
	{ "cupsd",                  GST_ROLE_PRINTER_SERVICE },
	{ "cupsys",                 GST_ROLE_PRINTER_SERVICE },
	{ "dbus",                   GST_ROLE_DBUS },
	{ "ddclient",               GST_ROLE_DYNAMIC_DNS_SERVICE },
	{ "dhcdbd",                 GST_ROLE_NETWORK },
	{ "dhcp3-server",           GST_ROLE_DHCP_SERVER },
	{ "dhis-client",            GST_ROLE_DYNAMIC_DNS_SERVICE },
	{ "dictd",                  GST_ROLE_DICTIONARY_SERVER },
	{ "dnsmasq",                GST_ROLE_DNS },
	{ "dovecot",                GST_ROLE_MTA },
	{ "eagle-usb",              GST_ROLE_EAGLE_USB_MODEMS_MANAGEMENT },
	{ "etc-setserial",          GST_ROLE_SERIAL_PORTS_MANAGEMENT },
	{ "evms",                   GST_ROLE_LVM_MANAGEMENT },
	{ "exim",                   GST_ROLE_MTA },
	{ "exim4",                  GST_ROLE_MTA },
	{ "fancontrol",             GST_ROLE_POWER_MANAGEMENT },
	{ "fcron",                  GST_ROLE_COMMAND_SCHEDULER },
	{ "festival",               GST_ROLE_SPEECH_SYNTHESIS },
	{ "fetchmail",              GST_ROLE_MAIL_FETCHER },
	{ "ftpd",                   GST_ROLE_FILE_SERVER_FTP },
	{ "gdm",                    GST_ROLE_DISPLAY_MANAGER },
	{ "gfs-tools",              GST_ROLE_CLUSTER_MANAGEMENT },
	{ "gfs2-tools",             GST_ROLE_CLUSTER_MANAGEMENT },
	{ "gnbd-client",            GST_ROLE_CLUSTER_MANAGEMENT },
	{ "gnbd-server",            GST_ROLE_CLUSTER_MANAGEMENT },
	{ "hdparm",                 GST_ROLE_HDD_MANAGEMENT },
	{ "heartbeat",              GST_ROLE_CLUSTER_MANAGEMENT },
	{ "hotkey-setup",           GST_ROLE_HOTKEYS_MANAGEMENT },
	{ "hplip",                  GST_ROLE_PRINTER_SERVICE },
	{ "httpd",                  GST_ROLE_WEB_SERVER },
	{ "inetd",                  GST_ROLE_NETWORK },
	{ "iptables",               GST_ROLE_FIREWALL_MANAGEMENT },
	{ "ipvsadm",                GST_ROLE_CLUSTER_MANAGEMENT },
	{ "irda",                   GST_ROLE_INFRARED_MANAGEMENT },
	{ "irda-setup",             GST_ROLE_INFRARED_MANAGEMENT },
	{ "irda-utils",             GST_ROLE_INFRARED_MANAGEMENT },
	{ "isdnutils",              GST_ROLE_ISDN_MANAGEMENT },
	{ "jackd",                  GST_ROLE_AUDIO_MANAGEMENT },
	{ "kde-guidance",           GST_ROLE_SYSTEM_CONFIGURATION_MANAGEMENT },
	{ "kdm",                    GST_ROLE_DISPLAY_MANAGER },
	{ "kdump",                  GST_ROLE_AUTOMATED_CRASH_REPORTS_MANAGEMENT },
	{ "keepalived",             GST_ROLE_CLUSTER_MANAGEMENT },
	{ "kerneloops",             GST_ROLE_AUTOMATED_CRASH_REPORTS_MANAGEMENT },
	{ "klogd",                  GST_ROLE_SYSTEM_LOGGER },
	{ "landscape-client",       GST_ROLE_SYSTEM_MONITORING },
	{ "laptop-mode",            GST_ROLE_POWER_MANAGEMENT },
	{ "lighttpd",               GST_ROLE_WEB_SERVER },
	{ "lm-sensors",             GST_ROLE_HARDWARE_MONITORING },
	{ "lpd",                    GST_ROLE_PRINTER_SERVICE },
	{ "lpdng",                  GST_ROLE_PRINTER_SERVICE },
	{ "ltsp-client",            GST_ROLE_LTSP_CLIENT },
	{ "lvm",                    GST_ROLE_LVM_MANAGEMENT },
	{ "mailman",                GST_ROLE_MAILING_LISTS_MANAGER },
	{ "mailscanner",            GST_ROLE_ANTIVIRUS },
	{ "mdadm",                  GST_ROLE_RAID_MANAGEMENT },
	{ "mdadm-raid",             GST_ROLE_RAID_MANAGEMENT },
	{ "messagebus",             GST_ROLE_DBUS },
	{ "metalog",                GST_ROLE_SYSTEM_LOGGER },
	{ "mgetty-fax",             GST_ROLE_FAX_MANAGEMENT },
	{ "mksd",                   GST_ROLE_ANTIVIRUS },
	{ "muddleftpd",             GST_ROLE_FILE_SERVER_FTP },
	{ "multipath-tools",        GST_ROLE_LVM_MANAGEMENT },
	{ "mysql",                  GST_ROLE_DATABASE_SERVER },
	{ "mysql-ndb",              GST_ROLE_DATABASE_SERVER },
	{ "mysql-ndb-mgm",          GST_ROLE_DATABASE_SERVER },
	{ "nagios",                 GST_ROLE_SECURITY_AUDITING },
	{ "named",                  GST_ROLE_DNS },
	{ "nbd-client",             GST_ROLE_DISK_CLIENT },
	{ "nbd-server",             GST_ROLE_DISK_SERVER },
	{ "nessusd",                GST_ROLE_SECURITY_AUDITING },
	{ "network/nfs/server",     GST_ROLE_FILE_SERVER_NFS },
	{ "nfs",                    GST_ROLE_FILE_SERVER_NFS },
	{ "nfs-common",             GST_ROLE_FILE_SERVER_NFS },
	{ "nfs-kernel-server",      GST_ROLE_FILE_SERVER_NFS },
	{ "nfs-server",             GST_ROLE_FILE_SERVER_NFS },
	{ "nfs-user-server",        GST_ROLE_FILE_SERVER_NFS },
	{ "nginx",                  GST_ROLE_WEB_SERVER },
	{ "nis",                    GST_ROLE_NSS },
	{ "ntp",                    GST_ROLE_NTP_SERVER },
	{ "ntp-server",             GST_ROLE_NTP_SERVER },
	{ "ntp-simple",             GST_ROLE_NTP_SERVER },
	{ "ntpd",                   GST_ROLE_NTP_SERVER },
	{ "o2cb",                   GST_ROLE_CLUSTER_MANAGEMENT },
	{ "ocfs2-tools",            GST_ROLE_CLUSTER_MANAGEMENT },
	{ "oem-config",             GST_ROLE_OEM_CONFIGURATION_MANAGEMENT },
	{ "oftpd",                  GST_ROLE_FILE_SERVER_FTP },
	{ "ondemand",               GST_ROLE_CPUFREQ_MANAGEMENT },
	{ "openais",                GST_ROLE_CLUSTER_MANAGEMENT },
	{ "openvpn",                GST_ROLE_VPN_SERVER },
	{ "pbbuttonsd",             GST_ROLE_HOTKEYS_MANAGEMENT },
	{ "portmap",                GST_ROLE_RPC_MAPPER },
	{ "postfix",                GST_ROLE_MTA },
	{ "postgresql",             GST_ROLE_DATABASE_SERVER },
	{ "postgresql-7.4",         GST_ROLE_DATABASE_SERVER },
	{ "postgresql-8.0",         GST_ROLE_DATABASE_SERVER },
	{ "postgresql-8.1",         GST_ROLE_DATABASE_SERVER },
	{ "postgresql-8.2",         GST_ROLE_DATABASE_SERVER },
	{ "postgresql-8.3",         GST_ROLE_DATABASE_SERVER },
	{ "postgresql-8.4",         GST_ROLE_DATABASE_SERVER },
	{ "postgresql-8.5",         GST_ROLE_DATABASE_SERVER },
	{ "postgresql-9.0",         GST_ROLE_DATABASE_SERVER },
	{ "powernowd",              GST_ROLE_CPUFREQ_MANAGEMENT },
	{ "powernowd.early",        GST_ROLE_CPUFREQ_MANAGEMENT },
	{ "pppd-dns",               GST_ROLE_NETWORK },
	{ "pptpd",                  GST_ROLE_VPN_SERVER },
	{ "proftpd",                GST_ROLE_FILE_SERVER_FTP },
	{ "pulseaudio",             GST_ROLE_AUDIO_MANAGEMENT },
	{ "pure-ftpd",              GST_ROLE_FILE_SERVER_FTP },
	{ "rc-inetd",               GST_ROLE_NETWORK },
	{ "qmail",                  GST_ROLE_MTA },
	{ "quagga",                 GST_ROLE_ROUTE_SERVER },
	{ "quota",                  GST_ROLE_QUOTA_MANAGEMENT },
	{ "racoon",                 GST_ROLE_IPSEC_KEY_EXCHANGE_SERVER },
	{ "radvd",                  GST_ROLE_ROUTER_ADVERTISEMENT_SERVER },
	{ "rdate",                  GST_ROLE_NTP_SERVER },
	{ "rgmanager",              GST_ROLE_CLUSTER_MANAGEMENT },
	{ "rsync",                  GST_ROLE_REMOTE_BACKUP },
	{ "rsyncd",                 GST_ROLE_REMOTE_BACKUP },
	{ "samba",                  GST_ROLE_FILE_SERVER_SMB },
	{ "schoolbell",             GST_ROLE_WEB_CALENDAR_SERVER },
	{ "schooltool",             GST_ROLE_SCHOOL_MANAGEMENT_PLATFORM },
	{ "screen",                 GST_ROLE_TERMINAL_MULTIPLEXOR },
	{ "sendmail",               GST_ROLE_MTA },
	{ "sensord",                GST_ROLE_HARDWARE_MONITORING },
	{ "shorewall",              GST_ROLE_FIREWALL_MANAGEMENT },
	{ "slapd",                  GST_ROLE_LDAP_SERVER },
	{ "slim",                   GST_ROLE_DISPLAY_MANAGER },
	{ "smartmontools",          GST_ROLE_HARDWARE_MONITORING },
	{ "smb",                    GST_ROLE_FILE_SERVER_SMB },
	{ "snmpd",                  GST_ROLE_SNMP_SERVER },
	{ "spamassassin",           GST_ROLE_SPAM_FILTER },
	{ "speech-dispatcher",      GST_ROLE_SPEECH_SYNTHESIS },
	{ "squid",                  GST_ROLE_PROXY_CACHE },
	{ "ssh",                    GST_ROLE_SECURE_SHELL_SERVER },
	{ "sshd",                   GST_ROLE_SECURE_SHELL_SERVER },
	{ "sysklogd",               GST_ROLE_SYSTEM_LOGGER },
	{ "syslog",                 GST_ROLE_SYSTEM_LOGGER },
	{ "syslog-ng",              GST_ROLE_SYSTEM_LOGGER },
	{ "tftpd-hpa",              GST_ROLE_FILE_SERVER_TFTP },
	{ "ufw",                    GST_ROLE_FIREWALL_MANAGEMENT },
	{ "vcron",                  GST_ROLE_COMMAND_SCHEDULER },
	{ "virtualbox",             GST_ROLE_VIRTUAL_MACHINE_MANAGEMENT },
	{ "virtualbox-ose",         GST_ROLE_VIRTUAL_MACHINE_MANAGEMENT },
	{ "vixie-cron",             GST_ROLE_COMMAND_SCHEDULER },
	{ "vmware-player",          GST_ROLE_VIRTUAL_MACHINE_MANAGEMENT },
	{ "vsftpd",                 GST_ROLE_FILE_SERVER_FTP },
	{ "wacom-tools",            GST_ROLE_GRAPHIC_TABLETS_MANAGEMENT },
	{ "wdm",                    GST_ROLE_DISPLAY_MANAGER },
	{ "winbind",                GST_ROLE_NSS },
	{ "wu-ftpd",                GST_ROLE_FILE_SERVER_FTP },
	{ "wzdftpd",                GST_ROLE_FILE_SERVER_FTP },
	{ "xdm",                    GST_ROLE_DISPLAY_MANAGER },
	{ "xinetd",                 GST_ROLE_NETWORK },
	{ "zmailer",                GST_ROLE_MTA },
	{ "zope3",                  GST_ROLE_APPLICATION_SERVER },
	{ "network-manager",        GST_ROLE_NETWORK },
	{ "NetworkManager",         GST_ROLE_NETWORK },
};

static int
compare_services (const void *p1, const void *p2)
{
	ServiceRole *role1 = (ServiceRole *) p1;
	ServiceRole *role2 = (ServiceRole *) p2;

	return strcmp (role1->service, role2->service);
}

GstServiceRole
gst_service_get_role (OobsService *service)
{
	ServiceRole role, *ret;
	const gchar *name;

	name = oobs_service_get_name (service);
	role.service = (gchar *) name;

	ret = (ServiceRole *) bsearch (&role, services, G_N_ELEMENTS (services),
				       sizeof (ServiceRole), compare_services);

	return (ret) ? ret->role : GST_ROLE_NONE;
}
