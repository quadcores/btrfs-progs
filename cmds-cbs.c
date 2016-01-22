/*
 * Copyright (C) 2015 Fujitsu.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <getopt.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>

#include "ctree.h"
#include "ioctl.h"

#include "commands.h"
#include "utils.h"
#include "kerncompat.h"
#include "cbs.h"

static const char * const cbs_cmd_group_usage[] = {
	"btrfs cbs <command> [options] <path>",
	NULL
};

static const char cbs_cmd_group_info[] =
"manage inband(write time) de-duplication";

static const char * const cmd_cbs_enable_usage[] = {
	"btrfs cbs enable [options] <path>",
	"Enable in-band(write time) de-duplication of a btrfs.",
	"",
	"-a|--hash-algorithm <HASH>",
	"           specify hash algorithm",
	"           only 'sha256' is supported yet",
	NULL
};

static int cmd_cbs_enable(int argc, char **argv)
{
	int ret;
	int fd;
	char *path;
	u16 hash_type = BTRFS_CBS_HASH_SHA256;
	struct btrfs_ioctl_cbs_args dargs;
	DIR *dirstream;

	while (1) {
		int c;
		static const struct option long_options[] = {
			{ "hash-algorithm", required_argument, NULL, 'a'},
			{ NULL, 0, NULL, 0}
		};

		c = getopt_long(argc, argv, "a:", long_options, NULL);
		if (c < 0)
			break;
		if (strcmp("sha256", optarg)) {
			error("unsupported cbs hash algorithm: %s",
			      optarg);
			return 1;
		}
	}

	path = argv[optind];
	if (check_argc_exact(argc - optind, 1))
		usage(cmd_cbs_enable_usage);

	/* Validation check */

	fd = open_file_or_dir(path, &dirstream);
	if (fd < 0) {
		error("failed to open file or directory: %s", path);
		return 1;
	}
	memset(&dargs, 0, sizeof(dargs));
	dargs.cmd = BTRFS_CBS_CTL_ENABLE;
	dargs.hash_type = hash_type;

	ret = ioctl(fd, BTRFS_IOC_CBS_CTL, &dargs);
	if (ret < 0) {
		char *error_message = NULL;
		/* Special case, provide better error message */
		error("failed to enable cbs: %s",
		      error_message ? error_message : strerror(errno));
		ret = 1;
		goto out;
	}
	ret = 0;

out:
	close_file_or_dir(fd, dirstream);
	return ret;
}

static const char * const cmd_cbs_disable_usage[] = {
	"btrfs cbs disable <path>",
	"Disable in-band(write time) de-duplication of a btrfs.",
	NULL
};

static int cmd_cbs_disable(int argc, char **argv)
{
	struct btrfs_ioctl_cbs_args dargs;
	DIR *dirstream;
	char *path;
	int fd;
	int ret;

	printk(KERN_ERR " ##### In %s ##### \n", __func__);

	if (check_argc_exact(argc, 2))
		usage(cmd_cbs_disable_usage);

	path = argv[1];
	fd = open_file_or_dir(path, &dirstream);
	if (fd < 0) {
		error("failed to open file or directory: %s", path);
		return 1;
	}
	memset(&dargs, 0, sizeof(dargs));
	dargs.cmd = BTRFS_CBS_CTL_DISABLE;

	ret = ioctl(fd, BTRFS_IOC_CBS_CTL, &dargs);
	if (ret < 0) {
		error("failed to disable cbs: %s",
		      strerror(errno));
		ret = 1;
		goto out;
	}
	ret = 0;

out:
	close_file_or_dir(fd, dirstream);
	return 0;
}

static const char * const cmd_cbs_status_usage[] = {
	"btrfs cbs status <path>",
	"Show status of content-based storage in btrfs.",
	NULL
};

static int cmd_cbs_status(int argc, char **argv)
{
	struct btrfs_ioctl_cbs_args dargs;
	DIR *dirstream;
	char *path;
	int fd;
	int ret;

	//printk(KERN_ERR " ##### In %s : %lx ##### \n", __func__, BTRFS_IOC_CBS_CTL);

	if (check_argc_exact(argc, 2))
		usage(cmd_cbs_status_usage);

	path = argv[1];
	fd = open_file_or_dir(path, &dirstream);
	if (fd < 0) {
		error("failed to open file or directory: %s", path);
		ret = 1;
		goto out;
	}
	memset(&dargs, 0, sizeof(dargs));
	dargs.cmd = BTRFS_CBS_CTL_STATUS;

	ret = ioctl(fd, BTRFS_IOC_CBS_CTL, &dargs);
	if (ret < 0) {
		error("failed to get cbs status: %s",
		      strerror(errno));
		ret = 1;
		goto out;
	}
	ret = 0;
	if (dargs.status == 0) {
		printf("\nStatus: \t\t\tDisabled\n");
		goto out;
	}
	printf("\nStatus:\t\t\tEnabled\n");

	if (dargs.hash_type == BTRFS_CBS_HASH_SHA256)
		printf("Hash algorithm:\t\tSHA-256\n");
	else
		printf("Hash algorithm:\t\tUnrecognized(%x)\n",
			dargs.hash_type);

	printf("\nContent-based storage by QuadCores\n\n");
	
out:
	close_file_or_dir(fd, dirstream);
	return ret;
}

const struct cmd_group cbs_cmd_group = {
	cbs_cmd_group_usage, cbs_cmd_group_info, {
		{ "enable", cmd_cbs_enable, cmd_cbs_enable_usage, NULL, 0},
		{ "disable", cmd_cbs_disable, cmd_cbs_disable_usage,
		  NULL, 0},
		{ "status", cmd_cbs_status, cmd_cbs_status_usage,
		  NULL, 0},
		NULL_CMD_STRUCT
	}
};

int cmd_cbs(int argc, char **argv)
{
	return handle_command_group(&cbs_cmd_group, argc, argv);
}