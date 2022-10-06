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
#include <stdbool.h>
#include <InnoTekLIBC/backend.h>
#define OPENAT_BUFFER_SIZE PATH_MAX
enum { TIMESPEC_HZ = 1000000000 };


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
        if (proc_file != proc_buf)
          free (proc_file);
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

        if (proc_file != proc_buf)
          free (proc_file);

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
	unlink(to_file);
	ret = rename(from_file, to_file);
        if (from_file != from_buf)
          free (from_file);
        if (to_file != to_buf)
          free (to_file);
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
        if (proc_file != proc_buf)
          free (proc_file);
	return (ret);
}

int mkdirat(int dirfd, const char *pathname, mode_t mode)
{
	int error, ret;

	if (dirfd == AT_FDCWD || pathname[0]=='/' || pathname[1]==':')
		return mkdir(pathname, mode);

	char proc_buf[OPENAT_BUFFER_SIZE];
	char *proc_file = openat_proc_name (proc_buf, dirfd, pathname);
	ret = mkdir(proc_file, mode);
        if (proc_file != proc_buf)
          free (proc_file);
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
        if (proc_file != proc_buf)
          free (proc_file);
	return (ret);
}

int linkat(int olddirfd, const char *oldpath,
           int newdirfd, const char *newpath, int flags)
{
	return ENOTSUP;
}

int fstatat(int dirfd, const char *pathname, struct stat *buf,
            int flags)
{
	/* @TODO - handle flags */
	int error, ret;

	if (dirfd == AT_FDCWD || pathname[0]=='/' || pathname[1]==':')
		return stat(pathname, buf);

	char proc_buf[OPENAT_BUFFER_SIZE];
	char *proc_file = openat_proc_name (proc_buf, dirfd, pathname);
	ret = stat(proc_file, buf);
        if (proc_file != proc_buf)
          free (proc_file);
	return (ret);
}


/* adapted from gnulib utimens.c */
#define UTIME_NOW	((1l << 30) - 1l)
#define UTIME_OMIT	((1l << 30) - 2l)

void
gettime (struct timespec *ts)
{
  clock_gettime (CLOCK_REALTIME, ts);
}


/* Return *ST's access time.  */
struct timespec 
get_stat_atime (struct stat const *st)
{
  struct timespec t;
  t.tv_sec = st->st_atime;
  t.tv_nsec = 0;
  return t;
}

/* Return *ST's data modification time.  */
struct timespec 
get_stat_mtime (struct stat const *st)
{
  struct timespec t;
  t.tv_sec = st->st_mtime;
  t.tv_nsec = 0;
  return t;
}

static int
validate_timespec (struct timespec timespec[2])
{
  int result = 0;
  int utime_omit_count = 0;
  if ((timespec[0].tv_nsec != UTIME_NOW
       && timespec[0].tv_nsec != UTIME_OMIT
       && ! (0 <= timespec[0].tv_nsec
             && timespec[0].tv_nsec < TIMESPEC_HZ))
      || (timespec[1].tv_nsec != UTIME_NOW
          && timespec[1].tv_nsec != UTIME_OMIT
          && ! (0 <= timespec[1].tv_nsec
                && timespec[1].tv_nsec < TIMESPEC_HZ)))
    {
      errno = EINVAL;
      return -1;
    }
  /* Work around Linux kernel 2.6.25 bug, where utimensat fails with
     EINVAL if tv_sec is not 0 when using the flag values of tv_nsec.
     Flag a Linux kernel 2.6.32 bug, where an mtime of UTIME_OMIT
     fails to bump ctime.  */
  if (timespec[0].tv_nsec == UTIME_NOW
      || timespec[0].tv_nsec == UTIME_OMIT)
    {
      timespec[0].tv_sec = 0;
      result = 1;
      if (timespec[0].tv_nsec == UTIME_OMIT)
        utime_omit_count++;
    }
  if (timespec[1].tv_nsec == UTIME_NOW
      || timespec[1].tv_nsec == UTIME_OMIT)
    {
      timespec[1].tv_sec = 0;
      result = 1;
      if (timespec[1].tv_nsec == UTIME_OMIT)
        utime_omit_count++;
    }
  return result + (utime_omit_count == 1);
}

/* Normalize any UTIME_NOW or UTIME_OMIT values in (*TS)[0] and (*TS)[1],
   using STATBUF to obtain the current timestamps of the file.  If
   both times are UTIME_NOW, set *TS to NULL (as this can avoid some
   permissions issues).  If both times are UTIME_OMIT, return true
   (nothing further beyond the prior collection of STATBUF is
   necessary); otherwise return false.  */
static bool
update_timespec (struct stat const *statbuf, struct timespec **ts)
{
  struct timespec *timespec = *ts;
  if (timespec[0].tv_nsec == UTIME_OMIT
      && timespec[1].tv_nsec == UTIME_OMIT)
    return true;
  if (timespec[0].tv_nsec == UTIME_NOW
      && timespec[1].tv_nsec == UTIME_NOW)
    {
      *ts = NULL;
      return false;
    }

  if (timespec[0].tv_nsec == UTIME_OMIT)
    timespec[0] = get_stat_atime (statbuf);
  else if (timespec[0].tv_nsec == UTIME_NOW)
    gettime (&timespec[0]);

  if (timespec[1].tv_nsec == UTIME_OMIT)
    timespec[1] = get_stat_mtime (statbuf);
  else if (timespec[1].tv_nsec == UTIME_NOW)
    gettime (&timespec[1]);

  return false;
}

/* Set the access and modification timestamps of FD (a.k.a. FILE) to be
   TIMESPEC[0] and TIMESPEC[1], respectively.
   FD must be either negative -- in which case it is ignored --
   or a file descriptor that is open on FILE.
   If FD is nonnegative, then FILE can be NULL, which means
   use just futimes (or equivalent) instead of utimes (or equivalent),
   and fail if on an old system without futimes (or equivalent).
   If TIMESPEC is null, set the timestamps to the current time.
   Return 0 on success, -1 (setting errno) on failure.  */

int
fdutimens (int fd, char const *file, struct timespec const timespec[2])
{
  struct timespec adjusted_timespec[2];
  struct timespec *ts = timespec ? adjusted_timespec : NULL;
  int adjustment_needed = 0;
  struct stat st;

  if (ts)
    {
      adjusted_timespec[0] = timespec[0];
      adjusted_timespec[1] = timespec[1];
      adjustment_needed = validate_timespec (ts);
    }
  if (adjustment_needed < 0)
    return -1;

  /* Require that at least one of FD or FILE are potentially valid, to avoid
     a Linux bug where futimens (AT_FDCWD, NULL) changes "." rather
     than failing.  */
  if (fd < 0 && !file)
    {
      errno = EBADF;
      return -1;
    }

  /* The platform lacks an interface to set file timestamps with
     nanosecond resolution, so do the best we can, discarding any
     fractional part of the timestamp.  */

  if (adjustment_needed)
    {
      if (adjustment_needed != 3
          && (fd < 0 ? stat (file, &st) : fstat (fd, &st)))
        return -1;
      if (ts && update_timespec (&st, &ts))
        return 0;
    }

  {
    struct timeval timeval[2];
    struct timeval *t;
    if (ts)
      {
        timeval[0].tv_sec = ts[0].tv_sec;
        timeval[0].tv_usec = ts[0].tv_nsec / 1000;
        timeval[1].tv_sec = ts[1].tv_sec;
        timeval[1].tv_usec = ts[1].tv_nsec / 1000;
        t = timeval;
      }
    else
      t = NULL;

    if (fd < 0)
      {
# if HAVE_FUTIMESAT
        return futimesat (AT_FDCWD, file, t);
# endif
      }
    else
      {
        /* If futimesat or futimes fails here, don't try to speed things
           up by returning right away.  glibc can incorrectly fail with
           errno == ENOENT if /proc isn't mounted.  Also, Mandrake 10.0
           in high security mode doesn't allow ordinary users to read
           /proc/self, so glibc incorrectly fails with errno == EACCES.
           If errno == EIO, EPERM, or EROFS, it's probably safe to fail
           right away, but these cases are rare enough that they're not
           worth optimizing, and who knows what other messed-up systems
           are out there?  So play it safe and fall back on the code
           below.  */
      }

    if (!file)
      {
        return -1;
      }

    return utimes (file, t);
  }
}

/* Set the access and modification timestamps of FILE to be
   TIMESPEC[0] and TIMESPEC[1], respectively.  */
int
utimens (char const *file, struct timespec const timespec[2])
{
  return fdutimens (-1, file, timespec);
}

int utimensat(int dirfd, const char *pathname,
              struct timespec times[2], int flags)
{
	/* @TODO - handle flags */
	int error, ret;

	if (dirfd == AT_FDCWD || pathname[0]=='/' || pathname[1]==':')
		return utimens(pathname, times);

	char proc_buf[OPENAT_BUFFER_SIZE];
	char *proc_file = openat_proc_name (proc_buf, dirfd, pathname);
	ret = utimens(proc_file, times);
        if (proc_file != proc_buf)
          free (proc_file);
	return (ret);
}

/* Set the access and modification timestamps of FD to be
   TIMESPEC[0] and TIMESPEC[1], respectively.
   Fail with ENOSYS on systems without futimes (or equivalent).
   If TIMESPEC is null, set the timestamps to the current time.
   Return 0 on success, -1 (setting errno) on failure.  */
int
futimens (int fd, struct timespec const times[2])
{
  /* fdutimens also works around bugs in native futimens, when running
     with glibc compiled against newer headers but on a Linux kernel
     older than 2.6.32.  */
  return fdutimens (fd, NULL, times);
}

int
fchmodat (int dirfd, char const *pathname, mode_t mode, int flags)
{
	int error, ret;

	if (dirfd == AT_FDCWD || pathname[0]=='/' || pathname[1]==':')
		return chmod(pathname, mode);

	char proc_buf[OPENAT_BUFFER_SIZE];
	char *proc_file = openat_proc_name (proc_buf, dirfd, pathname);
	ret = chmod(proc_file, mode);
        if (proc_file != proc_buf)
          free (proc_file);
	return (ret);
}

#if 0
int main() {
	int dirfd = open("e:/website", 0);
	printf("dirfd = %d, errno = %d\n",dirfd,errno);
	int ret = openat(dirfd, "test.php", O_RDONLY, 0);
	printf("ret = %d\n");
}
#endif