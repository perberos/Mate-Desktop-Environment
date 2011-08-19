#groupadd gdm
#useradd -c "GDM Daemon Owner" -d /dev/null -g gdm -s /bin/bash gdm

./configure --prefix=/usr \
--libexecdir=/usr/sbin \
--sysconfdir=/etc/gnome --localstatedir=/var/lib \
--with-pam-prefix=/etc &&

make &&

make install
