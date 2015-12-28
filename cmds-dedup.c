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
#include "dedup.h"

static const char * const dedup_cmd_group_usage[] = {
	"btrfs dedup <command> [options] <path>",
	NULL
};

static const char dedup_cmd_group_info[] =
"manage inband(write time) de-duplication";

static const char * const cmd_dedup_enable_usage[] = {
	"btrfs dedup enable [options] <path>",
	"Enable in-band(write time) de-duplication of a btrfs.",
	"",
	"-s|--storage-backend <BACKEND>",
	"           specify dedup hash storage backend",
	"           supported backend: 'ondisk', 'inmemory'",
	"           inmemory is the default backend",
	"-b|--blocksize <BLOCKSIZE>",
	"           specify dedup block size",
	"           default value is 128K",
	"-a|--hash-algorithm <HASH>",
	"           specify hash algorithm",
	"           only 'sha256' is supported yet",
	"-l|--limit-hash <LIMIT>",
	"           specify maximum number of hashes stored in memory",
	"           only for 'inmemory' backend",
	"           default value is 32K if using 'inmemory' backend",
	"-m|--limit-mem <LIMIT>",
	"           specify maximum memory used for hashes",
	"           only for 'inmemory' backend",
	"           only one of '-m' and '-l' is allowed",
	NULL
};

static int cmd_dedup_enable(int argc, char **argv)
{
	int ret;
	int fd;
	char *path;
	u64 blocksize = BTRFS_DEDUP_BLOCKSIZE_DEFAULT;
	u16 hash_type = BTRFS_DEDUP_HASH_SHA256;
	u16 backend = BTRFS_DEDUP_BACKEND_INMEMORY;
	u64 limit_nr = 0;
	u64 limit_mem = 0;
	struct btrfs_ioctl_dedup_args dargs;
	DIR *dirstream;

	while (1) {
		int c;
		static const struct option long_options[] = {
			{ "storage-backend", required_argument, NULL, 's'},
			{ "blocksize", required_argument, NULL, 'b'},
			{ "hash-algorithm", required_argument, NULL, 'a'},
			{ "limit-hash", required_argument, NULL, 'l'},
			{ "limit-memory", required_argument, NULL, 'm'},
			{ NULL, 0, NULL, 0}
		};

		c = getopt_long(argc, argv, "s:b:a:l:m:", long_options, NULL);
		if (c < 0)
			break;
		switch (c) {
		case 's':
			if (!strcmp("ondisk", optarg))
				backend = BTRFS_DEDUP_BACKEND_ONDISK;
			else if (!strcmp("inmemory", optarg))
				backend = BTRFS_DEDUP_BACKEND_INMEMORY;
			else {
				error("unsupported dedup backend: %s", optarg);
				exit(1);
			}
			break;
		case 'b':
			blocksize = parse_size(optarg);
			break;
		case 'a':
			if (strcmp("sha256", optarg)) {
				error("unsupported dedup hash algorithm: %s",
				      optarg);
				return 1;
			}
			break;
		case 'l':
			limit_nr = parse_size(optarg);
			break;
		case 'm':
			limit_mem = parse_size(optarg);
			break;
		}
	}

	path = argv[optind];
	if (check_argc_exact(argc - optind, 1))
		usage(cmd_dedup_enable_usage);

	/* Validation check */
	if (!is_power_of_2(blocksize) ||
	    blocksize > BTRFS_DEDUP_BLOCKSIZE_MAX ||
	    blocksize < BTRFS_DEDUP_BLOCKSIZE_MIN) {
		error("invalid dedup blocksize: %llu, not in range [%u,%u] or power of 2",
		      blocksize, BTRFS_DEDUP_BLOCKSIZE_MIN,
		      BTRFS_DEDUP_BLOCKSIZE_MAX);
		return 1;
	}
	if ((limit_nr || limit_mem) && backend == BTRFS_DEDUP_BACKEND_ONDISK) {
		error("limit is only valid for 'inmemory' backend");
		return 1;
	}
	if (limit_nr && limit_mem) {
		error("limit-memory and limit-hash can't be given at the same time");
		return 1;
	}

	fd = open_file_or_dir(path, &dirstream);
	if (fd < 0) {
		error("failed to open file or directory: %s", path);
		return 1;
	}
	memset(&dargs, 0, sizeof(dargs));
	dargs.cmd = BTRFS_DEDUP_CTL_ENABLE;
	dargs.blocksize = blocksize;
	dargs.hash_type = hash_type;
	dargs.limit_nr = limit_nr;
	dargs.limit_mem = limit_mem;
	dargs.backend = backend;

	ret = ioctl(fd, BTRFS_IOC_DEDUP_CTL, &dargs);
	if (ret < 0) {
		char *error_message = NULL;
		/* Special case, provide better error message */
		if (backend == BTRFS_DEDUP_BACKEND_ONDISK &&
		    errno == -EOPNOTSUPP)
			error_message = "Need 'dedup' mkfs feature to enable ondisk backend";
		error("failed to enable inband deduplication: %s",
		      error_message ? error_message : strerror(errno));
		ret = 1;
		goto out;
	}
	ret = 0;

out:
	close_file_or_dir(fd, dirstream);
	return ret;
}

static const char * const cmd_dedup_disable_usage[] = {
	"btrfs dedup disable <path>",
	"Disable in-band(write time) de-duplication of a btrfs.",
	NULL
};

static int cmd_dedup_disable(int argc, char **argv)
{
	struct btrfs_ioctl_dedup_args dargs;
	DIR *dirstream;
	char *path;
	int fd;
	int ret;

	if (check_argc_exact(argc, 2))
		usage(cmd_dedup_disable_usage);

	path = argv[1];
	fd = open_file_or_dir(path, &dirstream);
	if (fd < 0) {
		error("failed to open file or directory: %s", path);
		return 1;
	}
	memset(&dargs, 0, sizeof(dargs));
	dargs.cmd = BTRFS_DEDUP_CTL_DISABLE;

	ret = ioctl(fd, BTRFS_IOC_DEDUP_CTL, &dargs);
	if (ret < 0) {
		error("failed to disable inband deduplication: %s",
		      strerror(errno));
		ret = 1;
		goto out;
	}
	ret = 0;

out:
	close_file_or_dir(fd, dirstream);
	return 0;
}

static const char * const cmd_dedup_status_usage[] = {
	"btrfs dedup status <path>",
	"Show current in-band(write time) de-duplication status of a btrfs.",
	NULL
};

static int cmd_dedup_status(int argc, char **argv)
{
	struct btrfs_ioctl_dedup_args dargs;
	DIR *dirstream;
	char *path;
	int fd;
	int ret;
	int print_limit = 1;

	if (check_argc_exact(argc, 2))
		usage(cmd_dedup_status_usage);

	path = argv[1];
	fd = open_file_or_dir(path, &dirstream);
	if (fd < 0) {
		error("failed to open file or directory: %s", path);
		ret = 1;
		goto out;
	}
	memset(&dargs, 0, sizeof(dargs));
	dargs.cmd = BTRFS_DEDUP_CTL_STATUS;

	ret = ioctl(fd, BTRFS_IOC_DEDUP_CTL, &dargs);
	if (ret < 0) {
		error("failed to get inband deduplication status: %s",
		      strerror(errno));
		ret = 1;
		goto out;
	}
	ret = 0;
	if (dargs.status == 0) {
		printf("Status: \t\t\tDisabled\n");
		goto out;
	}
	printf("Status:\t\t\tEnabled\n");

	if (dargs.hash_type == BTRFS_DEDUP_HASH_SHA256)
		printf("Hash algorithm:\t\tSHA-256\n");
	else
		printf("Hash algorithm:\t\tUnrecognized(%x)\n",
			dargs.hash_type);

	if (dargs.backend == BTRFS_DEDUP_BACKEND_INMEMORY) {
		printf("Backend:\t\tIn-memory\n");
		print_limit = 1;
	} else if (dargs.backend == BTRFS_DEDUP_BACKEND_ONDISK) {
		printf("Backend:\t\tOn-disk\n");
		print_limit = 0;
	} else  {
		printf("Backend:\t\tUnrecognized(%x)\n",
			dargs.backend);
	}

	printf("Dedup Blocksize:\t%llu\n", dargs.blocksize);

	if (print_limit) {
		printf("Number of hash: \t[%llu/%llu]\n", dargs.current_nr,
			dargs.limit_nr);
		printf("Memory usage: \t\t[%s/%s]\n",
			pretty_size(dargs.current_nr *
				(dargs.limit_mem / dargs.limit_nr)),
			pretty_size(dargs.limit_mem));
	}
out:
	close_file_or_dir(fd, dirstream);
	return ret;
}

const struct cmd_group dedup_cmd_group = {
	dedup_cmd_group_usage, dedup_cmd_group_info, {
		{ "enable", cmd_dedup_enable, cmd_dedup_enable_usage, NULL, 0},
		{ "disable", cmd_dedup_disable, cmd_dedup_disable_usage,
		  NULL, 0},
		{ "status", cmd_dedup_status, cmd_dedup_status_usage,
		  NULL, 0},
		NULL_CMD_STRUCT
	}
};

int cmd_dedup(int argc, char **argv)
{
	return handle_command_group(&dedup_cmd_group, argc, argv);
}
