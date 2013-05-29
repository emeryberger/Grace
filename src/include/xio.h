// -*- C++ -*-

/*
  Author: Emery Berger, http://www.cs.umass.edu/~emery
 
  Copyright (c) 2007-8 Emery Berger, University of Massachusetts Amherst.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef _XIO_H_
#define _XIO_H_

#include <dlfcn.h>
#include <iostream>
#include <list>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "xsection.h"
#include "xfile.h"
#include "privateheap.h"

using namespace std;

extern "C" 
{
  typedef ssize_t (*writeFunction) (int, const void *, size_t);
  typedef ssize_t (*readFunction) (int, void *, size_t);
  typedef int (*openFunction1) (const char *, int, ...);
  typedef int (*openFunction2) (const char *, int);
  typedef int (*closeFunction) (int);
  typedef int (*creatFunction) (const char *, mode_t);
  typedef int (*lseekFunction) (int, int, int);
  typedef int (*connectFunction) (int, struct sockaddr *, socklen_t );
  typedef int (*acceptFunction) (int, struct sockaddr *, socklen_t *);


#if REPLACE_IO_FUNCTIONS
  int creat (const char * pathname, mode_t mode) {
    gracepause();
    int result = remote_creat (pathname, mode);
    graceresume();
    return result;
  }

  int puts (const char * s) {
    char buf[2048];
    int len = strlen(s);
    memcpy (buf, s, len);
    buf[len] = '\n';
    gracepause();
    remote_write (1, (void *) buf, len+1);
    graceresume();
    return 0;
  }

  int printf (const char * format, ...) {
    gracepause();
    va_list ap;
    va_start (ap, format);
    int v = vprintf (format, ap);
    va_end (ap);
    graceresume();
    return v;
  }
#endif
  
}



class xio {
public:

  typedef std::list<xfile, HL::STLAllocator<xfile, privateheap> >
  filesetType;

  static void * malloc_internal (size_t sz) {
    return privateheap::malloc(sz);
  }

  static void free_internal (void * start) {
    privateheap::free(start);
    start = NULL;
  }

  void list_add (xfile file)
  {
    _filelist.push_back(file);
  }

  filesetType::iterator list_remove (filesetType::iterator file)
  {
    return _filelist.erase(file);
  }
	
  //filesetType::iterator get_file(int fd)
  xfile * get_file (int fd)
  {
    bool found = false;
    filesetType::iterator i;
		
    /* Get corresponding file information using fd information. */
    for(i = _filelist.begin(); i != _filelist.end(); ++i)
      {
	if ((*i).getFd() == fd) {
	  found = true;
	  break;	
	}		
      }
		
    if (!found) {
      return NULL;
    }

    return (&(*i));
  }
	
#if 0
  int local_read (xsection sect,
		  char * buf,
		  unsigned long start,
		  unsigned count)
  {
    char * local_buf = (char *)sect.buf;
    printf("%c %c %c %c and count is %d\n", local_buf[0], local_buf[1], local_buf[2], local_buf[3], count);
    memcpy(buf, local_buf+start, count);
    return count;	
  }
#endif

  int open (char * path, int flags, mode_t mode)
  {
    int fd = -1;
		
    //    printf ("before remote open\n");
    /* Call fopen functions */
    fd = remote_open(path, flags, mode); // was 0666
    //    printf("after remote open\n");
    if(fd != -1)
      {
	xfile file (fd);
	/* Record file information to xfile class. */
	                 
	/* Add current file to _filelist list at first. */
	list_add (file);
      }
    return fd;
  }
	
  int connect (int fd,
	       struct sockaddr *serv_addr,
	       socklen_t addrlen)
  {
    int result = -1;
		
    /* Call fopen functions */
    result = remote_connect(fd, serv_addr, addrlen);
    if (result != -1)
      {
	xfile file (fd);
	                 
	/* Add current file to _filelist list at first. */
	list_add(file);
      }
    return fd;
  }
	
  int accept (int fd,
	      struct sockaddr *addr,
	      socklen_t *addrlen)
  {
    int newfd = -1;
		
    /* Call fopen functions */
    newfd = remote_accept(fd, addr, addrlen);
    if (newfd != -1)
      {
	xfile file (newfd);

	/* Add current file to _filelist list at first. */
	list_add(file);
      }
    return newfd;
  }

  int lseek (int fd,
	     int offset, 
	     int whence)
  {
    int result = -1;

    commit();
    result = remote_lseek(fd, offset, whence);
    xfile  * file = NULL;
		
    if (result != -1)
      {
	file = get_file(fd);
	assert(file == NULL);

	/* Update current position for this file. */
	switch(whence)
	  {
	  case SEEK_SET:
	    file->setFpos (offset);
	    break;

	  case SEEK_CUR:
	    file->incFpos (offset);
	    break;
			
	  case SEEK_END:
	    //FIXME: how to set to the end of one file.
	    break;
	  }
      }
    return(result);
			
  }


  int read (int fd, void *buf, size_t count)
  {
    int num = 0;
		
    /* commit All IO operations before the actual read operation. 
     * In this way, we can get a correct result. For example, for 
     * one socket, if we don't send data to opposite, maybe we can not
     * get corresponding reply. So just commit at first.
     */
    commit();
	
    num = remote_read(fd, buf, count);
    return num;
  }


  int write (int fd, const void *buf, size_t count)
  {
    int num = -1;
    filesetType::iterator file;
    xsection sect;
    char * local_buf = NULL;
    unsigned long start = 0;
    unsigned long read_pos = 0;
    unsigned long end = 0;
    bool found = false;
		
    /* Get corresponding file information using fd information. */
    for(file = _filelist.begin(); file != _filelist.end(); ++file)
      {
	if (file->getFd() == fd) {
	  found = true;
	  break;	
	}		
      } 		
    if (!found)
      {
	printf("this file is not found\n");
	errno = EBADF;
	return -1;
      }

    // Malloc a buffer to store the data.
    local_buf = (char *)malloc_internal(count);

    // Copy the data to be written into the local buffer.
    memcpy (local_buf, buf, count);

    /* Initialize the section information. */
    sect.buf = local_buf;
    sect.offset = file->getFpos();
    sect.length = count;
		
    /* Add current section to current file. */
    //printf("before listadd, position is %d!\n", sect.offset);
    file->list_add(sect);
    //file->update_write_start(sect.offset);
    file->update_write_end(count);
    //printf("after listadd!\n");
    return (count);
  }

  void commit()
  {
    int fd = -1;
    filesetType::iterator file;
    xfile::sectionType::iterator sect;
    unsigned long start = 0;
    unsigned long read_pos = 0;
    unsigned long end = 0;
		
    unsigned long sect_start = 0;
    unsigned long sect_end = 0;
		
    // If there are no files in current transaction, return immediately.		
    if (_filelist.empty())
      return;

    /* Get corresponding file information using fd information. */
    for (file = _filelist.begin(); file != _filelist.end(); ++file)
      {
	// For each file, commit all write buffers in the same time.
	for (sect = file->sect_begin(); sect != file->sect_end(); ++sect)
	  {
	    if (remote_write (file->getFd(), sect->buf, sect->length) == -1)
	      {
		/* error? */
		printf("error!\n");
	      }
				
	    // Free the corresponding buffer.
	    free_internal (sect->buf);
		
	    // Remove the current section from write section list.
	    if (file->list_remove(sect) == file->sect_end())
	      break;
	  }
			
      } 
				
    for (file = _filelist.begin(); file != _filelist.end(); ++file)
      {
	/* If one file is closed, remove it from _filelist list. */
	if (file->isClosed())
	  {
	    /* Call remote close to close this file. */
	    if (remote_close(file->getFd()) == -1)
	      {
		printf("close error!\n");
	      }
	    if (list_remove(file) == _filelist.end())
	      break;
	  }
      }
  }

  int close(int fd)
  {
    bool found = false;

    filesetType::iterator file;
    for (file = _filelist.begin(); file != _filelist.end(); ++file)
      {
	if (file->getFd() == fd) {
	  file->setClosed();
	  found = true;
	  break;
	}
      }
		
    /* If we can not find corresponding fd in the open list, close will be failed. */
    if (found == 0) {
	return -1;
    } else {
      return 0;
    }
  }

private:

  static int remote_open (const char * pathname, int flags, mode_t mode) {
    static openFunction1 realOpen;
    int result;
    //    printf("1\n");
    realOpen = (openFunction1) dlsym (RTLD_NEXT, "open");
    //    printf("2\n");
    result = (realOpen)(pathname, flags, mode);
    // printf("3\n");
    return result;
  }
  
  static int remote_close (int fd) {
    static closeFunction realClose = (closeFunction) dlsym (RTLD_NEXT, "close");
    int result = (realClose)(fd);
    return result;
  }


  static int remote_creat (const char * pathname, mode_t mode) {
    static creatFunction realCreat = (creatFunction) dlsym (RTLD_NEXT, "creat");
    int result = (realCreat)(pathname, mode);
    return result;
  }

  static ssize_t remote_write (int fd, const void * buf, size_t count) {
    static writeFunction realWrite = (writeFunction) dlsym (RTLD_NEXT, "write");
    ssize_t result = (realWrite)(fd, buf, count);
    return result;
  }

  static ssize_t remote_read (int fd, void * buf, size_t count) {
    static readFunction realRead = (readFunction) dlsym (RTLD_NEXT, "read");
    ssize_t result = (realRead)(fd, buf, count);
    return result;
  }

  static ssize_t remote_lseek (int fd, int offset, int whence) {
    static lseekFunction realLseek = (lseekFunction) dlsym (RTLD_NEXT, "lseek");
    ssize_t result = (realLseek)(fd, offset, whence);
    return result;
  }

  static int remote_connect (int fd, struct sockaddr *serv_addr, socklen_t addrlen) {
    static connectFunction realConnect = (connectFunction) dlsym (RTLD_NEXT, "connect");
    int result = (realConnect)(fd, serv_addr, addrlen);
    return result;
  }

  static int remote_accept (int fd, struct sockaddr *addr, socklen_t * addrlen) {
    static acceptFunction realAccept = (acceptFunction) dlsym (RTLD_NEXT, "accept");
    int result = (realAccept)(fd, addr, addrlen);
    return result;
  }

  int remote_puts (const char * str) {
    char buf[2048]; // FIX ME THIS IS OBVIOUSLY NO GOOD.
    sprintf (buf, "%s\n", str);
    remote_write (1, buf, strlen(buf));
    return 0;
  }

public:

  filesetType _filelist;
};

#endif /* _XIO_H_ */
