/*
  PPTP CLIENT start/stop and configuration for Tomato
  by Jean-Yves Avenard (c) 2008-2011
*/

#include "rc.h"

#define BUF_SIZE 128
#define IF_SIZE 8


void start_pptp_client(void)
{
    FILE *fd;
    int ok = 0;
    int i;
    char *p;
    char buffer[BUF_SIZE];
    char *argv[5];
    int argc = 0;

    sprintf(buffer, "pptpclient");
    if ( pidof(buffer) >= 0 )
    {
        // PPTP already running
        return;
    }
    unlink("/etc/vpn/ip-down");
    unlink("/etc/vpn/ip-up");
    unlink("/etc/vpn/ip-vpn");
    unlink("/etc/vpn/options.vpn");
    unlink("/etc/vpn");
    mkdir("/etc/vpn",0700);
    ok |= symlink("/rom/etc/vpn/ip-down", "/etc/vpn/ip-down");
    ok |= symlink("/rom/etc/vpn/ip-up", "/etc/vpn/ip-up");
    // Make sure symbolic link exists
    sprintf(buffer, "/etc/vpn/pptpclient");
    unlink(buffer);
    ok |= symlink("/usr/sbin/pppd", buffer);

    if (ok)
    {
        stop_pptp_client();
        return;
    }

    if ( (fd = fopen("/etc/vpn/options.vpn", "w")) != NULL )
    {
        ok = 1;
        fprintf(fd,
            "lock\n"
            "noauth\n"
            "refuse-eap\n"
            "lcp-echo-failure 3\n"
            "lcp-echo-interval 2\n"
            "maxfail 0\n"
            "persist\n"
            "plugin pptp.so\n"
            "pptp_server %s\n", nvram_safe_get("pptp_client_srvip"));
        // Accept default route
        if (nvram_get_int("pptp_client_dfltroute"))
            fprintf(fd,"defaultroute\n");
        i = nvram_get_int("pptp_client_peerdns"); //-1 or 0: enable, 1 disable
        if (i != 1)
            fprintf(fd,"usepeerdns\n");
        fprintf(fd,"idle 0\n"
            "ip-up-script /etc/vpn/ip-up\n"
            "ip-down-script /etc/vpn/ip-down\n"
            "ipparam kelokepptpd\n");

        if ((p = nvram_get("pptp_client_mtu")) == NULL)
            p = "1450";
        if (!nvram_get_int("pptp_client_mtuenable"))
            p = "1450";
        fprintf(fd,"mtu %s\n", p);

        if (!nvram_get_int("pptp_client_mruenable"))
        {
            if ((p = nvram_get("pptp_client_mru")) == NULL)
                p = "1450";
            fprintf(fd,"mru %s\n", p);
        }

        if ((p = nvram_get("pptp_client_username")) == NULL)
            ok = 0;
        else
            fprintf(fd,"name %s\n", p);

        if ((p = nvram_get("pptp_client_passwd")) == NULL)
            ok = 0;
        else
            fprintf(fd,"password %s\n", p);
        switch (get_wan_proto())
        {
            case WP_PPPOE:
            case WP_PPTP:
            case WP_L2TP:
                p = "1";
                break;
            default:
                p = "0";
                break;
        }
        strcpy(buffer,"");
        switch (nvram_get_int("pptp_client_crypt"))
        {
            case 1:
                fprintf(fd, "nomppe\n");
                break;
            case 2:
                fprintf(fd, "nomppe-40\n");
                fprintf(fd, "require-mppe-128\n");
                break;
            case 3:
                fprintf(fd, "require-mppe\n");
                break;
            default:
                break;
        }
        if (!nvram_get_int("pptp_client_stateless"))
	    fprintf(fd, "mppe-stateful\n");
	else
            fprintf(fd, "nomppe-stateful\n");
        fprintf(fd, "unit %s\n", p);
        fprintf(fd, "%s\n", nvram_safe_get("pptp_client_custom"));
        fclose(fd);
    }
    if (ok)
    {
        sprintf(buffer, "/etc/vpn/pptpclient file /etc/vpn/options.vpn");
        for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
        if ( _eval(argv, NULL, 0, NULL) )
        {
            stop_pptp_client();
            return;
        }
    }
    else
        stop_pptp_client();
}


void stop_pptp_client(void)
{
    int argc;
    char *argv[7];
    char buffer[BUF_SIZE];

    killall("pptpclient", SIGTERM);

    sprintf(buffer, "rm -rf /etc/vpn/pptpclient /etc/vpn/ip-down /etc/vpn/ip-up /etc/vpn/options.vpn");
    for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
    _eval(argv, NULL, 0, NULL);

    rmdir("/etc/vpn");
}
