/*
 * Copyright (c) 2019 Matthew Martin <phy1729@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__APPLE__) || defined(__linux__)
void *reallocarray(void *optr, size_t nmemb, size_t size);
#endif

static void 		tree(char *, int);
static void		printdir(char *, char *, size_t, int);
static void		print_suffix(mode_t);
int			tree_cmp(const void *, const void *);
__dead static void	usage();

int aflag, dflag, Fflag, Icount, Isize, xflag;
const char **Iflag;


struct tree_ent {
	char	*path;
	char	*name;
	mode_t	 mode;
};

int
main(int argc, char *argv[]) {
	char ch, dot[] = ".";
	int Lflag = -1;

	while ((ch = getopt(argc, argv, "adFI:L:x")) != -1) {
		switch (ch) {
		case 'a':
			aflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'F':
			Fflag = 1;
			break;
		case 'I':
			if (Icount >= Isize) {
				Isize += 4;
				if ((Iflag = reallocarray(Iflag, Isize,
				    sizeof(*Iflag))) == NULL)
					err(1, "malloc");
			}
			Iflag[Icount++] = optarg;
			break;
		case 'L':
			{
#ifdef __OpenBSD__
				const char *errstr;
				Lflag = strtonum(optarg, 1, INT_MAX, &errstr);
				if (errstr != NULL)
					errx(1, "Level is %s: %s", errstr, optarg);
#else
				char *eptr;
				errno = 0;
				Lflag = strtol(optarg, &eptr, 10);
				if (errno || eptr == optarg || *eptr != '\0')
					errx(1, "Invalid level: %s", optarg);
#endif
			}
			break;
		case 'x':
			xflag = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

#ifdef __OpenBSD__
	if (pledge("stdio rpath", NULL) == -1)
		err(1, "pledge");
#endif

	if (argc)
		for ( ; *argv; argv++)
			tree(*argv, Lflag);
	else
		tree(dot, Lflag);
}

static void
tree(char *treebeard, int ttl) {
	struct stat sb;
	char prefix[] = "|";

	if (stat(treebeard, &sb) == -1)
		err(1, "stat %s", treebeard);

	printf("%s", treebeard);
	if (Fflag)
		print_suffix(sb.st_mode);
	printf("\n");
	if (S_ISDIR(sb.st_mode))
		printdir(treebeard, prefix, 1, ttl);
}

static void
printdir(char *dir, char *prefix, size_t prefix_len, int ttl) {
	DIR *dp;
	struct dirent *dent;
	struct tree_ent *ents;
	size_t ents_size = 16, i, len, n = 0;

	if (ttl > 0)
		ttl--;

	if ((ents = reallocarray(NULL, ents_size, sizeof(struct tree_ent)))
	    == NULL)
		err(1, "malloc");

	if ((dp = opendir(dir)) == NULL) {
		warn("%s", dir);
		errno = 0;
	}

	len = strlen(dir);
	if (dir[len - 1] == '/')
		dir[len - 1] = '\0';

	while ((dent = readdir(dp)) != NULL) {
		struct stat sb;
		char *path;

		if (dent->d_name[0] == '.') {
			if (!aflag)
				continue;
			if (strcmp(dent->d_name, ".") == 0 ||
			    strcmp(dent->d_name, "..") == 0)
				continue;
		}
		for (i = 0; i < Icount; i++)
			if (fnmatch(Iflag[i], dent->d_name, 0) == 0)
				goto next;

		if (asprintf(&path, "%s/%s", dir, dent->d_name) == -1)
			err(1, "asprintf");
		if (lstat(path, &sb) == -1) {
			warn("stat %s", path);
			errno = 0;
		} else {
			if (dflag && S_ISDIR(sb.st_mode))
				continue;
			if (n == ents_size - 1) {
				ents_size *= 2;
				if ((ents = reallocarray(ents, ents_size,
				    sizeof(struct tree_ent))) == NULL)
					err(1, "malloc");
			}
			if (S_ISLNK(sb.st_mode) && (ents[n].path = strdup(path)) == NULL)
				err(1, "strdup");
			if ((ents[n].name = strdup(dent->d_name)) == NULL)
				err(1, "strdup");
			ents[n].mode = sb.st_mode;
			n++;
		}
		free(path);
next:		;
	}
	closedir(dp);

	qsort(ents, n, sizeof(struct tree_ent), tree_cmp);

	for (i = 0; i < n; i++) {
		if (i == n - 1) {
			prefix[prefix_len - 1] = '`';
		}

		printf("%s-- %s", prefix, ents[i].name);

		if (S_ISLNK(ents[i].mode)) {
			char buf[PATH_MAX];
			ssize_t nbytes;
			nbytes = readlink(ents[i].path, buf, PATH_MAX);
			if (nbytes < 1)
				err(1, "readlink");
			printf(" -> %.*s\n", (int) nbytes, buf);
			free(ents[i].path);
		} else {
			if (Fflag)
				print_suffix(ents[i].mode);
			printf("\n");

			if (S_ISDIR(ents[i].mode)) {
				char *subdir, *subprefix;

				if (asprintf(&subdir, "%s/%s", dir,
				    ents[i].name) == -1)
					err(1, "asprintf");
				if (asprintf(&subprefix, "%s   |", prefix) == -1)
					err(1, "asprintf");
				if (i == n - 1) {
					subprefix[prefix_len - 1] = ' ';
				}
				if (ttl != 0)
					printdir(subdir, subprefix, prefix_len + 4,
					    ttl);
				free(subdir);
				free(subprefix);
			}
		}
		free(ents[i].name);
	}
	if (errno)
		err(1, "readdir");
}

void
print_suffix(mode_t mode) {
	switch (mode & S_IFMT) {
		case S_IFIFO:
			printf("|");
			break;
		case S_IFDIR:
			printf("/");
			break;
		case S_IFREG:
			if (mode & 0111)
				printf("*");
			break;
		case S_IFLNK:
			printf("@");
			break;
		case S_IFSOCK:
			printf("=");
			break;
	}
}

int
tree_cmp(const void *a, const void *b) {
	const struct tree_ent *a_ent = a, *b_ent = b;

	if (xflag) {
		if (S_ISDIR(a_ent->mode) && !S_ISDIR(b_ent->mode))
			return -1;
		if (!S_ISDIR(a_ent->mode) && S_ISDIR(b_ent->mode))
			return 1;
	}
	return strcmp(a_ent->name, b_ent->name);
}

__dead static void
usage() {
	errx(1, "tree [-adFx] [-I pattern] [-L depth]");
}


/*
 * The following is copied-and-pasted directly from OpenBSD's libc:
 *
 * Copyright (c) 2008 Otto Moerbeek <otto@drijf.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#if defined(__APPLE__) || defined(__linux__)
/*
 * This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
 * if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
 */
#define MUL_NO_OVERFLOW	((size_t)1 << (sizeof(size_t) * 4))

void *
reallocarray(void *optr, size_t nmemb, size_t size)
{
	if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    nmemb > 0 && SIZE_MAX / nmemb < size) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(optr, size * nmemb);
}
#endif
