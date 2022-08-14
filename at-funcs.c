/* Portions inspired by openat-proc.c from coreutils

   Copyright (C) 2006, 2009-2022 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */


#define AT_FDCWD		-100    /* Special value used to indicate
                                           openat should use the current
                                           working directory. */
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <io.h>
#include <sys/errno.h>
#include <sys/syslimits.h>
#include <string.h>
#include <InnoTekLIBC/backend.h>
#define OPENAT_BUFFER_SIZE PATH_MAX

/* Set BUF to the name of the subfile of the directory identified by
   FD, where the subfile is named FILE.  If successful, return BUF if
   the result fits in BUF, dynamically allocated memory otherwise.
   Return NULL (setting errno) on error.  */
char *
openat_proc_name (char buf[OPENAT_BUFFER_SIZE], int fd, char const *file)
{
  char *result = buf;
  int dirlen;

  /* Make sure the caller gets ENOENT when appropriate.  */
  if (!*file)
    {
      buf[0] = '\0';
      return buf;
    }

  /* OS/2 kLIBC provides a function to retrieve a path from a fd.  */
  {
    char dir[_MAX_PATH];
    size_t bufsize;

    if (__libc_Back_ioFHToPath (fd, dir, sizeof dir)){
      return NULL;
    }

    dirlen = strlen (dir);
    bufsize = dirlen + 1 + strlen (file) + 1; /* 1 for '/', 1 for null */
    if (OPENAT_BUFFER_SIZE < bufsize)
      {
        result = malloc (bufsize);
        if (! result)
          return NULL;
      }

    strcpy (result, dir);
    result[dirlen++] = '/';
  }

  strcpy (result + dirlen, file);
  return result;
}

int openat(int dirfd, const char *pathname, int flags, mode_t mode)
{
	int error, ret;

	if (dirfd == AT_FDCWD || pathname[0]=='/' || pathname[1]==':')
		return open(pathname, flags, mode);

	char proc_buf[OPENAT_BUFFER_SIZE];
	char *proc_file = openat_proc_name (proc_buf, dirfd, pathname);
	ret = open(proc_file, flags, mode);
	return (ret);
}

int
unlinkat(int dirfd, const char *pathname, int flags)
{
	int error, ret;

	if (dirfd == AT_FDCWD || pathname[0]=='/' || pathname[1]==':')
		return unlink(pathname);

	char proc_buf[OPENAT_BUFFER_SIZE];
	char *proc_file = openat_proc_name (proc_buf, dirfd, pathname);


	if (flags == AT_REMOVEDIR)
		ret = rmdir(proc_file);
	else
		ret = unlink(proc_file);

	return (ret);
}

int
renameat(int fromfd, const char *from, int tofd, const char *to)
{
	int error, ret;

	if (fromfd == AT_FDCWD || from[0]=='/' || from[1]==':')
		return rename(from, to);

	char from_buf[OPENAT_BUFFER_SIZE];
	char *from_file = openat_proc_name (from_buf, fromfd, from);
	char to_buf[OPENAT_BUFFER_SIZE];
	char *to_file = openat_proc_name (to_buf, tofd, to);

	ret = rename(from_file, to_file);
	return (ret);
}

int
symlinkat(const char *from, int tofd, const char *to)
{
	int error, ret;

	if (tofd == AT_FDCWD || to[0]=='/' || to[1]==':')
		return symlink(from, to);

	char proc_buf[OPENAT_BUFFER_SIZE];
	char *proc_file = openat_proc_name (proc_buf, tofd, to);
	ret = symlink(from, proc_file);
	return (ret);
}

//up to here
int mkdirat(int dirfd, const char *pathname, mode_t mode)
{
	int error, ret;

	if (dirfd == AT_FDCWD || pathname[0]=='/' || pathname[1]==':')
		return mkdir(pathname, mode);

	char proc_buf[OPENAT_BUFFER_SIZE];
	char *proc_file = openat_proc_name (proc_buf, dirfd, pathname);
	ret = mkdir(proc_file, mode);
	return (ret);
}



ssize_t readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz)
{
	int error, ret;

	if (dirfd == AT_FDCWD || pathname[0]=='/' || pathname[1]==':')
		return readlink(pathname, buf, bufsiz);

	char proc_buf[OPENAT_BUFFER_SIZE];
	char *proc_file = openat_proc_name (proc_buf, dirfd, pathname);
	ret = readlink(proc_file, buf, bufsiz);
	return (ret);
}

int linkat(int olddirfd, const char *oldpath,
           int newdirfd, const char *newpath, int flags)
{
	return ENOTSUP;
}

#if 0
int main() {
	int dirfd = open("e:/website", 0);
	printf("dirfd = %d, errno = %d\n",dirfd,errno);
	int ret = openat(dirfd, "test.php", O_RDONLY, 0);
	printf("ret = %d\n");
}
#endif