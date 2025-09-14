
#ifndef GOTO_H
#define GOTO_H

#define ROOTD "/"
#define DEVD "/dev/"
#define SYSD "/sys/"
#define ETCD "/etc/"
#define MEDIAD "/run/media/"
#define OPTD "/opt/"
#define RUND "/run/"
#define SRVD "/srv/"
#define TMPD "/tmp/"
#define VARD "/var/"
#define USRD "/usr/"
#define MNTD "/mnt/"

void
goto_cmd(int next_char);
void
goto_help();

#endif
