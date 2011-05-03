/* Shared library add-on to iptables to add recent matching support. */
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <ip6tables.h>
#include <linux/netfilter_ipv6/ip6_tables.h>
#include <linux/netfilter/xt_recent.h>

#ifndef XTABLES_VERSION
#define XTABLES_VERSION IPTABLES_VERSION
#endif

#ifdef IPT_LIB_DIR
#define xtables_target ip6tables_target
#define xtables_register_target register_target6
#endif

/* Need these in order to not fail when compiling against an older kernel. */
#ifndef RECENT_NAME
#define RECENT_NAME	"ip6t_recent"
#endif /* RECENT_NAME */

#ifndef RECENT_VER
#define RECENT_VER	"unknown"
#endif /* RECENT_VER */

/* Options for this module */
static struct option opts[] = {
	{ .name = "set",      .has_arg = 0, .flag = 0, .val = 201 }, 
	{ .name = "rcheck",   .has_arg = 0, .flag = 0, .val = 202 }, 
	{ .name = "update",   .has_arg = 0, .flag = 0, .val = 203 },
	{ .name = "seconds",  .has_arg = 1, .flag = 0, .val = 204 }, 
	{ .name = "hitcount", .has_arg = 1, .flag = 0, .val = 205 },
	{ .name = "remove",   .has_arg = 0, .flag = 0, .val = 206 },
	{ .name = "rttl",     .has_arg = 0, .flag = 0, .val = 207 },
	{ .name = "name",     .has_arg = 1, .flag = 0, .val = 208 },
	{ .name = "rsource",  .has_arg = 0, .flag = 0, .val = 209 },
	{ .name = "rdest",    .has_arg = 0, .flag = 0, .val = 210 },
	{ .name = 0,          .has_arg = 0, .flag = 0, .val = 0   }
};

/* Function which prints out usage message. */
static void
help(void)
{
	printf(
"recent v%s options:\n"
"[!] --set                       Add source address to list, always matches.\n"
"[!] --rcheck                    Match if source address in list.\n"
"[!] --update                    Match if source address in list, also update last-seen time.\n"
"[!] --remove                    Match if source address in list, also removes that address from list.\n"
"    --seconds seconds           For check and update commands above.\n"
"                                Specifies that the match will only occur if source address last seen within\n"
"                                the last 'seconds' seconds.\n"
"    --hitcount hits             For check and update commands above.\n"
"                                Specifies that the match will only occur if source address seen hits times.\n"
"                                May be used in conjunction with the seconds option.\n"
"    --rttl                      For check and update commands above.\n"
"                                Specifies that the match will only occur if the source address and the TTL\n"
"                                match between this packet and the one which was set.\n"
"                                Useful if you have problems with people spoofing their source address in order\n"
"                                to DoS you via this module.\n"
"    --name name                 Name of the recent list to be used.  DEFAULT used if none given.\n"
"    --rsource                   Match/Save the source address of each packet in the recent list table (default).\n"
"    --rdest                     Match/Save the destination address of each packet in the recent list table.\n"
RECENT_NAME " " RECENT_VER ": Stephen Frost <sfrost@snowman.net>.  http://snowman.net/projects/ipt_recent/\n"
,
IPTABLES_VERSION);

}
  
/* Initialize the match. */
static void
init(struct ip6t_entry_match *match, unsigned int *nfcache)
{
	struct xt_recent_mtinfo *info = (struct xt_recent_mtinfo *)(match)->data;


	strncpy(info->name,"DEFAULT",XT_RECENT_NAME_LEN);
	/* eventhough XT_RECENT_NAME_LEN is currently defined as 200,
	 * better be safe, than sorry */
	info->name[XT_RECENT_NAME_LEN-1] = '\0';
	info->side = XT_RECENT_SOURCE;
}

/* Function which parses command options; returns true if it
   ate an option */
static int
parse(int c, char **argv, int invert, unsigned int *flags,
      const struct ip6t_entry *entry,
      unsigned int *nfcache,
      struct ip6t_entry_match **match)
{
	struct xt_recent_mtinfo *info = (struct xt_recent_mtinfo *)(*match)->data;
	switch (c) {
		case 201:
			if (*flags) exit_error(PARAMETER_PROBLEM,
					"recent: only one of `--set', `--rcheck' "
					"`--update' or `--remove' may be set");
			check_inverse(optarg, &invert, &optind, 0);
			info->check_set |= XT_RECENT_SET;
			if (invert) info->invert = 1;
			*flags = 1;
			break;
			
		case 202:
			if (*flags) exit_error(PARAMETER_PROBLEM,
					"recent: only one of `--set', `--rcheck' "
					"`--update' or `--remove' may be set");
			check_inverse(optarg, &invert, &optind, 0);
			info->check_set |= XT_RECENT_CHECK;
			if(invert) info->invert = 1;
			*flags = 1;
			break;

		case 203:
			if (*flags) exit_error(PARAMETER_PROBLEM,
					"recent: only one of `--set', `--rcheck' "
					"`--update' or `--remove' may be set");
			check_inverse(optarg, &invert, &optind, 0);
			info->check_set |= XT_RECENT_UPDATE;
			if (invert) info->invert = 1;
			*flags = 1;
			break;

		case 206:
			if (*flags) exit_error(PARAMETER_PROBLEM,
					"recent: only one of `--set', `--rcheck' "
					"`--update' or `--remove' may be set");
			check_inverse(optarg, &invert, &optind, 0);
			info->check_set |= XT_RECENT_REMOVE;
			if (invert) info->invert = 1;
			*flags = 1;
			break;

		case 204:
			info->seconds = atoi(optarg);
			break;

		case 205:
			info->hit_count = atoi(optarg);
			break;

		case 207:
			info->check_set |= XT_RECENT_TTL;
			break;

		case 208:
			strncpy(info->name,optarg,XT_RECENT_NAME_LEN);
			info->name[XT_RECENT_NAME_LEN-1] = '\0';
			break;

		case 209:
			info->side = XT_RECENT_SOURCE;
			break;

		case 210:
			info->side = XT_RECENT_DEST;
			break;

		default:
			return 0;
	}

	return 1;
}

/* Final check; must have specified a specific option. */
static void
final_check(unsigned int flags)
{
	if (!flags)
		exit_error(PARAMETER_PROBLEM,
			"recent: you must specify one of `--set', `--rcheck' "
			"`--update' or `--remove'");
}

/* Prints out the matchinfo. */
static void
print(const struct ip6t_ip6 *ip,
      const struct ip6t_entry_match *match,
      int numeric)
{
	struct xt_recent_mtinfo *info = (struct xt_recent_mtinfo *)match->data;

	if (info->invert)
		fputc('!', stdout);

	printf("recent: ");
	if(info->check_set & XT_RECENT_SET) printf("SET ");
	if(info->check_set & XT_RECENT_CHECK) printf("CHECK ");
	if(info->check_set & XT_RECENT_UPDATE) printf("UPDATE ");
	if(info->check_set & XT_RECENT_REMOVE) printf("REMOVE ");
	if(info->seconds) printf("seconds: %d ",info->seconds);
	if(info->hit_count) printf("hit_count: %d ",info->hit_count);
	if(info->check_set & XT_RECENT_TTL) printf("TTL-Match ");
	if(info->name) printf("name: %s ",info->name);
	if(info->side == XT_RECENT_SOURCE) printf("side: source ");
	if(info->side == XT_RECENT_DEST) printf("side: dest");
}

/* Saves the union ip6t_matchinfo in parsable form to stdout. */
static void
save(const struct ip6t_ip6 *ip, const struct ip6t_entry_match *match)
{
	struct xt_recent_mtinfo *info = (struct xt_recent_mtinfo *)match->data;

	if (info->invert)
		printf("! ");

	if(info->check_set & XT_RECENT_SET) printf("--set ");
	if(info->check_set & XT_RECENT_CHECK) printf("--rcheck ");
	if(info->check_set & XT_RECENT_UPDATE) printf("--update ");
	if(info->check_set & XT_RECENT_REMOVE) printf("--remove ");
	if(info->seconds) printf("--seconds %d ",info->seconds);
	if(info->hit_count) printf("--hitcount %d ",info->hit_count);
	if(info->check_set & XT_RECENT_TTL) printf("--rttl ");
	if(info->name) printf("--name %s ",info->name);
	if(info->side == XT_RECENT_SOURCE) printf("--rsource ");
	if(info->side == XT_RECENT_DEST) printf("--rdest ");
}

/* Structure for iptables to use to communicate with module */
static struct ip6tables_match recent = { 
    .next          = NULL,
    .name          = "recent",
    .version       = XTABLES_VERSION,
    .size          = IP6T_ALIGN(sizeof(struct xt_recent_mtinfo)),
    .userspacesize = IP6T_ALIGN(sizeof(struct xt_recent_mtinfo)),
    .help          = &help,
    .init          = &init,
    .parse         = &parse,
    .final_check   = &final_check,
    .print         = &print,
    .save          = &save,
    .extra_opts    = opts,
};

void _init(void)
{
	register_match6(&recent);
}