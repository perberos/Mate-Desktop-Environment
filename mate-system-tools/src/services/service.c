/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.h: this file is part of services-admin, a mate-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2002 Ximian, Inc.
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
 * Authors: Carlos Garnacho <carlosg@mate.org>.
 */


#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gst.h"
#include "service.h"

/* WARNING: keep this array *always* in sync with src/common/gst-service-role.h enum order */
const ServiceDescription descriptions[] = {
	{ FALSE, "applications-internet", N_("Web server"),                      N_("Shares your web pages over the Internet") },  /* GST_ROLE_WEB_SERVER */
	{ FALSE, "stock_alarm",        N_("Actions scheduler"),               N_("Executes scheduled actions") },	        /* GST_ROLE_COMMAND_SCHEDULER */
	{ FALSE, "printer",            N_("Printer service"),                 N_("Allows applications to use printers") },      /* GST_ROLE_PRINTER_SERVICE */
	{ FALSE, "mail-send",          N_("Mail agent"),                      N_("Delivers your outgoing mail") },	        /* GST_ROLE_MTA */
	{ FALSE, "stock_lock",         N_("MTA authentication service"),      NULL }, /* GST_ROLE_MTA_AUTH */
	{ FALSE, "mail-receive",       N_("Mail fetcher"),                    N_("Downloads your mail from remote accounts") }, /* GST_ROLE_MAIL_FETCHER */
	{ TRUE,  "gdm",                N_("Graphical login manager"),         N_("Allows users to login graphically") },        /* GST_ROLE_DISPLAY_MANAGER */
	{ FALSE, "file-manager",       N_("Database server"),                 N_("Data storage system") },                      /* GST_ROLE_DATABASE_SERVER */
	{ FALSE, "mate-fs-smb",       N_("Folder sharing service"),          N_("Shares folders over your network") },         /* GST_ROLE_FILE_SERVER_SMB */
	{ FALSE, "mate-fs-nfs",       N_("Folder sharing service"),          N_("Shares folders over your network") },         /* GST_ROLE_FILE_SERVER_NFS */
	{ FALSE, "mate-fs-ftp",       N_("FTP service"),                     N_("Shares folders over the Internet") },	        /* GST_ROLE_FILE_SERVER_FTP */
	{ FALSE, "mate-fs-share",     N_("Folder sharing service"),          N_("Shares folders over the Internet") },	        /* GST_ROLE_FILE_SERVER_TFTP */
	{ FALSE, "clock",              N_("Clock synchronization service"),   N_("Synchronizes your computer "
										 "clock with Internet time servers") },         /* GST_ROLE_NTP_SERVER */
	{ FALSE, "stock_lock",         N_("Antivirus"),                       N_("Analyzes your incoming mail for virus") },    /* GST_ROLE_ANTIVIRUS */
	{ FALSE, "stock_lock",         N_("Firewall"),                        N_("Blocks undesired network "
										 "access to your computer") },                  /* GST_ROLE_FIREWALL_MANAGEMENT */
	{ FALSE, "accessories-dictionary", N_("Dictionary server"),           NULL }, /* GST_ROLE_DICTIONARY_SERVER */
	{ FALSE, "access",             N_("Speech synthesis support"),        NULL }, /* GST_ROLE_SPEECH_SYNTHESIS */
	{ FALSE, "logviewer",          N_("Computer activity logger"),        N_("Keeps a log of your computer activity") },    /* GST_ROLE_SYSTEM_LOGGER */
	{ FALSE, NULL,                 N_("Remote backup server"),            NULL }, /* GST_ROLE_REMOTE_BACKUP */
	{ FALSE, "mail-mark-junk",     N_("Spam filter"),                     NULL }, /* GST_ROLE_SPAM_FILTER */
	{ FALSE, "utilities-terminal", N_("Remote shell server"),             N_("Secure shell server") },                      /* GST_ROLE_SECURE_SHELL_SERVER */
	{ FALSE, "stock_script",       N_("Application server"),              NULL }, /* GST_ROLE_APPLICATION_SERVER */
	{ FALSE, NULL,                 N_("Automated crash reports support"), NULL }, /* GST_ROLE_AUTOMATED_CRASH_REPORTS_MANAGEMENT */
	{ TRUE,  NULL,                 N_("System communication bus"),        NULL }, /* GST_ROLE_DBUS, */
	{ FALSE, "gtk-preferences",    N_("System configuration manager"),    NULL }, /* GST_ROLE_SYSTEM_CONFIGURATION_MANAGEMENT */
	{ FALSE, "applications-education", N_("School management platform"),      NULL }, /* GST_ROLE_SCHOOL_MANAGEMENT_PLATFORM */
	{ FALSE, "stock_lock",         N_("Network security auditor"),        NULL }, /* GST_ROLE_SECURITY_AUDITING */
	{ FALSE, "stock_web-calendar", N_("Web calendar server"),             NULL }, /* GST_ROLE_WEB_CALENDAR_SERVER */
	{ FALSE, NULL,                 N_("OEM configuration manager"),       NULL }, /* GST_ROLE_OEM_CONFIGURATION_MANAGEMENT */
	{ FALSE, NULL,                 N_("Terminal multiplexor"),            NULL }, /* GST_ROLE_TERMINAL_MULTIPLEXOR */
	{ TRUE,  NULL,                 N_("Disk quota activation"),           NULL }, /* GST_ROLE_QUOTA_MANAGEMENT */
	{ FALSE, "package-x-generic",  N_("Package index monitor"),           NULL }, /* GST_ROLE_PACKAGE_INDEX_MONITORING */
	{ TRUE,  "preferences-system-network", N_("Network service"),         NULL }, /* GST_ROLE_NETWORK */
	{ FALSE, NULL,                 N_("Dynamic DNS services updater"),    NULL }, /* GST_ROLE_DYNAMIC_DNS_SERVICE */
	{ FALSE, NULL,                 N_("DHCP server"),                     NULL }, /* GST_ROLE_DHCP_SERVER */
	{ FALSE, NULL,                 N_("Domain name server"),              NULL }, /* GST_ROLE_DNS */
	{ FALSE, NULL,                 N_("Proxy cache service"),             NULL }, /* GST_ROLE_PROXY_CACHE */
	{ TRUE,  NULL,                 N_("LDAP server"),                     NULL }, /* GST_ROLE_LDAP_SERVER */
	{ FALSE, "mail-unread",        N_("Mailing lists manager"),           NULL }, /* GST_ROLE_MAILING_LISTS_MANAGER */
	{ TRUE,  NULL,                 N_("Multicast DNS service discovery"), NULL }, /* GST_ROLE_RENDEZVOUS */
	{ TRUE,  NULL,                 N_("Account information resolver"),    NULL }, /* GST_ROLE_NSS */
	{ TRUE,  NULL,                 N_("Virtual Private Network server"),  NULL }, /* GST_ROLE_VPN_SERVER */
	{ FALSE, NULL,                 N_("Router advertisement server"),     NULL }, /* GST_ROLE_ROUTER_ADVERTISEMENT_SERVER */
	{ FALSE, NULL,                 N_("IPSec key exchange server"),       NULL }, /* GST_ROLE_IPSEC_KEY_EXCHANGE_SERVER */
	{ FALSE, "disks",              N_("Disk server"),                     NULL }, /* GST_ROLE_DISK_SERVER */
	{ FALSE, "disks",              N_("Disk client"),                     NULL }, /* GST_ROLE_DISK_CLIENT */
	{ FALSE, NULL,                 N_("Route server"),                    NULL }, /* GST_ROLE_ROUTE_SERVER */
	{ TRUE,  NULL,                 N_("RPC mapper"),                      NULL }, /* GST_ROLE_RPC_MAPPER */
	{ TRUE,  NULL,                 N_("SNMP server"),                     NULL }, /* GST_ROLE_SNMP_SERVER */
	{ TRUE,  NULL,                 N_("Terminal server client"),          NULL }, /* GST_ROLE_LTSP_CLIENT */
	{ FALSE, "mate-mixer",        N_("Audio settings management"),       NULL }, /* GST_ROLE_AUDIO_MANAGEMENT */
	{ FALSE, "disks",              N_("Volumes mounter"),                 N_("Mounts your volumes automatically") },        /* GST_ROLE_AUTOMOUNTER */
	{ FALSE, "preferences-desktop-peripherals", N_("Infrared port management"),        NULL }, /* GST_ROLE_INFRARED_MANAGEMENT */
	{ TRUE,  "access",             N_("Braille display management"),      NULL }, /* GST_ROLE_BRAILLE_DISPLAY_MANAGEMENT */
	{ FALSE, "bluetooth",          N_("Bluetooth device management"),     NULL }, /* GST_ROLE_BLUETOOTH_MANAGEMENT */
	{ FALSE, "drive-harddisk",     N_("Hard disk tuning"),                NULL }, /* GST_ROLE_HDD_MANAGEMENT */
	{ FALSE, NULL,                 N_("Hotkeys management"),              NULL }, /* GST_ROLE_HOTKEYS_MANAGEMENT */
	{ FALSE, "battery",            N_("Power management"),                NULL }, /* GST_ROLE_POWER_MANAGEMENT */
	{ TRUE,  NULL,                 N_("Logical volume management"),       NULL }, /* GST_ROLE_LVM_MANAGEMENT */
	{ TRUE,  NULL,                 N_("Cluster management tool"),         NULL }, /* GST_ROLE_CLUSTER_MANAGEMENT */
	{ FALSE, NULL,                 N_("Fax settings management"),         NULL }, /* GST_ROLE_FAX_MANAGEMENT */
	{ TRUE,  "drive-harddisk",     N_("RAID disks management"),           NULL }, /* GST_ROLE_RAID_MANAGEMENT */
	{ FALSE, "input-tablet",       N_("Graphic tablets management"),      NULL }, /* GST_ROLE_GRAPHIC_TABLETS_MANAGEMENT */
	{ FALSE, NULL,                 N_("CPU Frequency manager"),           NULL }, /* GST_ROLE_CPUFREQ_MANAGEMENT */
	{ FALSE, NULL,                 N_("Eagle USB ADSL modems manager"),   NULL }, /* GST_ROLE_EAGLE_USB_MODEMS_MANAGEMENT */
	{ FALSE, NULL,                 N_("Serial port settings management"), NULL }, /* GST_ROLE_SERIAL_PORTS_MANAGEMENT */
	{ FALSE, "modem",              N_("ISDN modems manager"),             NULL }, /* GST_ROLE_ISDN_MANAGEMENT */
	{ FALSE, NULL,                 N_("Telstra Bigpond Cable Network client"), NULL }, /* GST_ROLE_TELSTRA_BIGPOND_NETWORK_CLIENT */
	{ FALSE, NULL,                 N_("Hardware monitor"),                NULL }, /* GST_ROLE_HARDWARE_MONITORING */
	{ FALSE, "utilities-system-monitor", N_("System monitor"),            NULL }, /* GST_ROLE_SYSTEM_MONITORING */
	{ FALSE, "computer",           N_("Virtual Machine management"),      NULL }, /* GST_ROLE_VIRTUAL_MACHINE_MANAGEMENT */
	{ FALSE }
};

const ServiceDescription*
service_search (OobsService *service)
{
	GstServiceRole role;

	role = gst_service_get_role (service);
	return (role >= GST_ROLE_NONE) ? NULL : &descriptions[role];
}
